"""Extract local PDF evidence or render a specified page with bundled PyMuPDF."""
import argparse
from pathlib import Path
import sys

sys.stdout.reconfigure(encoding="utf-8")

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / ".tools/hardware-python"))
import fitz

parser = argparse.ArgumentParser()
parser.add_argument("pdf")
parser.add_argument("--find")
parser.add_argument("--page", type=int, help="1-based PDF page")
parser.add_argument("--render")
args = parser.parse_args()
doc = fitz.open(args.pdf)
for index, page in enumerate(doc):
    if args.page and args.page != index + 1:
        continue
    content = page.get_text()
    if args.find and args.find.lower() not in content.lower():
        continue
    print(f"=== PDF PAGE {index+1}/{len(doc)} ===\n{content}")
    if args.render:
        page.get_pixmap(matrix=fitz.Matrix(1.5, 1.5)).save(args.render)
