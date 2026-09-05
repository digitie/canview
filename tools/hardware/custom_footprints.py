"""Local land patterns taken from the cited manufacturer package drawings."""
from kicad_model import LIB, q


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
    data.append(f'(fp_circle (center {-boundx} {-boundy-.3}) (end {-boundx+.15} {-boundy-.3}) (stroke (width .15) (type default)) (fill solid) (layer "F.SilkS"))')
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
    dual_row('TI_DYY0014A',(2.0,4.2),14,.5,1.5,(1.05,.3),'TCAN1046AV PDF pp36-38, TI DYY0014A drawing4224643/D; pad1 upper left')
    dual_row('TI_DRB0008A',(3,3),8,.65,1.4,(.6,.31),'TPS3431 PDF pp28-30, TI DRB0008A drawing4218875/A',ep=(9,1.5,1.75))
