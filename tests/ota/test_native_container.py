"""일반 native CLI의 실제 공식 도구·outer-valid/native-invalid 회귀."""
from __future__ import annotations
import contextlib
import copy
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zlib

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, padding, rsa, utils

from test_manifest import fixture
from test_container import named
from cbor import encode_document
from container import assemble_container, check_container, main as cli
from manifest_json import SCHEMA
from native import check_native_container

ROOT = Path(__file__).resolve().parents[2]
GOLDEN = ROOT / "tests/fixtures/ota-signed-golden"
MCUBOOT = Path(os.environ.get("MCUBOOT_ROOT", "C:/cv/mcuboot-2.4.0"))


def pem(key):
    return key.public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo)


class NativeContainerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.package = (GOLDEN / "communicator.cvota").read_bytes()
        cls.public = serialization.load_pem_public_key((GOLDEN / "manifest-public.pem").read_bytes())
        cls.esp_public = (GOLDEN / "esp-public.pem").read_bytes()
        cls.stm_public = (GOLDEN / "stm-public.pem").read_bytes()
        cls.identity = (1, "synthetic-board", "synthetic-layout", 7, 11)
        checked = check_container(cls.package, cls.identity, cls.public)
        cls.manifest = checked["manifest"]
        cls.images = [cls.package[offset:offset + entry[1]]
                      for offset, entry in zip(checked["offsets"], cls.manifest[8])]
        cls.signer = ec.generate_private_key(ec.SECP256R1())
        cls.esp_signer = rsa.generate_private_key(public_exponent=65537, key_size=3072)

    def sign(self, manifest):
        r, s = utils.decode_dss_signature(self.signer.sign(encode_document(manifest), ec.ECDSA(hashes.SHA256())))
        return r.to_bytes(32, "big") + s.to_bytes(32, "big")

    def pack(self, manifest, images):
        for entry, image in zip(manifest[8], images):
            entry[1], entry[2] = len(image), hashlib.sha256(image).digest()
        identity = (manifest[2], manifest[3], manifest[4], manifest[6], manifest[7])
        return assemble_container(manifest, images, self.sign(manifest), identity, self.signer.public_key()), identity

    def check(self, package, identity=None, public=None, esp_public=None, stm_public=None):
        return check_native_container(package, identity or self.identity, public or self.signer.public_key(),
                                      esp_public or self.esp_public, stm_public or self.stm_public, MCUBOOT)

    def test_preserved_golden(self):
        self.check(self.package, public=self.public)

    def test_outer_valid_native_tamper(self):
        for index, offsets in ((0, (512, len(self.images[0]) - 4096 + 812)), (1, (512, len(self.images[1]) - 1))):
            for offset in offsets:
                with self.subTest(index=index, offset=offset):
                    images = self.images.copy()
                    changed = bytearray(images[index])
                    changed[offset] ^= 1
                    images[index] = bytes(changed)
                    package, identity = self.pack(copy.deepcopy(self.manifest), images)
                    check_container(package, identity, self.signer.public_key())
                    with self.assertRaises(ValueError):
                        self.check(package)

    def test_esp_signature_block_binding(self):
        for offset in (1, 36, 420, 424, 808, 812):
            with self.subTest(block_offset=offset):
                images = self.images.copy()
                changed = bytearray(images[0])
                start = len(changed) - 4096
                changed[start + offset] ^= 1
                struct.pack_into("<I", changed, start + 1196,
                                 zlib.crc32(changed[start:start + 1196]) & 0xffffffff)
                images[0] = bytes(changed)
                package, identity = self.pack(copy.deepcopy(self.manifest), images)
                check_container(package, identity, self.signer.public_key())
                with self.assertRaises(ValueError):
                    self.check(package)

        block_bytes = 1216
        original = self.images[0][-4096:-4096 + block_bytes]
        for slot in range(3):
            with self.subTest(valid_slot=slot):
                sector = bytearray(b"\xff" * 4096)
                sector[slot * block_bytes:(slot + 1) * block_bytes] = original
                images = [self.images[0][:-4096] + sector, self.images[1]]
                package, _ = self.pack(copy.deepcopy(self.manifest), images)
                self.check(package)

        # key가 맞는 block과 서명만 맞는 다른 block을 결합해 승인하면 안 된다.
        sector = bytearray(b"\xff" * 4096)
        for slot, offset in ((0, 812), (1, 36)):
            block = bytearray(original)
            block[offset] ^= 1
            struct.pack_into("<I", block, 1196, zlib.crc32(block[:1196]) & 0xffffffff)
            sector[slot * block_bytes:(slot + 1) * block_bytes] = block
        images = [self.images[0][:-4096] + sector, self.images[1]]
        package, _ = self.pack(copy.deepcopy(self.manifest), images)
        with self.assertRaises(ValueError):
            self.check(package)

    def test_outer_valid_native_metadata_mismatch(self):
        for index in (0, 1):
            for field, value in ((3, "2.0.0+0"), (4, 0), (4, (1 << 53) + 1), (4, (1 << 64) - 2), (6, 3)):
                with self.subTest(index=index, field=field, value=value):
                    manifest = copy.deepcopy(self.manifest)
                    manifest[8][index][field] = value
                    package, _ = self.pack(manifest, self.images)
                    with self.assertRaises(ValueError):
                        self.check(package)

    def test_missing_wrong_roots_and_sdk(self):
        for esp_key, stm_key, root in ((None, self.stm_public, MCUBOOT),
                                      (self.esp_public, None, MCUBOOT),
                                      (pem(self.esp_signer.public_key()), self.stm_public, MCUBOOT),
                                      (self.esp_public, pem(self.signer.public_key()), MCUBOOT),
                                      (self.esp_public, self.stm_public, ROOT)):
            with self.assertRaises(ValueError):
                check_native_container(self.package, self.identity, self.public, esp_key, stm_key, root)

    def test_native_tool_failures(self):
        # 모형 실패 주입은 실제 SDK 암호 실행 증거와 구분한다.
        with mock.patch("esptool.__version__", "unsupported"):
            with self.assertRaises(ValueError):
                self.check(self.package, public=self.public)
        for failure in (ImportError("missing verifier"),
                        subprocess.TimeoutExpired("imgtool", 60),
                        subprocess.CalledProcessError(1, "git")):
            with self.subTest(failure=type(failure).__name__):
                with mock.patch("native._mcuboot_checkout", side_effect=failure):
                    with self.assertRaises(ValueError):
                        self.check(self.package, public=self.public)
        head = subprocess.CompletedProcess([], 0, stdout="6d3b3d2c38ab20c242e5b9abb04d050086383eb2\n")
        dirty = subprocess.CompletedProcess([], 0, stdout=" M scripts/imgtool.py\n")
        with mock.patch("native.subprocess.run", side_effect=[head, dirty]):
            with self.assertRaises(ValueError):
                self.check(self.package, public=self.public)

    def signed_esp(self, role, target, sequence):
        import espsecure
        from esptool.bin_image import ESP32S3FirmwareImage
        unsigned = bytearray(self.images[0][:-4096])
        metadata = struct.pack("<8sHH5IQ", b"CVIMG001", 1, 168, role, target, 7, 2, 0, sequence)
        metadata += b"alternate-board".ljust(64, b"\0") + b"alternate-layout".ljust(64, b"\0")
        unsigned[288:456] = metadata
        parsed = ESP32S3FirmwareImage(io.BytesIO(unsigned))
        self.assertTrue(parsed.append_digest)
        unsigned[parsed.data_length - 1] = parsed.calculate_checksum()
        unsigned[parsed.data_length:parsed.data_length + 32] = hashlib.sha256(unsigned[:parsed.data_length]).digest()
        signature = self.esp_signer.sign(bytes(unsigned), padding.PSS(mgf=padding.MGF1(hashes.SHA256()), salt_length=32),
                                         hashes.SHA256())
        with tempfile.TemporaryDirectory(prefix="canview-native-role-") as directory:
            path = Path(directory) / "image.bin"
            unsigned_stream = io.BytesIO(unsigned)
            unsigned_stream.name = "synthetic.bin"
            with contextlib.redirect_stdout(io.StringIO()):
                espsecure.sign_data("2", [], str(path), False, False, None,
                                    [io.BytesIO(pem(self.esp_signer.public_key()))], [io.BytesIO(signature)],
                                    unsigned_stream)
            return path.read_bytes()

    def test_other_identity_all_esp_roles(self):
        for role, target, sequence in ((1, 1, 0), (2, 3, (1 << 53) + 1), (3, 4, (1 << 64) - 1)):
            with self.subTest(role=role):
                manifest = fixture(role)
                manifest[3], manifest[4] = "alternate-board", "alternate-layout"
                manifest[8] = [manifest[8][0]]
                manifest[8][0][3], manifest[8][0][4] = "1.2.3+4", sequence
                image = self.signed_esp(role, target, sequence)
                package, identity = self.pack(manifest, [image])
                self.check(package, identity, esp_public=pem(self.esp_signer.public_key()))

    def test_cli_native_output_and_preservation(self):
        with tempfile.TemporaryDirectory(prefix="canview-native-cli-") as directory:
            root = Path(directory)
            public, manifest_path, signature_path, output = (root / name for name in
                ("manifest-public.pem", "manifest.json", "signature.bin", "output.cvota"))
            public.write_bytes(pem(self.signer.public_key()))
            manifest = copy.deepcopy(self.manifest)
            signature_path.write_bytes(self.sign(manifest))
            manifest_path.write_text(json.dumps(named(manifest, SCHEMA)), encoding="utf-8")
            paths = [root / f"image{index}.bin" for index in range(2)]
            for path, image in zip(paths, self.images):
                path.write_bytes(image)
            common = ["--public-key", str(public), "--role", "1", "--board", self.identity[1], "--layout", self.identity[2],
                      "--epoch", "7", "--key-id", "11", "--native", "--esp-public-key", str(GOLDEN / "esp-public.pem"),
                      "--stm-public-key", str(GOLDEN / "stm-public.pem"), "--mcuboot-root", str(MCUBOOT)]
            command = common + ["assemble", str(manifest_path), str(signature_path), str(output), *map(str, paths)]
            with contextlib.redirect_stdout(io.StringIO()) as captured:
                self.assertEqual(cli(command), 0)
            self.assertIn("NATIVE_SIGNATURES_AND_METADATA_MATCHED", captured.getvalue())
            self.assertIn("install NOT_VERIFIED", captured.getvalue())
            self.assertEqual(cli(common + ["check", str(output)]), 0)
            before = output.read_bytes()
            self.assertEqual(cli(command), 1)
            self.assertEqual(output.read_bytes(), before)
            output.unlink()
            self.assertEqual(cli([item for item in command if item != "--native"]), 1)
            self.assertFalse(output.exists())
            # Outer/native 입력은 정상인 채 실제 호출 경계에서 도구 고장을 주입한다.
            for failure in (subprocess.TimeoutExpired("imgtool", 60), OSError("tool unavailable")):
                with self.subTest(failure=type(failure).__name__):
                    with mock.patch("native._mcuboot_checkout", return_value=MCUBOOT / "scripts/imgtool.py"), \
                         mock.patch("native.subprocess.run", side_effect=failure):
                        self.assertEqual(cli(command), 1)
                    self.assertFalse(output.exists())
            with mock.patch("native._mcuboot_checkout", return_value=MCUBOOT / "scripts/imgtool.py"), \
                 mock.patch("native.subprocess.run", return_value=subprocess.CompletedProcess([], 1)):
                self.assertEqual(cli(command), 1)
            self.assertFalse(output.exists())
            changed = bytearray(self.images[0])
            start = len(changed) - 4096
            changed[start + 36] ^= 1
            struct.pack_into("<I", changed, start + 1196, zlib.crc32(changed[start:start + 1196]) & 0xffffffff)
            paths[0].write_bytes(changed)
            manifest[8][0][2] = hashlib.sha256(changed).digest()
            manifest_path.write_text(json.dumps(named(manifest, SCHEMA)), encoding="utf-8")
            signature_path.write_bytes(self.sign(manifest))
            self.assertEqual(cli(command), 1)
            self.assertFalse(output.exists())
            output.write_bytes(before)
            self.assertEqual(cli(command), 1)
            self.assertEqual(output.read_bytes(), before)
            output.unlink()
            paths[0].write_bytes(self.images[0])
            manifest = copy.deepcopy(self.manifest)
            manifest[8][1][4] -= 1
            manifest_path.write_text(json.dumps(named(manifest, SCHEMA)), encoding="utf-8")
            signature_path.write_bytes(self.sign(manifest))
            self.assertEqual(cli(command), 1)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
