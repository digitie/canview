#!/usr/bin/env python3
"""High-coverage contract tests for the ESP-NOW v1.3 schema and codec oracle."""

from __future__ import annotations

import json
import copy
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

    def _context_for(self, message_name: str) -> dict[str, object]:
        vector = next(vector for vector in self.schema["golden_vectors"] if vector["message"] == message_name)
        return generator.default_decode_context(
            self.schema,
            self.messages[message_name],
            vector.get("header", {}),
        )

    def _zero_payload(self, message_name: str) -> dict[str, object]:
        message = self.messages[message_name]
        payload = message["payload"]
        if payload["kind"] == "fixed":
            fields = payload["fields"]
        elif payload["kind"] in {"suffix", "tlv", "bounded"}:
            fields = payload["prefix"]["fields"]
        else:
            fields = payload["variants"][0]["fields"]
        values: dict[str, object] = {}
        for field in fields:
            values[field["name"]] = "00" * int(field["length"]) if field["type"] == "bytes" else 0
        if payload["kind"] == "bounded":
            values["records"] = []
        if payload["kind"] == "suffix":
            values[payload["suffix_name"]] = ""
        if payload["kind"] == "tlv":
            values["tlvs"] = []
        return values

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

    def test_navigation_companion_is_version_gated(self) -> None:
        companion = self.schema["companion_schemas"][0]
        navigation = json.loads((ROOT / companion["path"]).read_text(encoding="utf-8"))
        self.assertEqual(companion["minimum_version"], [1, 4])
        self.assertEqual(navigation["esp_now_min_version"], [1, 4])
        self.assertEqual(set(companion["message_ids"]), {
            message["id"] for message in navigation["messages"].values() if message["transport"] == "esp_now"
        })
        self.assertTrue(set(companion["message_ids"]).isdisjoint({message["id"] for message in self.messages.values()}))
        self.assertNotIn("SENSOR_CAPABILITIES", self.messages)
        self.assertNotIn("sensor.nav.v1", json.dumps(self.schema["messages"]))

    def test_message_contract_metadata_is_complete(self) -> None:
        contracts = self.schema["message_contracts"]
        self.assertEqual(set(contracts), set(self.messages))
        for name, contract in contracts.items():
            with self.subTest(message=name):
                self.assertEqual(contract["since"], "1.3")
                self.assertTrue(contract["idempotency_key"])
                fields = generator._message_field_names(self.messages[name])
                self.assertTrue(set(contract["idempotency_key"]).issubset(fields | {"canonical_argument_digest"}))
                self.assertIn(contract["sensitive_log_policy"], {"allow", "redact"})
                response = contract["response"]
                self.assertTrue(response is None or response in self.messages)

    def test_frame_policy_context_and_session_binding(self) -> None:
        def recalculate_crc(frame: bytearray) -> bytes:
            frame[28:32] = b"\x00\x00\x00\x00"
            frame[28:32] = (zlib.crc32(frame) & 0xFFFFFFFF).to_bytes(4, "little")
            return bytes(frame)

        hello = bytearray((generator.GOLDEN_DIR / "hello.bin").read_bytes())
        hello[7] = 1
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, recalculate_crc(hello), **self._context_for("HELLO"))
        self.assertEqual(context.exception.code, "bad_priority")

        discovery = bytearray((generator.GOLDEN_DIR / "pair-discovery.bin").read_bytes())
        discovery[6] = 0
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, recalculate_crc(discovery), **self._context_for("DISCOVERY"))
        self.assertEqual(context.exception.code, "bad_flags")
        discovery = (generator.GOLDEN_DIR / "pair-discovery.bin").read_bytes()
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, discovery, **{**self._context_for("DISCOVERY"), "sender_role": "PRIMARY_CONTROLLER"})
        self.assertEqual(context.exception.code, "unauthorized_sender")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, discovery, **{**self._context_for("DISCOVERY"), "receiver_role": "COMMUNICATOR"})
        self.assertEqual(context.exception.code, "unauthorized_receiver")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, discovery, **{**self._context_for("DISCOVERY"), "link_state": "ONLINE"})
        self.assertEqual(context.exception.code, "invalid_state")

        command = bytearray((generator.GOLDEN_DIR / "command-retry.bin").read_bytes())
        command[32 + 28:32 + 32] = (0xDEADBEEF).to_bytes(4, "little")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, recalculate_crc(command), **self._context_for("COMMAND_REQUEST"))
        self.assertEqual(context.exception.code, "session_mismatch")

        bulk = bytearray((generator.GOLDEN_DIR / "bulk-fragment.bin").read_bytes())
        bulk[32 + 24:32 + 26] = (int.from_bytes(bulk[32 + 24:32 + 26], "little") + 1).to_bytes(2, "little")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, recalculate_crc(bulk), **self._context_for("BULK_FRAGMENT"))
        self.assertEqual(context.exception.code, "bad_length")

        hello_frame = (generator.GOLDEN_DIR / "hello.bin").read_bytes()
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, hello_frame)
        self.assertEqual(context.exception.code, "unauthenticated")

        zero_session = bytearray(hello_frame)
        zero_session[8:12] = b"\x00\x00\x00\x00"
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(
                self.schema,
                recalculate_crc(zero_session),
                **{**self._context_for("HELLO"), "expected_session_id": 0},
            )
        self.assertEqual(context.exception.code, "session_required")

        response_on_command = bytearray((generator.GOLDEN_DIR / "command-retry.bin").read_bytes())
        response_on_command[6] |= 0x02
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, recalculate_crc(response_on_command), **self._context_for("COMMAND_REQUEST"))
        self.assertEqual(context.exception.code, "bad_flags")

        response_without_response_flag = bytearray((generator.GOLDEN_DIR / "capture-status.bin").read_bytes())
        response_without_response_flag[6] &= ~0x02
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(
                self.schema,
                recalculate_crc(response_without_response_flag),
                **self._context_for("CAN_CAPTURE_STATUS"),
            )
        self.assertEqual(context.exception.code, "bad_flags")

        last_without_fragment = bytearray(hello_frame)
        last_without_fragment[6] |= 0x10
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, recalculate_crc(last_without_fragment), **self._context_for("HELLO"))
        self.assertEqual(context.exception.code, "bad_flags")

        read_only_command = bytearray((generator.GOLDEN_DIR / "command-retry.bin").read_bytes())
        read_only_command[6] |= 0x40
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, recalculate_crc(read_only_command), **self._context_for("COMMAND_REQUEST"))
        self.assertEqual(context.exception.code, "bad_flags")

        capability = bytearray((generator.GOLDEN_DIR / "capabilities.bin").read_bytes())
        capability[32 + 16] = 2
        capability[32 + 24:32 + 26] = (1).to_bytes(2, "little")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, recalculate_crc(capability), **self._context_for("CAPABILITIES"))
        self.assertEqual(context.exception.code, "unauthorized_scope")

        role_mismatch = bytearray((generator.GOLDEN_DIR / "capabilities.bin").read_bytes())
        role_mismatch[6] |= 0x40
        role_mismatch[32 + 16] = 1
        role_mismatch[32 + 24:32 + 26] = (1).to_bytes(2, "little")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(
                self.schema,
                recalculate_crc(role_mismatch),
                **{**self._context_for("CAPABILITIES"), "sender_role": "READ_ONLY_CONTROLLER"},
            )
        self.assertEqual(context.exception.code, "role_mismatch")

        pair_confirm = self.messages["PAIR_CONFIRM"]
        with self.assertRaises(ValueError):
            generator.encode_frame(self.schema, pair_confirm, {}, self._zero_payload("PAIR_CONFIRM"))
        pair_frame = generator.encode_frame(
            self.schema,
            pair_confirm,
            {},
            self._zero_payload("PAIR_CONFIRM"),
            sender_role="PRIMARY_CONTROLLER",
            receiver_role="COMMUNICATOR",
            link_state="PAIRING",
            expected_session_id=0,
            authenticated=True,
        )
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, pair_frame)
        self.assertEqual(context.exception.code, "unauthenticated")
        generator.decode_frame(
            self.schema,
            pair_frame,
            **generator.default_decode_context(self.schema, pair_confirm, {}),
        )

        for message_name, field_name, invalid_value in (
            ("CONTROL_LEASE_REQUEST", "requested_scope", 0x8000),
            ("CONTROL_LEASE_STATUS", "granted_scope", 0x8000),
            ("COMMAND_RESULT", "command_id", 0xFFFF),
            ("CONFIG_GET", "key", 0xFFFF),
            ("COMMAND_REQUEST", "precondition_flags", 0x100),
        ):
            with self.subTest(invalid_field=f"{message_name}.{field_name}"):
                values = self._zero_payload(message_name)
                values[field_name] = invalid_value
                with self.assertRaises(ValueError):
                    generator.encode_payload(self.messages[message_name], values)

        invalid_command = bytearray((generator.GOLDEN_DIR / "command-retry.bin").read_bytes())
        invalid_command[32 + 8:32 + 10] = (0xFFFF).to_bytes(2, "little")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, recalculate_crc(invalid_command), **self._context_for("COMMAND_REQUEST"))
        self.assertEqual(context.exception.code, "bad_enum")

        malformed_command = bytearray(command[:32 + 72])
        malformed_command.extend(b"\xff")
        malformed_command[24:26] = (73).to_bytes(2, "little")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, recalculate_crc(malformed_command), **self._context_for("COMMAND_REQUEST"))
        self.assertEqual(context.exception.code, "bad_length")

        final_fragment_without_last = bytearray((generator.GOLDEN_DIR / "bulk-fragment.bin").read_bytes())
        final_fragment_without_last[32 + 16:32 + 20] = (3).to_bytes(4, "little")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(
                self.schema,
                recalculate_crc(final_fragment_without_last),
                **self._context_for("BULK_FRAGMENT"),
            )
        self.assertEqual(context.exception.code, "bad_fragment")

        non_final_fragment_with_last = bytearray((generator.GOLDEN_DIR / "bulk-fragment.bin").read_bytes())
        non_final_fragment_with_last[6] |= 0x10
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(
                self.schema,
                recalculate_crc(non_final_fragment_with_last),
                **self._context_for("BULK_FRAGMENT"),
            )
        self.assertEqual(context.exception.code, "bad_fragment")

        bulk_begin = next(vector for vector in self.schema["golden_vectors"] if vector["message"] == "BULK_BEGIN")
        with self.assertRaises(ValueError):
            generator.encode_frame(
                self.schema,
                self.messages["BULK_BEGIN"],
                {**bulk_begin["header"], "flags": 0},
                bulk_begin["payload"],
            )
        read_only_bulk = generator.encode_frame(
            self.schema,
            self.messages["BULK_BEGIN"],
            {**bulk_begin["header"], "flags": 0x41},
            bulk_begin["payload"],
            sender_role="READ_ONLY_CONTROLLER",
            receiver_role="COMMUNICATOR",
            link_state="ONLINE",
            expected_session_id=bulk_begin["header"]["session_id"],
            authenticated=True,
        )
        generator.decode_frame(
            self.schema,
            read_only_bulk,
            **{**self._context_for("BULK_BEGIN"), "sender_role": "READ_ONLY_CONTROLLER"},
        )

    def test_variant_authorization_semantics_and_owner_masks_are_enforced(self) -> None:
        diagnostic = self.messages["DIAGNOSTIC_LEASE"]
        request_values = {
            "variant": "request",
            "request_token": 1,
            "action": 1,
            "requested_ms": 1000,
            "lease_id": 2,
        }
        response_values = {
            "variant": "response",
            "request_token": 1,
            "lease_id": 2,
            "status": 0,
            "granted_ms": 1000,
            "expires_at_ms": 2000,
            "owner_device_id": 3,
        }
        with self.assertRaises(ValueError):
            generator.encode_payload(diagnostic, {**request_values, "action": 0xFF})
        with self.assertRaises(ValueError):
            generator.encode_payload(diagnostic, {**response_values, "status": 0xFF})
        request_header = {"flags": 0x41, "priority": 1, "session_id": 1, "sequence": 1, "sender_time_ms": 1}
        request_frame = generator.encode_frame(
            self.schema,
            diagnostic,
            request_header,
            request_values,
            sender_role="DIAGNOSTIC_BRIDGE",
            receiver_role="COMMUNICATOR",
            link_state="ONLINE",
            expected_session_id=1,
            authenticated=True,
        )
        generator.decode_frame(
            self.schema,
            request_frame,
            sender_role="DIAGNOSTIC_BRIDGE",
            receiver_role="COMMUNICATOR",
            link_state="ONLINE",
            authenticated=True,
            expected_session_id=1,
        )
        response_header = {
            "flags": 0x03,
            "priority": 1,
            "session_id": 1,
            "sequence": 2,
            "sender_time_ms": 2,
            "correlation_id": 1,
        }
        response_frame = generator.encode_frame(
            self.schema,
            diagnostic,
            response_header,
            response_values,
            sender_role="COMMUNICATOR",
            receiver_role="DIAGNOSTIC_BRIDGE",
            link_state="ONLINE",
            expected_session_id=1,
            authenticated=True,
        )
        generator.decode_frame(
            self.schema,
            response_frame,
            sender_role="COMMUNICATOR",
            receiver_role="DIAGNOSTIC_BRIDGE",
            link_state="ONLINE",
            authenticated=True,
            expected_session_id=1,
        )
        with self.assertRaises(ValueError):
            generator.encode_frame(
                self.schema,
                diagnostic,
                {**response_header, "flags": 1},
                response_values,
                sender_role="COMMUNICATOR",
                receiver_role="DIAGNOSTIC_BRIDGE",
                link_state="ONLINE",
                expected_session_id=1,
                authenticated=True,
            )
        with self.assertRaises(ValueError):
            generator.encode_frame(
                self.schema,
                diagnostic,
                {**response_header, "correlation_id": 0},
                response_values,
                sender_role="COMMUNICATOR",
                receiver_role="DIAGNOSTIC_BRIDGE",
                link_state="ONLINE",
                expected_session_id=1,
                authenticated=True,
            )

        filter_record = {
            "filter_id": 1,
            "can_id": 0,
            "can_id_mask": 0,
            "period_ms": 20,
            "bus_id": 0,
            "flags_value": 0x80,
            "flags_mask": 0,
            "min_dlc": 0,
            "max_dlc": 8,
            "max_records_per_period": 1,
            "enabled": 1,
            "reserved0": 0,
        }
        with self.assertRaises(ValueError):
            generator.encode_payload(
                self.messages["CAN_FILTER_SET"],
                {"action": 1, "config_revision": 1, "records": [filter_record]},
            )
        observer_values = self._zero_payload("CAN_OBSERVER_CONFIG")
        observer_values["bus_mask"] = 0x08
        with self.assertRaises(ValueError):
            generator.encode_payload(self.messages["CAN_OBSERVER_CONFIG"], observer_values)
        observer_values["bus_mask"] = 0
        observer_values["flags"] = 1
        with self.assertRaises(ValueError):
            generator.encode_payload(self.messages["CAN_OBSERVER_CONFIG"], observer_values)

        with self.assertRaises(ValueError):
            generator.encode_payload(
                self.messages["PAIR_CHALLENGE"],
                {**self._zero_payload("PAIR_CHALLENGE"), "allowed_message_classes": 0xFFFFFFFF},
            )

        for message_name, field_name, invalid_value in (
            ("COMMAND_REQUEST", "ttl_ms", 0),
            ("COMMAND_REQUEST", "ttl_ms", 30001),
            ("BULK_BEGIN", "total_size", 0),
            ("BULK_BEGIN", "total_size", 65537),
            ("BULK_BEGIN", "fragment_size", 0),
            ("BULK_BEGIN", "fragment_size", 193),
            ("BULK_BEGIN", "window_size", 0),
            ("BULK_BEGIN", "window_size", 5),
            ("BULK_FRAGMENT", "total_fragments", 0),
            ("BULK_FRAGMENT", "total_fragments", 65537),
            ("BULK_FRAGMENT", "payload_len", 0),
            ("BULK_FRAGMENT", "payload_len", 181),
        ):
            with self.subTest(range_field=f"{message_name}.{field_name}={invalid_value}"):
                values = self._zero_payload(message_name)
                values[field_name] = invalid_value
                if message_name == "BULK_FRAGMENT":
                    values["fragment_index"] = 0
                    if field_name == "total_fragments":
                        values["payload_len"] = 1
                        values["fragment_bytes"] = "00"
                    else:
                        values["total_fragments"] = 1
                        values["fragment_bytes"] = "00" * max(0, min(invalid_value, 180))
                with self.assertRaises(ValueError):
                    generator.encode_payload(self.messages[message_name], values)

        with self.assertRaises(ValueError):
            generator.encode_frame(
                self.schema,
                self.messages["CONTROL_LEASE_REQUEST"],
                {"flags": 1, "priority": 1, "session_id": 1, "sequence": 1, "sender_time_ms": 1},
                self._zero_payload("CONTROL_LEASE_REQUEST"),
                sender_role="COMMUNICATOR",
            )

        config_set = next(vector for vector in self.schema["golden_vectors"] if vector["message"] == "CONFIG_SET")
        rtc_record = {"key": 769, "value_type": 4, "value_bits": 1}
        with self.assertRaises(ValueError):
            generator.encode_frame(
                self.schema,
                self.messages["CONFIG_SET"],
                config_set["header"],
                {**config_set["payload"], "records": [rtc_record]},
                sender_role="PRIMARY_CONTROLLER",
                receiver_role="COMMUNICATOR",
                link_state="ONLINE",
                expected_session_id=config_set["header"]["session_id"],
                authenticated=True,
            )
        remote_config = next(vector for vector in self.schema["golden_vectors"] if vector["message"] == "REMOTE_CONFIG_REQUEST")
        with self.assertRaises(ValueError):
            generator.encode_frame(
                self.schema,
                self.messages["REMOTE_CONFIG_REQUEST"],
                remote_config["header"],
                {**remote_config["payload"], "records": [rtc_record]},
                sender_role="DIAGNOSTIC_BRIDGE",
                receiver_role="COMMUNICATOR",
                link_state="ONLINE",
                expected_session_id=remote_config["header"]["session_id"],
                authenticated=True,
            )
        generator.encode_frame(
            self.schema,
            self.messages["REMOTE_CONFIG_REQUEST"],
            remote_config["header"],
            {**remote_config["payload"], "records": [rtc_record]},
            sender_role="DIAGNOSTIC_BRIDGE",
            receiver_role="PRIMARY_CONTROLLER",
            link_state="ONLINE",
            expected_session_id=remote_config["header"]["session_id"],
            authenticated=True,
        )

    def test_singleton_tlv_and_pairing_canonical_contracts_are_enforced(self) -> None:
        capabilities = self.messages["CAPABILITIES"]
        payload = bytearray((generator.GOLDEN_DIR / "capabilities.bin").read_bytes()[32:])
        payload.extend(bytes.fromhex("01000800" + "0000000000000000"))
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_payload(self.schema, capabilities, bytes(payload))
        self.assertEqual(context.exception.code, "duplicate_tlv")

        phases = {phase["message"]: phase for phase in self.schema["pairing_phases"]}
        rendered = json.loads(generator.PAIRING_VECTOR_PATH.read_text(encoding="utf-8"))
        for message_name, phase in phases.items():
            with self.subTest(phase=message_name):
                self.assertTrue(set(phase["canonical_fields"]).issubset(generator._message_field_names(self.messages[message_name])))
                self.assertEqual(
                    phase["domain"],
                    next(item["domain"] for item in rendered["vectors"] if item["phase"] == message_name),
                )
                vector = next(item for item in rendered["vectors"] if item["phase"] == message_name)
                self.assertEqual(vector["phase_binding"]["message"], message_name)
                self.assertEqual(len(vector["canonical_contract_sha256"]), 64)

    def test_schema_validator_rejects_semantic_drift(self) -> None:
        invalid_cases = []

        changed = copy.deepcopy(self.schema)
        changed["encoding"]["crc"]["check_123456789"] += 1
        invalid_cases.append(("crc metadata", changed))

        changed = copy.deepcopy(self.schema)
        changed["header"]["crc_field"] = "reserved"
        invalid_cases.append(("crc field binding", changed))

        changed = copy.deepcopy(self.schema)
        changed["message_contracts"]["HELLO"]["idempotency_key"] = ["not_a_wire_field"]
        invalid_cases.append(("idempotency field", changed))

        changed = copy.deepcopy(self.schema)
        changed["pairing_phases"][0]["canonical_fields"].append("not_a_wire_field")
        invalid_cases.append(("pairing canonical field", changed))

        changed = copy.deepcopy(self.schema)
        changed["pairing_phases"][0]["canonical_fields"].reverse()
        invalid_cases.append(("pairing canonical order", changed))

        changed = copy.deepcopy(self.schema)
        changed["pairing_phases"].reverse()
        invalid_cases.append(("pairing phase order", changed))

        changed = copy.deepcopy(self.schema)
        changed["pairing_phases"][0]["domain"] = "CV-OTHER-1"
        invalid_cases.append(("pairing domain", changed))

        changed = copy.deepcopy(self.schema)
        changed["pairing_negative_vectors"][0]["phase"] = "PAIR_RESULT"
        invalid_cases.append(("pairing negative phase", changed))

        changed = copy.deepcopy(self.schema)
        changed["messages"][0]["senders"] = ["NOT_A_ROLE"]
        invalid_cases.append(("message sender role", changed))

        changed = copy.deepcopy(self.schema)
        changed["command_argument_tlv"]["header_size"] = 8
        invalid_cases.append(("command argument policy", changed))

        changed = copy.deepcopy(self.schema)
        changed["frame_policy"]["response_messages"].append("COMMAND_REQUEST")
        invalid_cases.append(("message flag policy", changed))

        changed = copy.deepcopy(self.schema)
        changed["golden_vectors"][1]["name"] = changed["golden_vectors"][0]["name"]
        invalid_cases.append(("duplicate golden vector", changed))

        changed = copy.deepcopy(self.schema)
        changed["frame_policy"]["response_correlation_required"] = False
        invalid_cases.append(("response correlation policy", changed))

        changed = copy.deepcopy(self.schema)
        changed["enums"][8]["value_kind"] = "bitmask"
        invalid_cases.append(("invalid bitmask enum", changed))

        changed = copy.deepcopy(self.schema)
        changed["enums"].append(copy.deepcopy(changed["enums"][0]))
        invalid_cases.append(("duplicate logical enum name", changed))

        changed = copy.deepcopy(self.schema)
        changed["config_policy"]["owner_roles"]["CAN_RX_BYTES_PER_SECOND"] = "PRIMARY_CONTROLLER"
        invalid_cases.append(("config owner binding", changed))

        changed = copy.deepcopy(self.schema)
        changed["enums"][0]["values"]["PRIMARY_CONTROLLER"] = 0
        invalid_cases.append(("enum numeric alias", changed))

        for name, invalid_schema in invalid_cases:
            with self.subTest(case=name):
                with self.assertRaises(generator.SchemaError):
                    generator.validate_schema(invalid_schema)

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

    def test_bulk_capture_and_config_contracts_have_exact_bounds(self) -> None:
        config = self.schema["config_policy"]
        self.assertEqual(config["schema_max_bytes"], 16384)
        self.assertEqual(config["max_records"], 25)
        config_keys = next(enum["values"] for enum in self.schema["enums"] if enum["name"] == "config_key")
        self.assertEqual(set(config["owner_roles"]), set(config_keys))
        self.assertEqual(config["authoritative_store"], "OTA_AB")
        self.assertEqual(config["cache_store"], "NVS_CACHE")

        expected_sizes = {
            "BULK_BEGIN": 72,
            "BULK_FRAGMENT": 28,
            "BULK_ACK": 32,
            "BULK_END": 60,
            "CAN_CAPTURE_STATUS": 44,
        }
        for name, expected_size in expected_sizes.items():
            with self.subTest(message=name):
                low, high = generator.payload_size_bounds(self.messages[name])
                self.assertEqual(low, expected_size if name != "BULK_FRAGMENT" else 28)
                self.assertLessEqual(high, 208)

        capture_fields = {field["name"]: field for field in self.messages["CAN_CAPTURE_STATUS"]["payload"]["fields"]}
        self.assertEqual(capture_fields["reason"]["type"], "u16")
        self.assertEqual(capture_fields["reason"]["offset"], 17)
        self.assertEqual(capture_fields["reserved1"]["offset"], 40)
        self.assertEqual(capture_fields["reserved1"]["type"], "u32")
        command_fields = {field["name"]: field for field in self.messages["COMMAND_REQUEST"]["payload"]["prefix"]["fields"]}
        self.assertEqual(command_fields["wireless_session_id"]["type"], "u32")
        self.assertEqual(command_fields["wireless_session_id"]["offset"], 28)

        golden_names = {vector["name"] for vector in self.schema["golden_vectors"]}
        self.assertTrue({
            "bulk-begin", "bulk-fragment", "bulk-ack", "bulk-end",
            "config-get", "config-set", "config-result", "config-schema-request",
            "remote-config-request", "remote-config-status",
        }.issubset(golden_names))

    def test_pairing_phase_order_and_negative_vectors_are_complete(self) -> None:
        phases = {phase["message"]: phase for phase in self.schema["pairing_phases"]}
        negatives = self.schema["pairing_negative_vectors"]
        rendered = json.loads(generator.PAIRING_VECTOR_PATH.read_text(encoding="utf-8"))
        self.assertEqual(rendered["schema_version"], self.schema["schema_version"])
        self.assertEqual({item["name"] for item in rendered["vectors"]}, {item["name"] for item in negatives})
        self.assertEqual({vector["phase"] for vector in negatives}, set(phases))
        for phase_name, phase in phases.items():
            with self.subTest(phase=phase_name):
                self.assertTrue(phase["domain"].startswith("CV-"))
                self.assertGreaterEqual(len(phase["canonical_fields"]), 3)
                self.assertTrue(any(vector["phase"] == phase_name for vector in negatives))
                self.assertTrue(any(item["domain"] == phase["domain"] for item in rendered["vectors"]))
        self.assertIn("peer_nonce", phases["DISCOVERY"]["must_not_bind"])
        self.assertIn("selected_version", phases["DISCOVERY"]["must_not_bind"])
        self.assertIn("requested_role", phases["PAIR_REQUEST"]["must_not_authorize"])

    def test_tlv_extension_policy(self) -> None:
        tlv = self.schema["tlv"]
        self.assertEqual(tlv["header_size"], 4)
        self.assertEqual(tlv["critical_bit"], 0x8000)
        self.assertEqual(tlv["max_nesting_depth"], 0)
        values = {item["value"] for item in tlv["types"]}
        self.assertEqual(len(values), len(tlv["types"]))
        self.assertIn(0x8001, values)
        command_tlv = self.schema["command_argument_tlv"]
        self.assertEqual(command_tlv["header_size"], 4)
        self.assertEqual(command_tlv["max_nesting_depth"], 0)
        self.assertEqual(command_tlv["types"][0]["size"], -1)

    def test_generated_header_is_reproducible(self) -> None:
        digest = __import__("hashlib").sha256(generator._canonical_schema_bytes()).hexdigest()
        expected = generator.render_header(self.schema, digest)
        self.assertEqual(generator.HEADER_PATH.read_text(encoding="utf-8"), expected)
        self.assertIn("GENERATED FILE - DO NOT EDIT", expected)
        self.assertIn("CANVIEW_MSG_CAN_EVENT_MARKER", expected)
        self.assertIn("CANVIEW_ROLE_KNOWN_MASK", expected)
        self.assertIn("CANVIEW_LINK_KNOWN_MASK", expected)
        self.assertIn("CANVIEW_BUS_FLAG_KNOWN_MASK UINT32_C(0x0000000F)", expected)
        self.assertIn("CANVIEW_PRECOND_KNOWN_MASK UINT32_C(0x000000FF)", expected)
        self.assertIn("CANVIEW_MSG_CLASS_KNOWN_MASK UINT32_C(0x0000000F)", expected)
        self.assertIn("CANVIEW_CONTROL_SCOPE_AUDIO_PROFILE", expected)
        self.assertIn("CANVIEW_SCOPE_KNOWN_MASK UINT32_C(0x000007FF)", expected)

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
                decoded = generator.decode_frame(
                    self.schema,
                    frame,
                    **generator.default_decode_context(self.schema, self.messages[vector["message"]], vector.get("header", {})),
                )
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
                    base = next(item for item in self.schema["golden_vectors"] if item["name"] == vector["base"])
                    generator.decode_frame(
                        self.schema,
                        raw,
                        **generator.default_decode_context(self.schema, self.messages[base["message"]], base.get("header", {})),
                    )
                self.assertEqual(context.exception.code, vector["expected"])

    def test_compatibility_vectors_cover_major_minor_and_tlv_rules(self) -> None:
        for vector in self.schema["compatibility_vectors"]:
            with self.subTest(vector=vector["name"]):
                raw = (generator.COMPATIBILITY_DIR / f"{vector['name']}.bin").read_bytes()
                if vector["expected"] == "accepted":
                    base = next(item for item in self.schema["golden_vectors"] if item["name"] == vector["base"])
                    decoded = generator.decode_frame(
                        self.schema,
                        raw,
                        **generator.default_decode_context(self.schema, self.messages[base["message"]], base.get("header", {})),
                    )
                    self.assertIn(decoded["message"], {"HELLO", "CAPABILITIES"})
                else:
                    with self.assertRaises(generator.ProtocolDecodeError) as context:
                        base = next(item for item in self.schema["golden_vectors"] if item["name"] == vector["base"])
                        generator.decode_frame(
                            self.schema,
                            raw,
                            **generator.default_decode_context(self.schema, self.messages[base["message"]], base.get("header", {})),
                        )
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
            "argument_tlv_length": 6,
            "argument_tlv": "01000200aabb",
            "control_tag": "00" * 16,
        }
        encoded = generator.encode_payload(command, common)
        self.assertEqual(len(encoded), 78)
        with self.assertRaises(ValueError):
            generator.encode_payload(command, {**common, "argument_tlv_length": 1})
        with self.assertRaises(ValueError):
            generator.encode_payload(
                command,
                {"prefix": {**common, "argument_tlv_length": 1}, "argument_tlv": common["argument_tlv"]},
            )

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
        known_base_critical = bytearray(tlv_payload)
        known_base_critical.extend(bytes.fromhex("028002000000"))
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_payload(self.schema, capabilities, bytes(known_base_critical))
        self.assertEqual(context.exception.code, "unsupported_tlv")

    def test_payload_reserved_bytes_are_rejected_after_crc_recalculation(self) -> None:
        vector = next(item for item in self.schema["golden_vectors"] if item["name"] == "hello")
        message = self.messages["HELLO"]
        raw = bytearray(
            generator.encode_frame(
                self.schema,
                message,
                vector["header"],
                vector["payload"],
                **self._context_for("HELLO"),
            )
        )
        raw[32 + 35] = 1
        raw[28:32] = b"\x00\x00\x00\x00"
        raw[28:32] = (zlib.crc32(raw) & 0xFFFFFFFF).to_bytes(4, "little")
        with self.assertRaises(generator.ProtocolDecodeError) as context:
            generator.decode_frame(self.schema, bytes(raw), **self._context_for("HELLO"))
        self.assertEqual(context.exception.code, "bad_reserved")


if __name__ == "__main__":
    unittest.main(verbosity=2)
