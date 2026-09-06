"""Communicator UART v1.0 schema/generator contract tests."""

from __future__ import annotations

import copy
import hashlib
import importlib.util
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
GENERATOR_PATH = ROOT / "tools" / "generate_uart_protocol.py"
SCHEMA_PATH = ROOT / "protocol" / "schema" / "uart-v1.0.yaml"


def load_generator():
    spec = importlib.util.spec_from_file_location("canview_uart_generator", GENERATOR_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"failed to import {GENERATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class UartSchemaTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.generator = load_generator()
        cls.schema = cls.generator.load_schema(SCHEMA_PATH)

    def test_physical_contract_is_independent_from_espnow(self) -> None:
        protocol = self.schema["protocol"]
        self.assertEqual((protocol["major"], protocol["minor"]), (1, 0))
        self.assertEqual(protocol["header_size"], 32)
        self.assertEqual(protocol["max_frame"], 1024)
        self.assertEqual(protocol["max_payload"], 992)
        self.assertEqual(protocol["max_encoded"], 1029)
        self.assertEqual(protocol["baud"], 4_000_000)
        self.assertEqual(protocol["crc"], "CRC-32/ISO-HDLC")
        self.generator.validate_schema(self.schema)

    def test_catalog_ids_and_payload_bounds_are_exact(self) -> None:
        expected = self.generator.EXPECTED_MESSAGES
        messages = {message["name"]: message for message in self.schema["messages"]}
        self.assertEqual(set(messages), set(expected))
        for name, message_id in expected.items():
            self.assertEqual(messages[name]["id"], message_id)
            minimum, maximum, _, _ = self.generator._payload_bounds(messages[name]["payload"], name)
            self.assertLessEqual(minimum, maximum)
            self.assertLessEqual(maximum, self.schema["protocol"]["max_payload"])
            if messages[name]["payload"]["kind"] == "fixed":
                self.assertEqual(minimum, maximum)

    def test_generation_digest_and_output_are_stable(self) -> None:
        source = self.generator.canonical_schema_bytes(SCHEMA_PATH)
        digest = hashlib.sha256(source).hexdigest()
        generated = (ROOT / "protocol" / "canview_uart_protocol.h").read_text(encoding="utf-8")
        self.assertIn(f'CANVIEW_UART_PROTOCOL_SCHEMA_SHA256 "{digest}"', generated)
        self.assertEqual(
            generated,
            self.generator.render(self.schema, digest),
        )

    def test_capture_marker_and_observer_operations_cannot_alias(self) -> None:
        self.assertEqual(self.schema["enums"]["capture_action"], {
            "ARM": 1,
            "START": 2,
            "STOP": 3,
            "CANCEL": 4,
        })
        self.assertNotIn("MARK", self.schema["enums"]["capture_action"])
        self.assertEqual(self.schema["enums"]["plan_operation"], {
            "BEGIN": 1,
            "CHUNK": 2,
            "COMMIT": 3,
            "ABORT": 4,
        })
        self.assertNotEqual(
            next(item["id"] for item in self.schema["messages"] if item["name"] == "CAN_EVENT_MARKER"),
            next(item["id"] for item in self.schema["messages"] if item["name"] == "CAN_CAPTURE_CONTROL"),
        )

    def test_mutations_are_rejected(self) -> None:
        moved = copy.deepcopy(self.schema)
        moved["messages"][0]["id"] = 0x7F
        with self.assertRaises(self.generator.SchemaError):
            self.generator.validate_schema(moved)

        mark = copy.deepcopy(self.schema)
        mark["enums"]["capture_action"]["MARK"] = 5
        with self.assertRaises(self.generator.SchemaError):
            self.generator.validate_schema(mark)

        gap = copy.deepcopy(self.schema)
        gap["messages"][0]["payload"]["fields"][1]["offset"] = 10
        with self.assertRaises(self.generator.SchemaError):
            self.generator.validate_schema(gap)

        flags = copy.deepcopy(self.schema)
        flags["messages"][0]["allowed_flags"] = 0x80
        with self.assertRaises(self.generator.SchemaError):
            self.generator.validate_schema(flags)


if __name__ == "__main__":
    unittest.main()
