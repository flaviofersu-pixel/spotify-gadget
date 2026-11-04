# Draws the documentation images for case v7: labelled top view and wiring diagram.
import sys,numpy as np,matplotlib;matplotlib.use('Agg');import matplotlib.pyplot as plt
sys.argv=['x','/tmp/claude-0/-home-user/6414c7e0-6700-548c-8596-c1af8381e930/scratchpad/case/v7tmp']
g={};exec(open('prototype_case_v7.py').read(),g)
def outline(ax,alpha):
    for poly in g['base'].slice(3.0).to_polygons():
        p=np.array(poly);p=np.vstack([p,p[:1]]);ax.fill(p[:,0],p[:,1],color='#3d9a6a',alpha=alpha);ax.plot(p[:,0],p[:,1],'#1c4a33',lw=.7)

# ---- top view ----
fig,ax=plt.subplots(figsize=(8,9));outline(ax,.55)
ax.add_patch(plt.Rectangle((g['X_X0'],g['X_Y0']),21,17.5,fill=False,ec='#1e40af',ls='--',lw=1.5))
ax.add_patch(plt.Rectangle((-14,1.0),28,20.5,fill=False,ec='#c2410c',ls='--',lw=1.5))
ax.add_patch(plt.Rectangle((-19.5,12.55),39,40,fill=False,ec='#555',ls=':',lw=1.2))
T=lambda x,y,s,c: ax.text(x,y,s,color=c,ha='center',va='center',fontsize=10,fontweight='bold')
T(10.2,36,'XIAO','#1e40af');T(0,11,'Battery','#c2410c');T(-9,47,'Display (on top)','#555')
A=lambda s,xy,xt: ax.annotate(s,xy=xy,xytext=xt,fontsize=9,arrowprops=dict(arrowstyle='->'),ha='center')
A('Snap clip',(-0.9,34.75),(-12,33));A('USB-C opening\n9.6 x 3.9 mm',(23.4,34.75),(31,24))
A('Side stops',(0.4,25.2),(-8,26));A('Gap for the red\nand black wires',(7,23),(14,-6));A('Grip bumps',(-14.4,7.6),(-28,0))
ax.set_aspect('equal');ax.set_xlim(-34,36);ax.set_ylim(-10,60);ax.axis('off')
ax.set_title('Case v7, top view (section 3 mm above the floor)',fontsize=11)
plt.tight_layout();plt.savefig('v7/case-v7-top-view.png',dpi=110);plt.close()

# ---- wiring ----
fig,ax=plt.subplots(figsize=(10,11));outline(ax,.3)
ax.add_patch(plt.Rectangle((-0.3,26),21,17.5,fc='#2a2d35',ec='k'))
ax.add_patch(plt.Rectangle((-14,1.0),28,20.5,fc='#e3e5e8',ec='#999'))
ax.text(0,17.5,'Battery',ha='center',va='center',fontsize=11,fontweight='bold',color='#555')
ax.text(8,34.75,'XIAO',ha='center',va='center',fontsize=12,fontweight='bold',color='w')
ax.text(20.3,34.75,'USB-C →',ha='right',va='center',fontsize=7,color='w')
DP=['EIN','DISP','EMD','CS','DI','CLK','GND','3V3','VIN']
dp={n:(-18,32.55-(i-4)*2.54) for i,n in enumerate(DP)}
ax.add_patch(plt.Rectangle((-19.3,21.2),2.6,22.7,fc='#111',ec='k'))
for n,(x,y) in dp.items():
    ax.plot(x,y,'o',ms=4,color='#e8c25a',zorder=7);ax.text(-20,y,n,ha='right',va='center',fontsize=7.5,fontweight='bold',color='#b4530a' if n in('CS','DI','CLK','GND','3V3','VIN') else '#999')
ax.text(-18,45.6,'Display\nheader',ha='center',fontsize=8,color='#555')
P=2.54;xp=lambda i:17.82-i*P
top={n:(xp(i),26) for i,n in enumerate(['5V','GND','3V3','D10','D9','D8','D7'])}
bot={n:(xp(i),43.5) for i,n in enumerate(['D0','D1','D2','D3','D4','D5','D6'])}
for d,dy in ((top,1.5),(bot,-1.5)):
    for n,(x,y) in d.items():
        ax.plot(x,y,'o',ms=4,color='#e8c25a',zorder=7);ax.text(x,y+dy,n,ha='center',va='center',fontsize=6.5,color='#9ec5ff',fontweight='bold',zorder=8)
C=dict(red='#d62828',black='#1b1b1b',yellow='#e0b000',green='#1f9d55',blue='#2563eb',white='#a8a8a8',purple='#7c3aed',orange='#ea7a14')
def w(pts,c,ls='-',lw=2.6): a=np.array(pts);ax.plot(a[:,0],a[:,1],ls,color=C[c],lw=lw,solid_capstyle='round',zorder=5)
w([top['3V3'],(12.74,23.0),(-15.6,23.0),(-15.6,dp['VIN'][1]),dp['VIN']],'red')
w([top['GND'],(15.28,23.8),(-14.6,23.8),(-14.6,dp['GND'][1]),dp['GND']],'black')
w([top['D10'],(10.2,24.6),(-13.6,24.6),(-13.6,dp['DI'][1]),dp['DI']],'green')
w([top['D8'],(5.12,25.3),(-12.6,25.3),(-12.6,dp['CLK'][1]),dp['CLK']],'yellow')
w([bot['D3'],(10.2,45.6),(-11.6,45.6),(-11.6,dp['CS'][1]),dp['CS']],'blue')
B=[(-13,6,'Previous','white','D0',-9.6,48.6,13.0),(0,12,'Play/\nPause','purple','D1',-8.6,47.8,14.0),(13,6,'Next','orange','D2',-7.6,47.0,15.0)]
for x,s,n,c,pin,lx,py,hy in B:
    ax.add_patch(plt.Rectangle((x-s/2,7-s/2),s,s,fc='#f0fdf4',ec='#14532d',lw=1.5,ls='--',zorder=4))
    ax.text(x,7,n,ha='center',va='center',fontsize=7.5,color='#14532d',fontweight='bold',zorder=6)
    sx,sy=x+s/2,7+s/2-0.9
    w([(sx,sy),(sx,hy),(lx,hy),(lx,py),(bot[pin][0],py),bot[pin]],c,lw=2.2)
    ax.plot(sx,sy,'o',ms=4,color='#e8c25a',zorder=7)
gl=[(x-s/2,7-s/2+0.9) for x,s,*_ in B]
for p in gl: ax.plot(*p,'o',ms=4,color='#e8c25a',zorder=7)
w([top['GND'],(16.6,23.8),(16.6,0.6),(gl[0][0],0.6),gl[0]],'black',lw=2.2)
for p in gl[1:]: w([(p[0],0.6),p],'black',lw=2.2)
w([(9,21.5),(9,28.5),(11.5,31)],'red','--',2.2);w([(6.5,21.5),(6.5,29),(9,33)],'black','--',2.2)
ax.text(12,30.3,'BAT+',fontsize=7,color='#ff8a8a',fontweight='bold',zorder=8);ax.text(9.3,33.6,'BAT−',fontsize=7,color='w',fontweight='bold',zorder=8)
ax.text(25.5,12,'– – –  wire under\nthe XIAO',fontsize=8,color='#444')
ax.text(25.5,4,'Yellow dot =\nsolder joint',fontsize=8,color='#444')
ax.text(-33,-5.5,'Buttons sit in the lid, above the battery. The back leg of each button goes to the XIAO along the left side.\nThe front leg joins the black GND chain, which runs along the front.',fontsize=8,color='#444')
ax.set_aspect('equal');ax.set_xlim(-34,38);ax.set_ylim(-9,59);ax.axis('off')
ax.set_title('Case v7, top view with wiring',fontsize=12)
plt.tight_layout();plt.savefig('v7/case-v7-wiring.png',dpi=120);print('ok')
