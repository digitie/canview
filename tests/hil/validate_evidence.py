#!/usr/bin/env python3
"""Validate machine-readable T-500 evidence without hardware assumptions."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys
from typing import Any


if __package__ in {None, ""}:
    TESTS_ROOT = Path(__file__).resolve().parents[1]
    if str(TESTS_ROOT) not in sys.path:
        sys.path.insert(0, str(TESTS_ROOT))

from hil.events import EventLogError, read_jsonl


VALID_STATUSES = {"PASS", "FAIL", "SKIPPED", "BLOCKED"}
SHA256 = re.compile(r"^[0-9a-f]{64}$")
GIT_SHA = re.compile(r"^[0-9a-f]{40}$")


class EvidenceError(ValueError):
    """evidence contract 위반."""


def _read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise EvidenceError(f"invalid JSON report {path}: {error}") from error
    if not isinstance(value, dict):
        raise EvidenceError("report root must be an object")
    return value


def _is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _validate_events(path: Path, expected_count: int) -> list[dict[str, Any]]:
    try:
        records = read_jsonl(path)
    except EventLogError as error:
        raise EvidenceError(str(error)) from error
    try:
        raw = path.read_bytes()
    except OSError as error:
        raise EvidenceError(f"unable to read event bytes for {path}: {error}") from error
    raw_lines = raw.splitlines(keepends=True)
    if raw and not raw.endswith(b"\n"):
        raise EvidenceError(f"event log has no final newline: {path}")
    if len(raw_lines) != len(records):
        raise EvidenceError(f"event line count mismatch for {path}")
    if len(records) != expected_count:
        raise EvidenceError(f"event count mismatch for {path}")
    previous_time: int | None = None
    expected_offset = 0
    for index, record in enumerate(records, 1):
        if record.get("schema_version") != 1 or record.get("sequence") != index:
            raise EvidenceError(f"event sequence/schema mismatch at {path}:{index}")
        if (not isinstance(record.get("source"), str)
                or not record["source"]
                or not isinstance(record.get("kind"), str)
                or not record["kind"]):
            raise EvidenceError(f"event source/kind is invalid at {path}:{index}")
        if not isinstance(record.get("fields"), dict):
            raise EvidenceError(f"event fields are not an object at {path}:{index}")
        log_offset = record.get("log_offset")
        if not _is_int(log_offset) or log_offset < 0:
            raise EvidenceError(f"invalid event offset at {path}:{index}")
        if log_offset != expected_offset:
            raise EvidenceError(f"event offset does not match bytes at {path}:{index}")
        expected_offset += len(raw_lines[index - 1])
        monotonic_ns = record.get("monotonic_ns")
        if not _is_int(monotonic_ns) or monotonic_ns < 0:
            raise EvidenceError(f"invalid event time at {path}:{index}")
        if previous_time is not None and monotonic_ns < previous_time:
            raise EvidenceError(f"event time moved backwards at {path}:{index}")
        previous_time = monotonic_ns
    return records


def validate(report_path: Path, expected_status: str | None = None) -> dict[str, Any]:
    report = _read_json(report_path)
    if report.get("schema_version") != 1:
        raise EvidenceError("unsupported report schema")
    status = report.get("status")
    if status not in VALID_STATUSES:
        raise EvidenceError(f"invalid report status: {status!r}")
    if expected_status is not None and status != expected_status:
        raise EvidenceError(f"expected {expected_status}, got {status}")
    seed = report.get("seed")
    if not _is_int(seed) or seed < 0 or seed > (1 << 64) - 1:
        raise EvidenceError("invalid seed")
    firmware = report.get("firmware")
    if (not isinstance(firmware, dict)
            or not isinstance(firmware.get("source_sha256"), str)
            or not SHA256.fullmatch(firmware["source_sha256"])):
        raise EvidenceError("firmware source digest is missing")
    harness = report.get("harness")
    if (not isinstance(harness, dict)
            or not isinstance(harness.get("version"), str)
            or not harness["version"]
            or not isinstance(harness.get("source_sha256"), str)
            or not SHA256.fullmatch(harness["source_sha256"])):
        raise EvidenceError("harness identity is missing")
    suite = report.get("suite")
    if suite not in {"host", "g2-readonly"}:
        raise EvidenceError(f"invalid suite: {suite!r}")
    physical = report.get("physical_hil")
    if not isinstance(physical, dict) or physical.get("status") not in VALID_STATUSES | {"NOT_RUN"}:
        raise EvidenceError("physical_hil status is invalid")
    scenario_results = report.get("scenario_results")
    if not isinstance(scenario_results, list):
        raise EvidenceError("scenario_results must be a list")
    if status == "PASS" and not scenario_results:
        raise EvidenceError("PASS report has no scenario results")
    if suite == "host" and physical.get("status") == "PASS":
        raise EvidenceError("host run cannot claim physical PASS")
    if suite == "host" and status == "PASS" and physical.get("status") != "NOT_RUN":
        raise EvidenceError("host PASS must keep physical_hil as NOT_RUN")
    seen_ids: set[str] = set()
    for item in scenario_results:
        if not isinstance(item, dict) or not isinstance(item.get("id"), str):
            raise EvidenceError("invalid scenario result")
        scenario_id = item["id"]
        if scenario_id in seen_ids:
            raise EvidenceError(f"duplicate scenario result: {scenario_id}")
        seen_ids.add(scenario_id)
        item_status = item.get("status")
        if item_status not in {"PASS", "FAIL"}:
            raise EvidenceError(f"invalid scenario status: {scenario_id}")
        scenario_digest = item.get("scenario_sha256")
        if (not isinstance(scenario_digest, str)
                or not SHA256.fullmatch(scenario_digest)):
            raise EvidenceError(f"scenario digest is missing: {scenario_id}")
        scenario_seed = item.get("seed")
        if (not _is_int(scenario_seed)
                or scenario_seed < 0
                or scenario_seed > (1 << 64) - 1):
            raise EvidenceError(f"invalid scenario seed: {scenario_id}")
        if not isinstance(item.get("source"), str) or not item["source"]:
            raise EvidenceError(f"scenario source is missing: {scenario_id}")
        if not isinstance(item.get("adapter"), str) or not item["adapter"]:
            raise EvidenceError(f"scenario adapter is missing: {scenario_id}")
        events_file = item.get("events_file")
        if not isinstance(events_file, str) or Path(events_file).is_absolute():
            raise EvidenceError(f"invalid events_file: {scenario_id}")
        event_path = (report_path.parent / events_file).resolve()
        if report_path.parent.resolve() not in event_path.parents:
            raise EvidenceError(f"events_file escapes report directory: {scenario_id}")
        event_count = item.get("event_count")
        if not _is_int(event_count) or event_count < 0:
            raise EvidenceError(f"invalid event_count: {scenario_id}")
        records = _validate_events(event_path, event_count)
        event_bytes = item.get("event_bytes")
        if not _is_int(event_bytes) or event_bytes < 0:
            raise EvidenceError(f"invalid event_bytes: {scenario_id}")
        if event_bytes != event_path.stat().st_size:
            raise EvidenceError(f"event byte count mismatch: {scenario_id}")
        tx = [record for record in records
              if str(record.get("kind", "")).upper() in {"CAN_TX", "VEHICLE_CAN_TX"}]
        if tx:
            raise EvidenceError(f"capture-only TX event in {scenario_id}")
        violations = item.get("violations")
        first = item.get("first_violation")
        if not isinstance(violations, list):
            raise EvidenceError(f"violations missing: {scenario_id}")
        for violation in violations:
            if (not isinstance(violation, dict)
                    or not isinstance(violation.get("invariant"), str)
                    or not _is_int(violation.get("log_offset"))
                    or violation["log_offset"] < 0):
                raise EvidenceError(f"invalid violation: {scenario_id}")
        if item_status == "PASS" and (violations or first is not None):
            raise EvidenceError(f"PASS scenario has a violation: {scenario_id}")
        if item_status == "FAIL":
            if (not isinstance(first, dict)
                    or not isinstance(first.get("invariant"), str)
                    or not _is_int(first.get("log_offset"))
                    or first["log_offset"] < 0):
                raise EvidenceError(f"FAIL scenario has no first violation: {scenario_id}")
            if first not in violations:
                raise EvidenceError(f"FAIL scenario first violation is not preserved: {scenario_id}")
    if status == "PASS":
        inventory = report.get("scenario_inventory")
        if (not isinstance(inventory, dict)
                or inventory.get("count") != len(scenario_results)
                or inventory.get("ids") != [item["id"] for item in scenario_results]):
            raise EvidenceError("scenario inventory does not match results")
    if status == "PASS" and any(item.get("status") != "PASS" for item in scenario_results):
        raise EvidenceError("PASS report contains failed scenario")
    if status == "FAIL" and not any(item.get("status") == "FAIL"
                                     for item in scenario_results):
        raise EvidenceError("FAIL report contains no failed scenario")
    return report


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", type=Path,
                        help="report.json or a directory containing report.json")
    parser.add_argument("--expect-status", choices=sorted(VALID_STATUSES))
    args = parser.parse_args(argv)
    report_path = args.path / "report.json" if args.path.is_dir() else args.path
    try:
        report = validate(report_path.resolve(), args.expect_status)
    except (EvidenceError, OSError) as error:
        print(f"FAIL evidence={report_path} reason={error}", file=sys.stderr)
        return 1
    print(f"PASS evidence={report_path} suite={report.get('suite')} "
          f"status={report.get('status')} scenarios={len(report['scenario_results'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
