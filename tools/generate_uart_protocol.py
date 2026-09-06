#!/usr/bin/env python3
"""Generate and validate the Communicator UART v1.0 payload ABI.

The schema is JSON-compatible YAML so the protocol check stays dependency-free
on host and target CI.  The generated header contains offsets and packed
inspection types, but runtime code still uses explicit little-endian accessors
and never casts a received buffer to a wire struct.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any, Mapping, Sequence


ROOT = Path(__file__).resolve().parents[1]
SCHEMA_PATH = ROOT / "protocol" / "schema" / "uart-v1.0.yaml"
HEADER_PATH = ROOT / "protocol" / "canview_uart_protocol.h"
IDENTIFIER = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
TYPE_INFO: dict[str, tuple[str, int]] = {
    "u8": ("uint8_t", 1),
    "i8": ("int8_t", 1),
    "u16": ("uint16_t", 2),
    "i16": ("int16_t", 2),
    "u32": ("uint32_t", 4),
    "i32": ("int32_t", 4),
    "u64": ("uint64_t", 8),
    "i64": ("int64_t", 8),
}
EXPECTED_MESSAGES = {
    "LINK_HELLO": 0x01,
    "LINK_HELLO_ACK": 0x02,
    "HEARTBEAT": 0x03,
    "ACK": 0x04,
    "ERROR": 0x05,
    "CAN_RX_BATCH": 0x10,
    "CAN_BUS_STATUS": 0x11,
    "SAFETY_SNAPSHOT": 0x12,
    "CAN_TX_AUDIT": 0x13,
    "CAN_ID_STATS": 0x14,
    "CAN_OBSERVER_PLAN": 0x15,
    "CAN_CAPTURE_CONTROL": 0x16,
    "CAN_CAPTURE_STATUS": 0x17,
    "CAN_EVENT_MARKER": 0x18,
    "COMMAND_REQUEST": 0x20,
    "COMMAND_RESULT": 0x21,
    "CONTROL_LEASE": 0x22,
    "CONFIG_GET": 0x30,
    "CONFIG_SET": 0x31,
    "CONFIG_RESULT": 0x32,
    "DIAGNOSTIC_COUNTERS": 0x40,
    "FIRMWARE_PREPARE": 0x50,
}


class SchemaError(ValueError):
    """Raised when the UART schema is not a complete wire contract."""


def load_schema(path: Path = SCHEMA_PATH) -> dict[str, Any]:
    raw = path.read_text(encoding="utf-8")
    try:
        parsed = json.loads(raw)
    except json.JSONDecodeError:
        try:
            import yaml  # type: ignore[import-not-found]
        except ImportError as exc:
            raise SchemaError("schema is not JSON-compatible and PyYAML is not installed") from exc
        parsed = yaml.safe_load(raw)
    if not isinstance(parsed, dict):
        raise SchemaError("schema root must be an object")
    return parsed


def canonical_schema_bytes(path: Path = SCHEMA_PATH) -> bytes:
    return path.read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")


def _type_size(field: Mapping[str, Any]) -> int:
    field_type = field.get("type")
    if field_type == "bytes":
        length = field.get("length")
        if type(length) is not int or length <= 0:
            raise SchemaError(f"bytes field requires positive length: {field}")
        return length
    if field_type not in TYPE_INFO:
        raise SchemaError(f"unknown field type: {field_type}")
    return TYPE_INFO[str(field_type)][1]


def _field_c_name(field: Mapping[str, Any]) -> str:
    value = field.get("c_name")
    if value is not None:
        return str(value)
    field_type = str(field.get("type"))
    name = str(field.get("name"))
    if field_type in TYPE_INFO and TYPE_INFO[field_type][1] > 1:
        return f"{name}_le"
    return name


def _validate_fields(fields: Any, expected_size: int, label: str) -> list[dict[str, Any]]:
    if not isinstance(fields, list) or not fields:
        raise SchemaError(f"{label} requires non-empty fields")
    normalized: list[dict[str, Any]] = []
    names: set[str] = set()
    cursor = 0
    for field in fields:
        if not isinstance(field, dict):
            raise SchemaError(f"{label} field is not an object")
        name = str(field.get("name", ""))
        c_name = _field_c_name(field)
        if not IDENTIFIER.fullmatch(name) or not IDENTIFIER.fullmatch(c_name):
            raise SchemaError(f"invalid field identifier in {label}: {field}")
        if name in names or c_name in names:
            raise SchemaError(f"duplicate field in {label}: {name}")
        offset = field.get("offset")
        if type(offset) is not int or offset != cursor:
            raise SchemaError(f"non-contiguous offset in {label}: {field}")
        field_size = _type_size(field)
        if bool(field.get("reserved", False)) and not name.lower().startswith("reserved"):
            raise SchemaError(f"reserved field must be named reserved*: {label}.{name}")
        item = dict(field)
        item["c_name"] = c_name
        item["size"] = field_size
        normalized.append(item)
        names.add(name)
        names.add(c_name)
        cursor += field_size
    if cursor != expected_size:
        raise SchemaError(f"{label} fields use {cursor} bytes, expected {expected_size}")
    return normalized


def _validate_layout(layout: Mapping[str, Any], label: str) -> list[dict[str, Any]]:
    size = layout.get("size")
    if type(size) is not int or size <= 0:
        raise SchemaError(f"{label} requires positive size")
    return _validate_fields(layout.get("fields"), size, label)


def _payload_bounds(payload: Mapping[str, Any], label: str) -> tuple[int, int, int, list[tuple[str, list[dict[str, Any]], int]]]:
    kind = str(payload.get("kind"))
    layouts: list[tuple[str, list[dict[str, Any]], int]] = []
    if kind == "fixed":
        size = payload.get("size")
        if type(size) is not int or size < 0:
            raise SchemaError(f"{label} fixed payload requires size")
        fields = _validate_fields(payload.get("fields"), size, label)
        layouts.append((str(payload["c_type"]), fields, size))
        return size, size, size, layouts
    if kind == "can_batch":
        expected = (payload.get("prefix_size"), payload.get("record_size"), payload.get("count_max"), payload.get("max_size"))
        if expected != (12, 16, 12, 204):
            raise SchemaError(f"{label} CAN batch foundation changed")
        return 12, 204, 0, layouts
    if kind == "suffix":
        prefix_size = payload.get("prefix_size")
        suffix_max = payload.get("suffix_max")
        if type(prefix_size) is not int or type(suffix_max) is not int or prefix_size < 0 or suffix_max < 0:
            raise SchemaError(f"{label} suffix bounds are invalid")
        fields = _validate_fields(payload.get("fields"), prefix_size, label)
        layouts.append((str(payload["c_type"]), fields, prefix_size))
        return prefix_size, prefix_size + suffix_max, prefix_size, layouts
    if kind == "bounded":
        count_max = payload.get("count_max")
        prefix = payload.get("prefix")
        record = payload.get("record")
        if type(count_max) is not int or count_max <= 0 or not isinstance(prefix, dict) or not isinstance(record, dict):
            raise SchemaError(f"{label} bounded payload is incomplete")
        prefix_size = prefix.get("size")
        record_size = record.get("size")
        if type(prefix_size) is not int or type(record_size) is not int:
            raise SchemaError(f"{label} bounded layout size is invalid")
        prefix_fields = _validate_fields(prefix.get("fields"), prefix_size, f"{label}.prefix")
        record_fields = _validate_fields(record.get("fields"), record_size, f"{label}.record")
        layouts.extend(((str(prefix["c_type"]), prefix_fields, prefix_size),
                        (str(record["c_type"]), record_fields, record_size)))
        return prefix_size, prefix_size + count_max * record_size, prefix_size, layouts
    if kind == "variants":
        variants = payload.get("variants")
        if not isinstance(variants, list) or not variants:
            raise SchemaError(f"{label} variants are missing")
        sizes: list[int] = []
        values: set[int] = set()
        for variant in variants:
            if not isinstance(variant, dict):
                raise SchemaError(f"{label} variant is not an object")
            value = variant.get("value")
            name = str(variant.get("name", ""))
            if type(value) is not int or value in values or not IDENTIFIER.fullmatch(name):
                raise SchemaError(f"invalid {label} variant: {variant}")
            values.add(value)
            variant_kind = str(variant.get("kind", "fixed"))
            if variant_kind == "bounded":
                count_max = variant.get("count_max")
                prefix_size = variant.get("prefix_size")
                record = variant.get("record")
                if type(count_max) is not int or type(prefix_size) is not int or not isinstance(record, dict):
                    raise SchemaError(f"invalid bounded variant: {label}.{name}")
                fields = _validate_fields(variant.get("fields"), prefix_size, f"{label}.{name}.prefix")
                record_size = record.get("size")
                if type(record_size) is not int:
                    raise SchemaError(f"invalid record size: {label}.{name}")
                record_fields = _validate_fields(record.get("fields"), record_size, f"{label}.{name}.record")
                layouts.extend(((str(variant["c_type"]), fields, prefix_size),
                                (str(record["c_type"]), record_fields, record_size)))
                size = prefix_size + count_max * record_size
            else:
                size = variant.get("size")
                if type(size) is not int or size <= 0:
                    raise SchemaError(f"invalid variant size: {label}.{name}")
                fields = _validate_fields(variant.get("fields"), size, f"{label}.{name}")
                layouts.append((str(variant["c_type"]), fields, size))
            sizes.append(size)
        return min(sizes), max(sizes), 0, layouts
    raise SchemaError(f"unknown payload kind in {label}: {kind}")


def validate_schema(schema: Mapping[str, Any]) -> None:
    if schema.get("schema_version") != 1:
        raise SchemaError("unsupported schema_version")
    protocol = schema.get("protocol")
    if not isinstance(protocol, dict):
        raise SchemaError("protocol section is required")
    expected_protocol = {
        "major": 1,
        "minor": 0,
        "header_size": 32,
        "max_frame": 1024,
        "max_payload": 992,
        "max_encoded": 1029,
        "baud": 4000000,
        "crc": "CRC-32/ISO-HDLC",
    }
    for key, expected in expected_protocol.items():
        if protocol.get(key) != expected:
            raise SchemaError(f"UART physical/wire contract changed: {key}")
    flags = schema.get("flags")
    if flags != {"ACK_REQUIRED": 1, "RESPONSE": 2, "ERROR": 4, "HIGH_PRIORITY": 8, "SNAPSHOT": 16}:
        raise SchemaError("UART flags contract changed")
    limits = schema.get("limits")
    if not isinstance(limits, dict):
        raise SchemaError("limits section is required")
    required_limits = {
        "command_cache_capacity": 256,
        "command_result_max": 82,
        "plan_max_chunks": 16,
        "plan_max_filters": 64,
        "plan_chunk_max_filters": 16,
        "can_batch_max_records": 12,
        "can_id_stats_max_records": 5,
        "config_max_records": 25,
        "diagnostic_counter_max_records": 12,
    }
    if limits != required_limits:
        raise SchemaError("reviewed UART resource limits changed")
    enums = schema.get("enums")
    if not isinstance(enums, dict) or enums.get("capture_action") != {"ARM": 1, "START": 2, "STOP": 3, "CANCEL": 4}:
        raise SchemaError("capture actions must not include MARK")
    if enums.get("plan_operation") != {"BEGIN": 1, "CHUNK": 2, "COMMIT": 3, "ABORT": 4}:
        raise SchemaError("observer plan operation contract changed")
    messages = schema.get("messages")
    if not isinstance(messages, list) or len(messages) != len(EXPECTED_MESSAGES):
        raise SchemaError("UART message catalog is incomplete")
    seen_ids: set[int] = set()
    seen_names: set[str] = set()
    for message in messages:
        if not isinstance(message, dict):
            raise SchemaError("message entry is not an object")
        name = str(message.get("name", ""))
        message_id = message.get("id")
        if name not in EXPECTED_MESSAGES or EXPECTED_MESSAGES[name] != message_id:
            raise SchemaError(f"unexpected or moved UART message: {message}")
        if name in seen_names or message_id in seen_ids:
            raise SchemaError(f"duplicate UART message: {name}")
        seen_names.add(name)
        seen_ids.add(message_id)
        for key in ("direction", "qos", "supported"):
            if key not in message:
                raise SchemaError(f"{name} missing {key}")
        allowed = message.get("allowed_flags")
        required = message.get("required_flags")
        if type(allowed) is not int or type(required) is not int or allowed & ~0x1F or required & ~allowed:
            raise SchemaError(f"invalid flags policy: {name}")
        minimum, maximum, _, layouts = _payload_bounds(message.get("payload", {}), name)
        if minimum < 0 or maximum > int(protocol["max_payload"]):
            raise SchemaError(f"payload bound exceeds UART max: {name}")
        for c_type, _, _ in layouts:
            if not IDENTIFIER.fullmatch(c_type):
                raise SchemaError(f"invalid C type in {name}: {c_type}")
    if seen_names != set(EXPECTED_MESSAGES):
        raise SchemaError("UART message catalog names do not match the reviewed set")


def _macro_name(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "_", value).upper()


def _c_literal(value: int, width: int) -> str:
    suffix = {1: "UINT8_C", 2: "UINT16_C", 4: "UINT32_C", 8: "UINT64_C"}[width]
    return f"{suffix}(0x{value:X})"


def _render_layout(lines: list[str], c_type: str, fields: list[dict[str, Any]], size: int) -> None:
    lines.append(f"typedef struct CANVIEW_UART_PACKED {{")
    for field in fields:
        field_type = str(field["type"])
        c_name = str(field["c_name"])
        if field_type == "bytes":
            lines.append(f"    uint8_t {c_name}[{int(field['length'])}U];")
        else:
            lines.append(f"    {TYPE_INFO[field_type][0]} {c_name};")
    lines.append(f"}} {c_type};")
    safe = _macro_name(c_type).lower()
    lines.append(f"typedef char canview_uart_assert_{safe}_size[(sizeof({c_type}) == {size}U) ? 1 : -1];")
    for field in fields:
        c_name = str(field["c_name"])
        lines.append(
            f"typedef char canview_uart_assert_{safe}_{_macro_name(c_name).lower()}_offset["
            f"(offsetof({c_type}, {c_name}) == {int(field['offset'])}U) ? 1 : -1];"
        )
    lines.append("")


def render(schema: Mapping[str, Any], digest: str) -> str:
    validate_schema(schema)
    protocol = schema["protocol"]
    limits = schema["limits"]
    flags = schema["flags"]
    messages = schema["messages"]
    lines = [
        "/* GENERATED FILE - DO NOT EDIT. */",
        "/* Source: protocol/schema/uart-v1.0.yaml */",
        "/* Regenerate with: python tools/generate_uart_protocol.py --write */",
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "#ifndef CANVIEW_UART_PROTOCOL_H",
        "#define CANVIEW_UART_PROTOCOL_H",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "#ifdef __cplusplus",
        "extern \"C\" {",
        "#endif",
        "",
        f"#define CANVIEW_UART_PROTOCOL_SCHEMA_SHA256 \"{digest}\"",
        f"#define CANVIEW_UART_PROTOCOL_NAME \"{protocol['name']}\"",
        f"#define CANVIEW_UART_PROTOCOL_MAJOR UINT8_C({int(protocol['major'])})",
        f"#define CANVIEW_UART_PROTOCOL_MINOR UINT8_C({int(protocol['minor'])})",
        f"#define CANVIEW_UART_HEADER_SIZE UINT16_C({int(protocol['header_size'])})",
        f"#define CANVIEW_UART_MAX_FRAME_SIZE UINT16_C({int(protocol['max_frame'])})",
        f"#define CANVIEW_UART_MAX_PAYLOAD_SIZE UINT16_C({int(protocol['max_payload'])})",
        f"#define CANVIEW_UART_MAX_ENCODED_SIZE UINT16_C({int(protocol['max_encoded'])})",
        "#define CANVIEW_UART_MAX_SERIAL_SIZE (CANVIEW_UART_MAX_ENCODED_SIZE + 1U)",
        f"#define CANVIEW_UART_BAUD UINT32_C({int(protocol['baud'])})",
        f"#define CANVIEW_UART_COMMAND_CACHE_CAPACITY UINT16_C({int(limits['command_cache_capacity'])})",
        f"#define CANVIEW_UART_COMMAND_RESULT_MAX UINT16_C({int(limits['command_result_max'])})",
        f"#define CANVIEW_UART_PLAN_MAX_CHUNKS UINT8_C({int(limits['plan_max_chunks'])})",
        f"#define CANVIEW_UART_PLAN_MAX_FILTERS UINT8_C({int(limits['plan_max_filters'])})",
        f"#define CANVIEW_UART_PLAN_CHUNK_MAX_FILTERS UINT8_C({int(limits['plan_chunk_max_filters'])})",
        f"#define CANVIEW_UART_CAN_BATCH_MAX_RECORDS UINT8_C({int(limits['can_batch_max_records'])})",
        f"#define CANVIEW_UART_CAN_ID_STATS_MAX_RECORDS UINT8_C({int(limits['can_id_stats_max_records'])})",
        f"#define CANVIEW_UART_CONFIG_MAX_RECORDS UINT8_C({int(limits['config_max_records'])})",
        f"#define CANVIEW_UART_DIAGNOSTIC_COUNTER_MAX_RECORDS UINT8_C({int(limits['diagnostic_counter_max_records'])})",
        "",
        f"#define CANVIEW_UART_FLAG_ACK_REQUIRED UINT8_C({flags['ACK_REQUIRED']})",
        f"#define CANVIEW_UART_FLAG_RESPONSE UINT8_C({flags['RESPONSE']})",
        f"#define CANVIEW_UART_FLAG_ERROR UINT8_C({flags['ERROR']})",
        f"#define CANVIEW_UART_FLAG_HIGH_PRIORITY UINT8_C({flags['HIGH_PRIORITY']})",
        f"#define CANVIEW_UART_FLAG_SNAPSHOT UINT8_C({flags['SNAPSHOT']})",
        "#define CANVIEW_UART_FLAG_KNOWN_MASK UINT8_C(0x1F)",
        "",
        "#if defined(_MSC_VER)",
        "#define CANVIEW_UART_PACKED",
        "#pragma pack(push, 1)",
        "#elif defined(__GNUC__) || defined(__clang__)",
        "#define CANVIEW_UART_PACKED __attribute__((packed))",
        "#else",
        "#define CANVIEW_UART_PACKED",
        "#endif",
        "",
        "typedef enum {",
    ]
    for message in messages:
        lines.append(f"    CANVIEW_UART_MSG_{message['name']} = UINT8_C(0x{int(message['id']):02X}),")
    lines += [
        "} canview_uart_message_type_t;",
        f"#define CANVIEW_UART_MESSAGE_COUNT UINT8_C({len(messages)})",
        "",
        "typedef enum {",
        "    CANVIEW_UART_PLAN_OP_BEGIN = UINT8_C(1),",
        "    CANVIEW_UART_PLAN_OP_CHUNK = UINT8_C(2),",
        "    CANVIEW_UART_PLAN_OP_COMMIT = UINT8_C(3),",
        "    CANVIEW_UART_PLAN_OP_ABORT = UINT8_C(4),",
        "} canview_uart_plan_operation_t;",
        "",
        "typedef enum {",
        "    CANVIEW_UART_CAPTURE_ARM = UINT8_C(1),",
        "    CANVIEW_UART_CAPTURE_START = UINT8_C(2),",
        "    CANVIEW_UART_CAPTURE_STOP = UINT8_C(3),",
        "    CANVIEW_UART_CAPTURE_CANCEL = UINT8_C(4),",
        "} canview_uart_capture_action_t;",
        "",
        "typedef enum {",
        "    CANVIEW_UART_PAYLOAD_FIXED = UINT8_C(0),",
        "    CANVIEW_UART_PAYLOAD_CAN_BATCH = UINT8_C(1),",
        "    CANVIEW_UART_PAYLOAD_BOUNDED = UINT8_C(2),",
        "    CANVIEW_UART_PAYLOAD_SUFFIX = UINT8_C(3),",
        "    CANVIEW_UART_PAYLOAD_VARIANTS = UINT8_C(4),",
        "} canview_uart_payload_kind_t;",
        "",
        "typedef struct {",
        "    uint8_t message_type;",
        "    uint16_t min_payload;",
        "    uint16_t max_payload;",
        "    uint8_t required_flags;",
        "    uint8_t allowed_flags;",
        "    uint8_t payload_kind;",
        "    uint8_t supported;",
        "} canview_uart_message_policy_t;",
        "",
    ]
    layout_names: set[str] = set()
    for message in messages:
        payload = message["payload"]
        _, _, _, layouts = _payload_bounds(payload, str(message["name"]))
        for c_type, fields, size in layouts:
            if c_type in layout_names:
                continue
            layout_names.add(c_type)
            _render_layout(lines, c_type, fields, size)
    if any(True for _ in layout_names):
        lines += ["#if defined(_MSC_VER)", "#pragma pack(pop)", "#endif", ""]
    lines += [
        "/* The following table is metadata, not a permission grant. Runtime code",
        " * still applies role, link, safety and lease checks before dispatch. */",
        "static const canview_uart_message_policy_t CANVIEW_UART_MESSAGE_POLICIES[] = {",
    ]
    for message in messages:
        minimum, maximum, _, _ = _payload_bounds(message["payload"], str(message["name"]))
        kind_value = {
            "fixed": "CANVIEW_UART_PAYLOAD_FIXED",
            "can_batch": "CANVIEW_UART_PAYLOAD_CAN_BATCH",
            "bounded": "CANVIEW_UART_PAYLOAD_BOUNDED",
            "suffix": "CANVIEW_UART_PAYLOAD_SUFFIX",
            "variants": "CANVIEW_UART_PAYLOAD_VARIANTS",
        }[str(message["payload"]["kind"])]
        lines.append(
            f"    {{CANVIEW_UART_MSG_{message['name']}, {minimum}U, {maximum}U, "
            f"UINT8_C({int(message['required_flags'])}), UINT8_C({int(message['allowed_flags'])}), "
            f"{kind_value}, UINT8_C({1 if message['supported'] else 0})}},"
        )
    lines += [
        "};",
        "#define CANVIEW_UART_MESSAGE_POLICY_COUNT "
        "(sizeof(CANVIEW_UART_MESSAGE_POLICIES) / sizeof(CANVIEW_UART_MESSAGE_POLICIES[0]))",
        "",
        "typedef char canview_uart_assert_protocol_max[",
        "    (CANVIEW_UART_MAX_PAYLOAD_SIZE + CANVIEW_UART_HEADER_SIZE == CANVIEW_UART_MAX_FRAME_SIZE) ? 1 : -1];",
        "typedef char canview_uart_assert_protocol_encoded[",
        "    (CANVIEW_UART_MAX_ENCODED_SIZE == 1029U) ? 1 : -1];",
        "",
        "#ifdef __cplusplus",
        "}",
        "#endif",
        "#endif",
        "",
    ]
    return "\n".join(lines)


def generate(schema_path: Path = SCHEMA_PATH, *, write: bool) -> str:
    schema = load_schema(schema_path)
    validate_schema(schema)
    digest = hashlib.sha256(canonical_schema_bytes(schema_path)).hexdigest()
    rendered = render(schema, digest)
    if write:
        HEADER_PATH.parent.mkdir(parents=True, exist_ok=True)
        HEADER_PATH.write_text(rendered, encoding="utf-8", newline="\n")
        print(f"UART protocol generated: {HEADER_PATH.relative_to(ROOT)}")
    else:
        if not HEADER_PATH.exists() or HEADER_PATH.read_text(encoding="utf-8") != rendered:
            raise SchemaError(f"generated file out of date: {HEADER_PATH.relative_to(ROOT)}")
        print("UART protocol generated output verified")
    return rendered


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--schema", type=Path, default=SCHEMA_PATH)
    parser.add_argument("--check", action="store_true", help="fail if generated output is stale")
    parser.add_argument("--write", action="store_true", help="write generated output")
    args = parser.parse_args(argv)
    if args.check and args.write:
        parser.error("--check and --write are mutually exclusive")
    schema_path = args.schema if args.schema.is_absolute() else ROOT / args.schema
    try:
        generate(schema_path, write=not args.check)
    except (OSError, SchemaError, ValueError, KeyError) as exc:
        print(f"UART protocol generation failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
