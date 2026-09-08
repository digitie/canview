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

from hil.adapter import HOST_ADAPTER_VERSION, LAB_ADAPTER_VERSION
from hil.analyze import analyze
from hil.events import MAX_EVENT_COUNT, EventLogError, read_jsonl
from hil.run import (BUDGET_PATH, RUNNER_VERSION, SCENARIO_DIR,
                     _path_label, _seed_for_scenario, firmware_identity,
                     harness_identity, load_budget)
from hil.scenario import ScenarioError, load_scenarios


VALID_STATUSES = {"PASS", "FAIL", "SKIPPED", "BLOCKED"}
SHA256 = re.compile(r"^[0-9a-f]{64}$")
GIT_SHA = re.compile(r"^[0-9a-f]{40}$")
MAX_REPORT_BYTES = 8 << 20


class EvidenceError(ValueError):
    """evidence contract 위반."""


def _read_json(path: Path) -> dict[str, Any]:
    try:
        if path.stat().st_size > MAX_REPORT_BYTES:
            raise EvidenceError("report is too large")
        value = json.loads(path.read_text(encoding="utf-8"),
                           object_pairs_hook=_object_pairs,
                           parse_constant=_reject_constant)
    except OSError as error:
        raise EvidenceError(f"invalid JSON report {path}: {error}") from error
    except EvidenceError:
        raise
    except (UnicodeError, json.JSONDecodeError, ValueError) as error:
        raise EvidenceError(f"invalid JSON report {path}: {error}") from error
    if not isinstance(value, dict):
        raise EvidenceError("report root must be an object")
    return value


def _is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _reject_constant(value: str) -> Any:
    raise ValueError(f"non-finite JSON number is not allowed: {value}")


def _object_pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate object key: {key}")
        result[key] = value
    return result


def _validate_events(path: Path, expected_count: int) -> list[dict[str, Any]]:
    if (not _is_int(expected_count) or expected_count <= 0
            or expected_count > MAX_EVENT_COUNT):
        raise EvidenceError(f"invalid event count for {path}")
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
    if report.get("runner_version") != RUNNER_VERSION:
        raise EvidenceError("unsupported runner version")
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
            or not SHA256.fullmatch(firmware["source_sha256"])
            or not isinstance(firmware.get("git_commit"), str)
            or not GIT_SHA.fullmatch(firmware["git_commit"])):
        raise EvidenceError("firmware source digest is missing")
    harness = report.get("harness")
    if (not isinstance(harness, dict)
            or not isinstance(harness.get("version"), str)
            or not harness["version"]
            or not isinstance(harness.get("source_sha256"), str)
            or not SHA256.fullmatch(harness["source_sha256"])):
        raise EvidenceError("harness identity is missing")
    try:
        if firmware != firmware_identity():
            raise EvidenceError("firmware identity does not match checkout")
        if harness != harness_identity():
            raise EvidenceError("harness identity does not match checkout")
    except OSError as error:
        raise EvidenceError("unable to compute source identity") from error
    suite = report.get("suite")
    if suite not in {"host", "g2-readonly"}:
        raise EvidenceError(f"invalid suite: {suite!r}")
    physical = report.get("physical_hil")
    if not isinstance(physical, dict) or physical.get("status") not in VALID_STATUSES | {"NOT_RUN"}:
        raise EvidenceError("physical_hil status is invalid")
    if physical.get("status") == "PASS":
        raise EvidenceError("T-500 validator cannot certify physical PASS")
    scenario_results = report.get("scenario_results")
    if not isinstance(scenario_results, list):
        raise EvidenceError("scenario_results must be a list")
    if status == "PASS" and not scenario_results:
        raise EvidenceError("PASS report has no scenario results")
    if suite == "host" and status == "PASS" and physical.get("status") != "NOT_RUN":
        raise EvidenceError("host PASS must keep physical_hil as NOT_RUN")
    adapter = report.get("adapter")
    if not isinstance(adapter, dict) or adapter.get("connected") is not False:
        raise EvidenceError("adapter connection state is invalid")
    expected_adapter = HOST_ADAPTER_VERSION if suite == "host" else LAB_ADAPTER_VERSION
    if adapter.get("name") != expected_adapter:
        raise EvidenceError("adapter identity does not match suite")
    if suite == "host" and adapter.get("hardware_execution") is not False:
        raise EvidenceError("host adapter cannot claim hardware execution")
    if suite == "g2-readonly" and status == "PASS":
        raise EvidenceError("g2-readonly has no connected backend")
    if status in {"BLOCKED", "SKIPPED"}:
        failure = report.get("failure")
        reason = failure.get("reason") if isinstance(failure, dict) else None
        if not isinstance(reason, str) or not reason:
            raise EvidenceError("blocked/skipped report has no failure reason")
        if scenario_results:
            raise EvidenceError("blocked/skipped report must have no scenario results")
        return report
    if not isinstance(report.get("generated_at_utc"), str):
        raise EvidenceError("generated_at_utc is missing")

    try:
        budget = load_budget()
        trusted_scenarios = {
            item.scenario_id: item
            for item in load_scenarios(SCENARIO_DIR, suite)
        }
    except (OSError, ScenarioError) as error:
        raise EvidenceError(f"trusted scenario inventory unavailable: {error}") from error

    if status == "PASS":
        inventory = report.get("scenario_inventory")
        expected_ids = list(trusted_scenarios)
        if (not isinstance(inventory, dict)
                or inventory.get("directory") != _path_label(SCENARIO_DIR)
                or inventory.get("count") != len(expected_ids)
                or inventory.get("ids") != expected_ids):
            raise EvidenceError("scenario inventory does not match trusted inventory")
    budget_manifest = report.get("budget_manifest")
    if scenario_results and budget_manifest != _path_label(BUDGET_PATH):
        raise EvidenceError("budget manifest identity is missing")

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
        metrics = item.get("metrics")
        if (not isinstance(metrics, dict)
                or any(not isinstance(key, str) or not _is_int(value)
                       for key, value in metrics.items())):
            raise EvidenceError(f"scenario metrics are invalid: {scenario_id}")
        events_file = item.get("events_file")
        if not isinstance(events_file, str) or Path(events_file).is_absolute():
            raise EvidenceError(f"invalid events_file: {scenario_id}")
        event_path = (report_path.parent / events_file).resolve()
        if report_path.parent.resolve() not in event_path.parents:
            raise EvidenceError(f"events_file escapes report directory: {scenario_id}")
        event_count = item.get("event_count")
        if not _is_int(event_count) or event_count <= 0:
            raise EvidenceError(f"invalid event_count: {scenario_id}")
        records = _validate_events(event_path, event_count)
        event_bytes = item.get("event_bytes")
        if not _is_int(event_bytes) or event_bytes < 0:
            raise EvidenceError(f"invalid event_bytes: {scenario_id}")
        if event_bytes != event_path.stat().st_size:
            raise EvidenceError(f"event byte count mismatch: {scenario_id}")
        tx = []
        invalid_tx_counts = []
        for record in records:
            kind = str(record.get("kind", "")).upper()
            if kind in {"CAN_TX", "VEHICLE_CAN_TX"}:
                tx.append(record)
            elif record.get("kind") == "CAN_CHANNEL_SUMMARY":
                tx_frames = record["fields"].get("tx_frames")
                if not _is_int(tx_frames) or tx_frames < 0:
                    invalid_tx_counts.append(record)
                elif tx_frames > 0:
                    tx.append(record)
        if item_status == "PASS" and (tx or invalid_tx_counts):
            raise EvidenceError(f"capture-only TX event in {scenario_id}")
        violations = item.get("violations")
        first = item.get("first_violation")
        if not isinstance(violations, list):
            raise EvidenceError(f"violations missing: {scenario_id}")
        for violation in violations:
            if (not isinstance(violation, dict)
                    or not isinstance(violation.get("invariant"), str)
                    or not _is_int(violation.get("log_offset"))
                    or violation["log_offset"] < 0
                    or not isinstance(violation.get("message"), str)
                    or (violation.get("event_sequence") is not None
                        and (not _is_int(violation.get("event_sequence"))
                             or violation["event_sequence"] < 1))):
                raise EvidenceError(f"invalid violation: {scenario_id}")
        checks = item.get("checks")
        if (not isinstance(checks, list)
                or any(not isinstance(check, dict)
                       or not isinstance(check.get("invariant"), str)
                       or not isinstance(check.get("passed"), bool)
                       for check in checks)):
            raise EvidenceError(f"checks missing or invalid: {scenario_id}")
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
        trusted = trusted_scenarios.get(scenario_id)
        if trusted is None:
            if item_status == "PASS":
                raise EvidenceError(f"PASS scenario is not trusted: {scenario_id}")
            if tx or invalid_tx_counts:
                if not isinstance(first, dict) or first.get("invariant") not in {
                        "capture_only.can_tx_zero", "capture_only.tx_count_valid"}:
                    raise EvidenceError(f"forbidden TX failure is not recorded: {scenario_id}")
            continue
        if (scenario_digest != trusted.digest
                or scenario_seed != _seed_for_scenario(seed, scenario_id)
                or item.get("source") != _path_label(trusted.source_path)
                or item.get("adapter") != HOST_ADAPTER_VERSION):
            raise EvidenceError(f"scenario identity does not match trusted source: {scenario_id}")
        expected = analyze(trusted, records, metrics, budget)
        if (item_status != expected["status"]
                or checks != expected["checks"]
                or violations != expected["violations"]
                or first != expected["first_violation"]):
            raise EvidenceError(f"scenario result does not match recomputed verdict: {scenario_id}")
    if status == "PASS" and any(item.get("status") != "PASS" for item in scenario_results):
        raise EvidenceError("PASS report contains failed scenario")
    if status == "PASS" and [item["id"] for item in scenario_results] != expected_ids:
        raise EvidenceError("PASS report does not contain the complete trusted inventory")
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
