"""보존된 합성 golden의 서명·C 수신 교차 검사. MCU 실행/설치 검증은 아니다."""
from __future__ import annotations

import argparse
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

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import utils

from test_body import LOCAL
from cbor import CborError
from container import assemble_container, check_container
from envelope import prefix_length
from check_sdk_metadata import check as check_metadata

ROOT = Path(__file__).resolve().parents[2]
GOLDEN = ROOT / "tests/fixtures/ota-signed-golden"
IDENTITY = (1, "synthetic-board", "synthetic-layout", 7, 11)


def point(public):
    numbers = public.public_numbers()
    return numbers.x.to_bytes(32, "big") + numbers.y.to_bytes(32, "big")


def run_probe(path, vectors, expected, calls=None):
    result = subprocess.run([str(path)], input=b"".join(vectors), capture_output=True, timeout=60, check=True)
    rows = [[int(value) for value in line.split()] for line in result.stdout.splitlines()]
    actual = [row[0] for row in rows]
    assert actual == expected, (actual, expected, result.stderr)
    if calls is not None:
        assert [row[1] for row in rows] == calls, (rows, calls)


def verify_esp(data, public):
    import espsecure
    import esptool
    assert esptool.__version__ == "5.4.0", esptool.__version__
    with contextlib.redirect_stdout(io.StringIO()):
        espsecure.verify_signature_v2(False, None, io.BytesIO(public), io.BytesIO(data))


def sdk_cases(esp, public):
    import esptool
    verify_esp(esp, public)
    from cryptography.hazmat.primitives.asymmetric import rsa
    other = rsa.generate_private_key(public_exponent=65537, key_size=3072).public_key().public_bytes(
        serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo)
    negatives = [(esp, other), (esp[:-1], public), (esp[:-4096], public), (esp + b"\0", public)]
    # 바깥 hash에 기대지 않고 native signature checker 자체의 거절을 시험한다.
    for offset in (0, 288, 455, len(esp) - 4097, len(esp) - 4096 + 812):
        changed = bytearray(esp)
        changed[offset] ^= 1
        negatives.append((bytes(changed), public))
    for data, key in negatives:
        try:
            verify_esp(data, key)
        except esptool.FatalError:
            continue
        raise AssertionError("ESP native tamper/wrong key accepted")
    print(f"PASS: official espsecure native RSA {1 + len(negatives)} cases (host only)")


def host_cases(package, checked, manifest_public, stm, stm_public, body_probe, stm_probe):
    size = prefix_length(package)
    vectors, expected = [], []
    for chunk in (31, 4096, 16384):
        for blob in (package, package[:-1], package[:size - 1] + bytes([package[size - 1] ^ 1]) + package[size:],
                     package[:-1] + bytes([package[-1] ^ 1])):
            try:
                check_container(blob, IDENTITY, manifest_public)
                status = 0
            except CborError as error:
                status = int(error.status)
            vectors.append(struct.pack("<7I", 1, size, len(blob) - size, chunk, 10, 0, 0) +
                           struct.pack("<8IQ", *LOCAL) + point(manifest_public) + blob)
            expected.append(status)
    run_probe(body_probe, vectors, expected)

    sdk = Path(os.environ.get("MCUBOOT_ROOT", "C:/cv/mcuboot-2.4.0"))
    from generate_signed_golden import checkout
    pin = json.loads((ROOT / "tools/toolchain-versions.json").read_text())["sdk"]["mcuboot"]
    checkout(sdk, pin["gitCommit"])
    sys.path.insert(0, str(sdk / "scripts"))
    from imgtool.image import Image, VerifyResult
    from imgtool.keys.ecdsa import ECDSA256P1Public
    with tempfile.TemporaryDirectory(prefix="canview-golden-verify-") as directory:
        path = Path(directory) / "stm.bin"
        path.write_bytes(stm)
        assert Image.verify(str(path), ECDSA256P1Public(stm_public))[0] == VerifyResult.OK
    entry = checked["manifest"][8][1]
    spki = stm_public.public_bytes(serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)
    key_hash = hashlib.sha256(spki).digest()
    vectors, expected, calls = [], [], []
    changed = bytearray(stm)
    changed[512] ^= 1
    signed_size = 512 + 256 + 176
    bad_signature = bytearray(changed)
    # 내부 SHA256 TLV까지 갱신하되 원본 서명은 유지하여 P256 실패 경로에 도달시킨다.
    bad_signature[signed_size + 8:signed_size + 40] = hashlib.sha256(bad_signature[:signed_size]).digest()
    for blob, root_hash, status, count in ((stm, key_hash, 0, 3), (bytes(changed), key_hash, 12, 2),
                                          (stm, bytes(32), 12, 0), (bytes(bad_signature), key_hash, 12, 3)):
        digest = hashlib.sha256(blob).digest()
        r, s = utils.decode_dss_signature(blob[signed_size + 80:])
        raw_signature = r.to_bytes(32, "big") + s.to_bytes(32, "big")
        header = struct.pack("<8IQ2I", len(blob), 0, 1, 2, 2, 2, 7, len(blob), entry[4], signed_size, 0)
        vectors.append(header + point(stm_public) + root_hash + digest + digest +
                       hashlib.sha256(blob[:signed_size]).digest() + raw_signature +
                       IDENTITY[1].encode().ljust(64, b"\0") + IDENTITY[2].encode().ljust(64, b"\0") +
                       entry[3].encode().ljust(64, b"\0") + blob)
        expected.append(status)
        calls.append(count)
    run_probe(stm_probe, vectors, expected, calls)
    print("PASS: golden/CNG C prefix+body 12 cases; official imgtool verify; CNG native STM 4 cases with call counts")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--esp-sdk", action="store_true")
    parser.add_argument("--body-probe", type=Path)
    parser.add_argument("--stm-probe", type=Path)
    args = parser.parse_args()
    if not args.esp_sdk and (args.body_probe is None or args.stm_probe is None):
        parser.error("provide --esp-sdk or both CNG probe paths")
    provenance = json.loads((GOLDEN / "provenance.json").read_text())
    assert provenance["syntheticOnly"] is True
    pins = json.loads((ROOT / "tools/toolchain-versions.json").read_text())["sdk"]
    assert provenance["espIdf"] == pins["espIdf"] and provenance["mcuboot"] == pins["mcuboot"]
    assert provenance["esptool"] == "5.4.0"
    assert set(provenance["files"]) == {"communicator.cvota", "manifest-public.pem", "esp-public.pem", "stm-public.pem"}
    files = {}
    for name, evidence in provenance["files"].items():
        with (GOLDEN / name).open("rb") as source:
            data = source.read(512 * 1024 + 1)
        assert len(data) == evidence["bytes"] and hashlib.sha256(data).hexdigest() == evidence["sha256"], name
        files[name] = data
    public = serialization.load_pem_public_key(files["manifest-public.pem"])
    package = files["communicator.cvota"]
    checked = check_container(package, IDENTITY, public)
    images = [package[offset:offset + entry[1]] for offset, entry in zip(checked["offsets"], checked["manifest"][8])]
    assert len(images) == 2
    check_metadata(images[0])
    assert hashlib.sha256(images[0][:-4096]).hexdigest() == provenance["unsignedEspSha256"]
    assert all(entry[4] == (1 << 64) - 1 for entry in checked["manifest"][8])
    signature = package[prefix_length(package) - 64:prefix_length(package)]
    assert assemble_container(checked["manifest"], images, signature, IDENTITY, public) == package
    if args.esp_sdk:
        sdk_cases(images[0], files["esp-public.pem"])
    else:
        host_cases(package, checked, public, images[1], serialization.load_pem_public_key(files["stm-public.pem"]),
                   args.body_probe, args.stm_probe)
    print("PASS: golden digest/outer P256/hash/u64/exact reassembly; device native execution/Flash/HIL NOT_RUN")


if __name__ == "__main__":
    main()
