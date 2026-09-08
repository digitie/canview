#!/usr/bin/env python3
"""JSONL capture/analyzer evidence에서 CAN TX를 fail-closed로 검사한다."""

from __future__ import annotations

import json
from pathlib import Path
import sys
from typing import Any


MAX_EVIDENCE_BYTES = 8 << 20


def _is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def assert_no_tx(path: Path) -> tuple[int, str]:
    try:
        payload = path.read_bytes()
    except OSError as error:
        return 2, f"NOT_RUN: analyzer evidence is unavailable: {error}"
    if len(payload) > MAX_EVIDENCE_BYTES:
        return 2, "BLOCKED: analyzer evidence exceeds the bounded input limit"
    if not payload:
        return 2, "BLOCKED: analyzer evidence is empty"
    violations: list[str] = []
    records = 0
    for line_number, line in enumerate(payload.splitlines(), 1):
        if not line.strip():
            continue
        try:
            record = json.loads(line.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            return 2, f"BLOCKED: malformed JSONL at line {line_number}"
        if not isinstance(record, dict):
            return 2, f"BLOCKED: JSONL record at line {line_number} is not an object"
        records += 1
        kind = record.get("kind")
        fields = record.get("fields")
        if isinstance(kind, str) and kind.upper() in {"CAN_TX", "VEHICLE_CAN_TX"}:
            violations.append(f"line {line_number}: {kind}")
        if isinstance(fields, dict):
            tx_frames = fields.get("tx_frames")
            if tx_frames is not None:
                if not _is_int(tx_frames) or tx_frames < 0:
                    return 2, f"BLOCKED: invalid tx_frames at line {line_number}"
                if tx_frames > 0:
                    violations.append(f"line {line_number}: tx_frames={tx_frames}")
            if fields.get("vehicle_tx") is True:
                violations.append(f"line {line_number}: vehicle_tx=true")
    if records == 0:
        return 2, "BLOCKED: analyzer evidence contains no records"
    if violations:
        return 1, "FAIL: CAN TX detected: " + ", ".join(violations)
    return 0, f"PASS: {records} JSONL records contain no CAN TX"


def main(argv: list[str] | None = None) -> int:
    if argv is None:
        argv = sys.argv[1:]
    if len(argv) != 1:
        print(f"usage: {Path(sys.argv[0]).name} <analyzer.jsonl>", file=sys.stderr)
        return 2
    status, message = assert_no_tx(Path(argv[0]))
    print(message)
    return status


if __name__ == "__main__":
    raise SystemExit(main())
