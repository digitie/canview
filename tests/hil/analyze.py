"""첫 violated invariant을 보존하는 T-500 evidence analyzer."""

from __future__ import annotations

from typing import Any

from .events import event_kinds
from .scenario import Scenario


def _is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _safe_offset(record: dict[str, Any] | None,
                 fallback_offset: int) -> int:
    if record is not None and _is_int(record.get("log_offset")):
        return max(0, int(record["log_offset"]))
    return fallback_offset


def _violation(invariant: str, message: str,
               record: dict[str, Any] | None,
               fallback_offset: int) -> dict[str, Any]:
    return {
        "invariant": invariant,
        "message": message,
        "log_offset": _safe_offset(record, fallback_offset),
        "event_sequence": (int(record["sequence"]) if record is not None
                            and _is_int(record.get("sequence")) else None),
    }


def analyze(scenario: Scenario, records: list[dict[str, Any]],
            metrics: dict[str, int],
            budget: dict[str, dict[str, int]]) -> dict[str, Any]:
    """event, expected assertion, capture-only와 budget을 fail-closed 검사한다."""
    checks: list[dict[str, Any]] = []
    violations: list[dict[str, Any]] = []
    fallback_offset = 0
    if records and isinstance(records[-1], dict):
        fallback_offset = _safe_offset(records[-1], 0)

    def check(invariant: str, passed: bool, message: str,
              record: dict[str, Any] | None = None) -> None:
        checks.append({"invariant": invariant, "passed": passed})
        if not passed:
            violations.append(_violation(invariant, message, record,
                                          fallback_offset))

    check("events.non_empty", bool(records), "scenario produced no events")

    unknown_actions = [record for record in records
                       if isinstance(record, dict)
                       and record.get("kind") == "UNKNOWN_ACTION_REJECTED"]
    check("adapter.no_unknown_action", not unknown_actions,
          "adapter rejected an unknown scenario action",
          unknown_actions[0] if unknown_actions else None)

    previous_time: int | None = None
    for index, record in enumerate(records, 1):
        record_ok = isinstance(record, dict)
        check("events.record_object", record_ok,
              "event record is not an object", record if record_ok else None)
        if not record_ok:
            previous_time = None
            continue
        sequence_ok = record.get("sequence") == index
        check("events.contiguous_sequence", sequence_ok,
              f"expected sequence {index}", record)
        current_time = record.get("monotonic_ns")
        time_ok = _is_int(current_time) and current_time >= 0
        if previous_time is not None and time_ok:
            time_ok = current_time >= previous_time
        check("events.monotonic", time_ok,
              "event time moved backwards or is invalid", record)
        previous_time = current_time if _is_int(current_time) else None

    tx_records = [
        record for record in records
        if (isinstance(record, dict)
            and ((str(record.get("kind", "")).upper()
                  in {"CAN_TX", "VEHICLE_CAN_TX"})
                 or (record.get("kind") == "CAN_CHANNEL_SUMMARY"
                     and isinstance(record.get("fields"), dict)
                     and _is_int(record["fields"].get("tx_frames"))
                     and record["fields"]["tx_frames"] > 0)))
    ]
    check("capture_only.can_tx_zero", not tx_records,
          "capture-only log contains a CAN TX event", tx_records[0]
          if tx_records else None)

    allowlist = scenario.expect.get("can_tx_allowlist", [])
    if not isinstance(allowlist, list):
        allowlist = []
    allowed = {int(item) for item in allowlist
               if _is_int(item)}
    unexpected: list[dict[str, Any]] = []
    for record in tx_records:
        fields = record.get("fields", {})
        if not isinstance(fields, dict) or fields.get("arbitration_id") not in allowed:
            unexpected.append(record)
    check("command.allowlist", not unexpected,
          "CAN TX frame is outside the scenario allow-list",
          unexpected[0] if unexpected else None)

    kinds = event_kinds(records)
    required_kinds = scenario.expect.get("required_kinds", [])
    if not isinstance(required_kinds, list):
        required_kinds = []
    for required in required_kinds:
        required_ok = isinstance(required, str) and bool(required)
        check(f"scenario.required_kind.{required}",
              required_ok and required in kinds,
              f"required event kind is missing: {required}")

    required_fields = scenario.expect.get("required_fields", [])
    if not isinstance(required_fields, list):
        required_fields = []
    for requirement in required_fields:
        if not isinstance(requirement, dict):
            check("scenario.required_fields.contract", False,
                  "required_fields entry is not an object")
            continue
        kind = requirement.get("kind")
        expected_fields = requirement.get("fields", {})
        matched = False
        expected_keys_ok = (isinstance(expected_fields, dict)
                            and all(isinstance(key, str)
                                    for key in expected_fields))
        if (isinstance(kind, str) and bool(kind) and expected_keys_ok):
            for record in records:
                if (not isinstance(record, dict)
                        or record.get("kind") != kind
                        or not isinstance(record.get("fields"), dict)):
                    continue
                if all(record["fields"].get(key) == value
                       for key, value in expected_fields.items()):
                    matched = True
                    break
        check(f"scenario.required_fields.{kind}", matched,
              f"required field assertion is missing: {kind}")

    for metric, limits in budget.items():
        value = metrics.get(metric)
        if not _is_int(value):
            check(f"budget.{metric}", False, f"budget metric is missing: {metric}")
            continue
        if "maximum" in limits:
            maximum = limits["maximum"]
            if not _is_int(maximum):
                check(f"budget.{metric}.maximum", False,
                      f"budget maximum is invalid: {metric}")
            else:
                check(f"budget.{metric}.maximum", value <= maximum,
                      f"{metric}={value} exceeds maximum {maximum}")
        if "minimum" in limits:
            minimum = limits["minimum"]
            if not _is_int(minimum):
                check(f"budget.{metric}.minimum", False,
                      f"budget minimum is invalid: {metric}")
            else:
                check(f"budget.{metric}.minimum", value >= minimum,
                      f"{metric}={value} is below minimum {minimum}")

    first = violations[0] if violations else None
    return {
        "status": "PASS" if first is None else "FAIL",
        "checks": checks,
        "violations": violations,
        "first_violation": first,
    }
