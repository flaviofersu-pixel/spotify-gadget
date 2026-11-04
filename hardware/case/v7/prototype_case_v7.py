import numpy as np, trimesh, sys, os
from manifold3d import Manifold as M, CrossSection as CS, JoinType
out=sys.argv[1]
def rbox(w,h,r,t,x=0,y=0,z=0):
    r=min(r,w/2-0.01,h/2-0.01)
    cs=CS.square((w-2*r,h-2*r),center=True).offset(r,JoinType.Round,circular_segments=64)
    return M.extrude(cs,t).translate((x,y,z))
def cyl(d,t,x,y,z): return M.cylinder(t,d/2,d/2,48).translate((x,y,z))
def box(w,h,t,x,y,z): return M.cube((w,h,t),center=False).translate((x-w/2,y-h/2,z))
def bx(x0,x1,y0,y1,z0,z1): return M.cube((x1-x0,y1-y0,z1-z0)).translate((x0,y0,z0))

WALL=1.6; FLOOR=1.2
IW,IH=43.8,54.0            # interior (unchanged from v6, so everything still fits)
OW,OH=IW+2*WALL,IH+2*WALL  # 47.0 x 57.2
CY=IH/2
BASE_H=11.1
R=8.0                      # v7: rounder corners (v6 was 5.0)
RI=R-WALL

# ---------------- display (same as v6) ----------------
PCB_W,PCB_H,PCB_T=39.0,40.0,4.6
PCB_TOP_Y=IH-1.45
PCB_CY=PCB_TOP_Y-PCB_H/2    # 32.55
PCB_FIT=0.25
base=rbox(OW,OH,R,BASE_H,0,CY,0) - rbox(IW,IH,RI,BASE_H,0,CY,FLOOR)
PCB_BOTTOM=BASE_H-PCB_T     # 6.5
PCB_TOP=PCB_BOTTOM+1.6      # 8.1
CRADLE_T=1.2; CRADLE_L=7.0
post_h=PCB_BOTTOM-FLOOR
for hx in (-(PCB_W/2-3), PCB_W/2-3):
    for hy in (PCB_CY-(PCB_H/2-3), PCB_CY+(PCB_H/2-3)):
        base+=cyl(5.0,post_h,hx,hy,FLOOR)
        base+=cyl(2.6,1.4,hx,hy,PCB_BOTTOM)
inner_w,inner_h=PCB_W+2*PCB_FIT,PCB_H+2*PCB_FIT
ring=(rbox(inner_w+2*CRADLE_T,inner_h+2*CRADLE_T,3+PCB_FIT+CRADLE_T,PCB_TOP-0.2-FLOOR,0,PCB_CY,FLOOR)
      -rbox(inner_w,inner_h,3+PCB_FIT,PCB_TOP,0,PCB_CY,0))
corners=M()
for sx in (-1,1):
    for sy in (-1,1):
        cx=sx*(inner_w/2+CRADLE_T-CRADLE_L/2); cy=PCB_CY+sy*(inner_h/2+CRADLE_T-CRADLE_L/2)
        corners+=box(CRADLE_L,CRADLE_L,BASE_H,cx,cy,0)
base+=ring^corners

# ---------------- v7: XIAO nRF52840 cradle (no tape, no glue) ----------------
# Board 21 x 17.5 mm, USB-C pointing +x. Receptacle mouth sits at the inner face of the right wall.
X_L,X_W=21.0,17.5
X_T=1.2                     # board thickness (top of lip leaves 0.1 mm play)
X_GAP=1.2                   # room under the board for the battery wires on BAT+/BAT-
X_X1=IW/2-1.2               # board edge on the USB side (receptacle overhangs ~1.2 mm)
X_X0=X_X1-X_L
X_CY=34.75
X_Y0,X_Y1=X_CY-X_W/2,X_CY+X_W/2
X_Z0=FLOOR+X_GAP            # 2.4: underside of the board
X_Z1=X_Z0+X_T               # 3.6: top of the board
# support rails under both long edges; the front one has a gap for the battery wires
for y0,y1 in ((X_Y0+0.2,X_Y0+1.6),(X_Y1-1.6,X_Y1-0.2)):
    base+=bx(X_X0+1,X_X0+5,y0,y1,FLOOR,X_Z0)
    base+=bx(X_X1-5,X_X1-1,y0,y1,FLOOR,X_Z0)
base+=bx(X_X0+1,X_X1-1,X_Y1-1.6,X_Y1-0.2,FLOOR,X_Z0)   # back rail is full length
# side locators at the back corners (behind the last solder pad, so they never touch the wires)
for y0,y1 in ((X_Y0-1.4,X_Y0-0.15),(X_Y1+0.15,X_Y1+1.4)):
    base+=bx(X_X0-0.2,X_X0+1.4,y0,y1,FLOOR,X_Z1)
# snap clip on the back edge: a thin flexing tab with a 0.6 mm lip that hooks over the board
CLIP_W=8.0; CLIP_T=1.0; LIP=0.6
cx0=X_X0-0.15-CLIP_T
base+=bx(cx0,cx0+CLIP_T,X_CY-CLIP_W/2,X_CY+CLIP_W/2,FLOOR,X_Z1+1.0)
# lip: flat underside hooks the board, sloped top lets the board push the tab back on the way down
xa=cx0+0.2; xb=X_X0-0.15+LIP
prof=CS([[(xa,X_Z1+0.1),(xb,X_Z1+0.1),(xa,X_Z1+1.5)]])
base+=M.extrude(prof,CLIP_W).rotate((90,0,0)).translate((0,X_CY+CLIP_W/2,0))
base+=bx(cx0,cx0+CLIP_T,X_CY-CLIP_W/2,X_CY+CLIP_W/2,X_Z1+1.0,X_Z1+1.6)

# ---------------- v7: USB-C opening sized to the plug ----------------
# Receptacle 8.94 x 3.26 mm; plug shell 8.3 x 2.5 mm. Stadium opening 9.6 x 3.9 mm.
USB_W,USB_H=9.6,3.9
usb_z=X_Z0+3.5-3.26/2       # XIAO is 3.5 mm thick including the receptacle -> centre about 4.27
usb=(M.extrude(CS.square((0.01,USB_W-USB_H),center=True).offset(USB_H/2,JoinType.Round,circular_segments=32),WALL+2)
     .rotate((0,90,0)).translate((IW/2-1,X_CY,usb_z)))
base-=usb

# ---------------- v7: battery pocket ----------------
# Battery about 28 x 20.5 x 4.2 mm, lying at the front (button side). 0.4 mm play all round.
B_L,B_W,B_T=28.0,20.5,4.2
B_X0,B_X1=-B_L/2-0.4,B_L/2+0.4
B_Y0,B_Y1=0.6,0.6+B_W+0.8
FEN_H=3.0
for xa,xb in ((B_X0-1.2,B_X0),(B_X1,B_X1+1.2)):
    base+=bx(xa,xb,B_Y0+3,B_Y1,FLOOR,FLOOR+FEN_H)
# back fence with a 10 mm gap on the right for the red and black wires
base+=bx(B_X0-1.2,2.0,B_Y1,B_Y1+1.2,FLOOR,FLOOR+FEN_H)
base+=bx(12.0,B_X1+1.2,B_Y1,B_Y1+1.2,FLOOR,FLOOR+FEN_H)
# small bumps on the side fences hold the battery without glue
for x,s in ((B_X0,1),(B_X1,-1)):
    for y in (B_Y0+7,):
        base+=cyl(1.2,FEN_H,x,y,FLOOR)

# ---------------- lid ----------------
LID_T=1.4
lid=rbox(OW,OH,R,LID_T,0,CY,0)
SK=3.0; ST=1.2; CL=0.05
skirt=rbox(IW-2*CL,IH-2*CL,RI-CL,SK,0,CY,-SK)-rbox(IW-2*CL-2*ST,IH-2*CL-2*ST,max(RI-CL-ST,0.5),SK,0,CY,-SK)
lid+=skirt
RIB=0.25; RL=6.0; RH=SK-0.6
sx=IW/2-CL; sy0=CY-(IH/2-CL); sy1=CY+(IH/2-CL)
for y in (CY-12,CY+12):
    for x in (-sx,sx): lid+=box(2*RIB,RL,RH,x,y,-SK)
for x in (-8,8):
    for y in (sy0,sy1): lid+=box(RL,2*RIB,RH,x,y,-SK)
# skirt must clear the battery fences and XIAO clip: it only reaches 3 mm down, fences top at 4.2, fine.
lid-=rbox(27.0,23.0,1.5,LID_T+SK+2,0,PCB_CY,-SK-1)
BTN_Y=7.0; LEDGE_W=0.8; LEDGE_T=0.6
BTNS=((-13,6.0),(0,12.0),(13,6.0))
for bxp,size in BTNS:
    hole=size+0.4
    lid-=box(hole,hole,LID_T+SK+2,bxp,BTN_Y,-SK-1)
    for s_ in (-1,1):
        lid+=box(hole,LEDGE_W,LEDGE_T,bxp,BTN_Y+s_*(hole/2-LEDGE_W/2),0)
lid=lid.rotate((180,0,0)).translate((0,2*CY,LID_T))

# ---------------- v7: button caps ----------------
# Printed top-face down. A round socket on the underside pushes onto the switch plunger.
# Small 6x6 switch: plunger 3.5 mm. Big 12x12 switch: round actuator ~6.8 mm.
def icon_prev(s):
    a=CS([[(-0.2,0),(0.25*s,0.22*s),(0.25*s,-0.22*s)]]).translate((-0.12*s,0))
    b=CS([[(-0.2,0),(0.25*s,0.22*s),(0.25*s,-0.22*s)]]).translate((0.12*s,0))
    bar=CS.square((0.07*s,0.44*s),center=True).translate((-0.2*s,0))
    return a+b+bar
def icon_play(s):
    tri=CS([[(-0.2*s,0.22*s),(-0.2*s,-0.22*s),(0.08*s,0)]]).translate((-0.04*s,0))
    p1=CS.square((0.07*s,0.44*s),center=True).translate((0.14*s,0))
    p2=CS.square((0.07*s,0.44*s),center=True).translate((0.26*s,0))
    return tri+p1+p2
def cap(size,bore,depth,icon):
    H=depth+1.4
    c=rbox(size,size,size*0.22,H)
    # soft top edge: slightly smaller first layer
    c=c-(rbox(size+2,size+2,1,0.4)-rbox(size-0.8,size-0.8,size*0.22-0.4,0.4))
    c-=cyl(bore,depth+0.01,0,0,H-depth)
    ic=M.extrude(icon(size),0.5)
    return c-ic.translate((0,0,-0.01))
caps=[cap(5.8,3.6,1.4,icon_prev),
      cap(11.4,7.0,2.4,icon_play),
      cap(5.8,3.6,1.4,lambda s: icon_prev(s).mirror((1,0)))]

def tm(man,color=None):
    mesh=man.to_mesh(); v=np.array(mesh.vert_properties)[:,:3]; f=np.array(mesh.tri_verts)
    t=trimesh.Trimesh(v,f)
    if color is not None: t.visual.face_colors=color
    return t
os.makedirs(out,exist_ok=True)
parts={'base':base,'lid':lid.translate((OW+6,0,0))}
for i,(c,x) in enumerate(zip(caps,(-14,0,14))):
    parts[f'cap-{["previous","play-pause","next"][i]}']=c.translate((x+ (OW+6)/2,-12,0))
scene=trimesh.Scene()
for n,m in parts.items():
    t=tm(m); print(n,t.is_watertight,np.round(t.bounds,2).tolist())
    scene.add_geometry(t,node_name=n,geom_name=n)
scene.export(f'{out}/prototype-case-v7-plate.3mf')
trimesh.util.concatenate(list(scene.geometry.values())).export(f'{out}/prototype-case-v7-plate.stl')
for n,m in (('base',base),('lid',lid)): tm(m).export(f'{out}/prototype-case-v7-{n}.stl')
