#!/usr/bin/env python3
"""Generate and validate the ESP-NOW v1.3 wire ABI.

The schema is deliberately JSON-compatible YAML.  That keeps the generator
usable in the repository's dependency-free Python CI job while still allowing
schema authors to move to richer YAML syntax later when the CI dependency is
made explicit.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
import zlib
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence


ROOT = Path(__file__).resolve().parents[1]
SCHEMA_PATH = ROOT / "protocol" / "schema" / "espnow-v1.3.yaml"
HEADER_PATH = ROOT / "protocol" / "canview_protocol.h"
GOLDEN_DIR = ROOT / "protocol" / "golden" / "espnow-v1.3"
MALFORMED_DIR = GOLDEN_DIR / "malformed"
COMPATIBILITY_DIR = GOLDEN_DIR / "compatibility"
PAIRING_VECTOR_PATH = GOLDEN_DIR / "pairing-negative.json"

TYPE_INFO: dict[str, tuple[str, int, bool]] = {
    "u8": ("uint8_t", 1, False),
    "i8": ("int8_t", 1, True),
    "u16": ("uint16_t", 2, False),
    "i16": ("int16_t", 2, True),
    "u32": ("uint32_t", 4, False),
    "i32": ("int32_t", 4, True),
    "u64": ("uint64_t", 8, False),
    "i64": ("int64_t", 8, True),
}
SCALAR_FORMAT = {
    "u8": "B",
    "i8": "b",
    "u16": "H",
    "i16": "h",
    "u32": "I",
    "i32": "i",
    "u64": "Q",
    "i64": "q",
}
IDENTIFIER = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
KNOWN_QOS = {"NONE", "Q0", "Q1", "Q1_WINDOW"}
KNOWN_ACK = {"none", "required"}
PAIRING_MESSAGES = frozenset({
    "DISCOVERY",
    "PAIR_REQUEST",
    "PAIR_CHALLENGE",
    "PAIR_CONFIRM",
    "PAIR_RESULT",
})
EXPECTED_PAIRING_PHASES: dict[str, dict[str, Any]] = {
    "DISCOVERY": {
        "domain": "CV-DISCOVERY-1",
        "canonical_fields": (
            "installation_id", "endpoint_id", "mac", "nonce", "channel",
            "proposed_major", "proposed_minor_min", "proposed_minor_max",
            "link_key_generation", "expires_at_ms",
        ),
        "must_not_bind": ("peer_nonce", "selected_version"),
    },
    "PAIR_REQUEST": {
        "domain": "CV-PAIR-REQUEST-1",
        "canonical_fields": (
            "request_token", "discovery_digest", "requester_endpoint_id",
            "requester_mac", "controller_nonce", "requested_role",
            "requested_major", "requested_minor", "expires_at_ms",
        ),
        "must_not_authorize": ("requested_role", "requested_scope"),
    },
    "PAIR_CHALLENGE": {
        "domain": "CV-PAIR-CHALLENGE-1",
        "canonical_fields": (
            "discovery_digest", "request_digest", "endpoint0_id", "endpoint1_id",
            "nonce0", "nonce1", "selected_major", "selected_minor", "channel",
            "authorized_role", "authorized_scope", "allowed_message_classes",
            "link_key_generation",
        ),
    },
    "PAIR_CONFIRM": {
        "domain": "CV-PAIR-CONFIRM-1",
        "canonical_fields": (
            "transcript_hash", "selected_major", "selected_minor",
            "authorized_role", "link_key_generation", "confirm_nonce",
        ),
    },
    "PAIR_RESULT": {
        "domain": "CV-PAIR-RESULT-1",
        "canonical_fields": (
            "request_token", "status", "assigned_role", "link_key_generation",
            "peer_id", "transcript_hash", "expires_at_ms",
        ),
    },
}
EXPECTED_PAIRING_PHASE_ORDER = (
    "DISCOVERY",
    "PAIR_REQUEST",
    "PAIR_CHALLENGE",
    "PAIR_CONFIRM",
    "PAIR_RESULT",
)
EXPECTED_PAIRING_NEGATIVES = {
    "discovery-peer-nonce": ("DISCOVERY", "bind_peer_nonce", "must_not_bind"),
    "discovery-selected-version": ("DISCOVERY", "bind_selected_version", "must_not_bind"),
    "request-authorizes-role": ("PAIR_REQUEST", "authorize_requested_role", "must_not_authorize"),
    "challenge-unauthorized-scope": (
        "PAIR_CHALLENGE", "accept_untrusted_scope", "reject_without_local_authorization"
    ),
    "confirm-transcript-order": ("PAIR_CONFIRM", "reorder_transcript_fields", "bad_canonical_order"),
    "result-transcript-digest": ("PAIR_RESULT", "change_transcript_hash", "bad_transcript"),
}
EXPECTED_RESPONSE_MESSAGES = frozenset({
    "HELLO", "CAPABILITIES", "TIME_SYNC_RESPONSE", "ACK", "ERROR",
    "STATE_SNAPSHOT", "CAN_FILTER_RESULT", "CAN_STREAM_STATUS", "CAN_ID_STATS",
    "CAN_CAPTURE_STATUS", "COMMAND_RESULT", "CONTROL_LEASE_STATUS", "CONFIG_RESULT",
    "REMOTE_CONFIG_STATUS", "BULK_ACK",
})
EXPECTED_RESPONSE_VARIANTS = frozenset({("DIAGNOSTIC_LEASE", "response")})
EXPECTED_FRAGMENT_MESSAGES = frozenset({"BULK_FRAGMENT"})
EXPECTED_AUTHENTICATED_SESSION_ZERO_MESSAGES = frozenset({"PAIR_CONFIRM", "PAIR_RESULT"})


class SchemaError(ValueError):
    """Raised when the machine-readable protocol contract is invalid."""


class ProtocolDecodeError(ValueError):
    """A deterministic parser error with a testable protocol error code."""

    def __init__(self, code: str, message: str):
        super().__init__(message)
        self.code = code


def load_schema(path: Path = SCHEMA_PATH) -> dict[str, Any]:
    raw = path.read_text(encoding="utf-8")
    try:
        parsed = json.loads(raw)
    except json.JSONDecodeError:
        try:
            import yaml  # type: ignore[import-not-found]
        except ImportError as exc:
            raise SchemaError(
                "schema is not JSON-compatible and PyYAML is not installed"
            ) from exc
        parsed = yaml.safe_load(raw)
    if not isinstance(parsed, dict):
        raise SchemaError("schema root must be an object")
    return parsed


def _type_size(field: Mapping[str, Any]) -> int:
    field_type = field.get("type")
    if field_type == "bytes":
        length = field.get("length")
        if not isinstance(length, int) or length <= 0:
            raise SchemaError(f"bytes field requires positive length: {field}")
        return length
    if field_type not in TYPE_INFO:
        raise SchemaError(f"unknown field type: {field_type}")
    return TYPE_INFO[field_type][1]


def _field_c_name(field: Mapping[str, Any]) -> str:
    explicit = field.get("c_name")
    if explicit is not None:
        return str(explicit)
    field_type = field.get("type")
    if field_type in TYPE_INFO and TYPE_INFO[field_type][1] > 1:
        return f"{field['name']}_le"
    return str(field["name"])


def _validate_identifier(value: str, context: str) -> None:
    if not IDENTIFIER.fullmatch(value):
        raise SchemaError(f"invalid C identifier for {context}: {value!r}")


def _validate_fields(fields: Sequence[Mapping[str, Any]], size: int, context: str) -> None:
    if not isinstance(fields, Sequence) or isinstance(fields, (str, bytes)):
        raise SchemaError(f"{context}.fields must be a list")
    cursor = 0
    wire_names: set[str] = set()
    c_names: set[str] = set()
    for index, field in enumerate(fields):
        if not isinstance(field, Mapping):
            raise SchemaError(f"{context}.fields[{index}] must be an object")
        name = field.get("name")
        if not isinstance(name, str):
            raise SchemaError(f"{context}.fields[{index}] has no name")
        _validate_identifier(name, f"{context}.{name}")
        c_name = _field_c_name(field)
        _validate_identifier(c_name, f"{context}.{name}.c_name")
        if name in wire_names or c_name in c_names:
            raise SchemaError(f"duplicate field name in {context}: {name}/{c_name}")
        wire_names.add(name)
        c_names.add(c_name)
        offset = field.get("offset")
        if not isinstance(offset, int) or offset != cursor:
            raise SchemaError(
                f"{context}.{name} has offset {offset}, expected contiguous offset {cursor}"
            )
        field_size = _type_size(field)
        if field.get("reserved") and not name.lower().startswith("reserved"):
            raise SchemaError(f"reserved field must be explicitly named reserved*: {context}.{name}")
        cursor += field_size
    if cursor != size:
        raise SchemaError(f"{context} fields end at {cursor}, declared size is {size}")


def _layout_entries(schema: Mapping[str, Any]) -> Iterable[tuple[str, int, Sequence[Mapping[str, Any]]]]:
    header = schema["header"]
    yield str(header["c_type"]), int(schema["encoding"]["header_size"]), header["fields"]
    yield "canview_tlv_header_t", int(schema["tlv"]["header_size"]), [
        {"name": "type", "c_name": "type_le", "type": "u16", "offset": 0},
        {"name": "length", "c_name": "length_le", "type": "u16", "offset": 2},
    ]
    seen: set[str] = set()
    for message in schema["messages"]:
        payload = message["payload"]
        kind = payload["kind"]
        candidates: list[tuple[str, int, Sequence[Mapping[str, Any]]]] = []
        if kind == "fixed":
            candidates.append((payload["c_type"], int(payload["size"]), payload["fields"]))
        elif kind in {"bounded", "suffix", "tlv"}:
            prefix = payload["prefix"]
            candidates.append((prefix["c_type"], int(prefix["size"]), prefix["fields"]))
            if kind == "bounded":
                record = payload["record"]
                candidates.append((record["c_type"], int(record["size"]), record["fields"]))
        elif kind == "variants":
            for variant in payload["variants"]:
                candidates.append((variant["c_type"], int(variant["size"]), variant["fields"]))
        else:
            raise SchemaError(f"unknown payload kind {kind!r}")
        for entry in candidates:
            if entry[0] not in seen:
                seen.add(entry[0])
                yield entry


def _message_field_names(message: Mapping[str, Any]) -> set[str]:
    payload = message["payload"]
    kind = payload["kind"]
    if kind == "fixed":
        return {str(field["name"]) for field in payload["fields"]}
    if kind in {"bounded", "suffix", "tlv"}:
        fields = list(payload["prefix"]["fields"])
        if kind == "bounded":
            fields.extend(payload["record"]["fields"])
        return {str(field["name"]) for field in fields}
    if kind == "variants":
        return {str(field["name"]) for variant in payload["variants"] for field in variant["fields"]}
    raise SchemaError(f"unknown payload kind {kind!r}")


def _message_variant_names(message: Mapping[str, Any]) -> set[str]:
    payload = message["payload"]
    if payload["kind"] != "variants":
        return set()
    return {str(variant["name"]) for variant in payload["variants"]}


def _validate_companion_schemas(schema: Mapping[str, Any], message_ids: set[int]) -> None:
    companions = schema.get("companion_schemas")
    if not isinstance(companions, Sequence) or isinstance(companions, (str, bytes)) or not companions:
        raise SchemaError("companion_schemas must be a non-empty list")
    for companion in companions:
        if not isinstance(companion, Mapping):
            raise SchemaError("companion schema entry must be an object")
        relative_path = companion.get("path")
        if not isinstance(relative_path, str) or not relative_path:
            raise SchemaError("companion schema path is required")
        companion_path = (ROOT / relative_path).resolve()
        try:
            companion_path.relative_to(ROOT)
        except ValueError as exc:
            raise SchemaError("companion schema path escapes the repository") from exc
        if not companion_path.is_file():
            raise SchemaError(f"companion schema is missing: {relative_path}")
        if companion.get("transport") != "esp_now":
            raise SchemaError(f"unsupported companion transport: {companion.get('transport')}")
        minimum_version = companion.get("minimum_version")
        if minimum_version != [1, 4]:
            raise SchemaError("navigation companion must require ESP-NOW 1.4")
        companion_schema = load_schema(companion_path)
        if companion_schema.get("esp_now_min_version") != minimum_version:
            raise SchemaError("companion schema version gate disagrees with its metadata")
        companion_messages = companion_schema.get("messages")
        if not isinstance(companion_messages, Mapping):
            raise SchemaError("companion schema messages must be an object")
        actual_ids = {
            int(message.get("id"))
            for message in companion_messages.values()
            if isinstance(message, Mapping) and message.get("transport") == "esp_now"
        }
        declared_ids = companion.get("message_ids")
        if not isinstance(declared_ids, Sequence) or set(declared_ids) != actual_ids:
            raise SchemaError("companion message IDs do not match the companion schema")
        if message_ids.intersection(actual_ids):
            raise SchemaError("v1.3 message IDs overlap a version-gated companion schema")
        declared_capabilities = companion.get("capabilities")
        actual_capabilities = {
            str(message.get("capability"))
            for message in companion_messages.values()
            if isinstance(message, Mapping)
            and message.get("transport") == "esp_now"
            and message.get("capability") is not None
        }
        if not isinstance(declared_capabilities, Sequence) or set(declared_capabilities) != actual_capabilities:
            raise SchemaError("companion capabilities do not match the companion schema")


def _validate_tlv_policy(policy: Any, label: str) -> None:
    if not isinstance(policy, Mapping):
        raise SchemaError(f"{label} policy must be an object")
    if policy.get("header_size") != 4 or policy.get("max_nesting_depth") != 0:
        raise SchemaError(f"{label} header/nesting contract is invalid")
    critical_bit = policy.get("critical_bit")
    if critical_bit != 0x8000:
        raise SchemaError(f"{label} critical bit changed")
    types = policy.get("types")
    if not isinstance(types, Sequence) or isinstance(types, (str, bytes)) or not types:
        raise SchemaError(f"{label}.types must be a non-empty list")
    type_values: set[int] = set()
    type_names: set[str] = set()
    for item in types:
        if not isinstance(item, Mapping):
            raise SchemaError(f"{label} type must be an object")
        name = item.get("name")
        value = item.get("value")
        size = item.get("size")
        if not isinstance(name, str) or not name or name in type_names:
            raise SchemaError(f"invalid or duplicate {label} type name {name!r}")
        if not isinstance(value, int) or not 0 < value <= 0xFFFF or value in type_values:
            raise SchemaError(f"invalid or duplicate {label} type {value}")
        if not isinstance(size, int) or size < -1:
            raise SchemaError(f"invalid {label} size for {value}")
        if value & (int(critical_bit) - 1) == 0:
            raise SchemaError(f"invalid {label} base type {value}")
        if not isinstance(item.get("singleton"), bool):
            raise SchemaError(f"{label} type {name} must declare singleton policy")
        type_names.add(name)
        type_values.add(value)


def _validate_config_and_pairing_policy(schema: Mapping[str, Any]) -> None:
    config_policy = schema.get("config_policy")
    if not isinstance(config_policy, Mapping):
        raise SchemaError("config_policy is required")
    if config_policy.get("schema_max_bytes") != 16384 or config_policy.get("max_records") != 25:
        raise SchemaError("config schema limits changed")
    if config_policy.get("authoritative_store") != "OTA_AB" or config_policy.get("cache_store") != "NVS_CACHE":
        raise SchemaError("config ownership storage policy changed")
    config_enum = next((enum for enum in schema["enums"] if enum.get("name") == "config_key"), None)
    owners = config_policy.get("owner_roles")
    if not isinstance(config_enum, Mapping) or not isinstance(owners, Mapping):
        raise SchemaError("config owner roles are required")
    if set(owners) != set(config_enum["values"]):
        raise SchemaError("every config key must have exactly one owner role")
    if not set(owners.values()).issubset({"PRIMARY_CONTROLLER", "COMMUNICATOR"}):
        raise SchemaError("config owner role is not authorized")
    if dict(owners) != {
        "SPORT_AUTOMATION_ENABLED": "COMMUNICATOR",
        "SPORT_ENTRY_SPEED_TENTH_KPH": "COMMUNICATOR",
        "SPORT_ACCELERATION_ENABLED": "COMMUNICATOR",
        "RTC_LOCAL_TIME": "PRIMARY_CONTROLLER",
        "SUNRISE_MINUTES": "PRIMARY_CONTROLLER",
        "SUNSET_MINUTES": "PRIMARY_CONTROLLER",
        "HEADLAMP_WARNING_ENABLED": "PRIMARY_CONTROLLER",
        "CAN_RX_STREAM_PERIOD_MS": "COMMUNICATOR",
        "CAN_RX_STREAM_MAX_RECORDS": "COMMUNICATOR",
        "CAN_RX_BYTES_PER_SECOND": "COMMUNICATOR",
    }:
        raise SchemaError("config owner roles changed")
    if config_policy.get("owner_binding") != {
        "CONFIG_GET": "receiver",
        "CONFIG_SET": "receiver",
        "REMOTE_CONFIG_REQUEST": "receiver",
    }:
        raise SchemaError("config owner binding policy changed")

    phases = schema.get("pairing_phases")
    negatives = schema.get("pairing_negative_vectors")
    phase_names = {str(phase.get("message")) for phase in phases} if isinstance(phases, Sequence) else set()
    if (
        not isinstance(phases, Sequence)
        or isinstance(phases, (str, bytes))
        or tuple(str(phase.get("message")) for phase in phases) != EXPECTED_PAIRING_PHASE_ORDER
    ):
        raise SchemaError("pairing phases are incomplete")
    messages_by_name = {str(message["name"]): message for message in schema["messages"]}
    if phase_names != set(EXPECTED_PAIRING_PHASES):
        raise SchemaError("pairing phase set changed")
    for phase in phases if isinstance(phases, Sequence) else ():
        if not isinstance(phase, Mapping) or phase.get("message") not in messages_by_name:
            raise SchemaError(f"pairing phase references an unknown message: {phase!r}")
        message_name = str(phase["message"])
        expected_phase = EXPECTED_PAIRING_PHASES[message_name]
        if phase.get("domain") != expected_phase["domain"]:
            raise SchemaError(f"pairing domain changed for {message_name}")
        if tuple(phase.get("canonical_fields", ())) != expected_phase["canonical_fields"]:
            raise SchemaError(f"pairing canonical order changed for {message_name}")
        expected_optional = {
            key: value for key, value in expected_phase.items()
            if key not in {"domain", "canonical_fields"}
        }
        actual_optional = {
            key: tuple(phase.get(key, ()))
            for key in expected_optional
        }
        if actual_optional != expected_optional:
            raise SchemaError(f"pairing mutation contract changed for {message_name}")
        unexpected_keys = set(phase) - {"message", "domain", "canonical_fields", *expected_optional}
        if unexpected_keys:
            raise SchemaError(f"pairing phase has unexpected keys: {unexpected_keys}")
        canonical_fields = phase.get("canonical_fields")
        if (
            not isinstance(canonical_fields, Sequence)
            or isinstance(canonical_fields, (str, bytes))
            or len(canonical_fields) != len(set(canonical_fields))
            or not set(canonical_fields).issubset(_message_field_names(messages_by_name[message_name]))
        ):
            raise SchemaError(f"pairing canonical fields do not match {message_name}")
    if not isinstance(negatives, Sequence) or isinstance(negatives, (str, bytes)):
        raise SchemaError("pairing_negative_vectors must be a list")
    seen_names: set[str] = set()
    for vector in negatives:
        if not isinstance(vector, Mapping):
            raise SchemaError("pairing negative vector must be an object")
        name = vector.get("name")
        if not isinstance(name, str) or name in seen_names:
            raise SchemaError("pairing negative vector names must be unique")
        expected_vector = EXPECTED_PAIRING_NEGATIVES.get(name)
        if expected_vector is None or (
            vector.get("phase"), vector.get("mutation"), vector.get("expected")
        ) != expected_vector:
            raise SchemaError(f"invalid pairing negative vector: {vector!r}")
        if set(vector) != {"name", "phase", "mutation", "expected"}:
            raise SchemaError(f"pairing negative vector has unexpected keys: {vector!r}")
        seen_names.add(name)
    if seen_names != set(EXPECTED_PAIRING_NEGATIVES):
        raise SchemaError("pairing negative vector catalog changed")
    if {str(vector.get("phase")) for vector in negatives} != phase_names:
        raise SchemaError("each pairing phase needs an independent negative vector")


def _validate_frame_and_semantic_policy(
    schema: Mapping[str, Any],
    messages_by_name: Mapping[str, Mapping[str, Any]],
) -> None:
    frame_policy = schema.get("frame_policy")
    if not isinstance(frame_policy, Mapping):
        raise SchemaError("frame_policy is required")
    if set(frame_policy) != {
        "response_messages", "response_variants", "variant_policies", "response_correlation_required", "fragment_messages", "session_zero_messages",
        "authenticated_session_zero_messages", "secure_messages"
    }:
        raise SchemaError("frame_policy keys changed")
    response_messages = frame_policy.get("response_messages")
    response_variants = frame_policy.get("response_variants")
    variant_policies = frame_policy.get("variant_policies")
    response_correlation_required = frame_policy.get("response_correlation_required")
    fragment_messages = frame_policy.get("fragment_messages")
    session_zero_messages = frame_policy.get("session_zero_messages")
    authenticated_session_zero_messages = frame_policy.get("authenticated_session_zero_messages")
    secure_messages = frame_policy.get("secure_messages")
    for label, values in (
        ("response_messages", response_messages),
        ("fragment_messages", fragment_messages),
        ("session_zero_messages", session_zero_messages),
        ("authenticated_session_zero_messages", authenticated_session_zero_messages),
        ("secure_messages", secure_messages),
    ):
        if not isinstance(values, Sequence) or isinstance(values, (str, bytes)):
            raise SchemaError(f"frame_policy.{label} must be a list")
        if len(values) != len(set(values)) or not set(values).issubset(messages_by_name):
            raise SchemaError(f"frame_policy.{label} has invalid message names")
    if not isinstance(response_variants, Sequence) or isinstance(response_variants, (str, bytes)):
        raise SchemaError("frame_policy.response_variants must be a list")
    parsed_response_variants: set[tuple[str, str]] = set()
    for entry in response_variants:
        if not isinstance(entry, Mapping) or set(entry) != {"message", "variant"}:
            raise SchemaError("frame_policy.response_variants entry is invalid")
        pair = (str(entry["message"]), str(entry["variant"]))
        if pair in parsed_response_variants or pair[0] not in messages_by_name:
            raise SchemaError("frame_policy.response_variants has invalid message")
        if pair[1] not in _message_variant_names(messages_by_name[pair[0]]):
            raise SchemaError("frame_policy.response_variants has invalid variant")
        parsed_response_variants.add(pair)
    if not isinstance(variant_policies, Sequence) or isinstance(variant_policies, (str, bytes)):
        raise SchemaError("frame_policy.variant_policies must be a list")
    expected_variant_policies = {
        ("DIAGNOSTIC_LEASE", "request", ("DIAGNOSTIC_BRIDGE",), ("COMMUNICATOR",)),
        ("DIAGNOSTIC_LEASE", "response", ("COMMUNICATOR",), ("DIAGNOSTIC_BRIDGE",)),
    }
    parsed_variant_policies: set[tuple[str, str, tuple[str, ...], tuple[str, ...]]] = set()
    for entry in variant_policies:
        if not isinstance(entry, Mapping) or set(entry) != {"message", "variant", "senders", "receivers"}:
            raise SchemaError("frame_policy.variant_policies entry is invalid")
        senders = entry["senders"]
        receivers = entry["receivers"]
        if (
            not isinstance(senders, Sequence)
            or isinstance(senders, (str, bytes))
            or not isinstance(receivers, Sequence)
            or isinstance(receivers, (str, bytes))
        ):
            raise SchemaError("frame_policy.variant_policies roles must be lists")
        message_name = str(entry["message"])
        variant_name = str(entry["variant"])
        policy = (message_name, variant_name, tuple(str(role) for role in senders), tuple(str(role) for role in receivers))
        if message_name not in messages_by_name or variant_name not in _message_variant_names(messages_by_name[message_name]):
            raise SchemaError("frame_policy.variant_policies has invalid message or variant")
        if policy in parsed_variant_policies or not set(policy[2]).issubset(set(messages_by_name[message_name]["senders"])):
            raise SchemaError("frame_policy.variant_policies has invalid sender roles")
        if not set(policy[3]).issubset(set(messages_by_name[message_name]["receivers"])):
            raise SchemaError("frame_policy.variant_policies has invalid receiver roles")
        parsed_variant_policies.add(policy)
    if response_correlation_required is not True:
        raise SchemaError("response correlation policy changed")
    if set(response_messages) != EXPECTED_RESPONSE_MESSAGES:
        raise SchemaError("response message policy changed")
    if parsed_response_variants != EXPECTED_RESPONSE_VARIANTS:
        raise SchemaError("response variant policy changed")
    if parsed_variant_policies != expected_variant_policies:
        raise SchemaError("variant direction policy changed")
    if set(fragment_messages) != EXPECTED_FRAGMENT_MESSAGES:
        raise SchemaError("fragment message policy changed")
    if set(session_zero_messages) != PAIRING_MESSAGES:
        raise SchemaError("session-zero message policy changed")
    if set(authenticated_session_zero_messages) != EXPECTED_AUTHENTICATED_SESSION_ZERO_MESSAGES:
        raise SchemaError("authenticated session-zero message policy changed")
    if not set(authenticated_session_zero_messages).issubset(set(session_zero_messages)):
        raise SchemaError("authenticated session-zero messages must be pairing messages")
    for message_name in authenticated_session_zero_messages:
        if not messages_by_name[message_name].get("encrypted"):
            raise SchemaError(f"authenticated session-zero message must be encrypted: {message_name}")
    if set(secure_messages) != set(messages_by_name) - PAIRING_MESSAGES:
        raise SchemaError("secure message policy does not cover the message catalog")
    if set(session_zero_messages) & set(secure_messages):
        raise SchemaError("session-zero and secure message policies overlap")
    encrypted_messages = {
        name for name, message in messages_by_name.items() if message.get("encrypted")
    }
    if encrypted_messages != set(secure_messages) | set(authenticated_session_zero_messages):
        raise SchemaError("encrypted messages must have secure or authenticated pre-session policy")

    semantic_policy = schema.get("semantic_policy")
    if not isinstance(semantic_policy, Mapping):
        raise SchemaError("semantic_policy is required")
    expected_role_fields = {
        ("HELLO", "role"),
        ("CAPABILITIES", "role"),
        ("PAIR_REQUEST", "requested_role"),
        ("PAIR_CHALLENGE", "authorized_role"),
        ("PAIR_CONFIRM", "authorized_role"),
        ("PAIR_RESULT", "assigned_role"),
    }
    expected_authenticated_role_fields = {("HELLO", "role"), ("CAPABILITIES", "role")}
    expected_scope_fields = {
        ("CAPABILITIES", "control_scope"),
        ("PAIR_CHALLENGE", "authorized_scope"),
        ("CONTROL_LEASE_REQUEST", "requested_scope"),
        ("CONTROL_LEASE_STATUS", "granted_scope"),
    }
    expected_command_fields = {("COMMAND_REQUEST", "command_id"), ("COMMAND_RESULT", "command_id")}
    expected_enum_fields = {
        ("ACK", "status", "ack_status"),
        ("ERROR", "code", "error_code"),
        ("HEARTBEAT", "link_state", "link_state"),
        ("STATE_SNAPSHOT", "link_state", "link_state"),
        ("CAN_FILTER_SET", "action", "filter_action"),
        ("CAN_FILTER_RESULT", "action", "filter_action"),
        ("CAN_FILTER_RESULT", "result", "filter_result"),
        ("CAN_OBSERVER_CONFIG", "mode", "observer_mode"),
        ("CAN_CAPTURE_CONTROL", "action", "capture_action"),
        ("CAN_CAPTURE_STATUS", "state", "capture_state"),
        ("CAN_EVENT_MARKER", "marker_kind", "marker_kind"),
        ("COMMAND_REQUEST", "command_id", "command_id"),
        ("COMMAND_RESULT", "command_id", "command_id"),
        ("COMMAND_RESULT", "stage", "command_stage"),
        ("CONFIG_RESULT", "stage", "remote_config_stage"),
        ("REMOTE_CONFIG_STATUS", "stage", "remote_config_stage"),
        ("DIAGNOSTIC_LEASE", "action", "diagnostic_lease_action"),
        ("DIAGNOSTIC_LEASE", "status", "diagnostic_lease_status"),
        ("BULK_ACK", "status", "bulk_status"),
        ("BULK_END", "status", "bulk_status"),
    }
    expected_record_enum_fields = {
        ("SIGNAL_BATCH", "value_type", "value_type"),
        ("SIGNAL_BATCH", "quality", "signal_quality"),
        ("SIGNAL_BATCH", "evidence_grade", "evidence_grade"),
        ("CONFIG_SET", "value_type", "value_type"),
        ("REMOTE_CONFIG_REQUEST", "value_type", "value_type"),
    }
    expected_config_key_fields = {
        ("CONFIG_GET", "key"),
        ("CONFIG_SET", "key"),
        ("REMOTE_CONFIG_REQUEST", "key"),
    }
    expected_bitmask_fields = {
        ("PAIR_CHALLENGE", "allowed_message_classes", "message_class"),
        ("COMMAND_REQUEST", "precondition_flags", "precondition"),
    }
    expected_record_bitmask_fields = {
        ("CAN_FILTER_SET", "flags_value", "can_flag"),
        ("CAN_FILTER_SET", "flags_mask", "can_flag"),
    }
    expected_fixed_bitmask_fields = {
        ("CAN_OBSERVER_CONFIG", "bus_mask", 7),
        ("CAN_OBSERVER_CONFIG", "flags", 0),
        ("BULK_ACK", "received_bitmap", 15),
    }
    expected_cross_field_constraints = {
        ("BULK_ACK", "bitmap_with_window", "received_bitmap", "window_size"),
    }
    expected_range_fields = {
        ("COMMAND_REQUEST", "ttl_ms", 500, 30000),
        ("BULK_BEGIN", "total_size", 1, 65536),
        ("BULK_BEGIN", "fragment_size", 1, 192),
        ("BULK_BEGIN", "window_size", 1, 4),
        ("BULK_BEGIN", "timeout_ms", 1000, 30000),
        ("BULK_FRAGMENT", "total_fragments", 1, 65536),
        ("BULK_FRAGMENT", "payload_len", 1, 180),
        ("BULK_ACK", "window_size", 1, 4),
        ("BULK_ACK", "received_bytes", 0, 65536),
        ("BULK_END", "total_size", 1, 65536),
    }
    enum_names = {str(enum.get("name")) for enum in schema.get("enums", ())}

    def policy_pairs(key: str) -> set[tuple[str, str]]:
        entries = semantic_policy.get(key)
        if not isinstance(entries, Sequence) or isinstance(entries, (str, bytes)):
            raise SchemaError(f"semantic_policy.{key} must be a list")
        pairs: set[tuple[str, str]] = set()
        for entry in entries:
            if not isinstance(entry, Mapping) or set(entry) != {"message", "field"}:
                raise SchemaError(f"semantic_policy.{key} entry is invalid")
            pair = (str(entry["message"]), str(entry["field"]))
            if pair in pairs or pair[0] not in messages_by_name:
                raise SchemaError(f"semantic_policy.{key} contains an invalid message")
            if pair[1] not in _message_field_names(messages_by_name[pair[0]]):
                raise SchemaError(f"semantic_policy.{key} references a missing field")
            pairs.add(pair)
        return pairs

    def policy_triples(key: str) -> set[tuple[str, str, str]]:
        entries = semantic_policy.get(key)
        if not isinstance(entries, Sequence) or isinstance(entries, (str, bytes)):
            raise SchemaError(f"semantic_policy.{key} must be a list")
        triples: set[tuple[str, str, str]] = set()
        for entry in entries:
            if not isinstance(entry, Mapping) or set(entry) != {"message", "field", "enum"}:
                raise SchemaError(f"semantic_policy.{key} entry is invalid")
            triple = (str(entry["message"]), str(entry["field"]), str(entry["enum"]))
            if triple in triples or triple[0] not in messages_by_name or triple[2] not in enum_names:
                raise SchemaError(f"semantic_policy.{key} contains an invalid field or enum")
            if triple[1] not in _message_field_names(messages_by_name[triple[0]]):
                raise SchemaError(f"semantic_policy.{key} references a missing field")
            triples.add(triple)
        return triples

    def policy_fixed_masks(key: str) -> set[tuple[str, str, int]]:
        entries = semantic_policy.get(key)
        if not isinstance(entries, Sequence) or isinstance(entries, (str, bytes)):
            raise SchemaError(f"semantic_policy.{key} must be a list")
        masks: set[tuple[str, str, int]] = set()
        for entry in entries:
            if not isinstance(entry, Mapping) or set(entry) != {"message", "field", "known_mask"}:
                raise SchemaError(f"semantic_policy.{key} entry is invalid")
            try:
                item = (str(entry["message"]), str(entry["field"]), int(entry["known_mask"]))
            except (TypeError, ValueError) as exc:
                raise SchemaError(f"semantic_policy.{key} entry has an invalid mask") from exc
            if item in masks or item[0] not in messages_by_name or item[2] < 0:
                raise SchemaError(f"semantic_policy.{key} contains an invalid field or mask")
            if item[1] not in _message_field_names(messages_by_name[item[0]]):
                raise SchemaError(f"semantic_policy.{key} references a missing field")
            masks.add(item)
        return masks

    def policy_ranges(key: str) -> set[tuple[str, str, int, int]]:
        entries = semantic_policy.get(key)
        if not isinstance(entries, Sequence) or isinstance(entries, (str, bytes)):
            raise SchemaError(f"semantic_policy.{key} must be a list")
        ranges: set[tuple[str, str, int, int]] = set()
        for entry in entries:
            if not isinstance(entry, Mapping) or set(entry) != {"message", "field", "minimum", "maximum"}:
                raise SchemaError(f"semantic_policy.{key} entry is invalid")
            try:
                item = (
                    str(entry["message"]),
                    str(entry["field"]),
                    int(entry["minimum"]),
                    int(entry["maximum"]),
                )
            except (TypeError, ValueError) as exc:
                raise SchemaError(f"semantic_policy.{key} entry has an invalid range") from exc
            if item in ranges or item[0] not in messages_by_name or item[2] > item[3]:
                raise SchemaError(f"semantic_policy.{key} contains an invalid range")
            if item[1] not in _message_field_names(messages_by_name[item[0]]):
                raise SchemaError(f"semantic_policy.{key} references a missing field")
            ranges.add(item)
        return ranges

    def policy_cross_field_constraints(key: str) -> set[tuple[str, str, str, str]]:
        entries = semantic_policy.get(key)
        if not isinstance(entries, Sequence) or isinstance(entries, (str, bytes)):
            raise SchemaError(f"semantic_policy.{key} must be a list")
        constraints: set[tuple[str, str, str, str]] = set()
        for entry in entries:
            if not isinstance(entry, Mapping) or set(entry) != {"message", "kind", "bitmap_field", "window_field"}:
                raise SchemaError(f"semantic_policy.{key} entry is invalid")
            item = (
                str(entry["message"]),
                str(entry["kind"]),
                str(entry["bitmap_field"]),
                str(entry["window_field"]),
            )
            if item in constraints or item[0] not in messages_by_name or item[1] != "bitmap_with_window":
                raise SchemaError(f"semantic_policy.{key} contains an invalid constraint")
            fields = _message_field_names(messages_by_name[item[0]])
            if item[2] not in fields or item[3] not in fields:
                raise SchemaError(f"semantic_policy.{key} references a missing field")
            constraints.add(item)
        return constraints

    if policy_pairs("role_fields") != expected_role_fields:
        raise SchemaError("role semantic field policy changed")
    if policy_pairs("authenticated_role_fields") != expected_authenticated_role_fields:
        raise SchemaError("authenticated role semantic field policy changed")
    if policy_pairs("scope_fields") != expected_scope_fields:
        raise SchemaError("scope semantic field policy changed")
    if policy_pairs("command_id_fields") != expected_command_fields:
        raise SchemaError("command semantic field policy changed")
    if policy_triples("enum_fields") != expected_enum_fields:
        raise SchemaError("enum semantic field policy changed")
    if policy_triples("record_enum_fields") != expected_record_enum_fields:
        raise SchemaError("record enum semantic field policy changed")
    if policy_pairs("config_key_fields") != expected_config_key_fields:
        raise SchemaError("config key semantic field policy changed")
    if policy_triples("bitmask_fields") != expected_bitmask_fields:
        raise SchemaError("bitmask semantic field policy changed")
    if policy_triples("record_bitmask_fields") != expected_record_bitmask_fields:
        raise SchemaError("record bitmask semantic field policy changed")
    if policy_fixed_masks("fixed_bitmask_fields") != expected_fixed_bitmask_fields:
        raise SchemaError("fixed bitmask semantic field policy changed")
    if policy_cross_field_constraints("cross_field_constraints") != expected_cross_field_constraints:
        raise SchemaError("cross-field semantic policy changed")
    if policy_ranges("range_fields") != expected_range_fields:
        raise SchemaError("range semantic field policy changed")
    capability = semantic_policy.get("capability_authorization")
    if not isinstance(capability, Mapping) or capability != {
        "message": "CAPABILITIES",
        "role_field": "role",
        "scope_field": "control_scope",
        "zero_scope_roles": ["READ_ONLY_CONTROLLER", "DIAGNOSTIC_BRIDGE"],
        "authorized_scope_roles": ["PRIMARY_CONTROLLER", "COMMUNICATOR"],
    }:
        raise SchemaError("capability authorization policy changed")


def validate_schema(schema: Mapping[str, Any]) -> None:
    version = schema.get("version")
    if version != {"major": 1, "minor": 3, "wire_name": "ESP-NOW v1.3"}:
        raise SchemaError(f"only ESP-NOW v1.3 is supported, got {version!r}")
    encoding = schema.get("encoding")
    if not isinstance(encoding, Mapping):
        raise SchemaError("encoding must be an object")
    if encoding.get("byte_order") != "little":
        raise SchemaError("wire byte order must be little")
    if encoding.get("header_size") != 32 or encoding.get("max_frame_size") != 240:
        raise SchemaError("v1.3 frame/header limits changed")
    if encoding.get("max_payload_size") != 208:
        raise SchemaError("v1.3 payload limit must be 208")
    crc = encoding.get("crc")
    if not isinstance(crc, Mapping) or crc.get("name") != "CRC-32/ISO-HDLC":
        raise SchemaError("unsupported CRC contract")
    if {
        "polynomial_reflected": crc.get("polynomial_reflected"),
        "initial": crc.get("initial"),
        "xor_out": crc.get("xor_out"),
        "check_123456789": crc.get("check_123456789"),
    } != {
        "polynomial_reflected": 3988292384,
        "initial": 4294967295,
        "xor_out": 4294967295,
        "check_123456789": 3421780262,
    }:
        raise SchemaError("CRC metadata does not match the codec oracle")

    header = schema.get("header")
    if not isinstance(header, Mapping):
        raise SchemaError("header must be an object")
    _validate_fields(header["fields"], 32, "header")
    if header.get("reserved_zero") != ["reserved"]:
        raise SchemaError("header reserved_zero must identify the reserved field")
    if header.get("flags_known_mask") != 0x7F:
        raise SchemaError("unexpected frame flag mask")
    crc_fields = [field for field in header["fields"] if field.get("name") == header.get("crc_field")]
    if header.get("crc_field") != "crc32" or len(crc_fields) != 1 or crc_fields[0].get("type") != "u32" or int(crc_fields[0].get("offset", -1)) != 28:
        raise SchemaError("header CRC field metadata does not match the wire codec")

    constants = schema.get("constants")
    if not isinstance(constants, Sequence):
        raise SchemaError("constants must be a list")
    constant_names: set[str] = set()
    for constant in constants:
        name = constant.get("c_name")
        if not isinstance(name, str):
            raise SchemaError("constant c_name is required")
        _validate_identifier(name, "constant")
        if name in constant_names:
            raise SchemaError(f"duplicate constant {name}")
        constant_names.add(name)
        if constant.get("type") not in TYPE_INFO:
            raise SchemaError(f"invalid constant type {constant.get('type')}")
        if not isinstance(constant.get("value"), int):
            raise SchemaError(f"constant {name} must have an integer value")
    try:
        magic_bytes = bytes.fromhex(str(encoding["magic_bytes_hex"]))
    except (KeyError, ValueError) as exc:
        raise SchemaError("encoding.magic_bytes_hex must be valid bytes") from exc
    if len(magic_bytes) != 2 or int.from_bytes(magic_bytes, "little") != next(
        constant["value"] for constant in constants if constant["c_name"] == "CANVIEW_MAGIC_LE"
    ):
        raise SchemaError("magic_bytes_hex does not match CANVIEW_MAGIC_LE")

    enums = schema.get("enums")
    if not isinstance(enums, Sequence):
        raise SchemaError("enums must be a list")
    enum_types: set[str] = set()
    enum_names: set[str] = set()
    enumerators: set[str] = set()
    enum_prefix_counts: dict[str, int] = {}
    enum_alias_prefixes: set[str] = set()
    for enum in enums:
        enum_name = enum.get("name")
        c_type = enum.get("c_type")
        prefix = enum.get("c_prefix")
        values = enum.get("values")
        if not isinstance(enum_name, str) or not isinstance(c_type, str) or not isinstance(prefix, str) or not isinstance(values, Mapping):
            raise SchemaError(f"invalid enum declaration {enum!r}")
        _validate_identifier(enum_name, "enum name")
        value_kind = enum.get("value_kind", "enum")
        if value_kind not in {"enum", "bitmask"}:
            raise SchemaError(f"invalid enum value_kind for {c_type}: {value_kind}")
        _validate_identifier(c_type, "enum c_type")
        _validate_identifier(prefix, "enum c_prefix")
        enum_prefix_counts[prefix] = enum_prefix_counts.get(prefix, 0) + 1
        alias_prefixes = enum.get("alias_prefixes", [])
        if (
            not isinstance(alias_prefixes, Sequence)
            or isinstance(alias_prefixes, (str, bytes))
        ):
            raise SchemaError(f"invalid enum alias_prefixes for {c_type}")
        alias_prefixes_seen: set[str] = set()
        for alias_prefix in alias_prefixes:
            if not isinstance(alias_prefix, str):
                raise SchemaError(f"invalid enum alias prefix for {c_type}")
            _validate_identifier(alias_prefix, "enum alias_prefix")
            if (
                alias_prefix == prefix
                or alias_prefix in alias_prefixes_seen
                or alias_prefix in enum_alias_prefixes
            ):
                raise SchemaError(f"duplicate enum public prefix {alias_prefix}")
            alias_prefixes_seen.add(alias_prefix)
            enum_alias_prefixes.add(alias_prefix)
        macro_prefix = enum.get("macro_prefix")
        if macro_prefix is not None:
            if not isinstance(macro_prefix, str):
                raise SchemaError(f"invalid enum macro_prefix for {c_type}")
            _validate_identifier(macro_prefix, "enum macro_prefix")
        if c_type in enum_types:
            raise SchemaError(f"duplicate enum type {c_type}")
        if enum_name in enum_names:
            raise SchemaError(f"duplicate enum name {enum_name}")
        enum_types.add(c_type)
        enum_names.add(enum_name)
        enum_values: set[int] = set()
        for name, value in values.items():
            enum_name = f"{prefix}_{name}"
            _validate_identifier(enum_name, "enum value")
            if enum_name in enumerators:
                raise SchemaError(f"duplicate enumerator {enum_name}")
            if not isinstance(value, int):
                raise SchemaError(f"enum value must be an integer: {enum_name}")
            if value_kind == "bitmask" and (value <= 0 or value & (value - 1)):
                raise SchemaError(f"bitmask enum value must be a single bit: {enum_name}")
            if value in enum_values:
                raise SchemaError(f"duplicate enum numeric value in {c_type}: {value}")
            enumerators.add(enum_name)
            enum_values.add(value)
    enum_public_prefixes = {
        str(enum["c_prefix"])
        for enum in enums
    } | enum_alias_prefixes
    if len(enum_public_prefixes) != len({str(enum["c_prefix"]) for enum in enums}) + len(enum_alias_prefixes):
        raise SchemaError("enum alias prefix collides with an enum c_prefix")
    alias_enumerators: set[str] = set()
    for enum in enums:
        for alias_prefix in enum.get("alias_prefixes", []):
            for name in enum["values"]:
                alias_name = f"{alias_prefix}_{name}"
                if alias_name in enumerators or alias_name in alias_enumerators:
                    raise SchemaError(f"duplicate enum alias {alias_name}")
                alias_enumerators.add(alias_name)
    effective_macro_prefixes: set[str] = set()
    effective_macro_prefix_by_enum: dict[str, str] = {}
    for enum in enums:
        enum_prefix = str(enum["c_prefix"])
        macro_prefix = str(enum.get("macro_prefix") or (
            enum_prefix
            if enum_prefix_counts[enum_prefix] == 1
            else f"{enum_prefix}_{str(enum['name']).upper()}"
        ))
        if macro_prefix in effective_macro_prefixes:
            raise SchemaError(f"duplicate effective enum macro_prefix {macro_prefix}")
        effective_macro_prefixes.add(macro_prefix)
        effective_macro_prefix_by_enum[str(enum["name"])] = macro_prefix

    generated_symbols: dict[str, str] = {}

    def register_generated_symbol(symbol: str, origin: str) -> None:
        previous = generated_symbols.get(symbol)
        if previous is not None:
            raise SchemaError(
                f"generated symbol collision {symbol}: {previous} vs {origin}"
            )
        generated_symbols[symbol] = origin

    for symbol in (
        "CANVIEW_PROTOCOL_H",
        "CANVIEW_PROTOCOL_SCHEMA_SHA256",
        "CANVIEW_PROTOCOL_WIRE_NAME",
        "CANVIEW_PACKED",
        "CANVIEW_PROTOCOL_VERSION",
        "CANVIEW_PROTOCOL_KNOWN_FLAG_MASK",
        "CANVIEW_MESSAGE_COUNT",
    ):
        register_generated_symbol(symbol, "fixed generated header symbol")
    for constant in schema["constants"]:
        register_generated_symbol(str(constant["c_name"]), "constant")
    for enum in enums:
        enum_name = str(enum["name"])
        prefix = str(enum["c_prefix"])
        for value_name in enum["values"]:
            register_generated_symbol(
                f"{prefix}_{value_name}", f"{enum_name} enumerator"
            )
        for alias_prefix in enum.get("alias_prefixes", ()):
            for value_name in enum["values"]:
                register_generated_symbol(
                    f"{alias_prefix}_{value_name}",
                    f"{enum_name} compatibility alias",
                )
        known_prefix = effective_macro_prefix_by_enum[enum_name]
        register_generated_symbol(
            f"{known_prefix}_KNOWN_VALUE_COUNT",
            f"{enum_name} known-value macro",
        )
        numeric_values = [int(value) for value in enum["values"].values()]
        if numeric_values and (
            enum.get("value_kind", "enum") == "bitmask"
            or (max(numeric_values) <= 31 and min(numeric_values) >= 0)
        ):
            register_generated_symbol(
                f"{known_prefix}_KNOWN_MASK",
                f"{enum_name} known-mask macro",
            )
    for message in schema["messages"]:
        register_generated_symbol(
            f"CANVIEW_MSG_{message['name']}", "message enumerator"
        )
    enum_by_name = {str(enum["name"]): enum for enum in enums}
    role_enum = enum_by_name.get("role")
    state_enum = enum_by_name.get("link_state")
    if not isinstance(role_enum, Mapping) or not isinstance(state_enum, Mapping):
        raise SchemaError("role and link_state enums are required")
    role_values = set(role_enum["values"])
    state_values = set(state_enum["values"])

    _validate_tlv_policy(schema.get("tlv"), "tlv")
    _validate_tlv_policy(schema.get("command_argument_tlv"), "command_argument_tlv")

    qos_values = {str(m.get("qos")) for m in schema["messages"]}
    if not qos_values.issubset(KNOWN_QOS):
        raise SchemaError(f"unknown QoS values: {qos_values - KNOWN_QOS}")
    message_ids: set[int] = set()
    message_names: set[str] = set()
    for message in schema.get("messages", []):
        message_id = message.get("id")
        message_name = message.get("name")
        if not isinstance(message_id, int) or not 0 < message_id < 256:
            raise SchemaError(f"invalid message id: {message_id}")
        if message_id in message_ids or message_name in message_names:
            raise SchemaError(f"duplicate message {message_id}/{message_name}")
        message_ids.add(message_id)
        message_names.add(message_name)
        _validate_identifier(str(message_name), "message name")
        if not message.get("senders") or not message.get("receivers") or not message.get("states"):
            raise SchemaError(f"message {message_name} must declare sender/receiver/state")
        for field_name, allowed in (("senders", role_values), ("receivers", role_values), ("states", state_values)):
            values = message.get(field_name)
            if (
                not isinstance(values, Sequence)
                or isinstance(values, (str, bytes))
                or len(values) != len(set(values))
                or not set(values).issubset(allowed)
            ):
                raise SchemaError(f"message {message_name} has invalid {field_name}")
        if not isinstance(message.get("broadcast"), bool) or not isinstance(message.get("encrypted"), bool):
            raise SchemaError(f"message {message_name} must declare boolean broadcast/encrypted policy")
        if message.get("ack") not in KNOWN_ACK:
            raise SchemaError(f"message {message_name} has invalid ack policy")
        if not isinstance(message.get("priority"), int) or not 0 <= message["priority"] <= 4:
            raise SchemaError(f"message {message_name} has invalid priority")
        payload = message.get("payload")
        if not isinstance(payload, Mapping):
            raise SchemaError(f"message {message_name} has no payload")
        kind = payload.get("kind")
        if kind == "fixed":
            size = payload.get("size")
            if not isinstance(size, int) or not 0 <= size <= 208:
                raise SchemaError(f"invalid fixed payload size for {message_name}")
            _validate_fields(payload["fields"], size, f"{message_name}.payload")
        elif kind == "bounded":
            prefix = payload.get("prefix")
            record = payload.get("record")
            count_max = payload.get("count_max")
            if not isinstance(prefix, Mapping) or not isinstance(record, Mapping):
                raise SchemaError(f"bounded payload {message_name} needs prefix and record")
            _validate_fields(prefix["fields"], int(prefix["size"]), f"{message_name}.prefix")
            _validate_fields(record["fields"], int(record["size"]), f"{message_name}.record")
            count_field = payload.get("count_field")
            count = next((f for f in prefix["fields"] if f["name"] == count_field), None)
            if count is None or count["type"] not in {"u8", "u16", "u32"}:
                raise SchemaError(f"bounded payload {message_name} has invalid count field")
            if not isinstance(count_max, int) or count_max < 0:
                raise SchemaError(f"bounded payload {message_name} has invalid count_max")
            if int(prefix["size"]) + int(record["size"]) * count_max > 208:
                raise SchemaError(f"bounded payload {message_name} exceeds 208 bytes")
        elif kind == "suffix":
            prefix = payload.get("prefix")
            if not isinstance(prefix, Mapping):
                raise SchemaError(f"suffix payload {message_name} needs prefix")
            _validate_fields(prefix["fields"], int(prefix["size"]), f"{message_name}.prefix")
            if int(payload.get("prefix_size", -1)) != int(prefix["size"]):
                raise SchemaError(f"suffix payload {message_name} prefix_size mismatch")
            if int(payload.get("suffix_max", -1)) + int(prefix["size"]) > 208:
                raise SchemaError(f"suffix payload {message_name} exceeds 208 bytes")
        elif kind == "tlv":
            prefix = payload.get("prefix")
            if not isinstance(prefix, Mapping):
                raise SchemaError(f"TLV payload {message_name} needs prefix")
            _validate_fields(prefix["fields"], int(prefix["size"]), f"{message_name}.prefix")
            if int(payload.get("prefix_size", -1)) != int(prefix["size"]):
                raise SchemaError(f"TLV payload {message_name} prefix_size mismatch")
            if int(prefix["size"]) + int(payload.get("max_tlv_bytes", -1)) > 208:
                raise SchemaError(f"TLV payload {message_name} exceeds 208 bytes")
        elif kind == "variants":
            variants = payload.get("variants")
            if not isinstance(variants, Sequence) or not variants:
                raise SchemaError(f"variants payload {message_name} is empty")
            for variant in variants:
                size = variant.get("size")
                _validate_fields(variant["fields"], int(size), f"{message_name}.{variant['name']}")
                if int(size) > 208:
                    raise SchemaError(f"variant {message_name}.{variant['name']} exceeds 208 bytes")
        else:
            raise SchemaError(f"unknown payload kind for {message_name}: {kind}")

    required = schema.get("required_message_ids")
    if sorted(required) != sorted(message_ids):
        raise SchemaError("required_message_ids must exactly match declared message IDs")
    _validate_vector_catalog(schema)
    _validate_companion_schemas(schema, message_ids)
    messages_by_name = {str(message["name"]): message for message in schema["messages"]}
    _validate_frame_and_semantic_policy(schema, messages_by_name)
    contracts = schema.get("message_contracts")
    if not isinstance(contracts, Mapping) or set(contracts) != message_names:
        raise SchemaError("message_contracts must describe every declared message exactly once")
    for message_name, contract in contracts.items():
        if not isinstance(contract, Mapping) or contract.get("since") != "1.3":
            raise SchemaError(f"message contract {message_name} must declare since=1.3")
        response = contract.get("response")
        if response is not None and response not in message_names:
            raise SchemaError(f"message contract {message_name} references unknown response {response}")
        idempotency_key = contract.get("idempotency_key")
        if (
            not isinstance(idempotency_key, Sequence)
            or isinstance(idempotency_key, (str, bytes))
            or not idempotency_key
            or len(idempotency_key) != len(set(idempotency_key))
            or not set(idempotency_key).issubset(_message_field_names(messages_by_name[message_name]) | {"canonical_argument_digest"})
        ):
            raise SchemaError(f"message contract {message_name} has no idempotency key")
        if contract.get("sensitive_log_policy") not in {"allow", "redact"}:
            raise SchemaError(f"message contract {message_name} has invalid log policy")
    for _, size, fields in _layout_entries(schema):
        if size > 208 and fields is not schema["header"]["fields"]:
            raise SchemaError("payload layout exceeds maximum")
        for field in fields:
            if field["name"] == "catalog_revision" and field["type"] != "u32":
                raise SchemaError("catalog_revision must be a u32 revision")
            if field["name"] in {"error_count", "rx_count", "bus_off_count", "telemetry_dropped", "protocol_error_count"} and field["type"] != "u64":
                raise SchemaError(f"boot cumulative counter {field['name']} must be saturating u64")
            if field["name"] == "reason" and field["type"] != "u16":
                raise SchemaError("common reason fields must be u16")

    security = schema.get("security")
    if not security.get("shared_installation_secret_forbidden") or security.get("bridge_control_root"):
        raise SchemaError("security policy permits a forbidden shared/control root")
    if security.get("pair_root_bits") != 256 or security.get("lmk_bits") != 128:
        raise SchemaError("unexpected pairing key sizes")
    if security.get("control_scope_zero_roles") != ["READ_ONLY_CONTROLLER", "DIAGNOSTIC_BRIDGE"]:
        raise SchemaError("read-only roles must advertise zero control scope")
    if security.get("control_scope_authorized_roles") != ["PRIMARY_CONTROLLER", "COMMUNICATOR"]:
        raise SchemaError("control scope authorization roles changed")
    if schema["control_envelope"]["idempotency_key"][:5] != [
        "origin_device_id", "origin_boot_id", "wireless_session_id", "control_generation", "request_token"
    ]:
        raise SchemaError("control idempotency key lost its boot/session binding")
    _validate_config_and_pairing_policy(schema)


def _c_literal(value: int, type_name: str) -> str:
    bits = int(type_name[1:])
    prefix = "UINT" if type_name.startswith("u") else "INT"
    if value < 0:
        return str(value)
    rendered = hex(value) if value > 9 else str(value)
    return f"{prefix}{bits}_C({rendered})"


def _c_field_type(field: Mapping[str, Any]) -> str:
    field_type = field["type"]
    if field_type == "bytes":
        return "uint8_t"
    return TYPE_INFO[field_type][0]


def _render_struct(c_type: str, size: int, fields: Sequence[Mapping[str, Any]]) -> list[str]:
    lines = [f"typedef struct CANVIEW_PACKED {{"]
    for field in fields:
        name = _field_c_name(field)
        if field["type"] == "bytes":
            declaration = f"    uint8_t {name}[{int(field['length'])}];"
        else:
            declaration = f"    {_c_field_type(field)} {name};"
        comment = " /* reserved, transmit as zero */" if field.get("reserved") else ""
        lines.append(declaration + comment)
    lines.extend([f"}} {c_type};", ""])
    return lines


def _render_enum(enum: Mapping[str, Any], macro_prefix: str | None = None) -> list[str]:
    lines = ["typedef enum {"]
    values = list(enum["values"].items())
    for index, (name, value) in enumerate(values):
        comma = "," if index < len(values) - 1 else ","
        lines.append(f"    {enum['c_prefix']}_{name} = {_c_literal(int(value), 'u32')}{comma}")
    lines.extend([f"}} {enum['c_type']};", ""])
    for alias_prefix in enum.get("alias_prefixes", ()):
        for name, _ in values:
            lines.append(f"#define {alias_prefix}_{name} {enum['c_prefix']}_{name}")
        lines.append("")
    numeric_values = [int(value) for _, value in values]
    known_prefix = macro_prefix or str(enum["c_prefix"])
    lines.append(
        f"#define {known_prefix}_KNOWN_VALUE_COUNT "
        f"UINT16_C({len(numeric_values)})"
    )
    if numeric_values and enum.get("value_kind", "enum") == "bitmask":
        known_mask = 0
        for value in numeric_values:
            known_mask |= value
        lines.append(
            f"#define {known_prefix}_KNOWN_MASK "
            f"UINT32_C(0x{known_mask:08X})"
        )
    elif numeric_values and max(numeric_values) <= 31 and min(numeric_values) >= 0:
        known_mask = sum(1 << value for value in numeric_values)
        lines.append(
            f"#define {known_prefix}_KNOWN_MASK "
            f"UINT32_C(0x{known_mask:08X})"
        )
    lines.append("")
    return lines


def render_header(schema: Mapping[str, Any], schema_digest: str) -> str:
    lines: list[str] = [
        "/*",
        " * GENERATED FILE - DO NOT EDIT.",
        " * Source: protocol/schema/espnow-v1.3.yaml",
        " * Regenerate with: python tools/generate_protocol.py",
        " * SPDX-License-Identifier: GPL-3.0-only",
        " */",
        "#ifndef CANVIEW_PROTOCOL_H",
        "#define CANVIEW_PROTOCOL_H",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        f'#define CANVIEW_PROTOCOL_SCHEMA_SHA256 "{schema_digest}"',
        f'#define CANVIEW_PROTOCOL_WIRE_NAME "{schema["version"]["wire_name"]}"',
        "",
        "#ifdef __cplusplus",
        'extern "C" {',
        "#endif",
        "",
        "#if defined(_MSC_VER)",
        "#define CANVIEW_PACKED",
        "#pragma pack(push, 1)",
        "#elif defined(__GNUC__) || defined(__clang__)",
        "#define CANVIEW_PACKED __attribute__((packed))",
        "#else",
        "#define CANVIEW_PACKED",
        "#endif",
        "",
    ]
    for constant in schema["constants"]:
        lines.append(
            f"#define {constant['c_name']} "
            f"{_c_literal(int(constant['value']), str(constant['type']))}"
        )
    lines.extend([
        "#define CANVIEW_PROTOCOL_VERSION UINT16_C(0x0103)",
        "#define CANVIEW_PROTOCOL_KNOWN_FLAG_MASK UINT8_C(0x7F)",
        "",
    ])

    lines.append("typedef enum {")
    for message in schema["messages"]:
        lines.append(f"    CANVIEW_MSG_{message['name']} = 0x{int(message['id']):02X},")
    lines.extend([
        "} canview_message_type_t;",
        f"#define CANVIEW_MESSAGE_COUNT UINT8_C({len(schema['messages'])})",
        "",
    ])
    enum_prefix_counts: dict[str, int] = {}
    for enum in schema["enums"]:
        enum_prefix = str(enum["c_prefix"])
        enum_prefix_counts[enum_prefix] = enum_prefix_counts.get(enum_prefix, 0) + 1
    for enum in schema["enums"]:
        enum_prefix = str(enum["c_prefix"])
        macro_prefix = str(enum.get("macro_prefix") or (
            enum_prefix
            if enum_prefix_counts[enum_prefix] == 1
            else f"{enum_prefix}_{str(enum['name']).upper()}"
        ))
        lines.extend(_render_enum(enum, macro_prefix))

    layouts = list(_layout_entries(schema))
    for c_type, size, fields in layouts:
        lines.extend(_render_struct(c_type, size, fields))

    lines.append("#if defined(__cplusplus)")
    for c_type, size, fields in layouts:
        lines.append(f"static_assert(sizeof({c_type}) == {size}U, \"{c_type} wire size\");")
        for field in fields:
            lines.append(
                f"static_assert(offsetof({c_type}, {_field_c_name(field)}) == "
                f"{int(field['offset'])}U, \"{c_type}.{_field_c_name(field)} offset\");"
            )
    lines.extend(["#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L",])
    for c_type, size, fields in layouts:
        lines.append(f"_Static_assert(sizeof({c_type}) == {size}U, \"{c_type} wire size\");")
        for field in fields:
            lines.append(
                f"_Static_assert(offsetof({c_type}, {_field_c_name(field)}) == "
                f"{int(field['offset'])}U, \"{c_type}.{_field_c_name(field)} offset\");"
            )
    lines.extend([
        "#endif",
        "",
        "#if defined(_MSC_VER)",
        "#pragma pack(pop)",
        "#endif",
        "",
        "#ifdef __cplusplus",
        "}",
        "#endif",
        "",
        "#endif /* CANVIEW_PROTOCOL_H */",
        "",
    ])
    return "\n".join(lines)


def message_by_name(schema: Mapping[str, Any], name: str) -> Mapping[str, Any]:
    for message in schema["messages"]:
        if message["name"] == name:
            return message
    raise KeyError(name)


def payload_size_bounds(message: Mapping[str, Any]) -> tuple[int, int]:
    payload = message["payload"]
    kind = payload["kind"]
    if kind == "fixed":
        size = int(payload["size"])
        return size, size
    if kind == "bounded":
        prefix = int(payload["prefix"]["size"])
        record = int(payload["record"]["size"])
        return prefix, prefix + record * int(payload["count_max"])
    if kind == "suffix":
        return int(payload["prefix_size"]), int(payload["prefix_size"]) + int(payload["suffix_max"])
    if kind == "tlv":
        return int(payload["prefix_size"]), int(payload["prefix_size"]) + int(payload["max_tlv_bytes"])
    if kind == "variants":
        sizes = [int(variant["size"]) for variant in payload["variants"]]
        return min(sizes), max(sizes)
    raise SchemaError(f"unknown payload kind {kind}")


def _value_bytes(value: Any, expected: int | None = None) -> bytes:
    if isinstance(value, str):
        try:
            result = bytes.fromhex(value)
        except ValueError as exc:
            raise ValueError(f"invalid hex bytes: {value!r}") from exc
    elif isinstance(value, (bytes, bytearray)):
        result = bytes(value)
    elif isinstance(value, Sequence) and not isinstance(value, (str, bytes, bytearray)):
        result = bytes(int(item) for item in value)
    else:
        raise ValueError(f"expected byte string, got {type(value).__name__}")
    if expected is not None and len(result) != expected:
        raise ValueError(f"expected {expected} bytes, got {len(result)}")
    return result


def _encode_fields(fields: Sequence[Mapping[str, Any]], values: Mapping[str, Any]) -> bytes:
    size = sum(_type_size(field) for field in fields)
    output = bytearray(size)
    for field in fields:
        name = str(field["name"])
        value = values.get(name)
        if value is None:
            if field.get("reserved"):
                value = bytes(int(field["length"])) if field["type"] == "bytes" else 0
            else:
                raise ValueError(f"missing field {name}")
        offset = int(field["offset"])
        if field["type"] == "bytes":
            raw = _value_bytes(value, int(field["length"]))
            output[offset : offset + len(raw)] = raw
        else:
            fmt = "<" + SCALAR_FORMAT[field["type"]]
            try:
                encoded = struct.pack(fmt, int(value))
            except (struct.error, ValueError) as exc:
                raise ValueError(f"invalid value for {name}: {value!r}") from exc
            output[offset : offset + len(encoded)] = encoded
    return bytes(output)


def _decode_fields(fields: Sequence[Mapping[str, Any]], data: bytes, *, reject_reserved: bool = True) -> dict[str, Any]:
    values: dict[str, Any] = {}
    for field in fields:
        name = str(field["name"])
        offset = int(field["offset"])
        size = _type_size(field)
        raw = data[offset : offset + size]
        if len(raw) != size:
            raise ProtocolDecodeError("bad_length", f"field {name} is truncated")
        if field["type"] == "bytes":
            value: Any = raw.hex()
            is_zero = not any(raw)
        else:
            value = struct.unpack("<" + SCALAR_FORMAT[field["type"]], raw)[0]
            is_zero = value == 0
        values[name] = value
        if reject_reserved and field.get("reserved") and not is_zero:
            raise ProtocolDecodeError("bad_reserved", f"reserved field {name} is non-zero")
    return values


def _enum_values(schema: Mapping[str, Any], name: str) -> set[int]:
    enum = next((item for item in schema["enums"] if item.get("name") == name), None)
    if not isinstance(enum, Mapping):
        raise SchemaError(f"missing enum {name}")
    return {int(value) for value in enum["values"].values()}


def _enum_value(schema: Mapping[str, Any], name: str, member: str) -> int:
    enum = next((item for item in schema["enums"] if item.get("name") == name), None)
    if not isinstance(enum, Mapping) or member not in enum["values"]:
        raise SchemaError(f"unknown {name} enum member {member}")
    return int(enum["values"][member])


def _enum_name(schema: Mapping[str, Any], name: str, value: int) -> str:
    enum = next((item for item in schema["enums"] if item.get("name") == name), None)
    if not isinstance(enum, Mapping):
        raise SchemaError(f"missing enum {name}")
    for member, member_value in enum["values"].items():
        if int(member_value) == int(value):
            return str(member)
    raise SchemaError(f"unknown {name} enum value {value}")


def _flatten_payload_values(values: Mapping[str, Any]) -> dict[str, Any]:
    flattened: dict[str, Any] = {}
    prefix = values.get("prefix")
    if isinstance(prefix, Mapping):
        flattened.update(prefix)
    flattened.update(values)
    return flattened


def _semantic_field_values(values: Mapping[str, Any], field_name: str) -> list[Any]:
    result: list[Any] = []
    if field_name in values:
        result.append(values[field_name])
    prefix = values.get("prefix")
    if isinstance(prefix, Mapping) and field_name in prefix:
        result.append(prefix[field_name])
    records = values.get("records")
    if isinstance(records, Sequence) and not isinstance(records, (str, bytes, bytearray)):
        for record in records:
            if isinstance(record, Mapping) and field_name in record:
                result.append(record[field_name])
    return result


def _semantic_record_field_values(values: Mapping[str, Any], field_name: str) -> list[Any]:
    result: list[Any] = []
    records = values.get("records")
    if isinstance(records, Sequence) and not isinstance(records, (str, bytes, bytearray)):
        for record in records:
            if isinstance(record, Mapping) and field_name in record:
                result.append(record[field_name])
    return result


def _semantic_config_key_values(values: Mapping[str, Any]) -> list[Any]:
    result: list[Any] = []
    if "key" in values:
        result.append(values["key"])
    records = values.get("records")
    if isinstance(records, Sequence) and not isinstance(records, (str, bytes, bytearray)):
        for record in records:
            if isinstance(record, Mapping) and "key" in record:
                result.append(record["key"])
    return result


def _semantic_error(
    decode: bool,
    code: str,
    message: str,
) -> None:
    if decode:
        raise ProtocolDecodeError(code, message)
    raise ValueError(message)


def _validate_payload_semantics(
    schema: Mapping[str, Any],
    message: Mapping[str, Any],
    values: Mapping[str, Any],
    *,
    decode: bool,
    authenticated_role: str | None = None,
    authenticated_receiver_role: str | None = None,
) -> None:
    message_name = str(message["name"])
    flattened = _flatten_payload_values(values)
    semantic_policy = schema["semantic_policy"]
    for entry in semantic_policy["role_fields"]:
        if entry["message"] != message_name or entry["field"] not in flattened:
            continue
        try:
            role = int(flattened[entry["field"]])
        except (TypeError, ValueError) as exc:
            _semantic_error(decode, "bad_enum", f"{message_name}.{entry['field']} is not an integer")
            raise AssertionError from exc
        if role not in _enum_values(schema, "role"):
            _semantic_error(decode, "bad_enum", f"{message_name}.{entry['field']} has an unknown role")

    for entry in semantic_policy["authenticated_role_fields"]:
        if entry["message"] != message_name or authenticated_role is None:
            continue
        field_values = _semantic_field_values(values, str(entry["field"]))
        if not field_values:
            continue
        if authenticated_role not in {
            str(member)
            for member in next(item for item in schema["enums"] if item.get("name") == "role")["values"]
        }:
            _semantic_error(decode, "unauthorized_sender", "authenticated sender role is unknown")
        expected_role = _enum_value(schema, "role", authenticated_role)
        if any(int(value) != expected_role for value in field_values):
            _semantic_error(
                decode,
                "role_mismatch",
                f"{message_name}.{entry['field']} does not match authenticated sender role",
            )

    for entry in semantic_policy["command_id_fields"]:
        if entry["message"] != message_name or entry["field"] not in flattened:
            continue
        if int(flattened[entry["field"]]) not in _enum_values(schema, "command_id"):
            _semantic_error(decode, "bad_enum", f"{message_name}.{entry['field']} has an unknown command")

    for entry in semantic_policy["enum_fields"]:
        if entry["message"] != message_name:
            continue
        for value in _semantic_field_values(values, str(entry["field"])):
            try:
                valid = int(value) in _enum_values(schema, str(entry["enum"]))
            except (TypeError, ValueError):
                valid = False
            if not valid:
                _semantic_error(decode, "bad_enum", f"{message_name}.{entry['field']} has an unknown value")

    for entry in semantic_policy["record_enum_fields"]:
        if entry["message"] != message_name:
            continue
        for value in _semantic_record_field_values(values, str(entry["field"])):
            try:
                valid = int(value) in _enum_values(schema, str(entry["enum"]))
            except (TypeError, ValueError):
                valid = False
            if not valid:
                _semantic_error(decode, "bad_enum", f"{message_name}.{entry['field']} has an unknown value")

    for entry in semantic_policy["bitmask_fields"]:
        if entry["message"] != message_name:
            continue
        known_mask = 0
        for enum_value in _enum_values(schema, str(entry["enum"])):
            known_mask |= enum_value
        for value in _semantic_field_values(values, str(entry["field"])):
            try:
                valid = int(value) & ~known_mask == 0
            except (TypeError, ValueError):
                valid = False
            if not valid:
                _semantic_error(decode, "bad_bitmask", f"{message_name}.{entry['field']} has unknown bits")

    for entry in semantic_policy["record_bitmask_fields"]:
        if entry["message"] != message_name:
            continue
        known_mask = 0
        for enum_value in _enum_values(schema, str(entry["enum"])):
            known_mask |= enum_value
        for value in _semantic_record_field_values(values, str(entry["field"])):
            try:
                valid = int(value) & ~known_mask == 0
            except (TypeError, ValueError):
                valid = False
            if not valid:
                _semantic_error(decode, "bad_bitmask", f"{message_name}.{entry['field']} has unknown bits")

    for entry in semantic_policy["fixed_bitmask_fields"]:
        if entry["message"] != message_name:
            continue
        known_mask = int(entry["known_mask"])
        for value in _semantic_field_values(values, str(entry["field"])):
            try:
                valid = int(value) & ~known_mask == 0
            except (TypeError, ValueError):
                valid = False
            if not valid:
                _semantic_error(decode, "bad_bitmask", f"{message_name}.{entry['field']} has unknown bits")

    for entry in semantic_policy["range_fields"]:
        if entry["message"] != message_name:
            continue
        minimum = int(entry["minimum"])
        maximum = int(entry["maximum"])
        for value in _semantic_field_values(values, str(entry["field"])):
            try:
                valid = minimum <= int(value) <= maximum
            except (TypeError, ValueError):
                valid = False
            if not valid:
                _semantic_error(
                    decode,
                    "out_of_range",
                    f"{message_name}.{entry['field']} is outside the permitted range",
                )

    for entry in semantic_policy["cross_field_constraints"]:
        if entry["message"] != message_name or entry["kind"] != "bitmap_with_window":
            continue
        bitmap_values = _semantic_field_values(values, str(entry["bitmap_field"]))
        window_values = _semantic_field_values(values, str(entry["window_field"]))
        if not bitmap_values or not window_values:
            continue
        try:
            bitmap = int(bitmap_values[0])
            window = int(window_values[0])
            valid = window > 0 and bitmap & ~((1 << window) - 1) == 0
        except (TypeError, ValueError, OverflowError):
            valid = False
        if not valid:
            _semantic_error(
                decode,
                "bad_bitmask",
                f"{message_name}.{entry['bitmap_field']} exceeds {entry['window_field']}",
            )

    for entry in semantic_policy["config_key_fields"]:
        if entry["message"] != message_name:
            continue
        for value in _semantic_field_values(values, str(entry["field"])):
            try:
                valid = int(value) in _enum_values(schema, "config_key")
            except (TypeError, ValueError):
                valid = False
            if not valid:
                _semantic_error(decode, "bad_enum", f"{message_name}.{entry['field']} has an unknown config key")
        binding = schema["config_policy"]["owner_binding"].get(message_name)
        if binding == "receiver" and authenticated_receiver_role is not None:
            config_values = schema["config_policy"]["owner_roles"]
            for value in _semantic_config_key_values(values):
                try:
                    key_name = _enum_name(schema, "config_key", int(value))
                    owner_role = str(config_values[key_name])
                except (KeyError, TypeError, ValueError, SchemaError):
                    continue
                if owner_role != authenticated_receiver_role:
                    _semantic_error(
                        decode,
                        "unauthorized_receiver",
                        f"{message_name} config key owner does not match authenticated receiver role",
                    )

    scope_values: dict[str, int] = {}
    for entry in semantic_policy["scope_fields"]:
        if entry["message"] != message_name or entry["field"] not in flattened:
            continue
        scope = int(flattened[entry["field"]])
        known_mask = _constant_value(schema, "CANVIEW_CONTROL_SCOPE_KNOWN_MASK")
        if scope & ~known_mask:
            _semantic_error(decode, "bad_scope", f"{message_name}.{entry['field']} has unknown scope bits")
        scope_values[entry["field"]] = scope

    capability = semantic_policy["capability_authorization"]
    if message_name == capability["message"]:
        role = int(flattened[capability["role_field"]])
        scope = int(flattened[capability["scope_field"]])
        if role in _enum_values(schema, "role"):
            role_name = authenticated_role if authenticated_role is not None else _enum_name(schema, "role", role)
            if role_name in capability["zero_scope_roles"] and scope != 0:
                _semantic_error(decode, "unauthorized_scope", "read-only capability advertises control scope")
            if role_name not in capability["authorized_scope_roles"] and scope != 0:
                _semantic_error(decode, "unauthorized_scope", "role is not authorized for control scope")
    if message_name == "PAIR_CHALLENGE" and "authorized_role" in flattened and "authorized_scope" in flattened:
        role = int(flattened["authorized_role"])
        scope = int(flattened["authorized_scope"])
        role_enum = next(item for item in schema["enums"] if item.get("name") == "role")
        role_name = next(name for name, value in role_enum["values"].items() if int(value) == role)
        if role_name in {"READ_ONLY_CONTROLLER", "DIAGNOSTIC_BRIDGE"} and scope != 0:
            _semantic_error(decode, "unauthorized_scope", "pairing challenge grants scope to a read-only role")


def _validate_tlv_bytes(policy: Mapping[str, Any], data: bytes) -> None:
    _decode_tlvs(policy, data)


def encode_payload(
    message: Mapping[str, Any],
    values: Mapping[str, Any],
    *,
    schema: Mapping[str, Any] | None = None,
) -> bytes:
    if schema is None:
        schema = load_schema()
    payload = message["payload"]
    kind = payload["kind"]
    if kind == "fixed":
        output = _encode_fields(payload["fields"], values)
        _validate_payload_semantics(schema, message, values, decode=False)
        return output
    if kind == "variants":
        variant_name = values.get("variant") or values.get("_variant")
        if not isinstance(variant_name, str):
            raise ValueError(f"{message['name']} requires variant")
        for variant in payload["variants"]:
            if variant["name"] == variant_name:
                output = _encode_fields(variant["fields"], values)
                _validate_payload_semantics(schema, message, values, decode=False)
                return output
        raise ValueError(f"unknown {message['name']} variant {variant_name}")
    if kind == "bounded":
        records = values.get("records", [])
        if not isinstance(records, Sequence) or isinstance(records, (str, bytes, bytearray)):
            raise ValueError(f"{message['name']} records must be a list")
        if len(records) > int(payload["count_max"]):
            raise ValueError(f"{message['name']} record count exceeds maximum")
        prefix_values = dict(values.get("prefix", values))
        count_field = str(payload["count_field"])
        declared_count = prefix_values.get(count_field)
        if declared_count is not None and int(declared_count) != len(records):
            raise ValueError(f"{message['name']} count does not match records")
        prefix_values[count_field] = len(records)
        output = bytearray(_encode_fields(payload["prefix"]["fields"], prefix_values))
        for record in records:
            if not isinstance(record, Mapping):
                raise ValueError(f"{message['name']} record must be an object")
            output.extend(_encode_fields(payload["record"]["fields"], record))
        _validate_payload_semantics(schema, message, values, decode=False)
        return bytes(output)
    if kind == "suffix":
        prefix_values = dict(values.get("prefix", values))
        output = bytearray(_encode_fields(payload["prefix"]["fields"], prefix_values))
        suffix_name = str(payload["suffix_name"])
        suffix = _value_bytes(values.get(suffix_name, b""))
        if len(suffix) > int(payload["suffix_max"]):
            raise ValueError(f"{message['name']} suffix exceeds maximum")
        length_field_name = payload.get("suffix_length_field")
        length_field = next(
            (f for f in payload["prefix"]["fields"]
             if f["name"] == length_field_name or (length_field_name is None and f["name"].endswith("_length"))),
            None,
        )
        if length_field is not None and int(prefix_values.get(length_field["name"], len(suffix))) != len(suffix):
            raise ValueError(f"{message['name']} suffix length field mismatch")
        if message["name"] == "COMMAND_REQUEST":
            _validate_tlv_bytes(schema["command_argument_tlv"], suffix)
        output.extend(suffix)
        _validate_payload_semantics(schema, message, values, decode=False)
        return bytes(output)
    if kind == "tlv":
        prefix_values = dict(values.get("prefix", values))
        output = bytearray(_encode_fields(payload["prefix"]["fields"], prefix_values))
        tlvs = values.get("tlvs", [])
        if not isinstance(tlvs, Sequence) or isinstance(tlvs, (str, bytes, bytearray)):
            raise ValueError(f"{message['name']} TLVs must be a list")
        for item in tlvs:
            if not isinstance(item, Mapping):
                raise ValueError("TLV item must be an object")
            raw = _value_bytes(item.get("value", b""))
            output.extend(struct.pack("<HH", int(item["type"]), len(raw)))
            output.extend(raw)
        if len(output) - int(payload["prefix_size"]) > int(payload["max_tlv_bytes"]):
            raise ValueError(f"{message['name']} TLVs exceed maximum")
        _validate_tlv_bytes(schema["tlv"], bytes(output[int(payload["prefix_size"]):]))
        _validate_payload_semantics(schema, message, values, decode=False)
        return bytes(output)
    raise SchemaError(f"unknown payload kind {kind}")


def _decode_tlvs(policy: Mapping[str, Any], data: bytes) -> list[dict[str, Any]]:
    known = {int(item["value"]): item for item in policy["types"]}
    critical_bit = int(policy["critical_bit"])
    tlvs: list[dict[str, Any]] = []
    seen_singletons: set[int] = set()
    cursor = 0
    while cursor < len(data):
        if len(data) - cursor < 4:
            raise ProtocolDecodeError("bad_length", "truncated TLV header")
        type_value, length = struct.unpack_from("<HH", data, cursor)
        cursor += 4
        if length > len(data) - cursor:
            raise ProtocolDecodeError("bad_length", "truncated TLV value")
        value = data[cursor : cursor + length]
        cursor += length
        base_type = type_value & (critical_bit - 1)
        definition = known.get(type_value)
        if definition is None and type_value & critical_bit:
            raise ProtocolDecodeError("unsupported_tlv", f"unknown critical TLV {type_value}")
        if definition is not None and definition.get("singleton"):
            if base_type in seen_singletons:
                raise ProtocolDecodeError("duplicate_tlv", f"singleton TLV {base_type} is repeated")
            seen_singletons.add(base_type)
        if definition is not None and int(definition["size"]) >= 0 and int(definition["size"]) != length:
            raise ProtocolDecodeError("bad_length", f"TLV {type_value} has invalid size")
        tlvs.append({"type": type_value, "value": value.hex()})
    return tlvs


def decode_payload(schema: Mapping[str, Any], message: Mapping[str, Any], data: bytes) -> dict[str, Any]:
    payload = message["payload"]
    kind = payload["kind"]
    low, high = payload_size_bounds(message)
    if not low <= len(data) <= high:
        raise ProtocolDecodeError("bad_length", f"{message['name']} payload length {len(data)} outside {low}..{high}")
    if kind == "fixed":
        if len(data) != int(payload["size"]):
            raise ProtocolDecodeError("bad_length", "fixed payload has unexpected length")
        result = _decode_fields(payload["fields"], data)
        _validate_payload_semantics(schema, message, result, decode=True)
        return result
    if kind == "variants":
        for variant in payload["variants"]:
            if len(data) == int(variant["size"]):
                result = _decode_fields(variant["fields"], data)
                result["variant"] = variant["name"]
                _validate_payload_semantics(schema, message, result, decode=True)
                return result
        raise ProtocolDecodeError("bad_length", "no variant matches payload length")
    if kind == "bounded":
        prefix_size = int(payload["prefix"]["size"])
        prefix = _decode_fields(payload["prefix"]["fields"], data[:prefix_size])
        count = int(prefix[str(payload["count_field"])])
        if count > int(payload["count_max"]):
            raise ProtocolDecodeError("bad_count", f"count {count} exceeds maximum")
        expected = prefix_size + count * int(payload["record"]["size"])
        if expected != len(data):
            raise ProtocolDecodeError("bad_length", "bounded payload length does not match count")
        records = []
        cursor = prefix_size
        for _ in range(count):
            end = cursor + int(payload["record"]["size"])
            records.append(_decode_fields(payload["record"]["fields"], data[cursor:end]))
            cursor = end
        prefix["records"] = records
        _validate_payload_semantics(schema, message, prefix, decode=True)
        return prefix
    if kind == "suffix":
        prefix_size = int(payload["prefix_size"])
        result = _decode_fields(payload["prefix"]["fields"], data[:prefix_size])
        suffix = data[prefix_size:]
        length_field_name = payload.get("suffix_length_field")
        length_field = next(
            (f for f in payload["prefix"]["fields"]
             if f["name"] == length_field_name or (length_field_name is None and f["name"].endswith("_length"))),
            None,
        )
        if length_field is not None and int(result[length_field["name"]]) != len(suffix):
            raise ProtocolDecodeError("bad_length", "suffix length field mismatch")
        if message["name"] == "COMMAND_REQUEST":
            _decode_tlvs(schema["command_argument_tlv"], suffix)
        result[str(payload["suffix_name"])] = suffix.hex()
        _validate_payload_semantics(schema, message, result, decode=True)
        return result
    if kind == "tlv":
        prefix_size = int(payload["prefix_size"])
        result = _decode_fields(payload["prefix"]["fields"], data[:prefix_size])
        result["tlvs"] = _decode_tlvs(schema["tlv"], data[prefix_size:])
        _validate_payload_semantics(schema, message, result, decode=True)
        return result
    raise SchemaError(f"unknown payload kind {kind}")


def _header_bytes(schema: Mapping[str, Any], values: Mapping[str, Any]) -> bytes:
    return _encode_fields(schema["header"]["fields"], values)


def _variant_roles(
    schema: Mapping[str, Any],
    message: Mapping[str, Any],
    variant_name: str | None,
) -> tuple[set[str], set[str]]:
    if variant_name is not None:
        for entry in schema["frame_policy"]["variant_policies"]:
            if entry["message"] == message["name"] and entry["variant"] == variant_name:
                return set(str(role) for role in entry["senders"]), set(str(role) for role in entry["receivers"])
    return set(str(role) for role in message["senders"]), set(str(role) for role in message["receivers"])


def _is_response_variant(schema: Mapping[str, Any], message: Mapping[str, Any], variant_name: str | None) -> bool:
    if message["name"] in schema["frame_policy"]["response_messages"]:
        return True
    return (str(message["name"]), variant_name) in {
        (str(entry["message"]), str(entry["variant"]))
        for entry in schema["frame_policy"]["response_variants"]
    }


def _allowed_frame_flags(
    schema: Mapping[str, Any],
    message: Mapping[str, Any],
    *,
    variant_name: str | None,
) -> int:
    flags = 0
    if message["ack"] == "required":
        flags |= 0x01
    if message["broadcast"]:
        flags |= 0x20
    if _is_response_variant(schema, message, variant_name):
        flags |= 0x02
    if message["name"] == "ERROR":
        flags |= 0x04
    if message["name"] in schema["frame_policy"]["fragment_messages"]:
        flags |= 0x08 | 0x10
    read_only_roles = set(schema["security"]["control_scope_zero_roles"])
    if read_only_roles.intersection(message["senders"]):
        flags |= 0x40
    return flags


def _validate_frame_flags(
    schema: Mapping[str, Any],
    message: Mapping[str, Any],
    flags: int,
    *,
    sender_role: str | None,
    variant_name: str | None,
    decode: bool,
) -> None:
    allowed = _allowed_frame_flags(schema, message, variant_name=variant_name)
    if flags & ~allowed:
        if decode:
            raise ProtocolDecodeError("bad_flags", f"flags are not allowed for {message['name']}")
        raise ValueError(f"flags are not allowed for {message['name']}")
    ack_expected = message["ack"] == "required"
    if bool(flags & 0x01) != ack_expected:
        if decode:
            raise ProtocolDecodeError("bad_flags", "ACK_REQUIRED does not match message policy")
        raise ValueError("ACK_REQUIRED does not match message policy")
    broadcast_expected = bool(message["broadcast"])
    if bool(flags & 0x20) != broadcast_expected:
        if decode:
            raise ProtocolDecodeError("bad_flags", "BROADCAST does not match message policy")
        raise ValueError("BROADCAST does not match message policy")
    response_expected = _is_response_variant(schema, message, variant_name)
    if bool(flags & 0x02) != response_expected:
        if decode:
            raise ProtocolDecodeError("bad_flags", "RESPONSE does not match message policy")
        raise ValueError("RESPONSE does not match message policy")
    if message["name"] == "ERROR" and not flags & 0x04:
        if decode:
            raise ProtocolDecodeError("bad_flags", "ERROR message must set ERROR")
        raise ValueError("ERROR message must set ERROR")
    if message["name"] in schema["frame_policy"]["fragment_messages"] and not flags & 0x08:
        if decode:
            raise ProtocolDecodeError("bad_flags", "fragment message must set FRAGMENT")
        raise ValueError("fragment message must set FRAGMENT")
    if flags & 0x10 and not flags & 0x08:
        if decode:
            raise ProtocolDecodeError("bad_flags", "LAST_FRAGMENT requires FRAGMENT")
        raise ValueError("LAST_FRAGMENT requires FRAGMENT")
    read_only_roles = set(schema["security"]["control_scope_zero_roles"])
    if flags & 0x40:
        if sender_role is None:
            if decode:
                raise ProtocolDecodeError("context_required", "READ_ONLY requires sender role context")
            raise ValueError("READ_ONLY requires sender role context")
        if sender_role not in read_only_roles:
            if decode:
                raise ProtocolDecodeError("unauthorized_sender", "READ_ONLY flag conflicts with sender role")
            raise ValueError("READ_ONLY flag conflicts with sender role")
    elif sender_role in read_only_roles:
        if decode:
            raise ProtocolDecodeError("bad_flags", "read-only sender must set READ_ONLY")
        raise ValueError("read-only sender must set READ_ONLY")


def _validate_fragment_payload(
    schema: Mapping[str, Any],
    message: Mapping[str, Any],
    payload: Mapping[str, Any],
    flags: int,
    *,
    decode: bool,
) -> None:
    if message["name"] not in schema["frame_policy"]["fragment_messages"]:
        return
    try:
        index = int(payload["fragment_index"])
        total = int(payload["total_fragments"])
    except (KeyError, TypeError, ValueError) as exc:
        if decode:
            raise ProtocolDecodeError("bad_fragment", "fragment index/total is missing") from exc
        raise ValueError("fragment index/total is missing") from exc
    if total <= 0 or index < 0 or index >= total:
        if decode:
            raise ProtocolDecodeError("bad_fragment", "fragment index is outside total")
        raise ValueError("fragment index is outside total")
    expected_last = index == total - 1
    actual_last = bool(flags & 0x10)
    if actual_last != expected_last:
        if decode:
            raise ProtocolDecodeError("bad_fragment", "LAST_FRAGMENT does not match fragment position")
        raise ValueError("LAST_FRAGMENT does not match fragment position")


def default_decode_context(
    schema: Mapping[str, Any],
    message: Mapping[str, Any],
    header: Mapping[str, Any],
) -> dict[str, Any]:
    """Return an explicit test/vector context for a declared message direction."""

    secure = message["name"] in set(schema["frame_policy"]["secure_messages"])
    authenticated_session_zero = message["name"] in set(schema["frame_policy"]["authenticated_session_zero_messages"])
    return {
        "authenticated": secure or authenticated_session_zero,
        "expected_session_id": int(header.get("session_id", 0)),
        "sender_role": message["senders"][0],
        "receiver_role": message["receivers"][0],
        "link_state": message["states"][0],
    }


def encode_frame(
    schema: Mapping[str, Any],
    message: Mapping[str, Any],
    header: Mapping[str, Any],
    payload_values: Mapping[str, Any],
    *,
    sender_role: str | None = None,
    receiver_role: str | None = None,
    link_state: str | None = None,
    expected_session_id: int | None = None,
    authenticated: bool = False,
) -> bytes:
    secure = message["name"] in set(schema["frame_policy"]["secure_messages"])
    authenticated_session_zero = message["name"] in set(schema["frame_policy"]["authenticated_session_zero_messages"])
    if sender_role is None or receiver_role is None or link_state is None or expected_session_id is None:
        raise ValueError("frame encoder requires explicit peer/session context")
    variant_name = payload_values.get("variant") or payload_values.get("_variant")
    variant_senders, variant_receivers = _variant_roles(
        schema,
        message,
        str(variant_name) if variant_name is not None else None,
    )
    resolved_sender_role = str(sender_role)
    resolved_receiver_role = str(receiver_role)
    if resolved_sender_role not in variant_senders:
        raise ValueError("sender role is not allowed for this message")
    if resolved_receiver_role not in variant_receivers:
        raise ValueError("receiver role is not allowed for this message")
    if link_state is not None and link_state not in set(message["states"]):
        raise ValueError("message is not allowed in the current link state")
    if secure or authenticated_session_zero:
        if not authenticated:
            raise ValueError("secure frame requires authenticated transport context")
    payload = encode_payload(message, payload_values, schema=schema)
    if len(payload) > int(schema["encoding"]["max_payload_size"]):
        raise ValueError("payload exceeds maximum")
    header_values = {
        "magic": int(schema["constants"][[c["c_name"] for c in schema["constants"]].index("CANVIEW_MAGIC_LE")]["value"]),
        "major": int(schema["version"]["major"]),
        "minor": int(schema["version"]["minor"]),
        "header_len": int(schema["encoding"]["header_size"]),
        "message_type": int(message["id"]),
        "flags": int(header.get("flags", 0)),
        "priority": int(header.get("priority", message.get("priority", 0))),
        "session_id": int(header.get("session_id", 0)),
        "sequence": int(header.get("sequence", 0)),
        "sender_time_ms": int(header.get("sender_time_ms", 0)),
        "correlation_id": int(header.get("correlation_id", 0)),
        "payload_len": len(payload),
        "reserved": 0,
        "crc32": 0,
    }
    if header_values["flags"] & ~int(schema["header"]["flags_known_mask"]):
        raise ValueError("unknown frame flag")
    _validate_frame_flags(
        schema,
        message,
        header_values["flags"],
        sender_role=resolved_sender_role,
        variant_name=str(variant_name) if variant_name is not None else None,
        decode=False,
    )
    if schema["frame_policy"]["response_correlation_required"] and _is_response_variant(
        schema, message, str(variant_name) if variant_name is not None else None
    ) and header_values["correlation_id"] == 0:
        raise ValueError("response frame requires non-zero correlation_id")
    if message["name"] in set(schema["frame_policy"]["session_zero_messages"]):
        if header_values["session_id"] != 0:
            raise ValueError(f"{message['name']} must use session_id=0")
        if int(expected_session_id) != 0:
            raise ValueError("pre-session context must use session zero")
    else:
        if header_values["session_id"] == 0:
            raise ValueError(f"{message['name']} requires a non-zero session_id")
        if secure and int(expected_session_id) != header_values["session_id"]:
            raise ValueError("frame session differs from transport session")
    decoded_payload = decode_payload(schema, message, payload)
    _validate_payload_semantics(
        schema,
        message,
        decoded_payload,
        decode=False,
        authenticated_role=resolved_sender_role,
        authenticated_receiver_role=resolved_receiver_role,
    )
    _validate_fragment_payload(schema, message, decoded_payload, header_values["flags"], decode=False)
    if "wireless_session_id" in decoded_payload and header_values["session_id"] != int(decoded_payload["wireless_session_id"]):
        raise ValueError("header and command session IDs differ")
    zero_crc_header = _header_bytes(schema, header_values)
    crc = zlib.crc32(zero_crc_header + payload) & 0xFFFFFFFF
    header_values["crc32"] = crc
    return _header_bytes(schema, header_values) + payload


def _constant_value(schema: Mapping[str, Any], name: str) -> int:
    for constant in schema["constants"]:
        if constant["c_name"] == name:
            return int(constant["value"])
    raise KeyError(name)


def decode_frame(
    schema: Mapping[str, Any],
    frame: bytes,
    *,
    sender_role: str | None = None,
    receiver_role: str | None = None,
    link_state: str | None = None,
    authenticated: bool = False,
    expected_session_id: int | None = None,
) -> dict[str, Any]:
    if len(frame) < int(schema["encoding"]["header_size"]):
        raise ProtocolDecodeError("bad_length", "frame is shorter than header")
    header_bytes = frame[:32]
    header = _decode_fields(schema["header"]["fields"], header_bytes)
    if int(header["magic"]) != _constant_value(schema, "CANVIEW_MAGIC_LE"):
        raise ProtocolDecodeError("bad_magic", "bad frame magic")
    if int(header["major"]) != int(schema["version"]["major"]):
        raise ProtocolDecodeError("incompatible_major", "unsupported protocol major")
    if int(header["header_len"]) != 32:
        raise ProtocolDecodeError("bad_length", "unexpected header length")
    if int(header["flags"]) & ~int(schema["header"]["flags_known_mask"]):
        raise ProtocolDecodeError("bad_flags", "unknown frame flag")
    if int(header["priority"]) > 4:
        raise ProtocolDecodeError("bad_priority", "priority is outside the protocol range")
    payload_len = int(header["payload_len"])
    header_size = int(schema["encoding"]["header_size"])
    if payload_len > int(schema["encoding"]["max_payload_size"]) or len(frame) != header_size + payload_len:
        raise ProtocolDecodeError("bad_length", "frame length does not match payload length")
    zero_crc = bytearray(header_bytes)
    zero_crc[28:32] = b"\x00\x00\x00\x00"
    expected_crc = zlib.crc32(bytes(zero_crc) + frame[32:]) & 0xFFFFFFFF
    if int(header["crc32"]) != expected_crc:
        raise ProtocolDecodeError("bad_crc", "CRC mismatch")
    message = next((m for m in schema["messages"] if int(m["id"]) == int(header["message_type"])), None)
    if message is None:
        raise ProtocolDecodeError("unsupported_message", "unknown message type")
    if (
        sender_role is None
        or receiver_role is None
        or link_state is None
        or expected_session_id is None
    ):
        raise ProtocolDecodeError(
            "context_required",
            "frame policy decode requires complete peer/session context",
        )
    if int(header["priority"]) != int(message["priority"]):
        raise ProtocolDecodeError("bad_priority", "header priority does not match message policy")
    payload = decode_payload(schema, message, frame[header_size:])
    variant_name = payload.get("variant")
    variant_senders, variant_receivers = _variant_roles(
        schema,
        message,
        str(variant_name) if variant_name is not None else None,
    )
    ack_required = bool(int(header["flags"]) & 0x01)
    if ack_required != (message["ack"] == "required"):
        raise ProtocolDecodeError("bad_flags", "ACK_REQUIRED does not match message policy")
    broadcast = bool(int(header["flags"]) & 0x20)
    if broadcast != bool(message["broadcast"]):
        raise ProtocolDecodeError("bad_flags", "BROADCAST does not match message policy")
    _validate_frame_flags(
        schema,
        message,
        int(header["flags"]),
        sender_role=sender_role,
        variant_name=str(variant_name) if variant_name is not None else None,
        decode=True,
    )
    if schema["frame_policy"]["response_correlation_required"] and _is_response_variant(
        schema, message, str(variant_name) if variant_name is not None else None
    ) and int(header["correlation_id"]) == 0:
        raise ProtocolDecodeError("bad_correlation", "response frame requires non-zero correlation_id")
    if message["name"] in set(schema["frame_policy"]["session_zero_messages"]):
        if int(header["session_id"]) != 0:
            raise ProtocolDecodeError("bad_session", "pairing message must use session_id=0")
        if message["name"] in set(schema["frame_policy"]["authenticated_session_zero_messages"]):
            if not authenticated:
                raise ProtocolDecodeError("unauthenticated", "encrypted pairing message requires authenticated context")
            if expected_session_id is None or sender_role is None or receiver_role is None or link_state is None:
                raise ProtocolDecodeError("context_required", "encrypted pairing message requires complete pre-session context")
            if int(expected_session_id) != 0:
                raise ProtocolDecodeError("session_mismatch", "pre-session context must use session zero")
    else:
        if int(header["session_id"]) == 0:
            raise ProtocolDecodeError("session_required", "secure message requires a non-zero session_id")
        if not authenticated:
            raise ProtocolDecodeError("unauthenticated", "secure message requires authenticated transport context")
        if expected_session_id is None or sender_role is None or receiver_role is None or link_state is None:
            raise ProtocolDecodeError("context_required", "secure message requires complete peer/session context")
        if int(expected_session_id) != int(header["session_id"]):
            raise ProtocolDecodeError("session_mismatch", "frame session differs from transport session")
    if sender_role not in variant_senders:
        raise ProtocolDecodeError("unauthorized_sender", "sender role is not allowed for this message")
    if receiver_role not in variant_receivers:
        raise ProtocolDecodeError("unauthorized_receiver", "receiver role is not allowed for this message")
    if link_state not in set(message["states"]):
        raise ProtocolDecodeError("invalid_state", "message is not allowed in the current link state")
    _validate_payload_semantics(
        schema,
        message,
        payload,
        decode=True,
        authenticated_role=sender_role,
        authenticated_receiver_role=receiver_role,
    )
    _validate_fragment_payload(schema, message, payload, int(header["flags"]), decode=True)
    if "wireless_session_id" in payload and int(header["session_id"]) != int(payload["wireless_session_id"]):
        raise ProtocolDecodeError("session_mismatch", "header and command session IDs differ")
    return {"header": header, "message": message["name"], "message_id": message["id"], "payload": payload}


def _vector_frame(schema: Mapping[str, Any], vector: Mapping[str, Any]) -> tuple[bytes, dict[str, Any]]:
    message = message_by_name(schema, str(vector["message"]))
    context = default_decode_context(schema, message, vector.get("header", {}))
    frame = encode_frame(
        schema,
        message,
        vector.get("header", {}),
        vector.get("payload", {}),
        sender_role=context["sender_role"],
        receiver_role=context["receiver_role"],
        link_state=context["link_state"],
        expected_session_id=context["expected_session_id"],
        authenticated=context["authenticated"],
    )
    header = decode_frame(
        schema,
        frame,
        **context,
    )["header"]
    payload = frame[32:]
    rendered = {
        "schema_version": schema["version"],
        "name": vector["name"],
        "message": message["name"],
        "message_id": message["id"],
        "header": header,
        "payload": vector.get("payload", {}),
        "payload_hex": payload.hex(),
        "payload_size": len(payload),
        "frame_hex": frame.hex(),
        "frame_size": len(frame),
        "crc32": f"{int(header['crc32']):08x}",
    }
    return frame, rendered


def _mutate_frame(schema: Mapping[str, Any], frame: bytes, mutation: Mapping[str, Any]) -> bytes:
    result = bytearray(frame)
    kind = mutation["mutation"]
    if kind == "truncate":
        return bytes(result[: int(mutation["length"])])
    if kind == "magic":
        struct.pack_into("<H", result, 0, int(mutation["value"]))
    elif kind == "major":
        result[2] = int(mutation["value"])
    elif kind == "minor":
        result[3] = int(mutation["value"])
    elif kind == "flags":
        result[6] = int(mutation["value"])
    elif kind == "reserved":
        struct.pack_into("<H", result, 26, int(mutation["value"]))
    elif kind == "crc":
        struct.pack_into("<I", result, 28, int(mutation["value"]))
        return bytes(result)
    elif kind == "payload_len":
        struct.pack_into("<H", result, 24, int(mutation["value"]))
        return bytes(result)
    elif kind in {"append_noncritical_tlv", "append_critical_tlv"}:
        raw = _value_bytes(mutation["value"])
        result.extend(struct.pack("<HH", int(mutation["tlv_type"]), len(raw)))
        result.extend(raw)
        struct.pack_into("<H", result, 24, len(result) - 32)
    else:
        raise SchemaError(f"unknown mutation {kind}")
    if len(result) >= 32:
        struct.pack_into("<I", result, 28, 0)
        crc = zlib.crc32(bytes(result[:32]) + bytes(result[32:])) & 0xFFFFFFFF
        struct.pack_into("<I", result, 28, crc)
    return bytes(result)


def _render_negative(schema: Mapping[str, Any], base_frames: Mapping[str, bytes]) -> dict[str, tuple[bytes, str]]:
    output: dict[str, tuple[bytes, str]] = {}
    for vector in schema["negative_vectors"]:
        raw = _mutate_frame(schema, base_frames[str(vector["base"])], vector)
        metadata = {
            "schema_version": schema["version"],
            "name": vector["name"],
            "base": vector["base"],
            "mutation": vector["mutation"],
            "expected": vector["expected"],
            "frame_hex": raw.hex(),
            "frame_size": len(raw),
        }
        output[str(vector["name"])] = (raw, json.dumps(metadata, indent=2, sort_keys=True) + "\n")
    return output


def _render_compatibility(schema: Mapping[str, Any], base_frames: Mapping[str, bytes]) -> dict[str, tuple[bytes, str]]:
    output: dict[str, tuple[bytes, str]] = {}
    for vector in schema["compatibility_vectors"]:
        raw = _mutate_frame(schema, base_frames[str(vector["base"])], vector)
        metadata = {
            "schema_version": schema["version"],
            "name": vector["name"],
            "base": vector["base"],
            "mutation": vector["mutation"],
            "expected": vector["expected"],
            "frame_hex": raw.hex(),
            "frame_size": len(raw),
        }
        output[str(vector["name"])] = (raw, json.dumps(metadata, indent=2, sort_keys=True) + "\n")
    return output


def _render_pairing_vectors(schema: Mapping[str, Any]) -> str:
    phases = {str(phase["message"]): phase for phase in schema["pairing_phases"]}
    vectors = []
    for vector in schema["pairing_negative_vectors"]:
        phase = phases[str(vector["phase"])]
        canonical_contract = {
            "message": phase["message"],
            "domain": phase["domain"],
            "canonical_fields": phase["canonical_fields"],
        }
        vectors.append({
            **vector,
            "domain": phase["domain"],
            "canonical_fields": phase["canonical_fields"],
            "phase_binding": canonical_contract,
            "canonical_contract_sha256": hashlib.sha256(
                json.dumps(canonical_contract, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
            ).hexdigest(),
        })
    return json.dumps({"schema_version": schema["schema_version"], "vectors": vectors}, indent=2, sort_keys=True) + "\n"


def _canonical_schema_bytes(path: Path = SCHEMA_PATH) -> bytes:
    """Return schema bytes with platform line endings removed from the digest."""

    text = path.read_text(encoding="utf-8")
    return text.replace("\r\n", "\n").replace("\r", "\n").encode("utf-8")


def _validate_vector_catalog(schema: Mapping[str, Any]) -> None:
    golden_names = [str(vector.get("name")) for vector in schema.get("golden_vectors", ())]
    if len(golden_names) != len(set(golden_names)):
        raise SchemaError("golden vector names must be unique")
    golden_set = set(golden_names)
    for label in ("negative_vectors", "compatibility_vectors"):
        vectors = schema.get(label, ())
        names = [str(vector.get("name")) for vector in vectors]
        if len(names) != len(set(names)):
            raise SchemaError(f"{label} names must be unique")
        for vector in vectors:
            base = vector.get("base")
            if base not in golden_set:
                raise SchemaError(f"{label} references unknown golden base: {base}")


def _expected_outputs(schema: Mapping[str, Any]) -> tuple[str, dict[str, tuple[bytes, str]], dict[str, tuple[bytes, str]], dict[str, str], str]:
    _validate_vector_catalog(schema)
    digest = hashlib.sha256(_canonical_schema_bytes()).hexdigest()
    header = render_header(schema, digest)
    golden: dict[str, tuple[bytes, str]] = {}
    base_frames: dict[str, bytes] = {}
    for vector in schema["golden_vectors"]:
        frame, rendered = _vector_frame(schema, vector)
        base_frames[str(vector["name"])] = frame
        golden[str(vector["name"])] = (frame, json.dumps(rendered, indent=2, sort_keys=True) + "\n")
    negative = _render_negative(schema, base_frames)
    compatibility = _render_compatibility(schema, base_frames)
    pairing = _render_pairing_vectors(schema)
    return header, golden, negative, compatibility, pairing


def _check_or_write_file(path: Path, content: bytes, write: bool) -> None:
    if write:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
        return
    if not path.exists() or path.read_bytes() != content:
        raise SchemaError(f"generated file out of date: {path.relative_to(ROOT)}")


def _expected_generated_paths(
    golden: Mapping[str, tuple[bytes, str]],
    negative: Mapping[str, tuple[bytes, str]],
    compatibility: Mapping[str, tuple[bytes, str]],
) -> set[Path]:
    paths = {HEADER_PATH, PAIRING_VECTOR_PATH}
    paths.update(GOLDEN_DIR / f"{name}.{suffix}" for name in golden for suffix in ("bin", "json"))
    paths.update(MALFORMED_DIR / f"{name}.{suffix}" for name in negative for suffix in ("bin", "json"))
    paths.update(COMPATIBILITY_DIR / f"{name}.{suffix}" for name in compatibility for suffix in ("bin", "json"))
    return paths


def _validate_generated_inventory(expected: set[Path], *, write: bool) -> None:
    actual: set[Path] = set()
    if HEADER_PATH.exists():
        actual.add(HEADER_PATH)
    for directory in (GOLDEN_DIR,):
        if directory.exists():
            actual.update(path for path in directory.rglob("*") if path.is_file())
    missing = sorted(path.relative_to(ROOT).as_posix() for path in expected - actual)
    extra = sorted(path.relative_to(ROOT).as_posix() for path in actual - expected)
    if extra or (missing and not write):
        details = []
        if missing:
            details.append(f"missing={missing}")
        if extra:
            details.append(f"extra={extra}")
        raise SchemaError("generated output inventory mismatch: " + "; ".join(details))


def generate(write: bool) -> None:
    schema = load_schema()
    validate_schema(schema)
    header, golden, negative, compatibility, pairing = _expected_outputs(schema)
    _validate_generated_inventory(_expected_generated_paths(golden, negative, compatibility), write=write)
    _check_or_write_file(HEADER_PATH, header.encode("utf-8"), write)
    for name, (frame, metadata) in golden.items():
        _check_or_write_file(GOLDEN_DIR / f"{name}.bin", frame, write)
        _check_or_write_file(GOLDEN_DIR / f"{name}.json", metadata.encode("utf-8"), write)
    for directory, vectors in ((MALFORMED_DIR, negative), (COMPATIBILITY_DIR, compatibility)):
        for name, (frame, metadata) in vectors.items():
            _check_or_write_file(directory / f"{name}.bin", frame, write)
            _check_or_write_file(directory / f"{name}.json", metadata.encode("utf-8"), write)
    _check_or_write_file(PAIRING_VECTOR_PATH, pairing.encode("utf-8"), write)
    mode = "generated" if write else "verified"
    print(f"protocol {mode}: header + {len(golden)} golden + {len(negative)} malformed + {len(compatibility)} compatibility + pairing")


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail if generated outputs are stale")
    parser.add_argument("--write", action="store_true", help="write generated outputs")
    args = parser.parse_args(argv)
    if args.check and args.write:
        parser.error("--check and --write are mutually exclusive")
    try:
        generate(write=not args.check)
    except (OSError, SchemaError, ValueError, KeyError) as exc:
        print(f"protocol generation failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
