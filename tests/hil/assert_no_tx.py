#!/usr/bin/env python3
"""JSONL capture evidence를 bounded하게 읽고 CAN TX를 fail-closed로 검사한다."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any, BinaryIO


MAX_EVIDENCE_BYTES = 8 << 20
MAX_RECORD_BYTES = 1 << 20
MAX_INTEGER_DIGITS = 20
MAX_IDENTITY_LENGTH = 128
UINT64_MAX = (1 << 64) - 1
REQUIRED_RECORD_KEYS = {
    "schema_version",
    "source",
    "kind",
    "sequence",
    "monotonic_ns",
    "log_offset",
    "fields",
}
COMMON_IDENTITY_FIELDS = frozenset({"execution_id", "firmware_identity"})
EVENT_FIELD_ALLOWLIST = {
    "RADIO_SUMMARY": COMMON_IDENTITY_FIELDS | {
        "loss_percent", "sent", "delivered", "dropped", "duplicate_deliveries", "reordered",
    },
    "RADIO_DELAY_SUMMARY": COMMON_IDENTITY_FIELDS | {"max_delay_ms"},
    "RESET_REQUEST": COMMON_IDENTITY_FIELDS | {"target", "reason"},
    "BOOT_EPOCH": COMMON_IDENTITY_FIELDS | {"target", "epoch"},
    "UART_FAULT_REJECTED": COMMON_IDENTITY_FIELDS | {"fault", "parser_state"},
    "CAN_CHANNEL_SUMMARY": COMMON_IDENTITY_FIELDS | {
        "channel", "rx_frames", "tx_frames", "ack_frames", "bus_state", "error_counter",
    },
    "RESOURCE_SUMMARY": COMMON_IDENTITY_FIELDS | {
        "pool", "queue_depth", "heap_free_bytes", "rejected", "observer_drops",
    },
    "SAFETY_DECISION": COMMON_IDENTITY_FIELDS | {"check", "decision", "vehicle_tx", "reason"},
    "COMMAND_REPLAY": COMMON_IDENTITY_FIELDS | {"request_token", "executed", "result"},
    "FEEDBACK_SEQUENCE": COMMON_IDENTITY_FIELDS | {"case", "sequence"},
    "FEEDBACK_RESULT": COMMON_IDENTITY_FIELDS | {"case", "result", "tx_permitted"},
    "POWER_EVENT": COMMON_IDENTITY_FIELDS | {"stage", "tx_gate", "capture_state"},
    "SECURITY_REJECT": COMMON_IDENTITY_FIELDS | {"vector", "accepted"},
    "GUARDIAN_TIMEOUT": COMMON_IDENTITY_FIELDS | {"guardian", "tx_gate", "reset_requested"},
    "RADIO_BUDGET": COMMON_IDENTITY_FIELDS | {
        "softap_kbps", "observer_kbps", "control_kbps", "rssi_dbm", "overflow_policy",
    },
    "CAN_RX": COMMON_IDENTITY_FIELDS | {"channel", "frame_count", "capture_only"},
    "BUDGET_SAMPLE": COMMON_IDENTITY_FIELDS | {"metrics"},
    "UNKNOWN_ACTION_REJECTED": COMMON_IDENTITY_FIELDS | {"action_type"},
    "TX_GATE_STATE": COMMON_IDENTITY_FIELDS | {"vehicle_tx", "mode"},
    "HARNESS_COMPLETE": COMMON_IDENTITY_FIELDS | {"scenario", "firmware_mode"},
}
REQUIRED_EVENT_FIELDS = {
    "CAN_CHANNEL_SUMMARY": {"channel", "rx_frames", "tx_frames", "ack_frames", "bus_state"},
    "CAN_RX": {"channel", "frame_count", "capture_only"},
    "TX_GATE_STATE": {"vehicle_tx", "mode"},
    "HARNESS_COMPLETE": {"scenario", "firmware_mode"},
}
FORBIDDEN_KINDS = frozenset({"CAN_TX", "VEHICLE_CAN_TX"})


class _DuplicateKey(ValueError):
    """JSON object에 중복 key가 있는 경우."""


def _reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    record: dict[str, Any] = {}
    for key, value in pairs:
        if key in record:
            raise _DuplicateKey(f"duplicate key: {key}")
        record[key] = value
    return record


def _bounded_int(value: str) -> int:
    digits = value.lstrip("-")
    if len(digits) > MAX_INTEGER_DIGITS:
        raise ValueError("integer is too wide")
    parsed = int(value)
    if parsed < 0 or parsed > UINT64_MAX:
        raise ValueError("integer is outside the unsigned 64-bit range")
    return parsed


def _reject_float(_: str) -> float:
    raise ValueError("floating-point values are not part of the evidence schema")


def _reject_constant(value: str) -> str:
    raise ValueError(f"non-finite JSON constant: {value}")


def _is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _require_bounded_identity(value: Any, expected: str, label: str) -> None:
    if (not isinstance(value, str) or not value or len(value) > MAX_IDENTITY_LENGTH
            or value != expected):
        raise ValueError(f"{label} does not match the expected execution identity")


def _require_nonnegative_int(fields: dict[str, Any], name: str) -> None:
    value = fields.get(name)
    if not _is_int(value) or value < 0:
        raise ValueError(f"{name} is not a non-negative integer")


def _validate_event_fields(kind: str, fields: dict[str, Any]) -> None:
    allowed = EVENT_FIELD_ALLOWLIST.get(kind)
    if allowed is None:
        if kind not in FORBIDDEN_KINDS:
            raise ValueError(f"unknown evidence event kind: {kind}")
    elif not set(fields).issubset(allowed):
        raise ValueError(f"unknown fields for evidence event kind: {kind}")
    required = REQUIRED_EVENT_FIELDS.get(kind, set())
    if not required.issubset(fields):
        raise ValueError(f"required fields are missing for evidence event kind: {kind}")
    if kind == "CAN_CHANNEL_SUMMARY":
        channel = fields.get("channel")
        if not _is_int(channel) or channel not in {1, 2, 3}:
            raise ValueError("invalid CAN channel")
        for name in ("rx_frames", "tx_frames", "ack_frames"):
            _require_nonnegative_int(fields, name)
        if not isinstance(fields.get("bus_state"), str) or not fields["bus_state"]:
            raise ValueError("invalid CAN bus state")
        if "error_counter" in fields:
            _require_nonnegative_int(fields, "error_counter")
    elif kind == "CAN_RX":
        channel = fields.get("channel")
        if not _is_int(channel) or channel not in {1, 2, 3}:
            raise ValueError("invalid CAN RX channel")
        _require_nonnegative_int(fields, "frame_count")
        if fields.get("capture_only") is not True:
            raise ValueError("CAN RX event is not capture-only")
    elif kind == "TX_GATE_STATE":
        if fields.get("vehicle_tx") is not False or fields.get("mode") != "CAPTURE_ONLY":
            raise ValueError("invalid capture-only gate state")
    elif kind == "HARNESS_COMPLETE":
        if (not isinstance(fields.get("scenario"), str) or not fields["scenario"]
                or fields.get("firmware_mode") != "CAPTURE_ONLY"):
            raise ValueError("invalid capture-only completion")


def _decode_record(line: bytes, expected_source: str, expected_execution_id: str,
                   expected_firmware_identity: str) -> dict[str, Any]:
    record = json.loads(
        line.decode("utf-8"),
        object_pairs_hook=_reject_duplicate_keys,
        parse_int=_bounded_int,
        parse_float=_reject_float,
        parse_constant=_reject_constant,
    )
    if not isinstance(record, dict):
        raise ValueError("record is not an object")
    if set(record) != REQUIRED_RECORD_KEYS:
        raise ValueError("record schema keys are incomplete or unknown")
    if record.get("schema_version") != 1:
        raise ValueError("unsupported schema_version")
    if record.get("source") != expected_source:
        raise ValueError("source does not match the expected execution identity")
    if not isinstance(record.get("kind"), str) or not record["kind"]:
        raise ValueError("kind is not a non-empty string")
    if not _is_int(record.get("sequence")) or record["sequence"] == 0:
        raise ValueError("sequence is not a positive integer")
    if not _is_int(record.get("monotonic_ns")):
        raise ValueError("monotonic_ns is not an integer")
    if not _is_int(record.get("log_offset")):
        raise ValueError("log_offset is not an integer")
    if not isinstance(record.get("fields"), dict):
        raise ValueError("fields is not an object")
    fields = record["fields"]
    _require_bounded_identity(fields.get("execution_id"), expected_execution_id, "execution_id")
    _require_bounded_identity(fields.get("firmware_identity"), expected_firmware_identity,
                              "firmware_identity")
    _validate_event_fields(record["kind"], fields)
    return record


def _bounded_lines(stream: BinaryIO):
    total = 0
    while True:
        line = stream.readline(MAX_RECORD_BYTES + 1)
        if not line:
            return
        total += len(line)
        if total > MAX_EVIDENCE_BYTES:
            raise ValueError("evidence exceeds the bounded input limit")
        if len(line) > MAX_RECORD_BYTES:
            raise ValueError("JSONL record exceeds the bounded line limit")
        if not line.endswith(b"\n"):
            raise ValueError("final JSONL record is truncated")
        yield line


def assert_no_tx(path: Path, *, expected_source: str | None = None,
                 expected_execution_id: str | None = None,
                 expected_firmware_identity: str | None = None) -> tuple[int, str]:
    expected_identity = (expected_source, expected_execution_id, expected_firmware_identity)
    if any(not isinstance(value, str) or not value or len(value) > MAX_IDENTITY_LENGTH
           for value in expected_identity):
        return 2, "BLOCKED: expected source, execution ID, and firmware identity are required"
    try:
        with path.open("rb") as evidence:
            records = 0
            previous_sequence = 0
            previous_monotonic_ns = 0
            previous_log_offset = -1
            channel_summaries = 0
            channels_seen: set[int] = set()
            second_last_kind = ""
            second_last_fields: dict[str, Any] = {}
            last_kind = ""
            last_fields: dict[str, Any] = {}
            violations: list[str] = []
            for line_number, line in enumerate(_bounded_lines(evidence), 1):
                try:
                    record = _decode_record(line, expected_source, expected_execution_id,
                                            expected_firmware_identity)
                except (UnicodeDecodeError, json.JSONDecodeError, RecursionError, ValueError):
                    return 2, f"BLOCKED: invalid or incomplete evidence at line {line_number}"

                sequence = record["sequence"]
                monotonic_ns = record["monotonic_ns"]
                log_offset = record["log_offset"]
                if sequence != previous_sequence + 1:
                    return 2, f"BLOCKED: non-contiguous sequence at line {line_number}"
                if records != 0 and monotonic_ns < previous_monotonic_ns:
                    return 2, f"BLOCKED: monotonic time moved backwards at line {line_number}"
                if records != 0 and log_offset <= previous_log_offset:
                    return 2, f"BLOCKED: log offset moved backwards at line {line_number}"

                kind = record["kind"]
                fields = record["fields"]
                upper_kind = kind.upper()
                if upper_kind in FORBIDDEN_KINDS:
                    violations.append(f"line {line_number}: {kind}")

                for boolean_name in ("vehicle_tx", "tx_permitted"):
                    if boolean_name in fields:
                        value = fields[boolean_name]
                        if not isinstance(value, bool):
                            return 2, f"BLOCKED: {boolean_name} is not boolean at line {line_number}"
                        if value:
                            violations.append(f"line {line_number}: {boolean_name}=true")

                for count_name in ("tx_frames", "ack_frames"):
                    if count_name in fields:
                        count = fields[count_name]
                        if not _is_int(count) or count < 0:
                            return 2, f"BLOCKED: invalid {count_name} at line {line_number}"
                        if count > 0:
                            violations.append(f"line {line_number}: {count_name}={count}")

                if kind == "CAN_CHANNEL_SUMMARY":
                    required_fields = {"channel", "rx_frames", "tx_frames", "ack_frames"}
                    if not required_fields.issubset(fields):
                        return 2, f"BLOCKED: incomplete channel summary at line {line_number}"
                    channel = fields["channel"]
                    rx_frames = fields["rx_frames"]
                    if not _is_int(channel) or channel not in {1, 2, 3}:
                        return 2, f"BLOCKED: invalid CAN channel at line {line_number}"
                    if not _is_int(rx_frames) or rx_frames < 0:
                        return 2, f"BLOCKED: invalid rx_frames at line {line_number}"
                    channel_summaries += 1
                    channels_seen.add(channel)

                second_last_kind = last_kind
                second_last_fields = last_fields
                last_kind = kind
                last_fields = fields
                records += 1
                previous_sequence = sequence
                previous_monotonic_ns = monotonic_ns
                previous_log_offset = log_offset

            if records == 0:
                return 2, "BLOCKED: analyzer evidence contains no records"
            if channel_summaries == 0 or channels_seen != {1, 2, 3}:
                return 2, "BLOCKED: complete three-channel capture summary is missing"
            if second_last_kind != "TX_GATE_STATE":
                return 2, "BLOCKED: final capture-only gate record is missing"
            if (not isinstance(second_last_fields.get("vehicle_tx"), bool)
                    or second_last_fields["vehicle_tx"] is not False
                    or second_last_fields.get("mode") != "CAPTURE_ONLY"):
                return 2, "BLOCKED: final capture-only gate is invalid"
            if last_kind != "HARNESS_COMPLETE":
                return 2, "BLOCKED: final record is not HARNESS_COMPLETE"
            if last_fields.get("firmware_mode") != "CAPTURE_ONLY":
                return 2, "BLOCKED: harness completion is not capture-only"
            if not isinstance(last_fields.get("scenario"), str) or not last_fields["scenario"]:
                return 2, "BLOCKED: harness completion scenario is missing"
            if violations:
                return 1, "FAIL: CAN TX/ACK activity detected: " + ", ".join(violations)
            return 0, f"PASS: {records} complete JSONL records contain no CAN TX or ACK"
    except (OSError, ValueError) as error:
        message = str(error)
        if "exceeds" in message:
            return 2, "BLOCKED: analyzer evidence exceeds the bounded input limit"
        return 2, f"NOT_RUN: analyzer evidence is unavailable: {error}"


def main(argv: list[str] | None = None) -> int:
    if argv is None:
        argv = sys.argv[1:]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", type=Path)
    parser.add_argument("--expected-source", required=True)
    parser.add_argument("--expected-execution-id", required=True)
    parser.add_argument("--expected-firmware-identity", required=True)
    try:
        args = parser.parse_args(argv)
    except SystemExit:
        return 2
    status, message = assert_no_tx(
        args.path,
        expected_source=args.expected_source,
        expected_execution_id=args.expected_execution_id,
        expected_firmware_identity=args.expected_firmware_identity,
    )
    print(message)
    return status


if __name__ == "__main__":
    raise SystemExit(main())
