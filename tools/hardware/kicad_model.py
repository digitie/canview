"""Small deterministic KiCad schematic generator, not a netlist substitute.

The connectivity input is an auditable component/pin map. KiCad 10 itself exports
the electrical netlist, PDF and ERC; validators compare those exports to input.
"""
from __future__ import annotations

import csv
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import uuid

ROOT = Path(__file__).resolve().parents[2]
KICAD = Path("C:/Program Files/KiCad/10.0/share/kicad")
LIB = ROOT / "hardware/libraries"


def uid(value):
    return str(uuid.uuid5(uuid.NAMESPACE_URL, "canview-hardware-r1/" + value))


def q(value):
    return json.dumps(str(value), ensure_ascii=False)


def parse(text):
    tokens = re.findall(r'"(?:\\.|[^"\\])*"|[^\s()]+|[()]', text)
    stack, root = [], None
    for token in tokens:
        if token == "(":
            node = []
            if stack:
                stack[-1].append(node)
            stack.append(node)
        elif token == ")":
            root = stack.pop()
        else:
            stack[-1].append(json.loads(token) if token.startswith('"') else token)
    return root


def children(node, key):
    return [x for x in node if isinstance(x, list) and x and x[0] == key]


def child(node, key):
    values = children(node, key)
    return values[0] if values else None


_cache = {}


def library_pins(library, name):
    if library not in _cache:
        _cache[library] = parse((KICAD / "symbols" / (library + ".kicad_sym")).read_text(encoding="utf-8"))
    nodes = {s[1]: s for s in children(_cache[library], "symbol")}
    node = nodes[name]
    parent = child(node, "extends")
    if parent:
        node = nodes[parent[1]]
    pins = {}
    for section in children(node, "symbol"):
        for pin in children(section, "pin"):
            pins[child(pin, "number")[1]] = [child(pin, "name")[1], pin[1]]
    return pins


def footprint(source):
    family, name = source.split(":", 1)
    local_name = family + "__" + name
    destination = LIB / "CANView.pretty" / (local_name + ".kicad_mod")
    destination.parent.mkdir(parents=True, exist_ok=True)
    origin = KICAD / "footprints" / (family + ".pretty") / (name + ".kicad_mod")
    data = origin.read_text(encoding="utf-8")
    # Embed all pad geometry locally. Optional 3D paths remain KiCad references.
    data = re.sub(r'\(footprint\s+"[^"]+"', '(footprint ' + q(local_name), data, count=1)
    destination.write_text(data, encoding="utf-8")
    return "CANView:" + local_name


class Circuit:
    def __init__(self, name):
        self.name = name
        self.sheets = {}
        self.parts = []
        self.counts = {}

    def sheet(self, name, note):
        self.sheets[name] = note
        return name

    def add(self, sheet, ref, value, fp, pins, *, mpn=None, spec="See manufacturer datasheet", source="", dnp=False, note=""):
        assert sheet in self.sheets
        assert ref not in {x['ref'] for x in self.parts}, ref
        pins = {str(k): v for k, v in pins.items()}
        assert all(len(v) == 3 for v in pins.values()), ref
        local_fp = footprint(fp) if not fp.startswith("CANView:") else fp
        item = dict(sheet=sheet, ref=ref, value=value, footprint=local_fp, original_footprint=fp,
                    pins=pins, mpn=mpn or value, spec=spec, source=source, dnp=dnp, note=note)
        self.parts.append(item)
        return item

    def automatic(self, prefix):
        self.counts[prefix] = self.counts.get(prefix, 0) + 1
        return prefix + str(self.counts[prefix])

    def two(self, sheet, prefix, value, a, b, *, fp, mpn, spec, source="", dnp=False, note=""):
        return self.add(sheet, self.automatic(prefix), value, fp,
                        {"1": ["1", "passive", a], "2": ["2", "passive", b]},
                        mpn=mpn, spec=spec, source=source, dnp=dnp, note=note)

    def r(self, sheet, value, a, b, *, dnp=False, power="0.1W", voltage="50V", tolerance="1%", size="0402", note="", mpn=None):
        return self.two(sheet, "R", value, a, b, fp=f"Resistor_SMD:R_{size}_{'1005' if size=='0402' else '1608' if size=='0603' else '2012'}Metric",
                        mpn=mpn or f"GEN-R-{size}-{value}-{tolerance}",
                        spec=f"{power}; {voltage}; ±{tolerance}; ±100ppm/K; -55..155C", dnp=dnp, note=note)

    def c(self, sheet, value, a, b="GND", *, voltage="16V", dielectric="X7R", tolerance="10%", size="0603", dnp=False, note="", mpn=None):
        return self.two(sheet, "C", value, a, b, fp=f"Capacitor_SMD:C_{size}_{'1005' if size=='0402' else '1608' if size=='0603' else '2012' if size=='0805' else '3216'}Metric",
                        mpn=mpn or f"GEN-C-{size}-{value}-{voltage}-{dielectric}",
                        spec=f"{voltage}; ±{tolerance}; {dielectric}; -55..125C; verify effective capacitance under DC bias", dnp=dnp, note=note)

    def ic(self, sheet, ref, value, fp, names, nets, *, types=None, **kw):
        types = types or {}
        if isinstance(names, list):
            names = {str(i+1): name for i, name in enumerate(names)}
        pins = {str(n): [name, types.get(str(n), "passive"), nets.get(str(n))] for n, name in names.items()}
        assert set(nets) <= set(pins), (ref, set(nets)-set(pins))
        return self.add(sheet, ref, value, fp, pins, **kw)

    def from_library(self, sheet, ref, value, fp, lib, name, nets, **kw):
        pins = library_pins(lib, name)
        assert set(nets) <= set(pins), (ref, set(nets)-set(pins))
        return self.add(sheet, ref, value, fp, {n: [*v, nets.get(n)] for n,v in pins.items()}, **kw)


def font(size=1.0, justify=""):
    return f'(effects (font (size {size} {size})) {"(justify " + justify + ")" if justify else ""})'


def prop(name, value, x, y, hidden=False):
    return f'(property {q(name)} {q(value)} (at {x:.4f} {y:.4f} 0) {"(hide yes)" if hidden else ""} {font()})'


def symbol(part):
    name = part['libname']
    pins = list(part['pins'].items())
    is_two = len(pins) == 2
    half = 1 if is_two else math.ceil(len(pins)/2)
    width = 5.08 if is_two else 20.32
    pitch = 5.08
    height = 2.54 if is_two else (half + 1) * pitch / 2
    geometry = f'(rectangle (start {-width} {height}) (end {width} {-height}) (stroke (width 0.254) (type default)) (fill (type background)))'
    if part['ref'].startswith('C') and is_two:
        geometry = ''.join(f'(polyline (pts (xy {x} -2.54) (xy {x} 2.54)) (stroke (width 0.254) (type default)) (fill (type none)))' for x in [-0.762,0.762])
        geometry += ''.join(f'(polyline (pts (xy {x} 0) (xy {y} 0)) (stroke (width 0.254) (type default)) (fill (type none)))' for x,y in [(-5.08,-0.762),(0.762,5.08)])
    result = [f'(symbol {q(name)} (pin_names (offset 0.762)) (in_bom yes) (on_board yes)',
              prop("Reference", part['ref'][0],0,height+5.08), prop("Value", part['value'],0,height+2.54),
              prop("Footprint",part['footprint'],0,0,True),
              f'(symbol {q(name + "_0_1")} {geometry})', f'(symbol {q(name + "_1_1")}']
    coords = {}
    for i, (number, (label, electrical, net)) in enumerate(pins):
        side = -1 if i < half else 1
        x = side * (width+5.08)
        y = 0 if is_two else ((half-1)/2 - i%half) * pitch
        angle = 0 if side < 0 else 180
        result.append(f'(pin {electrical} line (at {x} {y} {angle}) (length 5.08) (name {q(label)} {font(.9)}) (number {q(number)} {font(.9)}))')
        coords[number] = (x,y,side)
    result.append('))')
    return '\n'.join(result), coords, height


def generate(circuit):
    base = ROOT / 'hardware' / circuit.name
    dest = base / 'kicad'
    dest.mkdir(parents=True, exist_ok=True)
    root_id = uid(circuit.name)
    common = f'(kicad_sch (version 20250114) (generator "eeschema") (generator_version "10.0") (uuid "{{uuid}}") (paper "A3") (title_block (title "CANView / {{title}}") (date "2026-09-05") (rev "R1 REVIEW") (company "digitie/canview") (comment 1 "SCHEMATIC REVIEW ONLY - PCB/HIL release gates remain open"))'
    root = [common.format(uuid=root_id,title=circuit.name), '(lib_symbols)']
    definitions = []
    for page, (sheet, note) in enumerate(circuit.sheets.items(), start=2):
        sid = uid(circuit.name + '/sheet/' + sheet)
        filename = sheet + '.kicad_sch'
        sy = 35 + (page-2)%8 * 24
        sx = 30 + (page-2)//8 * 185
        root.append(f'(sheet (at {sx} {sy}) (size 160 17) (stroke (width 0.254) (type default)) (fill (color 0 0 0 0)) (uuid {q(sid)}) {prop("Sheetname",sheet,sx+80,sy-2)} {prop("Sheetfile",filename,sx+80,sy+20)} (instances (project {q(circuit.name)} (path {q("/"+root_id)} (page {q(page)})))))')
        selected = [p for p in circuit.parts if p['sheet']==sheet]
        for p in selected:
            p['libname'] = 'CANView_' + circuit.name + '_' + p['ref']
        symbols = {p['ref']: symbol(p) for p in selected}
        definitions.extend(v[0] for v in symbols.values())
        embedded = [re.sub(r'^\(symbol "', '(symbol "CANView:', v[0], count=1) for v in symbols.values()]
        content = [common.format(uuid=uid(circuit.name+'/file/'+sheet),title=sheet), '(lib_symbols ' + '\n'.join(embedded) + ')']
        content.append(f'(text {q(note)} (at 15.24 15.24 0) {font(1.2,"left")} (uuid {q(uid(sid+"note"))}))')
        # Large ICs are placed in distinct columns; support parts use a separate
        # compact bank. Stable grid positions keep generated revisions reviewable.
        large = [p for p in selected if len(p['pins'])>2]
        small = [p for p in selected if len(p['pins'])<=2]
        positions = {}
        col_heights = [35.56,35.56]
        for p in sorted(large,key=lambda p:-len(p['pins'])):
            col = min(range(2),key=lambda c:col_heights[c])
            height = symbols[p['ref']][2]
            positions[p['ref']] = (88.9 + col*165.1, col_heights[col]+height+10.16)
            col_heights[col] += 2*height+35.56
        start_y = max(col_heights) + 5.08
        if len(large)==1 and len(large[0]['pins'])>=30:
            # Tall MCUs use an adjacent support bank instead of an overflowing page.
            assert len(large)==1, (sheet, "split ICs onto another sheet")
            for i,p in enumerate(small):
                positions[p['ref']] = (213.36+(i%3)*76.2,40.64+(i//3)*20.32)
        else:
            for i,p in enumerate(small):
                positions[p['ref']] = (50.8+(i%4)*91.44,start_y+(i//4)*20.32)
        for p in selected:
            px,py = positions[p['ref']]
            _, coords, height = symbols[p['ref']]
            assert py + height < 274, (sheet,p['ref'],py,height,"page overflow; split sheet")
            instance = uid(circuit.name+'/'+p['ref'])
            content.append(f'(symbol (lib_id {q(p["libname"])}) (at {px:.4f} {py:.4f} 0) (unit 1) (in_bom yes) (on_board yes) (dnp {"yes" if p["dnp"] else "no"}) (uuid {q(instance)}) ' + prop('Reference',p['ref'],px,py-height-5.08) + prop('Value',p['value'],px,py-height-2.54) + prop('Footprint',p['footprint'],px,py,True) + prop('MPN',p['mpn'],px,py,True) + prop('Specification',p['spec'],px,py,True) + prop('Source',p['source'],px,py,True) + prop('Assembly', 'DNP' if p['dnp'] else 'FIT',px,py,True) + f'(instances (project {q(circuit.name)} (path {q("/"+root_id+"/"+sid)} (reference {q(p["ref"])}) (unit 1)))))')
            for number, (_, _, net) in p['pins'].items():
                rx,ry,side = coords[number]
                x,y = px+rx, py-ry
                wireid = uid(instance+'/'+number)
                if net is None:
                    content.append(f'(no_connect (at {x:.4f} {y:.4f}) (uuid {q(wireid)}))')
                else:
                    end = x + side*7.62
                    content.append(f'(wire (pts (xy {x:.4f} {y:.4f}) (xy {end:.4f} {y:.4f})) (stroke (width 0) (type default)) (uuid {q(wireid)}))')
                    # Local labels on single-pin component islands would not link
                    # across hierarchy; explicit global labels are intentional.
                    content.append(f'(global_label {q(net)} (shape bidirectional) (at {end:.4f} {y:.4f} {0 if side<0 else 180}) {font(.95, "left" if side<0 else "right")} (uuid {q(uid(wireid+"label"))}) {prop("Intersheetrefs","${INTERSHEET_REFS}",end,y,True)})')
        for p in selected:
            content = [line.replace('(lib_id ' + q(p['libname']) + ')', '(lib_id ' + q('CANView:' + p['libname']) + ')') for line in content]
        content.append(')')
        (dest/filename).write_text('\n'.join(content)+'\n',encoding='utf-8')
    root.extend([f'(sheet_instances (path "/" (page "1")))', ')'])
    (dest/(circuit.name+'.kicad_sch')).write_text('\n'.join(root)+'\n',encoding='utf-8')
    (dest/(circuit.name+'.kicad_pro')).write_text(json.dumps({'meta':{'version':1},'erc':{'erc_exclusions':[]}},indent=2)+'\n',encoding='utf-8')
    (dest/'fp-lib-table').write_text('(fp_lib_table (version 7) (lib (name "CANView") (type "KiCad") (uri "${KIPRJMOD}/../../libraries/CANView.pretty") (options "") (descr "Vendored KiCad 10 footprints and reviewed custom patterns")))\n',encoding='utf-8')
    (dest/'sym-lib-table').write_text('(sym_lib_table (version 7) (lib (name "CANView") (type "KiCad") (uri "${KIPRJMOD}/../../libraries/CANView.kicad_sym") (options "") (descr "CANView pin-verified functional symbols")))\n',encoding='utf-8')
    (base/'connectivity.json').write_text(json.dumps({'revision':'R1-review','board':circuit.name,'sheets':circuit.sheets,'components':circuit.parts},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    with (base/'bom.csv').open('w',newline='',encoding='utf-8-sig') as f:
        fields=['ref','value','mpn','footprint','spec','source','dnp','sheet','note']
        writer=csv.DictWriter(f,fieldnames=fields,extrasaction='ignore')
        writer.writeheader();writer.writerows(circuit.parts)
    with (base/'pinmap.csv').open('w',newline='',encoding='utf-8-sig') as f:
        writer=csv.writer(f);writer.writerow(['reference','mpn','pin','pin_name','electrical_type','net','sheet'])
        for p in circuit.parts:
            for n,(name,electrical,net) in p['pins'].items():
                writer.writerow([p['ref'],p['mpn'],n,name,electrical,net or 'NC',p['sheet']])
    return definitions
