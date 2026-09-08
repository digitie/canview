"""JSON-compatible YAML 1.2 scenario inventory loader."""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import re
from typing import Any


SCENARIO_SCHEMA_VERSION = 1
SCENARIO_ID = re.compile(r"^[a-z0-9][a-z0-9-]{2,63}$")
MAX_SCENARIO_BYTES = 1 << 20
MAX_SCENARIO_ACTIONS = 256
MAX_NESTING_DEPTH = 16
MAX_COLLECTION_ITEMS = 1024
MAX_STRING_LENGTH = 4096
MAX_NAMED_ITEMS = 128
MAX_PACKET_COUNT = 10_000
MAX_EVENT_FIELDS_BYTES = 48 << 10


class ScenarioError(ValueError):
    """Scenario가 제한된 YAML/contract를 만족하지 않을 때 발생한다."""


def _reject_constant(value: str) -> Any:
    raise ValueError(f"non-finite JSON number is not allowed: {value}")


def _object_pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate object key: {key}")
        result[key] = value
    return result


def _validate_value(value: Any, path: Path, label: str,
                    depth: int = 0) -> None:
    """입력 하나가 parser/resource exhaustion 경계를 넘지 않는지 검사한다."""
    if depth > MAX_NESTING_DEPTH:
        raise ScenarioError(f"{label} nesting is too deep: {path}")
    if isinstance(value, str):
        if len(value) > MAX_STRING_LENGTH:
            raise ScenarioError(f"{label} string is too long: {path}")
        return
    if isinstance(value, float):
        if not math.isfinite(value):
            raise ScenarioError(f"{label} contains a non-finite number: {path}")
        return
    if isinstance(value, list):
        if len(value) > MAX_COLLECTION_ITEMS:
            raise ScenarioError(f"{label} list is too large: {path}")
        for item in value:
            _validate_value(item, path, label, depth + 1)
        return
    if isinstance(value, dict):
        if len(value) > MAX_COLLECTION_ITEMS:
            raise ScenarioError(f"{label} object is too large: {path}")
        for key, item in value.items():
            if not isinstance(key, str) or len(key) > MAX_STRING_LENGTH:
                raise ScenarioError(f"{label} has an invalid key: {path}")
            _validate_value(item, path, label, depth + 1)


def load_yaml_object(path: Path) -> dict[str, Any]:
    """외부 parser 없이 읽는 JSON-compatible YAML 1.2 object."""
    try:
        if path.stat().st_size > MAX_SCENARIO_BYTES:
            raise ScenarioError(f"scenario is too large: {path}")
        value = json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=_object_pairs,
            parse_constant=_reject_constant,
        )
    except OSError as error:
        raise ScenarioError(f"unable to read scenario {path}: {error}") from error
    except ScenarioError:
        raise
    except (UnicodeError, json.JSONDecodeError, ValueError) as error:
        raise ScenarioError(
            f"{path} must use the JSON-compatible YAML 1.2 subset: {error}") from error
    if not isinstance(value, dict):
        raise ScenarioError(f"scenario root must be an object: {path}")
    _validate_value(value, path, "scenario")
    return value


@dataclass(frozen=True)
class Scenario:
    """하나의 host/lab 공용 fault scenario."""

    scenario_id: str
    title: str
    suites: tuple[str, ...]
    mode: str
    actions: tuple[dict[str, Any], ...]
    expect: dict[str, Any]
    source_path: Path

    @property
    def digest(self) -> str:
        """source 경로가 아닌 canonical scenario 내용의 digest."""
        canonical = {
            "schema_version": SCENARIO_SCHEMA_VERSION,
            "id": self.scenario_id,
            "title": self.title,
            "suites": list(self.suites),
            "mode": self.mode,
            "actions": list(self.actions),
            "expect": self.expect,
        }
        encoded = json.dumps(canonical, ensure_ascii=False, sort_keys=True,
                             separators=(",", ":")).encode("utf-8")
        return hashlib.sha256(encoded).hexdigest()


def _require_string(value: Any, label: str, path: Path) -> str:
    if not isinstance(value, str) or not value:
        raise ScenarioError(f"{label} must be a non-empty string: {path}")
    return value


def _require_int(value: Any, label: str, path: Path,
                 minimum: int, maximum: int) -> int:
    if (not isinstance(value, int) or isinstance(value, bool)
            or value < minimum or value > maximum):
        raise ScenarioError(
            f"{label} must be an integer in {minimum}..{maximum}: {path}")
    return value


def _require_named_items(action: dict[str, Any], key: str, path: Path,
                         index: int) -> list[str]:
    value = action.get(key)
    if not isinstance(value, list) or not value or len(value) > MAX_NAMED_ITEMS:
        raise ScenarioError(f"action {index} {key} must be a bounded non-empty list: {path}")
    return [_require_string(item, f"action {index} {key} item", path)
            for item in value]


def _validate_action(action: dict[str, Any], path: Path, index: int) -> None:
    action_type = action["type"]
    if action_type == "radio_loss":
        rates = action.get("rates")
        if not isinstance(rates, list) or not rates or len(rates) > MAX_NAMED_ITEMS:
            raise ScenarioError(f"action {index} rates must be a bounded list: {path}")
        for rate in rates:
            _require_int(rate, f"action {index} rate", path, 0, 100)
        _require_int(action.get("packets", 64),
                     f"action {index} packets", path, 1, MAX_PACKET_COUNT)
        _require_int(action.get("duplicate_deliveries", 0),
                     f"action {index} duplicate_deliveries", path, 0, MAX_PACKET_COUNT)
        _require_int(action.get("reordered", 0),
                     f"action {index} reordered", path, 0, MAX_PACKET_COUNT)
    elif action_type == "radio_delay":
        _require_int(action.get("max_ms"), f"action {index} max_ms", path,
                     0, 86_400_000)
    elif action_type == "reset":
        _require_named_items(action, "targets", path, index)
    elif action_type == "uart_fault":
        _require_named_items(action, "faults", path, index)
    elif action_type == "can_load":
        channels = action.get("channels")
        if not isinstance(channels, list) or not channels or len(channels) > 3:
            raise ScenarioError(f"action {index} channels must contain 1..3 entries: {path}")
        channel_ids: list[int] = []
        for channel in channels:
            if not isinstance(channel, dict):
                raise ScenarioError(f"action {index} channel must be an object: {path}")
            channel_ids.append(_require_int(channel.get("channel"),
                                             f"action {index} channel", path, 1, 3))
            _require_int(channel.get("rx_frames", 128),
                         f"action {index} rx_frames", path, 0, MAX_PACKET_COUNT)
            _require_string(channel.get("bus_state", "ERROR_PASSIVE"),
                            f"action {index} bus_state", path)
            _require_int(channel.get("error_counter", 0),
                         f"action {index} error_counter", path, 0, 255)
        if len(channel_ids) != len(set(channel_ids)):
            raise ScenarioError(f"action {index} channels must be unique: {path}")
    elif action_type == "resource":
        _require_int(action.get("queue_depth", 64),
                     f"action {index} queue_depth", path, 0, MAX_PACKET_COUNT)
        _require_int(action.get("heap_free_bytes", 8192),
                     f"action {index} heap_free_bytes", path, 0, 2**31 - 1)
        _require_int(action.get("rejected", 0),
                     f"action {index} rejected", path, 0, MAX_PACKET_COUNT)
        _require_int(action.get("observer_drops", 0),
                     f"action {index} observer_drops", path, 0, MAX_PACKET_COUNT)
    elif action_type == "safety_gates":
        _require_named_items(action, "checks", path, index)
    elif action_type == "duplicate_command":
        _require_named_items(action, "tokens", path, index)
    elif action_type == "feedback":
        _require_named_items(action, "cases", path, index)
    elif action_type == "power":
        _require_named_items(action, "stages", path, index)
    elif action_type == "security":
        _require_named_items(action, "vectors", path, index)
    elif action_type == "guardian":
        _require_named_items(action, "guardians", path, index)
    elif action_type == "radio_pressure":
        _require_int(action.get("softap_kbps", 0),
                     f"action {index} softap_kbps", path, 0, 1_000_000)
        _require_int(action.get("observer_kbps", 0),
                     f"action {index} observer_kbps", path, 0, 1_000_000)
        _require_int(action.get("control_kbps", 0),
                     f"action {index} control_kbps", path, 0, 1_000_000)
        _require_int(action.get("rssi_dbm", -60),
                     f"action {index} rssi_dbm", path, -127, 0)
    elif action_type == "capture":
        channels = action.get("channels")
        if not isinstance(channels, list) or not channels or len(channels) > 3:
            raise ScenarioError(f"action {index} channels must contain 1..3 entries: {path}")
        channel_ids: list[int] = []
        for channel in channels:
            channel_ids.append(_require_int(channel, f"action {index} capture channel",
                                            path, 1, 3))
        if len(channel_ids) != len(set(channel_ids)):
            raise ScenarioError(f"action {index} capture channels must be unique: {path}")
    elif action_type == "budget":
        values = action.get("values")
        if not isinstance(values, dict) or not values:
            raise ScenarioError(f"action {index} values must be a non-empty object: {path}")
        for metric, value in values.items():
            _require_string(metric, f"action {index} metric", path)
            _require_int(value, f"action {index} {metric}", path, 0, 2**31 - 1)
    elif action_type == "event":
        if "kind" in action:
            _require_string(action["kind"], f"action {index} kind", path)
        fields = action.get("fields", {})
        if not isinstance(fields, dict):
            raise ScenarioError(f"action {index} fields must be an object: {path}")
        try:
            encoded = json.dumps(fields, ensure_ascii=False, sort_keys=True,
                                 separators=(",", ":"), allow_nan=False)
        except (TypeError, ValueError, OverflowError) as error:
            raise ScenarioError(f"action {index} fields are not JSON-compatible: {path}") from error
        if len(encoded.encode("utf-8")) > MAX_EVENT_FIELDS_BYTES:
            raise ScenarioError(f"action {index} fields are too large: {path}")


def _validate_expect(expect: dict[str, Any], path: Path) -> None:
    ordered_events = expect.get("ordered_events")
    if ordered_events is not None:
        if (not isinstance(ordered_events, list) or not ordered_events
                or len(ordered_events) > MAX_COLLECTION_ITEMS):
            raise ScenarioError(f"ordered_events must be a bounded non-empty list: {path}")
        for event in ordered_events:
            if not isinstance(event, dict):
                raise ScenarioError(f"ordered event must be an object: {path}")
            _require_string(event.get("kind"), "ordered event kind", path)
            fields = event.get("fields", {})
            if not isinstance(fields, dict) or any(not isinstance(key, str)
                                                   for key in fields):
                raise ScenarioError(f"ordered event fields must be an object: {path}")

    required_kinds = expect.get("required_kinds")
    if required_kinds is not None:
        if (not isinstance(required_kinds, list) or not required_kinds
                or len(required_kinds) > MAX_NAMED_ITEMS):
            raise ScenarioError(f"required_kinds must be a bounded list: {path}")
        for kind in required_kinds:
            _require_string(kind, "required kind", path)

    required_fields = expect.get("required_fields")
    if required_fields is not None:
        if not isinstance(required_fields, list) or len(required_fields) > MAX_NAMED_ITEMS:
            raise ScenarioError(f"required_fields must be a bounded list: {path}")
        for requirement in required_fields:
            if not isinstance(requirement, dict):
                raise ScenarioError(f"required_fields entry must be an object: {path}")
            _require_string(requirement.get("kind"), "required field kind", path)
            fields = requirement.get("fields")
            if not isinstance(fields, dict) or any(not isinstance(key, str)
                                                   for key in fields):
                raise ScenarioError(f"required field fields must use string keys: {path}")
            _require_int(requirement.get("minimum", 1), "required field minimum",
                         path, 1, MAX_NAMED_ITEMS)

    minimum_counts = expect.get("minimum_counts")
    if minimum_counts is not None:
        if not isinstance(minimum_counts, dict) or not minimum_counts:
            raise ScenarioError(f"minimum_counts must be a non-empty object: {path}")
        for kind, minimum in minimum_counts.items():
            _require_string(kind, "minimum count kind", path)
            _require_int(minimum, "minimum count", path, 1, MAX_NAMED_ITEMS)

    allowlist = expect.get("can_tx_allowlist")
    if allowlist is not None:
        if not isinstance(allowlist, list) or len(allowlist) > MAX_NAMED_ITEMS:
            raise ScenarioError(f"can_tx_allowlist must be a bounded list: {path}")
        for arbitration_id in allowlist:
            _require_int(arbitration_id, "CAN TX allow-list id", path, 0, 0x1FFFFFFF)


def parse_scenario(path: Path) -> Scenario:
    """하나의 scenario 파일을 엄격하게 파싱한다."""
    raw = load_yaml_object(path)
    if raw.get("schema_version") != SCENARIO_SCHEMA_VERSION:
        raise ScenarioError(f"unsupported scenario schema: {path}")
    scenario_id = _require_string(raw.get("id"), "id", path)
    if not SCENARIO_ID.fullmatch(scenario_id):
        raise ScenarioError(f"invalid scenario id {scenario_id!r}: {path}")
    title = _require_string(raw.get("title"), "title", path)
    suites_value = raw.get("suites")
    if not isinstance(suites_value, list) or not suites_value:
        raise ScenarioError(f"suites must be a non-empty list: {path}")
    suites = tuple(_require_string(item, "suite", path) for item in suites_value)
    if any(item not in {"host", "g2-readonly"} for item in suites):
        raise ScenarioError(f"unsupported suite in {path}")
    mode = _require_string(raw.get("mode", "CAPTURE_ONLY"), "mode", path)
    if mode != "CAPTURE_ONLY":
        raise ScenarioError(f"T-500 host scenarios must be CAPTURE_ONLY: {path}")
    actions_value = raw.get("actions")
    if not isinstance(actions_value, list) or not actions_value:
        raise ScenarioError(f"actions must be a non-empty list: {path}")
    if len(actions_value) > MAX_SCENARIO_ACTIONS:
        raise ScenarioError(f"actions list is too large: {path}")
    actions: list[dict[str, Any]] = []
    for index, action in enumerate(actions_value, 1):
        if not isinstance(action, dict):
            raise ScenarioError(f"each action must be an object: {path}")
        if not isinstance(action.get("type"), str) or not action["type"]:
            raise ScenarioError(f"each action needs a type: {path}")
        _validate_action(action, path, index)
        actions.append(dict(action))
    expect = raw.get("expect", {})
    if not isinstance(expect, dict):
        raise ScenarioError(f"expect must be an object: {path}")
    _validate_expect(expect, path)
    return Scenario(scenario_id, title, suites, mode, tuple(actions),
                    dict(expect), path)


def load_scenarios(directory: Path, suite: str,
                   selected: list[str] | None = None) -> list[Scenario]:
    """directory의 scenario를 읽고 suite/선택 필터를 적용한다."""
    if suite not in {"host", "g2-readonly"}:
        raise ScenarioError(f"unsupported suite: {suite}")
    if selected is not None and len(selected) != len(set(selected)):
        raise ScenarioError("duplicate scenario selection")
    paths = sorted(directory.glob("*.yaml"))
    scenarios = [parse_scenario(path) for path in paths]
    if selected is not None:
        wanted = set(selected)
        scenarios = [item for item in scenarios if item.scenario_id in wanted]
        missing = wanted - {item.scenario_id for item in scenarios}
        if missing:
            raise ScenarioError(f"unknown scenario id(s): {', '.join(sorted(missing))}")
    scenarios = [item for item in scenarios if suite in item.suites]
    if not scenarios:
        raise ScenarioError(f"no scenarios selected for suite {suite}")
    ids = [item.scenario_id for item in scenarios]
    if len(ids) != len(set(ids)):
        raise ScenarioError("duplicate scenario id in inventory")
    return scenarios
