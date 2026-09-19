"""실제 primary 앱 BIN을 공식 imgtool로 임시 서명·검증한다. 개인키는 메모리 전용이다."""
from __future__ import annotations

import argparse
import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--mcuboot", type=Path, required=True)
    args = parser.parse_args()
    pin = json.loads((ROOT / "tools/toolchain-versions.json").read_text(encoding="utf-8"))["sdk"]["mcuboot"]
    commit = subprocess.check_output(["git", "-C", str(args.mcuboot), "rev-parse", "HEAD"], text=True).strip()
    dirty = subprocess.check_output(["git", "-C", str(args.mcuboot), "status", "--porcelain=v1",
                                     "--untracked-files=all"], text=True).strip()
    if commit != pin["gitCommit"] or dirty:
        raise ValueError("MCUboot pin/clean 검증 실패")
    sys.path.insert(0, str(args.mcuboot / "scripts"))
    from imgtool.image import Image, VerifyResult
    from imgtool.keys.ecdsa import ECDSA256P1
    from imgtool.version import decode_version
    from cryptography.hazmat.primitives.asymmetric import ec

    payload = args.binary.read_bytes()
    if not 0x1D8 <= len(payload) <= 179 * 1024:
        raise ValueError("primary payload budget 초과/부족")
    stack, reset = struct.unpack_from("<II", payload)
    if stack != 0x20018000 or reset & 1 == 0 or not 0x08010200 <= (reset & ~1) < 0x08010200 + len(payload):
        raise ValueError("primary vector mismatch")
    key = ECDSA256P1(ec.generate_private_key(ec.SECP256R1()))
    # 실차/production 인증물이 아닌 임시 synthetic 서명이다. T-007 metadata wire 계약 유지.
    metadata = struct.pack("<8sHH5IQ", b"CVIMG001", 1, 168, 1, 2, 1, 2, 0, 1)
    metadata += b"synthetic-board".ljust(64, b"\0") + b"synthetic-layout".ljust(64, b"\0")
    results = []
    with tempfile.TemporaryDirectory(prefix="canview-primary-sign-") as temporary:
        for name, code in (("actual-primary", payload),
                           ("padded-budget-boundary", payload.ljust(179 * 1024, b"\xff"))):
            image = Image(version=decode_version("1.0.0+1"), header_size=512, pad_header=True,
                          align=8, slot_size=192 * 1024, max_sectors=97)
            image.payload = bytearray(512) + code
            with contextlib.redirect_stdout(io.StringIO()):
                image.create(key, "hash", None, custom_tlvs={0xA0: metadata})
            signed = bytes(image.payload)
            # 공식 imgtool은 offset bootutil보다 보수적인 3-state trailer 산식을 사용한다.
            trailer = image._trailer_size(8, 97, False, None, False, 0)
            reserve = ((trailer + 2047) // 2048) * 2048
            if (len(signed) > 180 * 1024 or 512 + len(code) >= len(signed) or
                    len(signed) + reserve > 192 * 1024 or
                    2048 + len(signed) + reserve > 194 * 1024 or
                    signed[512:512 + len(code)] != code):
                raise ValueError("공식 signed image/TLV/trailer/offset-page budget 실패")
            path = Path(temporary) / f"{name}.bin"
            path.write_bytes(signed)
            if Image.verify(str(path), key)[0] != VerifyResult.OK:
                raise ValueError("공식 image signature 검증 실패")
            changed = bytearray(signed)
            changed[512] ^= 1
            path.write_bytes(changed)
            if Image.verify(str(path), key)[0] == VerifyResult.OK:
                raise ValueError("변조된 image를 공식 verifier가 허용함")
            results.append(dict(case=name, payload_bytes=len(code), signed_bytes=len(signed),
                                trailer_bytes=trailer, trailer_page_reserve=reserve,
                                payload_sha256=hashlib.sha256(code).hexdigest()))
    print(json.dumps(dict(mcuboot_commit=commit, cases=results, physical_hil="NOT_RUN",
                          provisioning="NOT_RUN", bootloader_execution="NOT_RUN"), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
