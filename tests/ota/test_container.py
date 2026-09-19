"""실제 manifest P256와 본문 SHA256·CNG C receiver 교차 시험. native/HIL이 아님."""
from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, utils

from test_manifest import fixture
from test_body import LOCAL
from cbor import CborError, Status, encode_document
from container import assemble_container, check_container, main
from envelope import HEADER, IMAGE_ALIGNMENT, MAX_BUNDLE, prefix_length
from manifest_json import SCHEMA

PROBE = sys.argv.pop(1) if len(sys.argv) > 1 and not sys.argv[1].startswith("-") else None


def named(value, schema):
    result = {}
    for name, field in schema["properties"].items():
        item = value[field["x-cbor-key"]]
        result[name] = item.hex() if field.get("x-cbor-hex") else item
    result["images"] = [{name: (image[field["x-cbor-key"]].hex() if field.get("x-cbor-hex")
                               else image[field["x-cbor-key"]])
                         for name, field in schema["$defs"]["image"]["properties"].items()}
                        for image in value[8]]
    result["compatibility"] = {name: value[9][field["x-cbor-key"]]
                               for name, field in schema["$defs"]["compatibility"]["properties"].items()}
    return result


class ContainerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.key = ec.generate_private_key(ec.SECP256R1())
        cls.public = cls.key.public_key()

    def prepare(self, role=1, length=97):
        manifest = fixture(role)
        images = [b"a" * length] + ([b"STM synthetic\x00"] if role == 1 else [])
        for image, data in zip(manifest[8], images):
            image[1], image[2] = len(data), hashlib.sha256(data).digest()
        signature = self.sign(manifest)
        identity = (role, manifest[3], manifest[4], manifest[6], manifest[7])
        data = assemble_container(manifest, images, signature, identity, self.public)
        return manifest, images, signature, identity, data

    def sign(self, manifest):
        der = self.key.sign(encode_document(manifest), ec.ECDSA(hashes.SHA256(), deterministic_signing=True))
        r, s = utils.decode_dss_signature(der)
        return r.to_bytes(32, "big") + s.to_bytes(32, "big")

    def reject(self, data, identity, expected):
        with self.assertRaises(CborError) as raised:
            check_container(data, identity, self.public)
        self.assertEqual(raised.exception.status, expected)

    def test_role_alignment_and_reproducibility(self):
        for role in (1, 2, 3):
            for length in (1, IMAGE_ALIGNMENT - 1, IMAGE_ALIGNMENT, IMAGE_ALIGNMENT + 1):
                with self.subTest(role=role, length=length):
                    manifest, images, signature, identity, data = self.prepare(role, length)
                    self.assertEqual(data, assemble_container(manifest, images, signature, identity, self.public))
                    checked = check_container(data, identity, self.public)
                    self.assertEqual(manifest, checked["manifest"])
                    self.assertEqual(len(data), HEADER.unpack_from(data)[-1])
                    self.assertTrue(all(offset % IMAGE_ALIGNMENT == 0 for offset in checked["offsets"]))
                    self.assertEqual(manifest[8][0][4], (1 << 64) - 1)

    def test_tamper_and_boundaries(self):
        manifest, images, signature, identity, data = self.prepare()
        size = prefix_length(data)
        for end in (0, 1, HEADER.size - 1, size - 1, size, IMAGE_ALIGNMENT - 1, len(data) - 1):
            self.reject(data[:end], identity, Status.INCOMPLETE)
        self.reject(data + b"\0", identity, Status.MALFORMED)
        self.reject(bytes(MAX_BUNDLE + 1), identity, Status.OVERSIZE)
        self.reject(bytearray(data), identity, Status.MALFORMED)
        checked = check_container(data, identity, self.public)
        previous = size
        for image, offset in zip(manifest[8], checked["offsets"]):
            for position in (previous, offset - 1, offset, offset + image[1] - 1):
                changed = bytearray(data)
                changed[position] ^= 1
                self.reject(bytes(changed), identity, Status.MALFORMED if position < offset else Status.AUTH_FAILED)
            previous = offset + image[1]
        for position in range(size - 64, size):
            changed = bytearray(data)
            changed[position] ^= 1
            self.reject(bytes(changed), identity, Status.AUTH_FAILED)
        for index, value in enumerate((2, "wrong-board", "wrong-layout", 8, 12)):
            wrong = list(identity)
            wrong[index] = value
            self.reject(data, tuple(wrong), Status.AUTH_FAILED)
        for public in (ec.generate_private_key(ec.SECP256R1()).public_key(),
                       ec.generate_private_key(ec.SECP384R1()).public_key(), None):
            with self.assertRaises(CborError):
                check_container(data, identity, public)
        for bad in (b"", bytes(63), bytes(65), bytes(64), bytearray(signature)):
            with self.assertRaises(CborError):
                assemble_container(manifest, images, bad, identity, self.public)
        wrong = copy.deepcopy(manifest)
        wrong[8][0][2] = bytes(32)
        with self.assertRaises(CborError):
            assemble_container(wrong, images, self.sign(wrong), identity, self.public)
        for bad_images in ([], images[::-1], images + [b"extra"], [b""]):
            with self.assertRaises(CborError):
                assemble_container(manifest, bad_images, signature, identity, self.public)

    def test_cli_preserves_existing_output_and_rejects_before_create(self):
        manifest, images, signature, identity, data = self.prepare()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            public = root / "public.pem"
            public.write_bytes(self.public.public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo))
            source, sig, output = root / "manifest.json", root / "signature.bin", root / "bundle.cvota"
            source.write_text(json.dumps(named(manifest, SCHEMA)), encoding="utf-8")
            sig.write_bytes(signature)
            paths = [root / f"image{index}.bin" for index in range(len(images))]
            for path, blob in zip(paths, images):
                path.write_bytes(blob)
            common = ["--public-key", str(public), "--role", str(identity[0]), "--board", identity[1],
                      "--layout", identity[2], "--epoch", str(identity[3]), "--key-id", str(identity[4])]
            create = common + ["assemble", str(source), str(sig), str(output), *map(str, paths)]
            self.assertEqual(main(create), 0)
            self.assertEqual(output.read_bytes(), data)
            self.assertEqual(main(common + ["check", str(output)]), 0)
            self.assertEqual(main(create), 1)
            self.assertEqual(output.read_bytes(), data)
            output.unlink()
            sig.write_bytes(bytes(64))
            self.assertEqual(main(create), 1)
            self.assertFalse(output.exists())
            sig.write_bytes(signature)
            source.write_bytes(b"x" * 32769)
            self.assertEqual(main(create), 1)
            self.assertFalse(output.exists())

    def test_cng_body_receiver(self):
        self.assertIsNotNone(PROBE, "Windows CNG body receiver probe required")
        point = self.public.public_numbers()
        root = point.x.to_bytes(32, "big") + point.y.to_bytes(32, "big")
        vectors, expected = [], []
        for role in (1, 2, 3):
            _, _, _, identity, data = self.prepare(role)
            size = prefix_length(data)
            bad_signature = data[:size - 1] + bytes([data[size - 1] ^ 1]) + data[size:]
            for chunk in (1, 31, 16384):
                for blob in (data, data[:-1], data[:size] + b"\1" + data[size + 1:],
                             data[:-1] + bytes([data[-1] ^ 1]), bad_signature):
                    try:
                        check_container(blob, identity, self.public)
                        status = Status.OK
                    except CborError as error:
                        status = error.status
                    for scenario in (0, 10):
                        vectors.append(struct.pack("<7I", role, size, len(blob) - size, chunk, scenario, 0, 0) +
                                       struct.pack("<8IQ", *LOCAL) + root + blob)
                        expected.append(int(status))
        result = subprocess.run([PROBE], input=b"".join(vectors), capture_output=True, check=True, timeout=60)
        self.assertEqual([int(line.split()[0]) for line in result.stdout.splitlines()], expected)
        print(f"PASS: {len(expected)} container/CNG C body vectors")


if __name__ == "__main__":
    unittest.main()
