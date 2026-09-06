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

    phases = schema.get("pairing_phases")
    negatives = schema.get("pairing_negative_vectors")
    phase_names = {str(phase.get("message")) for phase in phases} if isinstance(phases, Sequence) else set()
    if phase_names != {"DISCOVERY", "PAIR_REQUEST", "PAIR_CHALLENGE", "PAIR_CONFIRM", "PAIR_RESULT"}:
        raise SchemaError("pairing phases are incomplete")
    if not isinstance(negatives, Sequence) or isinstance(negatives, (str, bytes)):
        raise SchemaError("pairing_negative_vectors must be a list")
    seen_names: set[str] = set()
    for vector in negatives:
        if not isinstance(vector, Mapping):
            raise SchemaError("pairing negative vector must be an object")
        name = vector.get("name")
        if not isinstance(name, str) or name in seen_names:
            raise SchemaError("pairing negative vector names must be unique")
        if vector.get("phase") not in phase_names or not vector.get("mutation") or not vector.get("expected"):
            raise SchemaError(f"invalid pairing negative vector: {vector!r}")
        seen_names.add(name)
    if {str(vector.get("phase")) for vector in negatives} != phase_names:
        raise SchemaError("each pairing phase needs an independent negative vector")


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
    if not isinstance(encoding.get("crc"), Mapping) or encoding["crc"].get("name") != "CRC-32/ISO-HDLC":
        raise SchemaError("unsupported CRC contract")

    header = schema.get("header")
    if not isinstance(header, Mapping):
        raise SchemaError("header must be an object")
    _validate_fields(header["fields"], 32, "header")
    if header.get("reserved_zero") != ["reserved"]:
        raise SchemaError("header reserved_zero must identify the reserved field")
    if header.get("flags_known_mask") != 0x7F:
        raise SchemaError("unexpected frame flag mask")

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

    enums = schema.get("enums")
    if not isinstance(enums, Sequence):
        raise SchemaError("enums must be a list")
    enum_types: set[str] = set()
    enumerators: set[str] = set()
    for enum in enums:
        c_type = enum.get("c_type")
        prefix = enum.get("c_prefix")
        values = enum.get("values")
        if not isinstance(c_type, str) or not isinstance(prefix, str) or not isinstance(values, Mapping):
            raise SchemaError(f"invalid enum declaration {enum!r}")
        _validate_identifier(c_type, "enum c_type")
        _validate_identifier(prefix, "enum c_prefix")
        if c_type in enum_types:
            raise SchemaError(f"duplicate enum type {c_type}")
        enum_types.add(c_type)
        for name, value in values.items():
            enum_name = f"{prefix}_{name}"
            _validate_identifier(enum_name, "enum value")
            if enum_name in enumerators:
                raise SchemaError(f"duplicate enumerator {enum_name}")
            if not isinstance(value, int):
                raise SchemaError(f"enum value must be an integer: {enum_name}")
            enumerators.add(enum_name)

    tlv = schema.get("tlv")
    if not isinstance(tlv, Mapping) or tlv.get("header_size") != 4 or tlv.get("max_nesting_depth") != 0:
        raise SchemaError("TLV header/nesting contract is invalid")
    tlv_types = tlv.get("types")
    if not isinstance(tlv_types, Sequence):
        raise SchemaError("tlv.types must be a list")
    tlv_values: set[int] = set()
    for item in tlv_types:
        value = item.get("value")
        if not isinstance(value, int) or value in tlv_values:
            raise SchemaError(f"invalid or duplicate TLV type {value}")
        if not isinstance(item.get("size"), int) or item["size"] < -1:
            raise SchemaError(f"invalid TLV size for {value}")
        tlv_values.add(value)

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
    _validate_companion_schemas(schema, message_ids)
    contracts = schema.get("message_contracts")
    if not isinstance(contracts, Mapping) or set(contracts) != message_names:
        raise SchemaError("message_contracts must describe every declared message exactly once")
    for message_name, contract in contracts.items():
        if not isinstance(contract, Mapping) or contract.get("since") != "1.3":
            raise SchemaError(f"message contract {message_name} must declare since=1.3")
        response = contract.get("response")
        if response is not None and response not in message_names:
            raise SchemaError(f"message contract {message_name} references unknown response {response}")
        if not isinstance(contract.get("idempotency_key"), Sequence) or not contract["idempotency_key"]:
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


def _render_enum(enum: Mapping[str, Any]) -> list[str]:
    lines = ["typedef enum {"]
    values = list(enum["values"].items())
    for index, (name, value) in enumerate(values):
        comma = "," if index < len(values) - 1 else ","
        lines.append(f"    {enum['c_prefix']}_{name} = {_c_literal(int(value), 'u32')}{comma}")
    lines.extend([f"}} {enum['c_type']};", ""])
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
    for enum in schema["enums"]:
        lines.extend(_render_enum(enum))

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


def encode_payload(message: Mapping[str, Any], values: Mapping[str, Any]) -> bytes:
    payload = message["payload"]
    kind = payload["kind"]
    if kind == "fixed":
        return _encode_fields(payload["fields"], values)
    if kind == "variants":
        variant_name = values.get("variant") or values.get("_variant")
        if not isinstance(variant_name, str):
            raise ValueError(f"{message['name']} requires variant")
        for variant in payload["variants"]:
            if variant["name"] == variant_name:
                return _encode_fields(variant["fields"], values)
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
        return bytes(output)
    if kind == "suffix":
        prefix_values = dict(values.get("prefix", values))
        output = bytearray(_encode_fields(payload["prefix"]["fields"], prefix_values))
        suffix_name = str(payload["suffix_name"])
        suffix = _value_bytes(values.get(suffix_name, b""))
        if len(suffix) > int(payload["suffix_max"]):
            raise ValueError(f"{message['name']} suffix exceeds maximum")
        length_field = next((f for f in payload["prefix"]["fields"] if f["name"].endswith("_length")), None)
        if length_field is not None and int(values.get(length_field["name"], len(suffix))) != len(suffix):
            raise ValueError(f"{message['name']} suffix length field mismatch")
        output.extend(suffix)
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
        return bytes(output)
    raise SchemaError(f"unknown payload kind {kind}")


def _decode_tlvs(schema: Mapping[str, Any], data: bytes) -> list[dict[str, Any]]:
    known = {int(item["value"]): item for item in schema["tlv"]["types"]}
    tlvs: list[dict[str, Any]] = []
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
        base_type = type_value & 0x7FFF
        definition = known.get(type_value) or known.get(base_type)
        if definition is None and type_value & int(schema["tlv"]["critical_bit"]):
            raise ProtocolDecodeError("unsupported_tlv", f"unknown critical TLV {type_value}")
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
        return _decode_fields(payload["fields"], data)
    if kind == "variants":
        for variant in payload["variants"]:
            if len(data) == int(variant["size"]):
                result = _decode_fields(variant["fields"], data)
                result["variant"] = variant["name"]
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
        return prefix
    if kind == "suffix":
        prefix_size = int(payload["prefix_size"])
        result = _decode_fields(payload["prefix"]["fields"], data[:prefix_size])
        suffix = data[prefix_size:]
        length_field = next((f for f in payload["prefix"]["fields"] if f["name"].endswith("_length")), None)
        if length_field is not None and int(result[length_field["name"]]) != len(suffix):
            raise ProtocolDecodeError("bad_length", "suffix length field mismatch")
        result[str(payload["suffix_name"])] = suffix.hex()
        return result
    if kind == "tlv":
        prefix_size = int(payload["prefix_size"])
        result = _decode_fields(payload["prefix"]["fields"], data[:prefix_size])
        result["tlvs"] = _decode_tlvs(schema, data[prefix_size:])
        return result
    raise SchemaError(f"unknown payload kind {kind}")


def _header_bytes(schema: Mapping[str, Any], values: Mapping[str, Any]) -> bytes:
    return _encode_fields(schema["header"]["fields"], values)


def encode_frame(schema: Mapping[str, Any], message: Mapping[str, Any], header: Mapping[str, Any], payload_values: Mapping[str, Any]) -> bytes:
    payload = encode_payload(message, payload_values)
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
    zero_crc_header = _header_bytes(schema, header_values)
    crc = zlib.crc32(zero_crc_header + payload) & 0xFFFFFFFF
    header_values["crc32"] = crc
    return _header_bytes(schema, header_values) + payload


def _constant_value(schema: Mapping[str, Any], name: str) -> int:
    for constant in schema["constants"]:
        if constant["c_name"] == name:
            return int(constant["value"])
    raise KeyError(name)


def decode_frame(schema: Mapping[str, Any], frame: bytes) -> dict[str, Any]:
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
    payload_len = int(header["payload_len"])
    if payload_len > 208 or len(frame) != 32 + payload_len:
        raise ProtocolDecodeError("bad_length", "frame length does not match payload length")
    zero_crc = bytearray(header_bytes)
    zero_crc[28:32] = b"\x00\x00\x00\x00"
    expected_crc = zlib.crc32(bytes(zero_crc) + frame[32:]) & 0xFFFFFFFF
    if int(header["crc32"]) != expected_crc:
        raise ProtocolDecodeError("bad_crc", "CRC mismatch")
    message = next((m for m in schema["messages"] if int(m["id"]) == int(header["message_type"])), None)
    if message is None:
        raise ProtocolDecodeError("unsupported_message", "unknown message type")
    payload = decode_payload(schema, message, frame[32:])
    return {"header": header, "message": message["name"], "message_id": message["id"], "payload": payload}


def _vector_frame(schema: Mapping[str, Any], vector: Mapping[str, Any]) -> tuple[bytes, dict[str, Any]]:
    message = message_by_name(schema, str(vector["message"]))
    frame = encode_frame(schema, message, vector.get("header", {}), vector.get("payload", {}))
    header = decode_frame(schema, frame)["header"]
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
        vectors.append({
            **vector,
            "domain": phase["domain"],
            "canonical_fields": phase["canonical_fields"],
        })
    return json.dumps({"schema_version": schema["schema_version"], "vectors": vectors}, indent=2, sort_keys=True) + "\n"


def _canonical_schema_bytes(path: Path = SCHEMA_PATH) -> bytes:
    """Return schema bytes with platform line endings removed from the digest."""

    text = path.read_text(encoding="utf-8")
    return text.replace("\r\n", "\n").replace("\r", "\n").encode("utf-8")


def _expected_outputs(schema: Mapping[str, Any]) -> tuple[str, dict[str, tuple[bytes, str]], dict[str, tuple[bytes, str]], dict[str, str], str]:
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


def generate(write: bool) -> None:
    schema = load_schema()
    validate_schema(schema)
    header, golden, negative, compatibility, pairing = _expected_outputs(schema)
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
