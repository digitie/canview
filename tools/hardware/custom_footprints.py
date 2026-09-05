"""Local land patterns taken from the cited manufacturer package drawings."""
from kicad_model import LIB, q
import math


def pattern(name, body, pads, source, ep=None):
    width,height=body
    data=[f'(footprint {q(name)} (version 20241229) (generator "pcbnew") (layer "F.Cu") (descr {q(source)}) (attr smd)',
          '(property "Reference" "REF**" (at 0 -4 0) (layer "F.SilkS") (effects (font (size 1 1) (thickness .15))))',
          f'(property "Value" {q(name)} (at 0 4 0) (layer "F.Fab") (effects (font (size 1 1) (thickness .15))))',
          f'(fp_rect (start {-width/2} {-height/2}) (end {width/2} {height/2}) (stroke (width .1) (type default)) (fill none) (layer "F.Fab"))']
    for n,x,y,w,h in pads:
        data.append(f'(pad {q(n)} smd roundrect (at {x} {y}) (size {w} {h}) (layers "F.Cu" "F.Paste" "F.Mask") (roundrect_rratio .15))')
    if ep:
        n,w,h=ep
        data.append(f'(pad {q(n)} smd rect (at 0 0) (size {w} {h}) (layers "F.Cu" "F.Mask"))')
        for x in [-w/4,w/4]:
            for y in [-h/4,h/4]:
                data.append(f'(pad "" smd rect (at {x} {y}) (size {w*.38} {h*.38}) (layers "F.Paste"))')
    boundx=max(width/2,max(abs(x)+w/2 for n,x,y,w,h in pads))+.25
    boundy=max(height/2,max(abs(y)+h/2 for n,x,y,w,h in pads))+.25
    data.append(f'(fp_rect (start {-boundx} {-boundy}) (end {boundx} {boundy}) (stroke (width .05) (type default)) (fill none) (layer "F.CrtYd"))')
    marker_x=next(x for n,x,y,w,h in pads if str(n)=='1')
    data.append(f'(fp_circle (center {marker_x} {-boundy-.3}) (end {marker_x+.15} {-boundy-.3}) (stroke (width .15) (type default)) (fill solid) (layer "F.SilkS"))')
    data.append(')')
    dest=LIB/'CANView.pretty';dest.mkdir(parents=True,exist_ok=True)
    (dest/(name+'.kicad_mod')).write_text('\n'.join(data)+'\n',encoding='utf-8')


def dual_row(name, body, count, pitch, row_x, pad_wh, source, ep=None):
    half=count//2
    pads=[]
    for i in range(half):
        y=(i-(half-1)/2)*pitch
        pads.append((i+1,-row_x,y,*pad_wh))
        pads.append((count-i,row_x,y,*pad_wh))
    pattern(name,body,pads,source,ep)


def create():
    pattern('Murata_DFE252012PD',(2.5,2),[(1,-1,0,.8,2),(2,1,0,.8,2)],'murata-dfe252012 p1: recommended2.8mm span,1.2mm gap,2mm height')
    dual_row('TI_DYY0014A',(2.0,4.2),14,.5,1.5,(1.05,.3),'TCAN1046AV PDF pp36-38, TI DYY0014A drawing4224643/D; pad1 upper left')
    dual_row('TI_DRB0008A',(3,3),8,.65,1.4,(.6,.31),'TPS3431 PDF pp28-30, TI DRB0008A drawing4218875/A',ep=(9,1.5,1.75))
    # TOP PCB view, NOT the mirrored component underside. Pin1 is north centre.
    pads=[]
    for i,n in enumerate([4,3,2,1,28,27,26]):pads.append((n,(i-3)*1.27,-5.5,.76,2.2))
    for i,n in enumerate([5,6,7,8,9,10,11]):pads.append((n,-5.5,(i-3)*1.27,2.2,.76))
    for i,n in enumerate([12,13,14,15,16,17,18]):pads.append((n,(i-3)*1.27,5.5,.76,2.2))
    for i,n in enumerate([25,24,23,22,21,20,19]):pads.append((n,5.5,(i-3)*1.27,2.2,.76))
    pattern('Xsens_MTi_1_series',(12.1,12.1),pads,'xsens-mti1-current PDF pp44,83: footprint13.2mm span,8.8mm opening; no exposed copper under central8.8mm; pin1 north centre')
    pads=[(1,.25,-.7575,.270,.285),(2,-.25,-.7575,.270,.285),
          (3,-.7575,-.5,.285,.270),(4,-.7575,0,.285,.270),(5,-.7575,.5,.285,.270),
          (6,-.25,.7575,.270,.285),(7,.25,.7575,.270,.285),
          (8,.7575,.5,.285,.270),(9,.7575,0,.285,.270),(10,.7575,-.5,.285,.270)]
    pattern('Bosch_BMP384_LGA10',(2,2),pads,'bmp384 PDF pp45,48: manufacturer body lands as PCB pattern; underside mirrored to PCB top; pin1 upper right')
    pads=[(1,1.364,-.9,.522,.6),(2,.544,-.9,.522,.6),(4,-1.473,-1.05,.30,.30),
          (5,-1.473,1.05,.30,.30),(6,.544,.9,.522,.6),(7,1.364,.9,.522,.6),(8,1.364,0,.522,.6)]
    pattern('TDK_T5848_LGA8',(3.5,2.65),pads,'t5848 PDF pp37,39: bottom-port footprint mirrored to PCB top; pin1 upper right; ground3 annulus,0.8mm acoustic NPTH')
    dest=LIB/'CANView.pretty/TDK_T5848_LGA8.kicad_mod'
    # Custom annulus avoids a copper disk over the acoustic port. Paste has four
    # separate quadrants, never over the hole. The actual NPTH has no pad number.
    extra=['(pad "3" smd custom (at -.708 0) (size .01 .01) (layers "F.Cu" "F.Mask") (options (clearance outline) (anchor circle)) (primitives (gr_circle (center 0 0) (end .6625 0) (width .3) (fill no))))',
           '(pad "" np_thru_hole circle (at -.708 0) (size .8 .8) (drill .8) (layers "*.Cu" "*.Mask"))']
    for start in [5,95,185,275]:
        pts=[]
        for radius,angles in [(.78,range(start,start+81,5)),(.5625,range(start+80,start-1,-5))]:
            pts.extend(f'(xy {-.708+radius*math.cos(math.radians(a)):.5f} {radius*math.sin(math.radians(a)):.5f})' for a in angles)
        extra.append('(fp_poly (pts '+' '.join(pts)+') (stroke (width 0) (type default)) (fill solid) (layer "F.Paste"))')
    dest.write_text(dest.read_text(encoding='utf-8').rstrip()[:-1]+'\n'+'\n'.join(extra)+'\n)\n',encoding='utf-8')
