"""Schema 매핑·JSON 경계·서명 전 CBOR CLI와 기존 C typed parser의 교차 시험."""
from __future__ import annotations
import copy
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/ota"))
from cbor import CborError, MAX_BYTES, MAX_DEPTH, MAX_ITEMS, encode_document
from envelope import HEADER, MAGIC, FORMAT_VERSION, IMAGE_ALIGNMENT, SIGNATURE_BYTES
from manifest import IMAGE_LIMITS, ROLE_TARGETS
from manifest_json import SCHEMA, JSON_BYTES_MAX, load_manifest_json
from test_manifest import fixture, make_prefix

PROBE = sys.argv.pop(1) if len(sys.argv) > 1 and not sys.argv[1].startswith("-") else None
ROOT_NAMES = ["format_version", "package_id", "role", "board_revision", "layout_id", "release", "security_epoch", "key_id", "images", "compatibility", "config_schema", "requires"]
IMAGE_NAMES = ["target", "length", "sha256", "version", "release_sequence", "signature", "abi"]
COMPAT_NAMES = ["esp", "stm", "peer", "allowed_pairs"]


def named(role=1):
    wire = fixture(role)
    result = dict(zip(ROOT_NAMES, (wire[index] for index in range(12))))
    result["package_id"] = wire[1].hex()
    result["images"] = [dict(zip(IMAGE_NAMES, (image[index] for index in range(7)))) for image in wire[8]]
    for image in result["images"]:
        image["sha256"] = image["sha256"].hex()
    result["compatibility"] = dict(zip(COMPAT_NAMES, (wire[9][index] for index in range(4))))
    return result


def raw(value):
    return json.dumps(value, separators=(",", ":")).encode()


class ManifestJsonTests(unittest.TestCase):
    def test_schema_wire_and_code_drift(self):
        for schema, names in ((SCHEMA, ROOT_NAMES), (SCHEMA["$defs"]["image"], IMAGE_NAMES),
                              (SCHEMA["$defs"]["compatibility"], COMPAT_NAMES)):
            self.assertFalse(schema["additionalProperties"])
            self.assertEqual(set(schema["required"]), set(names))
            self.assertEqual({name: field["x-cbor-key"] for name, field in schema["properties"].items()}, dict(zip(names, range(len(names)))))
        wire = json.loads((ROOT / "protocol/schema/ota-container-v2.yaml").read_text(encoding="utf-8"))
        self.assertEqual(wire["header"]["size"], HEADER.size)
        self.assertEqual(wire["header"]["fields"], [["magic",0,8,MAGIC.decode()], ["version",8,2,FORMAT_VERSION], ["header_size",10,2,HEADER.size], ["manifest_size",12,4,None], ["signature_size",16,2,SIGNATURE_BYTES], ["image_count",18,2,None], ["total_size",20,4,None]])
        self.assertEqual(wire["limits"]["image_alignment"], IMAGE_ALIGNMENT)
        self.assertEqual(wire["limits"], {"manifest_bytes": MAX_BYTES, "cbor_depth": MAX_DEPTH,
            "cbor_items": MAX_ITEMS, "header_image_count": 3, "chunk_bytes": 16384,
            "image_alignment": IMAGE_ALIGNMENT, "text_bytes": 64, "abi_pairs": 16})
        native = wire["native_metadata"]
        self.assertEqual((native["magic"], native["version"], native["size"]), ("CVIMG001", 1, 168))
        self.assertEqual(native["fields"], [["magic",0,8],["version",8,2],["size",10,2],["role",12,4],
            ["target",16,4],["security_epoch",20,4],["abi",24,4],["reserved_zero",28,4],
            ["release_sequence",32,8],["board_revision",40,64],["layout_id",104,64]])
        self.assertIn(f'#define CANVIEW_OTA_NATIVE_METADATA_BYTES ({native["size"]}U)',
                      (ROOT / "shared/ota/src/native_metadata.h").read_text(encoding="utf-8"))
        self.assertEqual({int(key): value for key, value in wire["image_max_bytes"].items()}, IMAGE_LIMITS)
        self.assertEqual({int(key): set(value) for key, value in wire["role_targets"].items()}, ROLE_TARGETS)
        header = (ROOT / "shared/ota/src/manifest.h").read_text(encoding="utf-8")
        for group, prefix in (("roles", "ROLE"), ("targets", "TARGET")):
            for name, value in wire[group].items():
                self.assertRegex(header, rf"CANVIEW_OTA_{prefix}_{name}\s*=\s*{value}\b")
        for name, value in wire["image_signatures"].items():
            self.assertRegex(header, rf"CANVIEW_OTA_IMAGE_{name}\s*=\s*{value}\b")
        self.assertEqual(SCHEMA["$defs"]["u64"]["maximum"], (1 << 64) - 1)
        for value in ("abc\n", "abc\x00", "한글"):
            self.assertIsNone(re.search(SCHEMA["$defs"]["text"]["pattern"], value))

    def test_role_u64_c_roundtrip(self):
        self.assertIsNotNone(PROBE, "C typed parser probe required")
        for role in (1, 2, 3):
            vectors = []
            for sequence in (0, 1, (1 << 32) - 1, 1 << 32, (1 << 53) + 1, (1 << 64) - 1):
                source = named(role)
                expected = fixture(role)
                for index, image in enumerate(source["images"]):
                    image["release_sequence"] = sequence
                    expected[8][index][4] = sequence
                converted = load_manifest_json(raw(source))
                self.assertEqual(converted, expected)
                self.assertEqual(encode_document(converted), encode_document(expected))
                # JSON object insertion order must not alter canonical bytes.
                self.assertEqual(load_manifest_json(raw(dict(reversed(list(source.items()))))), expected)
                vectors.append((make_prefix(converted, lambda _: bytes(64)), sequence))
            result = subprocess.run([PROBE, str(role)], input=b"".join(struct.pack("<I", len(prefix)) + prefix for prefix, _ in vectors), capture_output=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            lines = result.stdout.splitlines()
            self.assertEqual(len(lines), len(vectors))
            for line, (_, sequence) in zip(lines, vectors):
                values = list(map(int, line.split()))
                self.assertEqual(values[0], 0)
                self.assertEqual(values[6::4], [sequence] * values[1])

    def test_reject_json_and_cross_fields(self):
        original = raw(named())
        invalid = [b"", b"[]", b"null", b"\xff", original + b"x", b" " * (JSON_BYTES_MAX + 1),
                   original.replace(b'"role":1', b'"role":1,"role":1'),
                   original.replace(b'"role":1', b'"role":true'),
                   original.replace(b'"role":1', b'"role":1.0'),
                   original.replace(b'"role":1', b'"role":NaN'),
                   b"[" * 1500 + b"0" + b"]" * 1500]
        invalid.extend(original[:end] for end in range(len(original)))
        for value in invalid:
            with self.assertRaises(CborError):
                load_manifest_json(value)
        mutations = [("format_version", 1), ("role", 4), ("package_id", "AA" * 16),
                     ("package_id", "00" * 16 + "\n"), ("board_revision", "x\n"),
                     ("board_revision", "x" * 64), ("security_epoch", 1 << 32),
                     ("config_schema", [3, 1, 2]), ("requires", [1, 2, 1 << 64])]
        for key, value in mutations:
            source = named(); source[key] = value
            with self.assertRaises(CborError): load_manifest_json(raw(source))
        for key in ROOT_NAMES:
            source = named(); del source[key]
            with self.assertRaises(CborError): load_manifest_json(raw(source))
        for key, value in (("target", 4), ("length", 0), ("length", 4194305), ("sha256", "00" * 31),
                           ("release_sequence", -1), ("release_sequence", 1 << 64), ("release_sequence", "123"), ("signature", 2), ("abi", 99)):
            source = named(); source["images"][0][key] = value
            with self.assertRaises(CborError): load_manifest_json(raw(source))
        source = named(); source["images"][1] = copy.deepcopy(source["images"][0])
        with self.assertRaises(CborError): load_manifest_json(raw(source))
        for owner in ("root", "image", "compatibility"):
            source = named()
            target = source if owner == "root" else source["images"][0] if owner == "image" else source["compatibility"]
            target["path"] = "forbidden"
            with self.assertRaises(CborError): load_manifest_json(raw(source))

    def test_cli_exclusive_output(self):
        with tempfile.TemporaryDirectory(prefix="canview-manifest-json-") as directory:
            source, output = Path(directory) / "input.json", Path(directory) / "manifest.cbor"
            source.write_bytes(raw(named()))
            command = [sys.executable, "-B", str(ROOT / "tools/ota/manifest_json.py"), str(source), str(output)]
            result = subprocess.run(command, capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn(b"UNSIGNED_MANIFEST", result.stdout)
            expected = encode_document(fixture())
            self.assertEqual(output.read_bytes(), expected)
            self.assertNotEqual(subprocess.run(command, capture_output=True, timeout=10).returncode, 0)
            self.assertEqual(output.read_bytes(), expected)
            source.write_bytes(b'{"private_input_marker":NaN}')
            rejected = Path(directory) / "rejected.cbor"
            result = subprocess.run(command[:-1] + [str(rejected)], capture_output=True, timeout=10)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(rejected.exists())
            self.assertNotIn(b"private_input_marker", result.stderr)


if __name__ == "__main__":
    unittest.main()
