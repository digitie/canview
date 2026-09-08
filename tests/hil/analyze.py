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
    if not isinstance(scenario.expect, dict):
        checks.append({"invariant": "scenario.expect.contract", "passed": False})
        violations.append(_violation("scenario.expect.contract",
                                      "expect must be an object", None, 0))
        expect: dict[str, Any] = {}
    else:
        expect = scenario.expect
    if not isinstance(metrics, dict):
        checks.append({"invariant": "budget.metrics.contract", "passed": False})
        violations.append(_violation("budget.metrics.contract",
                                      "metrics must be an object", None, 0))
        metrics = {}
    if not isinstance(budget, dict):
        checks.append({"invariant": "budget.contract", "passed": False})
        violations.append(_violation("budget.contract",
                                      "budget must be an object", None, 0))
        budget = {}
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
        schema_ok = record.get("schema_version") == 1
        check("events.schema", schema_ok,
              "unsupported event schema", record)
        source_kind_ok = (isinstance(record.get("source"), str)
                          and bool(record.get("source"))
                          and isinstance(record.get("kind"), str)
                          and bool(record.get("kind")))
        check("events.source_kind", source_kind_ok,
              "event source or kind is invalid", record)
        fields_ok = isinstance(record.get("fields"), dict)
        check("events.fields_object", fields_ok,
              "event fields are not an object", record)
        offset = record.get("log_offset")
        offset_ok = _is_int(offset) and offset >= 0
        check("events.log_offset", offset_ok,
              "event log offset is invalid", record)
        sequence_ok = _is_int(record.get("sequence")) and record.get("sequence") == index
        check("events.contiguous_sequence", sequence_ok,
              f"expected sequence {index}", record)
        current_time = record.get("monotonic_ns")
        time_ok = _is_int(current_time) and current_time >= 0
        if previous_time is not None and time_ok:
            time_ok = current_time >= previous_time
        check("events.monotonic", time_ok,
              "event time moved backwards or is invalid", record)
        previous_time = current_time if _is_int(current_time) else None

    invalid_tx_count_records: list[dict[str, Any]] = []
    tx_records: list[dict[str, Any]] = []
    for record in records:
        if not isinstance(record, dict):
            continue
        kind = str(record.get("kind", "")).upper()
        if kind in {"CAN_TX", "VEHICLE_CAN_TX"}:
            tx_records.append(record)
            continue
        if record.get("kind") != "CAN_CHANNEL_SUMMARY":
            continue
        fields = record.get("fields")
        tx_count = fields.get("tx_frames") if isinstance(fields, dict) else None
        if not _is_int(tx_count) or tx_count < 0:
            invalid_tx_count_records.append(record)
        elif tx_count > 0:
            tx_records.append(record)
    check("capture_only.tx_count_valid", not invalid_tx_count_records,
          "CAN channel summary has an invalid tx_frames count",
          invalid_tx_count_records[0] if invalid_tx_count_records else None)
    check("capture_only.can_tx_zero", not tx_records,
          "capture-only log contains a CAN TX event", tx_records[0]
          if tx_records else None)

    allowlist = expect.get("can_tx_allowlist", [])
    if not isinstance(allowlist, list):
        check("scenario.can_tx_allowlist.contract", False,
              "can_tx_allowlist must be a list")
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

    unsafe_records: list[dict[str, Any]] = []
    feedback_results = {"SUCCESS", "RESULT_BEFORE_ACK", "MISMATCH",
                        "TIMEOUT", "MANUAL_OVERRIDE"}
    for record in records:
        if not isinstance(record, dict) or not isinstance(record.get("fields"), dict):
            continue
        kind = str(record.get("kind", "")).upper()
        fields = record["fields"]
        if kind == "SAFETY_DECISION":
            unsafe = not (
                isinstance(fields.get("check"), str)
                and bool(fields["check"])
                and fields.get("decision") == "DENY"
                and isinstance(fields.get("vehicle_tx"), bool)
                and fields["vehicle_tx"] is False
            )
        elif kind == "TX_GATE_STATE":
            unsafe = not (
                isinstance(fields.get("vehicle_tx"), bool)
                and fields["vehicle_tx"] is False
                and fields.get("mode") == "CAPTURE_ONLY"
            )
        elif kind in {"POWER_EVENT", "GUARDIAN_TIMEOUT"}:
            name_key = "stage" if kind == "POWER_EVENT" else "guardian"
            unsafe = not (
                isinstance(fields.get(name_key), str)
                and bool(fields[name_key])
                and fields.get("tx_gate") == "OFF"
            )
        elif kind == "SECURITY_REJECT":
            unsafe = not (
                isinstance(fields.get("vector"), str)
                and bool(fields["vector"])
                and isinstance(fields.get("accepted"), bool)
                and fields["accepted"] is False
            )
        elif kind == "COMMAND_REPLAY":
            unsafe = not (
                isinstance(fields.get("request_token"), str)
                and bool(fields["request_token"])
                and isinstance(fields.get("executed"), bool)
                and fields["executed"] is False
                and fields.get("result") in {"ACCEPTED", "DUPLICATE"}
            )
        elif kind == "FEEDBACK_RESULT":
            unsafe = not (
                isinstance(fields.get("case"), str)
                and bool(fields["case"])
                and fields.get("result") in feedback_results
                and isinstance(fields.get("tx_permitted"), bool)
                and fields["tx_permitted"] is False
            )
        elif kind == "FEEDBACK_SEQUENCE":
            unsafe = not (
                isinstance(fields.get("case"), str)
                and bool(fields["case"])
                and fields.get("sequence") == ["RESULT", "ACK"]
            )
        else:
            unsafe = False
        if unsafe:
            unsafe_records.append(record)
    check("safety.no_unsafe_outcome", not unsafe_records,
          "event contains an unsafe or fail-open outcome",
          unsafe_records[0] if unsafe_records else None)

    ordered_events = expect.get("ordered_events")
    if ordered_events is not None and not isinstance(ordered_events, list):
        check("scenario.ordered_events.contract", False,
              "ordered_events must be a list")
        ordered_events = []
    for event_index, expected_event in enumerate(ordered_events or [], 1):
        expected_kind = (expected_event.get("kind")
                         if isinstance(expected_event, dict) else None)
        expected_fields = (expected_event.get("fields", {})
                           if isinstance(expected_event, dict) else {})
        record = records[event_index - 1] if event_index <= len(records) else None
        matched = (isinstance(expected_event, dict)
                   and isinstance(expected_kind, str) and bool(expected_kind)
                   and isinstance(expected_fields, dict)
                   and isinstance(record, dict)
                   and record.get("kind") == expected_kind
                   and isinstance(record.get("fields"), dict)
                   and all(record["fields"].get(key) == value
                           for key, value in expected_fields.items()))
        check(f"scenario.ordered_event.{event_index}", matched,
              f"ordered event {event_index} does not match",
              record if isinstance(record, dict) else None)

    kinds = event_kinds(records)
    required_kinds = expect.get("required_kinds", [])
    if not isinstance(required_kinds, list):
        check("scenario.required_kinds.contract", False,
              "required_kinds must be a list")
        required_kinds = []
    for required in required_kinds:
        required_ok = isinstance(required, str) and bool(required)
        check(f"scenario.required_kind.{required}",
              required_ok and required in kinds,
              f"required event kind is missing: {required}")

    required_fields = expect.get("required_fields", [])
    if not isinstance(required_fields, list):
        check("scenario.required_fields.contract", False,
              "required_fields must be a list")
        required_fields = []
    for requirement in required_fields:
        if not isinstance(requirement, dict):
            check("scenario.required_fields.contract", False,
                  "required_fields entry is not an object")
            continue
        kind = requirement.get("kind")
        expected_fields = requirement.get("fields", {})
        matched = False
        minimum = requirement.get("minimum", 1)
        expected_keys_ok = (isinstance(expected_fields, dict)
                            and all(isinstance(key, str)
                                    for key in expected_fields))
        matching_count = 0
        if (isinstance(kind, str) and bool(kind) and expected_keys_ok
                and _is_int(minimum) and minimum > 0):
            for record in records:
                if (not isinstance(record, dict)
                        or record.get("kind") != kind
                        or not isinstance(record.get("fields"), dict)):
                    continue
                if all(record["fields"].get(key) == value
                       for key, value in expected_fields.items()):
                    matching_count += 1
            matched = matching_count >= minimum
        check(f"scenario.required_fields.{kind}", matched,
              f"required field assertion is missing: {kind} (minimum={minimum})")

    minimum_counts = expect.get("minimum_counts", {})
    if not isinstance(minimum_counts, dict):
        check("scenario.minimum_counts.contract", False,
              "minimum_counts must be an object")
        minimum_counts = {}
    for kind, minimum in minimum_counts.items():
        count = sum(1 for record in records
                    if isinstance(record, dict) and record.get("kind") == kind)
        valid = (isinstance(kind, str) and bool(kind) and _is_int(minimum)
                 and minimum > 0 and count >= minimum)
        check(f"scenario.minimum_count.{kind}", valid,
              f"event count for {kind} is {count}, minimum is {minimum}")

    for record in records:
        if not isinstance(record, dict) or not isinstance(record.get("fields"), dict):
            continue
        kind = str(record.get("kind", "")).upper()
        fields = record["fields"]
        if kind == "RESOURCE_SUMMARY":
            summary_ok = (
                isinstance(fields.get("pool"), str)
                and bool(fields["pool"])
                and _is_int(fields.get("queue_depth"))
                and fields["queue_depth"] >= 0
                and _is_int(fields.get("heap_free_bytes"))
                and fields["heap_free_bytes"] >= 0
                and _is_int(fields.get("rejected"))
                and fields["rejected"] >= 0
                and _is_int(fields.get("observer_drops"))
                and fields["observer_drops"] >= 0
            )
            check("resource.summary.contract", summary_ok,
                  "resource summary fields are invalid", record)
            queue_limits = budget.get("queue_depth")
            if isinstance(queue_limits, dict) and _is_int(queue_limits.get("maximum")):
                queue_depth = fields.get("queue_depth")
                check("budget.event.queue_depth.maximum",
                      _is_int(queue_depth) and queue_depth <= queue_limits["maximum"],
                      "resource summary queue depth exceeds maximum", record)
            heap_limits = budget.get("heap_free_bytes")
            if isinstance(heap_limits, dict) and _is_int(heap_limits.get("minimum")):
                check("budget.event.heap_free_bytes.minimum",
                      _is_int(fields.get("heap_free_bytes"))
                      and fields["heap_free_bytes"] >= heap_limits["minimum"],
                      "resource summary free heap is below minimum", record)
        elif kind == "BUDGET_SAMPLE":
            sample = fields.get("metrics")
            sample_ok = isinstance(sample, dict)
            check("budget.event_sample.contract", sample_ok,
                  "budget sample metrics are not an object", record)
            if not sample_ok:
                continue
            for metric, limits in budget.items():
                if not isinstance(metric, str) or not isinstance(limits, dict):
                    continue
                value = sample.get(metric)
                if not _is_int(value):
                    check(f"budget.event.{metric}", False,
                          f"budget sample metric is invalid: {metric}", record)
                    continue
                if _is_int(limits.get("maximum")):
                    check(f"budget.event.{metric}.maximum",
                          value <= limits["maximum"],
                          f"budget sample {metric} exceeds maximum", record)
                if _is_int(limits.get("minimum")):
                    check(f"budget.event.{metric}.minimum",
                          value >= limits["minimum"],
                          f"budget sample {metric} is below minimum", record)

    for metric, limits in budget.items():
        if not isinstance(metric, str) or not metric or not isinstance(limits, dict):
            check("budget.entry.contract", False,
                  "budget entry is invalid")
            continue
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

    final_gate = records[-2] if len(records) >= 2 else None
    completion = records[-1] if records else None
    final_gate_ok = (
        isinstance(final_gate, dict)
        and final_gate.get("kind") == "TX_GATE_STATE"
        and isinstance(final_gate.get("fields"), dict)
        and final_gate["fields"].get("vehicle_tx") is False
        and final_gate["fields"].get("mode") == "CAPTURE_ONLY"
    )
    check("events.final_gate", final_gate_ok,
          "final event before completion is not a capture-only gate state",
          final_gate if isinstance(final_gate, dict) else None)
    completion_ok = (
        isinstance(completion, dict)
        and completion.get("kind") == "HARNESS_COMPLETE"
        and isinstance(completion.get("fields"), dict)
        and completion["fields"].get("scenario") == scenario.scenario_id
        and completion["fields"].get("firmware_mode") == "CAPTURE_ONLY"
    )
    check("events.completion", completion_ok,
          "final event is not a valid harness completion",
          completion if isinstance(completion, dict) else None)

    first = violations[0] if violations else None
    return {
        "status": "PASS" if first is None else "FAIL",
        "checks": checks,
        "violations": violations,
        "first_violation": first,
    }
