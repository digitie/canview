"""Validate real KiCad exports, all named footprint pads and selected safety nets.

Run with KiCad's Windows Python (pcbnew). This is NOT electrical qualification.
"""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import sys
import subprocess
import xml.etree.ElementTree as ET
import pcbnew

ROOT = Path(__file__).resolve().parents[2]
BOARDS = ('communicator', 'bridge', 'controller-adapter', 'microphone')


def validate_board(board):
    base = ROOT / 'hardware' / board
    model = json.loads((base/'connectivity.json').read_text(encoding='utf-8'))
    parts = {p['ref']: p for p in model['components'] if not p['ref'].startswith('#')}
    xml = ET.parse(base/'netlist.xml').getroot()
    exported = {p.attrib['ref']: p for p in xml.findall('./components/comp')}
    errors = []
    def check(condition, message):
        if not condition: errors.append(message)
    check(set(parts) == set(exported), 'component membership drift')
    pins = {}
    for net in xml.findall('./nets/net'):
        for node in net.findall('node'):
            key = (node.attrib['ref'], node.attrib['pin'])
            check(key not in pins, f'duplicate exported pin {key}')
            pins[key] = net.attrib['name']
    pad_count = 0
    for ref, part in parts.items():
        check(bool(part['spec'] and part['mpn'] and part['footprint']), f'{ref}: missing BOM specification')
        if ref not in exported: continue
        check(exported[ref].findtext('footprint') == part['footprint'], f'{ref}: footprint drift')
        check(exported[ref].findtext('value') == part['value'], f'{ref}: value drift')
        name = part['footprint'].split(':', 1)[1]
        path = ROOT/'hardware/libraries/CANView.pretty'/f'{name}.kicad_mod'
        if not path.is_file():
            errors.append(f'{ref}: missing local footprint {name}')
            continue
        fp = pcbnew.FootprintLoad(str(path.parent), name)
        check(fp is not None, f'{ref}: KiCad cannot load footprint')
        if fp is None: continue
        pads = {p.GetNumber() for p in fp.Pads() if p.GetNumber()}
        pad_count += len(pads)
        check(pads == set(part['pins']), f'{ref}: pad mismatch footprint-only={sorted(pads-set(part["pins"]))}, symbol-only={sorted(set(part["pins"])-pads)}')
        for n, (_label, _type, net) in part['pins'].items():
            actual = pins.get((ref, n))
            if net is None:
                check(actual is None or actual.startswith('unconnected-'), f'{ref}.{n}: intended NC joined to {actual}')
            else:
                check(actual == net, f'{ref}.{n}: intended {net}, exported {actual}')
        if ref.startswith('R'):
            check(all(x in part['spec'] for x in ['W;', 'V;', '%;', 'ppm/K']), f'{ref}: resistor ratings incomplete')
        if ref.startswith('C'):
            check('V;' in part['spec'] and any(x in part['spec'] for x in ['C0G','X7R','X5R']), f'{ref}: capacitor rating/dielectric missing')
    with (base/'bom.csv').open(encoding='utf-8-sig', newline='') as stream:
        bom = list(csv.DictReader(stream))
    check({p['ref'] for p in bom} == set(parts), 'BOM membership drift')
    check(len(bom) == len(parts), 'duplicate BOM reference')
    for row in bom:
        check(row['footprint'] == parts[row['ref']]['footprint'], f'{row["ref"]}: CSV footprint drift')
    erc = json.loads((base/'erc.json').read_text(encoding='utf-8'))
    violations = [v for s in erc['sheets'] for v in s['violations']]
    check(not violations, f'ERC has {len(violations)} violations')
    if board == 'communicator':
        # Independent regression oracle for historically confused physical pins.
        expected = {
            ('U11','2'):'3V3', ('U11','3'):'ESP_RESET_N', ('U10','7'):'STM_RESET_N',
            ('U11','31'):'USB_SERVICE_SENSE', ('U11','39'):'STM_RESET_CMD_N',
            ('U11','38'):'STM_BOOT0_REQ', ('U11','17'):'STM_RECOVERY_N', ('U10','47'):'STM_RECOVERY_N',
            ('U6','2'):'GLOBAL_RESET_N', ('U17','2'):'GLOBAL_RESET_N', ('U50','4'):'STM_RESET_N',
            ('U36','3'):'RUN_ALLOWED', ('U31','3'):'RUN_ALLOWED',
            ('U11','25'):'SERVICE_RUN_SENSE', ('U56','2'):'SERVICE_RUN',
            ('U56','4'):'SERVICE_RUN_SENSE_SRC', ('U52','6'):'SERVICE_RUN', ('U54','2'):'SERVICE_RUN',
            ('U20','5'):'GND', ('U20','7'):'CAN2_RX_PHY', ('U20','8'):'CAN2_STB', ('U20','11'):'PHY3V3',
            ('U24','8'):'PHY3V3', ('U28','5'):'AUTO5V', ('U29','5'):'AUTO5V',
            ('U28','1'):'TX_PERMIT', ('U29','1'):'RX_ALLOWED',
            ('U32','5'):'ARM_LATCH', ('U32','6'):'ARM_CLEAR_N', ('U37','6'):'TX_ARM',
            ('U8','15'):'AUTO5V', ('U4','2'):'SYS5V', ('U4','3'):'AUTO5V', ('U4','6'):'USB_LIMITED',
            ('U40','18'):'GPS_RX_TAP', ('U40','20'):'GPS_PPS', ('U41','2'):'BARO_SCK',
        }
        for key, net in expected.items(): check(pins.get(key) == net, f'independent safety pin regression {key}: expected {net}, got {pins.get(key)}')
        check(parts['U7']['pins']['13'][2] is None, 'LM74800 exposed pad must float')
        check(parts['U11']['mpn']=='ESP32-S3-WROOM-1-N16R8', 'Communicator module must be N16R8')
        for pad in ['28','29','30']:
            check(parts['U11']['pins'][pad][2] is None, f'R8 PSRAM reserved pad {pad} must be NC')
    if board == 'bridge':
        check(pins.get(('D3','1')) == 'GND' and pins.get(('D3','2')) == 'LED_A', 'LED pad1=K / pad2=A regression')
    hashes = {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [base/'netlist.xml',base/f'{board}.net',base/'schematic.pdf',base/'connectivity.json']}
    return dict(board=board, physical_items=len(parts), named_pads=pad_count,
                provisional_footprint_refs=[p['ref'] for p in parts.values() if 'FOOTPRINT PROVISIONAL' in p['note']],
                erc_violations=len(violations), errors=errors, artifact_sha256=hashes)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--git-revision', help='Read-only verification of stored hashes against immutable Git blobs')
    parser.add_argument('--git-executable', default='git')
    args = parser.parse_args()
    if args.git_revision:
        def read(path):
            return subprocess.check_output([args.git_executable, '-C', str(ROOT), 'show', args.git_revision+':'+path])
        report = json.loads(read('hardware/validation.json'))
        errors = []
        for board in report['boards']:
            for name, expected in board['artifact_sha256'].items():
                path = 'hardware/'+board['board']+'/'+name
                if hashlib.sha256(read(path)).hexdigest() != expected:
                    errors.append(path)
        print(json.dumps(dict(revision=args.git_revision, git_blob_digest_mismatches=errors)))
        return int(bool(errors))
    results = [validate_board(b) for b in BOARDS]
    report = dict(status='PASS' if not any(r['errors'] for r in results) else 'FAIL',
                  scope='KiCad export/pad/BOM consistency only; no PCB, transient, thermal, RF or HIL approval',
                  boards=results)
    (ROOT/'hardware/validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(report,indent=2))
    return int(report['status'] != 'PASS')


if __name__ == '__main__': sys.exit(main())
