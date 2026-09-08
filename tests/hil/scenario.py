"""JSON-compatible YAML 1.2 scenario inventory loader."""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
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
    for action in actions_value:
        if not isinstance(action, dict):
            raise ScenarioError(f"each action must be an object: {path}")
        if not isinstance(action.get("type"), str) or not action["type"]:
            raise ScenarioError(f"each action needs a type: {path}")
        actions.append(dict(action))
    expect = raw.get("expect", {})
    if not isinstance(expect, dict):
        raise ScenarioError(f"expect must be an object: {path}")
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
