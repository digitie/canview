#!/usr/bin/env python3
"""High-coverage contract tests for the ESP-NOW v1.3 schema and codec oracle."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools import generate_protocol as generator


class EspNowSchemaTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.schema = generator.load_schema()
        generator.validate_schema(cls.schema)
        cls.messages = {message["name"]: message for message in cls.schema["messages"]}

    def test_version_and_transport_limits_are_frozen(self) -> None:
        self.assertEqual(self.schema["version"], {"major": 1, "minor": 3, "wire_name": "ESP-NOW v1.3"})
        self.assertEqual(self.schema["encoding"]["byte_order"], "little")
        self.assertEqual(self.schema["encoding"]["header_size"], 32)
        self.assertEqual(self.schema["encoding"]["max_frame_size"], 240)
        self.assertEqual(self.schema["encoding"]["max_payload_size"], 208)
        self.assertEqual(self.schema["header"]["flags_known_mask"], 0x7F)

    def test_message_catalog_has_required_wire_ids(self) -> None:
        ids = [message["id"] for message in self.schema["messages"]]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertEqual(sorted(ids), sorted(self.schema["required_message_ids"]))
        self.assertEqual(self.messages["CAN_EVENT_MARKER"]["id"], 0x2C)
        self.assertNotIn("CAN_EVENT_MARKER", self.messages["CAN_CAPTURE_CONTROL"]["name"])
        self.assertEqual(self.messages["CAPABILITIES"]["payload"]["kind"], "tlv")

    def test_every_wire_layout_is_contiguous_and_bounded(self) -> None:
        for message in self.schema["messages"]:
            low, high = generator.payload_size_bounds(message)
            self.assertLessEqual(low, high, message["name"])
            self.assertLessEqual(high, 208, message["name"])
            payload = message["payload"]
            if payload["kind"] == "bounded":
                self.assertLessEqual(payload["prefix"]["size"] + payload["record"]["size"] * payload["count_max"], 208)
                self.assertGreaterEqual(payload["count_max"], 0)
            if payload["kind"] in {"suffix", "tlv"}:
                self.assertEqual(payload["prefix_size"], payload["prefix"]["size"])

    def test_reserved_fields_are_explicit(self) -> None:
        for _, _, fields in generator._layout_entries(self.schema):
            for field in fields:
                if field.get("reserved"):
                    self.assertTrue(field["name"].lower().startswith("reserved"))
        self.assertEqual(self.schema["header"]["reserved_zero"], ["reserved"])

    def test_security_pairing_and_control_policy(self) -> None:
        security = self.schema["security"]
        self.assertTrue(security["shared_installation_secret_forbidden"])
        self.assertFalse(security["bridge_control_root"])
        self.assertEqual(security["pair_root_bits"], 256)
        self.assertEqual(security["lmk_bits"], 128)
        self.assertEqual(security["control_root_roles"], ["PRIMARY_CONTROLLER", "COMMUNICATOR"])
        self.assertEqual(security["control_scope_zero_roles"], ["READ_ONLY_CONTROLLER", "DIAGNOSTIC_BRIDGE"])
        self.assertEqual(security["control_scope_authorized_roles"], ["PRIMARY_CONTROLLER", "COMMUNICATOR"])
        self.assertIn("origin_boot_id", self.schema["control_envelope"]["retry_immutable_fields"])
        self.assertIn("wireless_session_id", self.schema["control_envelope"]["idempotency_key"])

    def test_message_contract_metadata_is_complete(self) -> None:
        contracts = self.schema["message_contracts"]
        self.assertEqual(set(contracts), set(self.messages))
        for name, contract in contracts.items():
            with self.subTest(message=name):
                self.assertEqual(contract["since"], "1.3")
                self.assertTrue(contract["idempotency_key"])
                self.assertIn(contract["sensitive_log_policy"], {"allow", "redact"})
                response = contract["response"]
                self.assertTrue(response is None or response in self.messages)

    def test_revision_reason_and_cumulative_counter_widths(self) -> None:
        cumulative = {"error_count", "rx_count", "bus_off_count", "telemetry_dropped", "protocol_error_count"}
        for _, _, fields in generator._layout_entries(self.schema):
            for field in fields:
                if field["name"] == "catalog_revision":
                    self.assertEqual(field["type"], "u32")
                if field["name"] in cumulative:
                    self.assertEqual(field["type"], "u64")
                if field["name"] == "reason":
                    self.assertEqual(field["type"], "u16")

    def test_tlv_extension_policy(self) -> None:
        tlv = self.schema["tlv"]
        self.assertEqual(tlv["header_size"], 4)
        self.assertEqual(tlv["critical_bit"], 0x8000)
        self.assertEqual(tlv["max_nesting_depth"], 0)
        values = {item["value"] for item in tlv["types"]}
        self.assertEqual(len(values), len(tlv["types"]))
        self.assertIn(0x8001, values)

    def test_generated_header_is_reproducible(self) -> None:
        digest = __import__("hashlib").sha256(generator._canonical_schema_bytes()).hexdigest()
        expected = generator.render_header(self.schema, digest)
        self.assertEqual(generator.HEADER_PATH.read_text(encoding="utf-8"), expected)
        self.assertIn("GENERATED FILE - DO NOT EDIT", expected)
        self.assertIn("CANVIEW_MSG_CAN_EVENT_MARKER", expected)

    def test_schema_digest_is_platform_independent(self) -> None:
        canonical = generator._canonical_schema_bytes()
        with tempfile.TemporaryDirectory() as directory:
            crlf_path = Path(directory) / "schema.yaml"
            crlf_path.write_bytes(canonical.replace(b"\n", b"\r\n"))
            self.assertEqual(generator._canonical_schema_bytes(crlf_path), canonical)

    def test_golden_frames_round_trip_with_crc(self) -> None:
        for vector in self.schema["golden_vectors"]:
            with self.subTest(vector=vector["name"]):
                frame_path = generator.GOLDEN_DIR / f"{vector['name']}.bin"
                metadata_path = generator.GOLDEN_DIR / f"{vector['name']}.json"
                frame = frame_path.read_bytes()
                metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
                self.assertEqual(frame.hex(), metadata["frame_hex"])
                self.assertEqual(len(frame), metadata["frame_size"])
                decoded = generator.decode_frame(self.schema, frame)
                self.assertEqual(decoded["message"], vector["message"])
                self.assertEqual(decoded["message_id"], self.messages[vector["message"]]["id"])
                self.assertEqual(decoded["header"]["crc32"], int(metadata["crc32"], 16))
                self.assertEqual(zlib.crc32(frame[:28] + b"\x00\x00\x00\x00" + frame[32:]) & 0xFFFFFFFF,
                                 int(metadata["crc32"], 16))

    def test_golden_payload_wire_details(self) -> None:
        hello = (generator.GOLDEN_DIR / "hello.bin").read_bytes()
        self.assertEqual(hello[:4], bytes.fromhex("43560103"))
        self.assertEqual(hello[8:12], bytes.fromhex("04030201"))
        self.assertEqual(hello[32:40], bytes.fromhex("0807060504030201"))
        command = (generator.GOLDEN_DIR / "command-retry.bin").read_bytes()
        self.assertEqual(len(command), 32 + 80)
        self.assertEqual(command[32 + 52 : 32 + 54], bytes.fromhex("0800"))

    def test_malformed_vectors_reject_with_expected_reason(self) -> None:
        for vector in self.schema["negative_vectors"]:
            with self.subTest(vector=vector["name"]):
                metadata = json.loads((generator.MALFORMED_DIR / f"{vector['name']}.json").read_text(encoding="utf-8"))
                raw = (generator.MALFORMED_DIR / f"{vector['name']}.bin").read_bytes()
                self.assertEqual(raw.hex(), metadata["frame_hex"])
                with self.assertRaises(generator.ProtocolDecodeError) as context:
                    generator.decode_frame(self.schema, raw)
                self.assertEqual(context.exception.code, vector["expected"])

    def test_compatibility_vectors_cover_major_minor_and_tlv_rules(self) -> None:
        for vector in self.schema["compatibility_vectors"]:
            with self.subTest(vector=vector["name"]):
                raw = (generator.COMPATIBILITY_DIR / f"{vector['name']}.bin").read_bytes()
                if vector["expected"] == "accepted":
                    decoded = generator.decode_frame(self.schema, raw)
                    self.assertIn(decoded["message"], {"HELLO", "CAPABILITIES"})
                else:
                    with self.assertRaises(generator.ProtocolDecodeError) as context:
                        generator.decode_frame(self.schema, raw)
                    self.assertEqual(context.exception.code, vector["expected"])

    def test_bounded_payload_limits_and_count_binding(self) -> None:
        message = self.messages["CAN_BATCH"]
        record = {"delta_us": 1, "bus_id": 0, "flags_dlc": 8, "can_id": 0x123, "data": "0001020304050607"}
        values = {"base_time_us": 100, "dropped_since_last": 0, "records": [record] * 12}
        encoded = generator.encode_payload(message, values)
        self.assertEqual(len(encoded), 204)
        decoded = generator.decode_payload(self.schema, message, encoded)
        self.assertEqual(decoded["count"], 12)
        self.assertEqual(len(decoded["records"]), 12)
        with self.assertRaises(ValueError):
            generator.encode_payload(message, {**values, "records": [record] * 13})
        with self.assertRaises(ValueError):
            generator.encode_payload(message, {"base_time_us": 100, "count": 2, "records": [record]})

    def test_suffix_length_and_tlv_unknown_extension_handling(self) -> None:
        command = self.messages["COMMAND_REQUEST"]
        common = {
            "request_token": 1,
            "command_id": 0x0101,
            "ttl_ms": 2000,
            "origin_device_id": 2,
            "origin_boot_id": 3,
            "wireless_session_id": 4,
            "control_generation": 5,
            "issued_at_controller_ms": 6,
            "control_sync_generation": 7,
            "expected_state_revision": 8,
            "precondition_flags": 0,
            "argument_tlv_length": 2,
            "argument_tlv": "aabb",
            "control_tag": "00" * 16,
        }
        encoded = generator.encode_payload(command, common)
        self.assertEqual(len(encoded), 74)
        with self.assertRaises(ValueError):
            generator.encode_payload(command, {**common, "argument_tlv_length": 1})

        capabilities = self.messages["CAPABILITIES"]
        values = {
            "device_id": 1, "boot_id": 2, "role": 0, "selected_major": 1,
            "selected_minor": 3, "max_frame": 240, "bus_count": 1,
            "support_flags": 0, "control_scope": 0, "max_filters": 1,
            "max_peers": 1, "max_batch_records": 1, "profile_id": 1,
            "catalog_revision": 1, "config_schema_version": 1,
            "profile_digest": "00" * 32, "catalog_digest": "11" * 32,
            "build_id_digest": "22" * 16, "tlvs": [{"type": 16, "value": "aabb"}],
        }
        tlv_payload = generator.encode_payload(capabilities, values)
        decoded = generator.decode_payload(self.schema, capabilities, tlv_payload)
        self.assertEqual(decoded["tlvs"][-1]["type"], 16)
        critical = bytearray(tlv_payload)
        critical.extend(bytes.fromhex("10800200aabb"))
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_payload(self.schema, capabilities, bytes(critical))
        self.assertEqual(context.exception.code, "unsupported_tlv")

    def test_payload_reserved_bytes_are_rejected_after_crc_recalculation(self) -> None:
        vector = next(item for item in self.schema["golden_vectors"] if item["name"] == "hello")
        message = self.messages["HELLO"]
        raw = bytearray(generator.encode_frame(self.schema, message, vector["header"], vector["payload"]))
        raw[32 + 35] = 1
        raw[28:32] = b"\x00\x00\x00\x00"
        raw[28:32] = (zlib.crc32(raw) & 0xFFFFFFFF).to_bytes(4, "little")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, bytes(raw))
        self.assertEqual(context.exception.code, "bad_reserved")


if __name__ == "__main__":
    unittest.main(verbosity=2)
