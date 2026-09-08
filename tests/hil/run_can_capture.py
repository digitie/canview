#!/usr/bin/env python3
"""T-103 세 채널 capture-only host scenario 선택 실행기."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
if str(ROOT / "tests") not in sys.path:
    sys.path.insert(0, str(ROOT / "tests"))

from hil.run import main as run_main  # noqa: E402


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--channels", type=int, default=3)
    parser.add_argument("--mode", choices=("capture-only",), default="capture-only")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--output", type=Path, default=ROOT / "build" / "hil-t103-capture")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.channels != 3 or args.mode != "capture-only":
        print("BLOCKED: T-103 requires exactly three capture-only channels", file=sys.stderr)
        return 2
    return run_main([
        "--suite", "host",
        "--scenario", "can-load",
        "--seed", str(args.seed),
        "--output", str(args.output),
    ])


if __name__ == "__main__":
    raise SystemExit(main())
