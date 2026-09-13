"""공식 MCUboot imgtool 생성물과 C profile 검사 교차 시험. private key는 메모리 전용."""
from __future__ import annotations
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
MAXIMUM = 184320
OK, INVALID, MALFORMED, OVERSIZE, INCOMPLETE, AUTH, BUSY = 0, 1, 3, 7, 8, 12, 13


def text(value):
    raw = value.encode("ascii") if isinstance(value, str) else value
    assert len(raw) <= 64
    return raw.ljust(64, b"\0")


def main():
    model = "--model" in sys.argv[2:]
    sdk = Path(os.environ.get("MCUBOOT_ROOT", "C:/cv/mcuboot-2.4.0"))
    pin = json.loads((ROOT / "tools/toolchain-versions.json").read_text())["sdk"]["mcuboot"]
    actual = subprocess.check_output(["git", "-C", str(sdk), "rev-parse", "HEAD"], text=True).strip()
    assert actual == pin["gitCommit"], (actual, pin)
    assert not subprocess.check_output(["git", "-C", str(sdk), "status", "--porcelain=v1", "--untracked-files=all"], text=True).strip()
    version = subprocess.check_output([sys.executable, "-B", str(sdk / "scripts/imgtool.py"), "version"], text=True).strip()
    assert version == pin["version"].removeprefix("v"), version
    sys.path.insert(0, str(sdk / "scripts"))
    from imgtool.image import Image, VerifyResult
    from imgtool.keys.ecdsa import ECDSA256P1
    from imgtool.version import decode_version
    from cryptography.exceptions import InvalidSignature
    from cryptography.hazmat.primitives.asymmetric import ec, utils
    from cryptography.hazmat.primitives import hashes

    key = ECDSA256P1(ec.generate_private_key(ec.SECP256R1()))
    public = key.key.public_key().public_numbers()
    xy = public.x.to_bytes(32, "big") + public.y.to_bytes(32, "big")
    key_hash = hashlib.sha256(key.get_public_bytes()).digest()
    metadata = struct.pack("<8sHH5IQ", b"CVIMG001", 1, 168, 1, 2, 7, 2, 0, 0xFFFFFFFFFFFFFFFF)
    metadata += text("synthetic-board") + text("synthetic-layout")
    assert len(metadata) == 168

    def make(meta=metadata, code=bytes(range(256)), version="1.2.3+4"):
        image = Image(version=decode_version(version), header_size=512, pad_header=True,
                      align=8, slot_size=192 * 1024, max_sectors=96)
        image.payload = bytearray(512) + code
        with contextlib.redirect_stdout(io.StringIO()):
            image.create(key, "hash", None, custom_tlvs={0xA0: meta})
        return bytes(image.payload)

    original = make()
    with tempfile.TemporaryDirectory(prefix="canview-imgtool-") as directory:
        path = Path(directory) / "synthetic.bin"
        path.write_bytes(original)
        assert Image.verify(str(path), key)[0] == VerifyResult.OK
    vectors = []

    def add(name, data, expected=OK, *, fault=0, role=1, target=2, kind=2, abi=2, epoch=7,
            length=None, sequence=0xFFFFFFFFFFFFFFFF, board="synthetic-board", layout="synthetic-layout",
            version="1.2.3+4", expected_hash=None, root=xy, root_hash=key_hash):
        digest = hashlib.sha256(data).digest()
        signed_size = 512 + struct.unpack_from("<I", data, 12)[0] + 176 if len(data) >= 16 else 0
        signed_hash = hashlib.sha256(data[:signed_size]).digest()
        raw = bytes(64)
        signature_status = AUTH
        try:
            der = data[signed_size + 80:]
            r, s = utils.decode_dss_signature(der)
            raw = r.to_bytes(32, "big") + s.to_bytes(32, "big")
            key.key.public_key().verify(der, data[:signed_size], ec.ECDSA(hashes.SHA256()))
            signature_status = OK
        except (ValueError, InvalidSignature, OverflowError):
            pass
        header = struct.pack("<8IQ2I", len(data), fault, role, target, kind, abi, epoch,
                             len(data) if length is None else length, sequence,
                             min(signed_size, 0xFFFFFFFF), signature_status)
        wire = (header + root + root_hash + (digest if expected_hash is None else expected_hash) +
                digest + signed_hash + raw + text(board) + text(layout) + text(version) + data)
        vectors.append((name, wire, expected))

    add("official-imgtool", original)
    for fault in (1, 2, 3):
        add(f"provider-failure-{fault}", original, BUSY, fault=fault)
    add("wrong-whole-image-hash", original, AUTH, expected_hash=bytes(32))
    add("wrong-root-hash", original, AUTH, root_hash=bytes(32))
    other = ec.generate_private_key(ec.SECP256R1()).public_key().public_numbers()
    if not model:
        add("wrong-actual-verification-key", original, AUTH,
            root=other.x.to_bytes(32, "big") + other.y.to_bytes(32, "big"))
    for end in range(len(original)):
        add(f"truncated-{end}", original[:end], None)
    for offset in range(len(original)):
        modified = bytearray(original)
        modified[offset] ^= 1
        # 바깥 manifest digest를 재계산해도 native 구조/서명에서 거부한다.
        add(f"mutation-{offset}", bytes(modified), None)
    for field, values in (("role", (0, 2, 3, 0xFFFFFFFF)), ("target", (0, 1, 3, 4, 0xFFFFFFFF)),
                          ("kind", (0, 1, 3)), ("abi", (0, 1, 3, 0xFFFFFFFF)),
                          ("epoch", (0, 8, 0xFFFFFFFF)), ("sequence", (0, 1, (1 << 53) + 1, (1 << 64) - 2)),
                          ("length", (0, len(original) - 1, len(original) + 1, 0xFFFFFFFF))):
        for value in values:
            add(f"local-{field}-{value}", original, AUTH, **{field: value})
    for field in ("board", "layout"):
        for value in ("", "other", b"x" * 64, b"\x01", b"\x7f"):
            add(f"local-{field}-{value!r}", original, AUTH, **{field: value})
    for value in ("", "1", "1.2.3", "01.2.3+4", "1.2.3+04", "1.2.3+4294967296", b"9" * 64,
                  "1.2.3+5", "256.2.3+4", "1.65536.3+4", "1.2.3+4x"):
        add(f"version-{value!r}", original, AUTH, version=value)
    for offset in (0, 8, 10, 12, 16, 20, 24, 28, 32, 40, 55, 104, 120, 167):
        meta = bytearray(metadata)
        meta[offset] ^= 1
        add(f"resigned-metadata-mismatch-{offset}", make(bytes(meta)), AUTH)
    for sequence in (0, 1, 1 << 32, 1 << 53, (1 << 53) + 1, (1 << 64) - 1):
        meta = bytearray(metadata)
        struct.pack_into("<Q", meta, 32, sequence)
        add(f"u64-{sequence}", make(bytes(meta)), sequence=sequence)
    for abi in (0, 0xFFFFFFFF):
        meta = bytearray(metadata)
        struct.pack_into("<I", meta, 24, abi)
        add(f"abi-{abi}", make(bytes(meta)), abi=abi)
    add("maximum-version", make(version="255.255.65535+4294967295"), version="255.255.65535+4294967295")
    add("minimum-code", make(code=bytes(8)))
    add("zero-version", make(version="0.0.0+0"), version="0.0.0+0")
    signed_size = 512 + 256 + 176
    def replace_der(der):
        value = bytearray(original[:signed_size + 80] + der)
        struct.pack_into("<H", value, signed_size + 2, 80 + len(der))
        struct.pack_into("<H", value, signed_size + 78, len(der))
        return bytes(value)
    for name, der in (
        ("short", b"\x30\x06\x02\x01\x01\x02\x01\x02"),
        ("zero-integer", b"\x30\x06\x02\x01\x00\x02\x01\x00"),
        ("empty-integer", b"\x30\x06\x02\x00\x00\x02\x01\x02"),
        ("negative", b"\x30\x06\x02\x01\x80\x02\x01\x02"),
        ("wrong-integer-tag", b"\x30\x06\x03\x01\x01\x02\x01\x02"),
        ("missing-second", b"\x30\x06\x02\x04\x01\x01\x01\x01"),
        ("redundant-zero", b"\x30\x07\x02\x02\x00\x01\x02\x01\x02"),
        ("integer-long", b"\x30\x07\x02\x81\x01\x01\x02\x01\x02"),
        ("overrun", b"\x30\x06\x02\x20\x01\x02\x01\x02"),
        ("33bytes-positive", b"\x30\x26\x02\x21" + b"\x01" * 33 + b"\x02\x01\x02"),
    ):
        add(f"der-{name}", replace_der(der), None)
    for tlv in (original[signed_size + 4:signed_size + 40], original[signed_size + 76:]):
        duplicated = bytearray(original + tlv)
        struct.pack_into("<H", duplicated, signed_size + 2, len(duplicated) - signed_size)
        add("duplicate-native-tlv", bytes(duplicated), MALFORMED)
    add("maximum-image-budget", make(code=bytes(MAXIMUM - 512 - 176 - 152)))
    add("oversized-image", bytes(MAXIMUM + 1), OVERSIZE)
    add("trailing-data", original + b"\0", None)
    command = ["wsl.exe", "--exec", sys.argv[1]] if "--wsl" in sys.argv[2:] else [sys.argv[1]]
    result = subprocess.run(command, input=b"".join(v[1] for v in vectors),
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
    assert result.returncode == 0, (result.returncode, result.stderr.decode(errors="replace"), result.stdout[-200:])
    lines = result.stdout.decode("ascii").splitlines()
    assert len(lines) == len(vectors), (len(lines), len(vectors))
    for (name, _, expected), line in zip(vectors, lines):
        status, calls = map(int, line.split())
        assert (status != OK if expected is None else status == expected), (name, status, expected)
        assert calls <= 3 and (status != OK or calls == 3), (name, line)
    print(f"MCUboot {'non-crypto model' if model else 'CNG P256 + SHA256'} cases: {len(vectors)}; commit={actual}")


if __name__ == "__main__":
    main()
