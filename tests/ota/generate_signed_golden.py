"""공식 SDK로 합성 golden을 생성한다. 시험 개인키는 메모리에만 존재한다."""
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

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, padding, rsa, utils

from test_manifest import fixture
from cbor import encode_document
from container import assemble_container
from check_sdk_metadata import check

ROOT = Path(__file__).resolve().parents[2]
ESPTOOL_VERSION = "5.4.0"


def checkout(path, expected):
    actual = subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"], text=True).strip()
    if actual != expected:
        raise ValueError("SDK commit mismatch")
    dirty = subprocess.check_output(
        ["git", "-C", str(path), "status", "--porcelain=v1", "--untracked-files=all"], text=True)
    if dirty.strip():
        raise ValueError("SDK checkout is dirty")


def public_pem(key):
    return key.public_key().public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo)


def stream(data, name):
    result = io.BytesIO(data)
    result.name = name
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("unsigned_esp", type=Path)
    parser.add_argument("output_directory", type=Path, help="존재하지 않는 새 디렉터리만 허용")
    args = parser.parse_args()
    if args.output_directory.exists():
        raise ValueError("refusing to overwrite golden directory")
    import espsecure
    import esptool
    if esptool.__version__ != ESPTOOL_VERSION:
        raise ValueError("esptool version mismatch")
    pins = json.loads((ROOT / "tools/toolchain-versions.json").read_text())["sdk"]
    idf = Path(os.environ.get("IDF_PATH", "C:/cv/esp-idf-6.0.3"))
    mcuboot = Path(os.environ.get("MCUBOOT_ROOT", "C:/cv/mcuboot-2.4.0"))
    checkout(idf, pins["espIdf"]["gitCommit"])
    checkout(mcuboot, pins["mcuboot"]["gitCommit"])
    sys.path.insert(0, str(mcuboot / "scripts"))
    from imgtool.image import Image, VerifyResult
    from imgtool.keys.ecdsa import ECDSA256P1
    from imgtool.version import decode_version

    with args.unsigned_esp.open("rb") as source:
        unsigned = source.read(4 * 1024 * 1024 + 1)
    check(unsigned)
    if len(unsigned) % 4096:
        raise ValueError("SDK input must already be sector aligned")
    esp_key = rsa.generate_private_key(public_exponent=65537, key_size=3072)
    stm_key = ECDSA256P1(ec.generate_private_key(ec.SECP256R1()))
    manifest_key = ec.generate_private_key(ec.SECP256R1())
    signature = esp_key.sign(unsigned, padding.PSS(mgf=padding.MGF1(hashes.SHA256()), salt_length=32), hashes.SHA256())
    with tempfile.TemporaryDirectory(prefix="canview-signed-golden-") as directory:
        signed_path = Path(directory) / "esp.bin"
        with contextlib.redirect_stdout(io.StringIO()):
            espsecure.sign_data("2", [], str(signed_path), False, False, None,
                                [stream(public_pem(esp_key), "public.pem")], [stream(signature, "signature.bin")],
                                stream(unsigned, "unsigned.bin"))
            esp = signed_path.read_bytes()
            espsecure.verify_signature_v2(False, None, io.BytesIO(public_pem(esp_key)), io.BytesIO(esp))
        metadata = struct.pack("<8sHH5IQ", b"CVIMG001", 1, 168, 1, 2, 7, 2, 0, (1 << 64) - 1)
        metadata += b"synthetic-board".ljust(64, b"\0") + b"synthetic-layout".ljust(64, b"\0")
        image = Image(version=decode_version("1.2.3+4"), header_size=512, pad_header=True,
                      align=8, slot_size=192 * 1024, max_sectors=96)
        image.payload = bytearray(512) + bytes(range(256))
        with contextlib.redirect_stdout(io.StringIO()):
            image.create(stm_key, "hash", None, custom_tlvs={0xA0: metadata})
        stm = bytes(image.payload)
        stm_path = Path(directory) / "stm.bin"
        stm_path.write_bytes(stm)
        if Image.verify(str(stm_path), stm_key)[0] != VerifyResult.OK:
            raise ValueError("imgtool verification failed")
    manifest = fixture(1)
    for entry, data in zip(manifest[8], (esp, stm)):
        entry[1], entry[2], entry[3] = len(data), hashlib.sha256(data).digest(), "1.2.3+4"
    encoded = encode_document(manifest)
    r, s = utils.decode_dss_signature(manifest_key.sign(encoded, ec.ECDSA(hashes.SHA256())))
    detached = r.to_bytes(32, "big") + s.to_bytes(32, "big")
    package = assemble_container(manifest, [esp, stm], detached,
                                 (1, "synthetic-board", "synthetic-layout", 7, 11), manifest_key.public_key())
    files = {"communicator.cvota": package, "manifest-public.pem": public_pem(manifest_key),
             "esp-public.pem": public_pem(esp_key), "stm-public.pem": public_pem(stm_key.key)}
    provenance = {"syntheticOnly": True, "esptool": ESPTOOL_VERSION,
                  "espIdf": pins["espIdf"], "mcuboot": pins["mcuboot"],
                  "unsignedEspSha256": hashlib.sha256(unsigned).hexdigest(),
                  "files": {name: {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                            for name, data in files.items()}}
    # 생성물만 기록한다. private key를 직렬화하거나 저장하지 않는다.
    args.output_directory.mkdir()
    for name, data in files.items():
        with (args.output_directory / name).open("xb") as destination:
            destination.write(data)
    with (args.output_directory / "provenance.json").open("x", encoding="utf-8", newline="\n") as destination:
        json.dump(provenance, destination, indent=2)
        destination.write("\n")
    print(f"SYNTHETIC_ONLY: {len(package)} bytes; SHA256={hashlib.sha256(package).hexdigest()}")
    print("private keys discarded; device native execution/Flash/HIL NOT_RUN; vehicle TX NO-GO")


if __name__ == "__main__":
    main()
