# Mini remote main board v0.1: XIAO nRF52840 + 3 tactile switches + header for the Sharp display.
# Generates Gerber (RS-274X) + Excellon files for JLCPCB/PCBWay, a preview PNG, a 1:1 PDF template
# and runs a simple clearance check. Units: mm, origin at the board's bottom-left corner.
import os, sys, zipfile, math, itertools
import numpy as np
from shapely.geometry import Point, LineString, box as sbox, Polygon
from shapely.ops import unary_union
import matplotlib; matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.textpath import TextPath
from matplotlib.font_manager import FontProperties

OUT = sys.argv[1]; os.makedirs(OUT, exist_ok=True)
NAME = 'mini-remote-v0.1'
BW, BH, BR = 43.0, 53.0, 3.0          # board size and corner radius (fits case interior 43.8 x 54)
P = 2.54
TRACE, THIN = 0.30, 0.25              # THIN is used between the XIAO pads
VIA_D, VIA_H = 0.60, 0.30
CLEAR_MIN = 0.15

pads = []    # dict(x,y,shape,w,h,hole,net,label)
tracks = []  # dict(layer,pts,w,net)
vias = []    # dict(x,y,net)
npth = []    # (x,y,d)
silk = []    # ('line',pts) or ('text',x,y,s,size,anchor)

def pad(x, y, net, shape='C', w=1.7, h=None, hole=1.0, cx=None, cy=None):
    pads.append(dict(x=x, y=y, cx=x if cx is None else cx, cy=y if cy is None else cy,
                     shape=shape, w=w, h=h or w, hole=hole, net=net))
def trk(layer, net, *pts, w=TRACE): tracks.append(dict(layer=layer, pts=list(pts), w=w, net=net))
def via(x, y, net): vias.append(dict(x=x, y=y, net=net))

# ---------------- XIAO nRF52840 (USB-C pointing +x, overhangs the right edge) ----------------
XC, YC = 32.5, 30.0                    # centre of the XIAO board (21 x 17.5)
D_ROW = ['D0','D1','D2','D3','D4','D5','D6']          # back row (y = YC + 7.62)
P_ROW = ['5V','GND','3V3','D10','D9','D8','D7']       # front row (y = YC - 7.62)
xpin = lambda k: XC + 3*P - k*P        # index 0 is next to the USB-C
XP = {}
for k, n in enumerate(D_ROW):
    XP[n] = (xpin(k), YC + 7.62); pad(*XP[n], net=n, shape='O', w=1.6, h=3.0, cy=YC + 7.62 + 0.7)
for k, n in enumerate(P_ROW):
    XP[n] = (xpin(k), YC - 7.62); pad(*XP[n], net=n, shape='O', w=1.6, h=3.0, cy=YC - 7.62 - 0.7)
XPAD = lambda n, s: (XP[n][0], XP[n][1] + 0.7*s)   # pad centre (s=+1 back row, -1 front row)

# ---------------- display header (Adafruit 3502 breakout, 1x9, left edge) ----------------
HX = 3.5
H_ORDER = ['VIN','3Vo','GND','CLK','DI','CS','EMD','DISP','EXTIN']   # top to bottom
HY = {n: 33.0 + (4 - j)*P for j, n in enumerate(H_ORDER)}
H_NET = {'VIN':'3V3','GND':'GND','CLK':'D8','DI':'D10','CS':'D3'}
for n in H_ORDER: pad(HX, HY[n], net=H_NET.get(n, 'NC_'+n))

# ---------------- tactile switches (only two diagonal legs are wired) ----------------
# Firmware pin map for this board: PREVIOUS = D2, PLAY/PAUSE = D1, NEXT = D0
SW = [('PREV', 8.0, 7.5, 6.0, 3.25, 2.25, 'D2', 1.0, 1.7),
      ('PLAY', 21.5, 7.5, 12.0, 6.25, 2.5, 'D1', 1.1, 1.9),
      ('NEXT', 35.0, 7.5, 6.0, 3.25, 2.25, 'D0', 1.0, 1.7)]
SWPIN = {}
for name, x, y, s, dx, dy, sig, hole, pd in SW:
    for sx in (-1, 1):
        for sy in (-1, 1):
            net = sig if (sx, sy) == (1, 1) else ('GND' if (sx, sy) == (-1, -1) else 'NC_'+name+str(sx)+str(sy))
            pad(x + sx*dx, y + sy*dy, net=net, w=pd, hole=hole)
    SWPIN[name] = dict(sig=(x + dx, y + dy), gnd=(x - dx, y - dy))

# ---------------- display mounting holes (M2.5) and battery window ----------------
for h in ((5.0, 16.0), (5.0, 50.0), (38.0, 50.0)): npth.append((*h, 2.7))
BATWIN = (23.5, 24.6, 35.1, 35.4)      # cut-out under the XIAO to reach BAT+ / BAT-

# ---------------- routing ----------------
# Buttons: bottom layer, out of the back-row pads, down the gaps between the front-row pads.
ch = {'D2': 36.31, 'D1': 38.85, 'D0': 41.80}
for n in ('D2', 'D1', 'D0'):
    px, py = XPAD(n, +1)
    trk('B', n, (px, py), (px, 36.3), (ch[n], 35.0 if n != 'D0' else 34.6), w=THIN)
pv, pl, nx = SWPIN['PREV']['sig'], SWPIN['PLAY']['sig'], SWPIN['NEXT']['sig']
trk('B', 'D2', (ch['D2'], 35.0), (ch['D2'], 16.0), (pv[0], 16.0), pv, w=THIN)
trk('B', 'D1', (ch['D1'], 35.0), (ch['D1'], 14.5), (pl[0], 14.5), pl, w=THIN)
trk('B', 'D0', (ch['D0'], 34.6), (ch['D0'], 12.5), (nx[0], 12.5), nx, w=THIN)
# Ground: top layer. XIAO GND -> bus along the front -> each switch -> up the left edge to the display GND.
gx, gy = XPAD('GND', -1)
trk('T', 'GND', (gx, gy), (gx, 14.0), (35.0, 12.5), (35.0, 3.0), (1.4, 3.0), (1.4, HY['GND']), (HX, HY['GND']))
for name in ('PREV', 'PLAY', 'NEXT'):
    p = SWPIN[name]['gnd']; trk('T', 'GND', p, (p[0], 3.0))
# Display signals: bottom drop from the pad, top horizontal, bottom lane, top into the header pad.
def disp(net, hname, lane, ylow, start_row):
    px, py = XPAD(net, start_row)
    edge = py - 1.3*1 if start_row < 0 else py + 1.3
    trk('B', net, (px, py), (px, ylow)); via(px, ylow, net)
    trk('T', net, (px, ylow), (lane, ylow)); via(lane, ylow, net)
    trk('B', net, (lane, ylow), (lane, HY[hname])); via(lane, HY[hname], net)
    trk('T', net, (lane, HY[hname]), (HX, HY[hname]))
disp('3V3', 'VIN', 9.6, 19.6, -1)
disp('D10', 'DI', 7.2, 18.8, -1)
disp('D8', 'CLK', 8.4, 18.0, -1)
disp('D3', 'CS', 6.0, 41.0, +1)

# ---------------- silkscreen ----------------
def rect(x0, y0, x1, y1): silk.append(('line', [(x0,y0),(x1,y0),(x1,y1),(x0,y1),(x0,y0)]))
rect(22.0, 21.25, 43.0, 38.75)                                   # XIAO outline
silk.append(('text', 39.6, 30.6, 'USB', 0.9, 'c')); silk.append(('line', [(38.2,29.6),(41.6,29.6),(40.9,30.1)]))
for name, x, y, s, *_ in SW:
    rect(x - s/2, y - s/2, x + s/2, y + s/2)
silk.append(('text', 8.0, 0.6, 'PREV', 0.9, 'c')); silk.append(('text', 21.5, 14.2, 'PLAY', 0.9, 'c')); silk.append(('text', 35.0, 0.6, 'NEXT', 0.9, 'c'))
for n in H_ORDER: silk.append(('text', 4.8, HY[n] - 0.35, n, 0.7, 'l'))
silk.append(('text', 29.45, 35.75, 'BAT', 0.8, 'c'))
silk.append(('text', 13.0, 47.5, 'MINI REMOTE', 1.1, 'c')); silk.append(('text', 13.0, 45.6, 'v0.1', 0.9, 'c'))
silk.append(('text', 13.0, 51.0, 'DISPLAY ON TOP', 0.7, 'c'))

# ---------------- clearance check ----------------
def pad_geom(p, extra=0.0):
    if p['shape'] == 'C': return Point(p['cx'], p['cy']).buffer(p['w']/2 + extra, 32)
    w, h = p['w'], p['h']; r = min(w, h)/2
    a = (p['cx'], p['cy'] - (h/2 - r)); b = (p['cx'], p['cy'] + (h/2 - r))
    return LineString([a, b]).buffer(r + extra, 32)
def copper(layer):
    items = []
    for p in pads: items.append((p['net'], pad_geom(p)))
    for v in vias: items.append((v['net'], Point(v['x'], v['y']).buffer(VIA_D/2, 24)))
    for t in tracks:
        if t['layer'] == layer: items.append((t['net'], LineString(t['pts']).buffer(t['w']/2, 16)))
    return items
problems = []
outline = Polygon(sbox(BR, 0, BW-BR, BH).union(sbox(0, BR, BW, BH-BR)).union(unary_union([Point(x, y).buffer(BR, 48) for x in (BR, BW-BR) for y in (BR, BH-BR)])))
holes_geom = [Point(x, y).buffer(d/2, 32) for x, y, d in npth] + [sbox(*BATWIN)]
for layer in ('T', 'B'):
    items = copper(layer)
    nets = {}
    for n, g in items: nets.setdefault(n, []).append(g)
    merged = {n: unary_union(g) for n, g in nets.items()}
    for (n1, g1), (n2, g2) in itertools.combinations(merged.items(), 2):
        d = g1.distance(g2)
        if d < CLEAR_MIN: problems.append(f'{layer}: {n1} vs {n2} gap {d:.3f}')
    for n, g in merged.items():
        if not outline.buffer(-0.3).contains(g.difference(sbox(43.0 - 0.01, 0, 60, 60))): pass
        for h in holes_geom:
            if g.distance(h) < 0.3: problems.append(f'{layer}: {n} too close to a hole/cut-out ({g.distance(h):.2f})')
    # every net that should be one piece must be connected on the union of both layers (checked below)
# connectivity (both layers + plated holes join layers)
allg = {}
for layer in ('T', 'B'):
    for n, g in copper(layer): allg.setdefault(n, []).append(g)
for n, gs in allg.items():
    if n.startswith('NC_') or n in ('5V', 'D4', 'D5', 'D6', 'D7', 'D9'): continue
    u = unary_union(gs)
    k = 1 if u.geom_type == 'Polygon' else len(u.geoms)
    if k != 1: problems.append(f'net {n} is split into {k} pieces')
print('CHECK:', 'OK' if not problems else '\n  '.join(['problems:'] + problems))

# ---------------- gerber writer ----------------
class Gerber:
    def __init__(s, fn, func):
        s.f = open(os.path.join(OUT, fn), 'w'); s.ap = {}; s.n = 10
        s.f.write(f'%TF.FileFunction,{func}*%\n%FSLAX46Y46*%\n%MOMM*%\n%LPD*%\n')
    def c(s, v): return f'{int(round(v*1e6))}'
    def aperture(s, key):
        if key not in s.ap:
            kind, a, b = key
            d = f'%ADD{s.n}C,{a:.4f}*%' if kind == 'C' else f'%ADD{s.n}O,{a:.4f}X{b:.4f}*%'
            s.f.write(d + '\n'); s.ap[key] = s.n; s.n += 1
        s.f.write(f'D{s.ap[key]}*\n')
    def flash(s, x, y, key): s.aperture(key); s.f.write(f'X{s.c(x)}Y{s.c(y)}D03*\n')
    def line(s, pts, w):
        s.aperture(('C', w, w)); x, y = pts[0]; s.f.write(f'X{s.c(x)}Y{s.c(y)}D02*\n')
        for x, y in pts[1:]: s.f.write(f'X{s.c(x)}Y{s.c(y)}D01*\n')
    def region(s, pts, clear=False):
        if clear: s.f.write('%LPC*%\n')
        x, y = pts[0]; s.f.write(f'G36*\nX{s.c(x)}Y{s.c(y)}D02*\n')
        for x, y in pts[1:]: s.f.write(f'X{s.c(x)}Y{s.c(y)}D01*\n')
        s.f.write('G37*\n')
        if clear: s.f.write('%LPD*%\n')
    def close(s): s.f.write('M02*\n'); s.f.close()
def padkey(p, extra=0.0):
    return ('C', p['w'] + extra, None) if p['shape'] == 'C' else ('O', p['w'] + extra, p['h'] + extra)
for layer, fn, func in (('T', f'{NAME}.GTL', 'Copper,L1,Top'), ('B', f'{NAME}.GBL', 'Copper,L2,Bot')):
    g = Gerber(fn, func)
    for p in pads: g.flash(p['cx'], p['cy'], padkey(p))
    for v in vias: g.flash(v['x'], v['y'], ('C', VIA_D, None))
    for t in tracks:
        if t['layer'] == layer: g.line(t['pts'], t['w'])
    g.close()
for fn, func in ((f'{NAME}.GTS', 'Soldermask,Top'), (f'{NAME}.GBS', 'Soldermask,Bot')):
    g = Gerber(fn, func)
    for p in pads: g.flash(p['cx'], p['cy'], padkey(p, 0.1))
    g.close()
# silkscreen (text as filled outlines, holes cleared)
fp = FontProperties(family='DejaVu Sans', weight='bold')
def text_polys(x, y, s, size, anchor):
    tp = TextPath((0, 0), s, size=size/0.72, prop=fp)
    ext = tp.get_extents(); ox = {'l': 0, 'c': -ext.width/2}[anchor]
    polys = tp.to_polygons(closed_only=True)
    return [np.array(p) + [x + ox - ext.x0, y] for p in polys if len(p) > 2]
g = Gerber(f'{NAME}.GTO', 'Legend,Top')
silk_geom = []
for it in silk:
    if it[0] == 'line': g.line(it[1], 0.15); silk_geom.append(LineString(it[1]).buffer(0.075))
    else:
        polys = text_polys(*it[1:])
        sp = [Polygon(q) for q in polys]
        is_hole = [any(j != i and sp[j].contains(sp[i]) for j in range(len(sp))) for i in range(len(sp))]
        for q, h in zip(polys, is_hole):
            if not h: g.region([tuple(v) for v in q])
        for q, h in zip(polys, is_hole):
            if h: g.region([tuple(v) for v in q], clear=True)
        silk_geom.append(('text', polys))
g.close()
# outline + cut-out
g = Gerber(f'{NAME}-Edge_Cuts.gm1', 'Profile,NP')
ext = list(outline.exterior.coords); g.line(ext, 0.1)
x0, y0, x1, y1 = BATWIN; g.line([(x0,y0),(x1,y0),(x1,y1),(x0,y1),(x0,y0)], 0.1)
g.close()
# drills
def excellon(fn, holes, plated):
    sizes = sorted({d for *_, d in holes})
    with open(os.path.join(OUT, fn), 'w') as f:
        f.write('M48\n; ' + ('PLATED' if plated else 'NON-PLATED') + '\nMETRIC,TZ\n')
        for i, d in enumerate(sizes, 1): f.write(f'T{i}C{d:.3f}\n')
        f.write('%\nG90\nG05\n')
        for i, d in enumerate(sizes, 1):
            f.write(f'T{i}\n')
            for x, y, dd in holes:
                if dd == d: f.write(f'X{x:.3f}Y{y:.3f}\n')
        f.write('T0\nM30\n')
excellon(f'{NAME}-PTH.drl', [(p['x'], p['y'], p['hole']) for p in pads] + [(v['x'], v['y'], VIA_H) for v in vias], True)
excellon(f'{NAME}-NPTH.drl', npth, False)
with zipfile.ZipFile(os.path.join(OUT, f'{NAME}-gerbers.zip'), 'w') as z:
    for fn in os.listdir(OUT):
        if fn.startswith(NAME) and not fn.endswith(('.zip', '.png', '.pdf')): z.write(os.path.join(OUT, fn), fn)

# ---------------- preview + 1:1 template ----------------
from matplotlib.patches import PathPatch
from matplotlib.transforms import Affine2D
def text_patch(x, y, s, size, anchor, color):
    tp = TextPath((0, 0), s, size=size/0.72, prop=fp); ext = tp.get_extents()
    ox = {'l': 0, 'c': -ext.width/2}[anchor]
    return PathPatch(tp.transformed(Affine2D().translate(x + ox - ext.x0, y)), fc=color, ec='none')
def draw(ax, scale_text=1.0, show_bottom=True):
    ax.add_patch(plt.Polygon(np.array(outline.exterior.coords), fc='#1f6f3f', ec='#d9c27a', lw=1))
    x0, y0, x1, y1 = BATWIN; ax.add_patch(plt.Rectangle((x0, y0), x1-x0, y1-y0, fc='white', ec='#d9c27a'))
    if show_bottom:
        for t in tracks:
            if t['layer'] == 'B': a = np.array(t['pts']); ax.plot(a[:,0], a[:,1], color='#4f8fe0', lw=t['w']*7*scale_text, alpha=.8, solid_capstyle='round')
    for t in tracks:
        if t['layer'] == 'T': a = np.array(t['pts']); ax.plot(a[:,0], a[:,1], color='#e3a03a', lw=t['w']*7*scale_text, solid_capstyle='round')
    for p in pads:
        xy = np.array(pad_geom(p).exterior.coords); ax.add_patch(plt.Polygon(xy, fc='#e6c36a', ec='none'))
        ax.add_patch(plt.Circle((p['x'], p['y']), p['hole']/2, fc='#111'))
    for v in vias:
        ax.add_patch(plt.Circle((v['x'], v['y']), VIA_D/2, fc='#e6c36a')); ax.add_patch(plt.Circle((v['x'], v['y']), VIA_H/2, fc='#111'))
    for x, y, d in npth: ax.add_patch(plt.Circle((x, y), d/2, fc='white', ec='#d9c27a'))
    for it in silk:
        if it[0] == 'line': a = np.array(it[1]); ax.plot(a[:,0], a[:,1], color='white', lw=0.8*scale_text)
        else: ax.add_patch(text_patch(*it[1:], 'white'))
    ax.set_aspect('equal'); ax.set_xlim(-2, BW+3); ax.set_ylim(-2, BH+2); ax.axis('off')
fig, ax = plt.subplots(figsize=(8, 9.5)); draw(ax)
ax.set_title('Mini remote main board v0.1, top view (orange = top copper, blue = bottom copper)', fontsize=9)
plt.tight_layout(); plt.savefig(os.path.join(OUT, f'{NAME}-preview.png'), dpi=150); plt.close()
# 1:1 template on an A4 page: print at 100 % ("actual size") and lay the parts on it
fig = plt.figure(figsize=(210/25.4, 297/25.4)); ax = fig.add_axes([0, 0, 1, 1])
ax.set_xlim(-60, 150); ax.set_ylim(-200, 97); ax.set_aspect('equal'); ax.axis('off')
ax.add_patch(plt.Polygon(np.array(outline.exterior.coords), fc='none', ec='k', lw=0.6))
for p in pads:
    ax.add_patch(plt.Polygon(np.array(pad_geom(p).exterior.coords), fc='none', ec='k', lw=0.4)); ax.add_patch(plt.Circle((p['x'], p['y']), p['hole']/2, fc='k'))
for x, y, d in npth: ax.add_patch(plt.Circle((x, y), d/2, fc='none', ec='k', lw=0.4))
x0, y0, x1, y1 = BATWIN; ax.add_patch(plt.Rectangle((x0, y0), x1-x0, y1-y0, fc='none', ec='k', lw=0.4, ls='--'))
for it in silk:
    if it[0] == 'line': a = np.array(it[1]); ax.plot(a[:,0], a[:,1], color='#666', lw=0.4)
    else: ax.add_patch(text_patch(*it[1:], '#444'))
ax.plot([0, 50], [-12, -12], 'k', lw=0.8); ax.text(25, -16, '50 mm check line: measure it after printing', ha='center', fontsize=7)
ax.text(-55, 92, 'Mini remote v0.1, 1:1 template. Print at 100 % (actual size), cut out, place the real parts on it.', fontsize=7)
fig.savefig(os.path.join(OUT, f'{NAME}-template-1to1.pdf')); plt.close()
print('files:', sorted(os.listdir(OUT)))
