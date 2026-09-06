"""MCU 독립 envelope 기반 상수 생성. 전체 v1.3 ABI 동결기는 아니다."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "protocol/schema/transport-foundation-v1.json"
OUTPUT = ROOT / "shared/protocol/include/canview_wire_layout.h"


def render(source: bytes) -> str:
    spec = json.loads(source)
    if spec["schema_version"] != 1:
        raise ValueError("unsupported schema")
    lines = [
        "/* DO NOT EDIT. generate_transport.py v1; SPDX-License-Identifier: GPL-3.0-only",
        f" * source SHA256: {hashlib.sha256(source).hexdigest()}",
        " * Envelope foundation only; not complete/authorized runtime ABI. */",
        "#ifndef CANVIEW_WIRE_LAYOUT_H", "#define CANVIEW_WIRE_LAYOUT_H",
        "#include <stdint.h>", "",
    ]
    for transport in ("espnow", "uart"):
        item = spec[transport]
        expected = {"magic": 2, "major": 1, "minor": 1, "message_type": 1,
                    "flags": 1, "sequence": 4, "correlation_id": 4,
                    "payload_len": 2, "reserved": 2, "crc32": 4}
        expected.update({"header_len": 1, "priority": 1, "session_id": 4,
                         "sender_time_ms": 4} if transport == "espnow" else
                        {"header_len": 2, "sender_time_us": 8})
        if dict(item["header"]) != expected or item["header"][:3] != [["magic", 2], ["major", 1], ["minor", 1]]:
            raise ValueError("codec field contract changed; update codec and schema together")
        for key, limit in (("major", 255), ("minor", 255), ("magic", 65535), ("flags_mask", 255)):
            if type(item[key]) is not int or not 0 <= item[key] <= limit:
                raise ValueError("field value does not fit wire")
        if item["max_frame"] != (240 if transport == "espnow" else 1024):
            raise ValueError("reviewed transport budget changed")
        prefix = "CANVIEW_WIRE_" + transport.upper()
        offset = 0
        names = set()
        for name, size in item["header"]:
            if not re.fullmatch(r"[a-z][a-z0-9_]*", name) or name in names or size not in (1, 2, 4, 8):
                raise ValueError("duplicate field or invalid width")
            names.add(name)
            lines.append(f"#define {prefix}_{name.upper()}_OFFSET ({offset}U)")
            offset += size
        if offset != 32 or item["header"][-1] != ["crc32", 4]:
            raise ValueError("v1 envelope requires 32-byte header and trailing CRC")
        for key in ("major", "minor", "magic", "max_frame", "flags_mask"):
            lines.append(f"#define {prefix}_{key.upper()} ({item[key]}U)")
        lines += [f"#define {prefix}_HEADER_SIZE ({offset}U)",
                  f"#define {prefix}_MAX_PAYLOAD ({item['max_frame'] - offset}U)"]
    size = spec["uart"]["max_frame"]
    lines += [f"#define CANVIEW_WIRE_UART_MAX_ENCODED ({size + size // 254 + 1}U)",
              "#define CANVIEW_WIRE_UART_MAX_SERIAL (CANVIEW_WIRE_UART_MAX_ENCODED + 1U)",
              f"#define CANVIEW_WIRE_ESPNOW_MAX_PRIORITY ({spec['espnow']['max_priority']}U)",
              f"#define CANVIEW_WIRE_UART_BAUD ({spec['uart']['baud']}U)"]
    expected_can = {"espnow_message": 32, "uart_message": 16, "prefix_size": 12,
                    "record_size": 16, "max_records": 12, "bus_count": 3, "max_dlc": 8,
                    "standard_id_max": 2047, "extended_id_max": 536870911}
    if spec["can_batch"] != expected_can or spec["espnow"]["max_priority"] != 4:
        raise ValueError("CAN/priority codec contract changed")
    if spec["uart"]["baud"] != 4000000:
        raise ValueError("UART physical contract changed")
    if spec["crc"] != {"name": "CRC-32/ISO-HDLC", "polynomial_reflected": 0xEDB88320,
                       "initial": 0xFFFFFFFF, "xor_out": 0xFFFFFFFF, "check_123456789": 0xCBF43926}:
        raise ValueError("CRC contract changed")
    for key, value in spec["can_batch"].items():
        lines.append(f"#define CANVIEW_WIRE_CAN_{key.upper()} ({value}U)")
    for key in ("polynomial_reflected", "initial", "xor_out", "check_123456789"):
        lines.append(f"#define CANVIEW_WIRE_CRC_{key.upper()} UINT32_C(0x{spec['crc'][key]:08X})")
    lines += ["", "/* C99 compile-time contracts; never sizeof(host_struct) as wire ABI. */",
              "typedef char canview_assert_byte_width[(UINT8_MAX == 255U) ? 1 : -1];",
              "typedef char canview_assert_can_fits[((CANVIEW_WIRE_CAN_PREFIX_SIZE +",
              "    CANVIEW_WIRE_CAN_MAX_RECORDS * CANVIEW_WIRE_CAN_RECORD_SIZE) <=",
              "    CANVIEW_WIRE_ESPNOW_MAX_PAYLOAD) ? 1 : -1];"]
    return "\n".join(lines + ["", "#endif", ""])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    # Git/Windows CRLF 차이는 source 의미나 digest를 바꾸지 않는다.
    generated = render(SOURCE.read_bytes().replace(b"\r\n", b"\n"))
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != generated:
            print("FAIL: transport generated output drift")
            return 1
        print("PASS: transport foundation generated output")
    else:
        OUTPUT.parent.mkdir(parents=True, exist_ok=True)
        OUTPUT.write_text(generated, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
