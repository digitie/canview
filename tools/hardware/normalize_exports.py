"""Keep generated review bytes identical across Windows and Git checkouts."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BOARDS = ('communicator', 'bridge', 'controller-adapter', 'microphone')


def main():
    count = 0
    for board in BOARDS:
        base = ROOT/'hardware'/board
        # Only known generated files. Never touch references, user captures,
        # historical inputs or binary PDFs/PNG/vendor documents.
        files = [base/name for name in ('connectivity.json', 'bom.csv', 'pinmap.csv', 'erc.json', 'netlist.xml', board+'.net')]
        for path in files:
            data = path.read_bytes()
            data.decode('utf-8-sig')
            if b'\r\n' in data:
                path.write_bytes(data.replace(b'\r\n', b'\n'))
                count += 1
    print(f'Canonical LF: normalized {count} generated text exports; binary originals unchanged')


if __name__ == '__main__':
    main()
