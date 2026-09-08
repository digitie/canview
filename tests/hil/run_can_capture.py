#!/usr/bin/env python3
"""T-103 세 채널 capture-only host scenario 선택 실행기."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
if str(ROOT / "tests") not in sys.path:
    sys.path.insert(0, str(ROOT / "tests"))

from hil.adapter import HOST_EVENT_SOURCE  # noqa: E402
from hil.assert_no_tx import assert_no_tx  # noqa: E402
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
    result = run_main([
        "--suite", "host",
        "--scenario", "can-load",
        "--seed", str(args.seed),
        "--output", str(args.output),
    ])
    if result != 0:
        return result
    output = Path(args.output).absolute()
    try:
        report = json.loads((output / "report.json").read_text(encoding="utf-8"))
        event_identity = report["event_identity"]
        firmware = report["firmware"]
        scenario_results = report["scenario_results"]
        if (not isinstance(event_identity, dict)
                or not isinstance(firmware, dict)
                or not isinstance(scenario_results, list)
                or len(scenario_results) != 1
                or not isinstance(scenario_results[0], dict)
                or report.get("status") != "PASS"
                or scenario_results[0].get("id") != "can-load"
                or event_identity.get("firmware_identity") != firmware.get("source_sha256")):
            raise ValueError("capture report identity or selection is invalid")
        event_path = (output / scenario_results[0]["events_file"]).resolve()
        if output.resolve() not in event_path.parents:
            raise ValueError("capture event path escapes output directory")
        status, message = assert_no_tx(
            event_path,
            expected_source=HOST_EVENT_SOURCE,
            expected_execution_id=event_identity["execution_id"],
            expected_firmware_identity=event_identity["firmware_identity"],
        )
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"BLOCKED: capture-only evidence contract: {error}", file=sys.stderr)
        return 2
    print(message)
    return status


if __name__ == "__main__":
    raise SystemExit(main())
