"""Monotonic JSONL event log primitives for the T-500 harness."""

from __future__ import annotations

from copy import deepcopy
import json
import os
from pathlib import Path
import tempfile
from typing import Any, Iterable


EVENT_SCHEMA_VERSION = 1
MAX_EVENT_LOG_BYTES = 8 << 20
MAX_EVENT_LINE_BYTES = 64 << 10
MAX_EVENT_COUNT = 100_000


class EventLogError(ValueError):
    """Event log가 contract를 위반할 때 발생한다."""


def _encode(record: dict[str, Any]) -> str:
    try:
        encoded = json.dumps(record, ensure_ascii=False, sort_keys=True,
                             separators=(",", ":"), allow_nan=False)
    except (TypeError, ValueError, OverflowError) as error:
        raise EventLogError(f"event is not JSON-compatible: {error}") from error
    return encoded + "\n"


def _reject_constant(value: str) -> Any:
    raise ValueError(f"non-finite JSON number is not allowed: {value}")


def _object_pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate object key: {key}")
        result[key] = value
    return result


class EventLog:
    """Sequence와 byte offset을 함께 소유하는 bounded-in-test event log."""

    def __init__(self) -> None:
        self._records: list[dict[str, Any]] = []
        self._next_offset = 0

    @property
    def records(self) -> list[dict[str, Any]]:
        """현재 records의 복사본을 반환한다."""
        return deepcopy(self._records)

    @property
    def byte_length(self) -> int:
        """UTF-8 JSONL로 직렬화했을 때의 누적 길이."""
        return self._next_offset

    def append(self, monotonic_ns: int, source: str, kind: str,
               **fields: Any) -> dict[str, Any]:
        """하나의 event를 추가하고 immutable record를 반환한다."""
        if (not isinstance(monotonic_ns, int)
                or isinstance(monotonic_ns, bool) or monotonic_ns < 0):
            raise EventLogError("monotonic_ns must be a non-negative integer")
        if (not isinstance(source, str) or not source
                or not isinstance(kind, str) or not kind):
            raise EventLogError("source and kind are required")
        if len(self._records) >= MAX_EVENT_COUNT:
            raise EventLogError("event count is too large")
        record = {
            "schema_version": EVENT_SCHEMA_VERSION,
            "sequence": len(self._records) + 1,
            "monotonic_ns": monotonic_ns,
            "source": source,
            "kind": kind,
            "fields": deepcopy(fields),
            "log_offset": self._next_offset,
        }
        encoded = _encode(record).encode("utf-8")
        if len(encoded) > MAX_EVENT_LINE_BYTES:
            raise EventLogError("event line is too large")
        next_offset = self._next_offset + len(encoded)
        if next_offset > MAX_EVENT_LOG_BYTES:
            raise EventLogError("event log is too large")
        self._records.append(record)
        self._next_offset = next_offset
        return deepcopy(record)

    def write_jsonl(self, path: Path) -> None:
        """event를 deterministic JSONL로 저장한다."""
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            if path.parent.is_symlink() or path.is_symlink():
                raise EventLogError("refusing to write through a symlink")
            payload = "".join(_encode(record) for record in self._records)
            payload_bytes = payload.encode("utf-8")
            if len(payload_bytes) > MAX_EVENT_LOG_BYTES:
                raise EventLogError("event log is too large")
            descriptor, temporary_name = tempfile.mkstemp(
                prefix=".canview-events-", suffix=".tmp",
                dir=str(path.parent))
            try:
                with os.fdopen(descriptor, "wb") as stream:
                    stream.write(payload_bytes)
                    stream.flush()
                    os.fsync(stream.fileno())
                os.replace(temporary_name, path)
            except BaseException:
                try:
                    Path(temporary_name).unlink()
                except OSError:
                    pass
                raise
        except EventLogError:
            raise
        except OSError as error:
            raise EventLogError(f"unable to write event log {path}: {error}") from error


def read_jsonl(path: Path) -> list[dict[str, Any]]:
    """JSONL event log를 읽고 기본 형태를 검사한다."""
    try:
        if path.stat().st_size > MAX_EVENT_LOG_BYTES:
            raise EventLogError(f"event log is too large: {path}")
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        raise EventLogError(f"unable to read event log {path}: {error}") from error
    except EventLogError:
        raise
    except UnicodeError as error:
        raise EventLogError(f"event log is not UTF-8: {path}: {error}") from error
    records: list[dict[str, Any]] = []
    if len(lines) > MAX_EVENT_COUNT:
        raise EventLogError(f"event count is too large: {path}")
    for line_number, line in enumerate(lines, 1):
        if not line.strip():
            raise EventLogError(f"blank line at {path}:{line_number}")
        if len(line.encode("utf-8")) > MAX_EVENT_LINE_BYTES:
            raise EventLogError(f"event line is too large at {path}:{line_number}")
        try:
            record = json.loads(line, object_pairs_hook=_object_pairs,
                                parse_constant=_reject_constant)
        except (json.JSONDecodeError, ValueError) as error:
            raise EventLogError(
                f"invalid JSON at {path}:{line_number}: {error}") from error
        if not isinstance(record, dict):
            raise EventLogError(f"event at {path}:{line_number} is not an object")
        records.append(record)
    return records


def event_kinds(records: Iterable[dict[str, Any]]) -> set[str]:
    """event kind 집합을 반환한다."""
    return {str(record.get("kind", "")) for record in records
            if isinstance(record, dict)}
