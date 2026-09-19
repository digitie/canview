"""공식 imgtool의 임시 서명 → 실제 C boot_go/offset swap/revert 실행. 물리 시험이 아니다."""
from __future__ import annotations
import argparse
import contextlib
import io
import re
from pathlib import Path
import subprocess
import struct
import sys
import tempfile


def check_compatibility_copy(sdk, bootutil):
    """빌드 사본에서 허용한 반환형/초기화 외의 source 변조가 없는지 독립 대조한다."""
    counts = [0, 0]
    original = sdk / "boot/bootutil"
    sources = sorted(p.relative_to(original) for p in original.rglob("*")
                     if p.suffix in (".c", ".h"))
    actual = sorted(p.relative_to(bootutil) for p in bootutil.rglob("*")
                    if p.suffix in (".c", ".h"))
    if actual != sources:
        raise AssertionError("bootutil source inventory drift")
    for relative in sources:
        expected = (original / relative).read_text(encoding="utf-8")
        expected, count = re.subn(r"\bfih_ret(\s+[a-z_][A-Za-z_0-9]*\s*\()", r"int\1", expected)
        counts[0] += count
        expected, count = re.subn(r"\bfih_int(\s+[a-z_][A-Za-z_0-9]*\s*\()", r"canview_fih_result\1", expected)
        counts[1] += count
        if relative.name == "fault_injection_hardening.h":
            expected = expected.replace("typedef volatile struct {", "typedef volatile struct canview_fih_value {")
            expected = expected.replace("} fih_int;", "} fih_int;\ntypedef struct canview_fih_value canview_fih_result;")
            expected = expected.replace("typedef int fih_int;", "typedef int fih_int;\ntypedef int canview_fih_result;")
        if (bootutil / relative).read_text(encoding="utf-8") != expected:
            raise AssertionError(f"unexpected MCUboot source adaptation: {relative}")
    ecc = (sdk / "ext/tinycrypt/lib/source/ecc.c").read_text(encoding="utf-8")
    declaration = "uECC_word_t t5[NUM_ECC_WORDS];"
    if counts != [35, 4] or ecc.count(declaration) != 3:
        raise AssertionError("pinned source adaptation count mismatch")
    if (bootutil.parent / "ecc.c").read_text(encoding="utf-8") != ecc.replace(
            declaration, "uECC_word_t t5[NUM_ECC_WORDS] = {0};"):
        raise AssertionError("unexpected TinyCrypt source adaptation")
    print(f"source adaptation audit: {len(sources)} files, returns35/4 + array-init3 only")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--mcuboot", type=Path, required=True)
    parser.add_argument("--bootutil", type=Path, required=True)
    args = parser.parse_args()
    check_compatibility_copy(args.mcuboot, args.bootutil)
    sys.path.insert(0, str(args.mcuboot / "scripts"))
    from imgtool.image import Image
    from imgtool.keys.ecdsa import ECDSA256P1
    from imgtool.version import decode_version
    from cryptography.hazmat.primitives.asymmetric import ec
    from cryptography.hazmat.primitives import serialization

    private = ec.generate_private_key(ec.SECP256R1())
    key = ECDSA256P1(private)

    def signed(major, size=8192, signing_key=key):
        image = Image(version=decode_version(f"{major}.0.0+1"), header_size=512,
                      pad_header=True, align=8, slot_size=192 * 1024, max_sectors=97)
        image.payload = bytearray(512) + bytes((index * 7 + major) % 256 for index in range(size))
        with contextlib.redirect_stdout(io.StringIO()):
            image.create(signing_key, "hash", None)
        return bytes(image.payload)

    with tempfile.TemporaryDirectory(prefix="canview-boot-model-") as temporary:
        root = Path(temporary)
        public = root / "public.der"
        public.write_bytes(private.public_key().public_bytes(serialization.Encoding.DER,
                           serialization.PublicFormat.SubjectPublicKeyInfo))
        first = signed(1)
        second = signed(2, 12288)
        damaged = bytearray(second)
        damaged[512] ^= 1
        cases = [("boot", first, b""), ("reject", b"", b""),
                 ("reject", bytes(damaged), b""), ("swap", first, second),
                 ("confirm", first, second), ("bad-secondary", first, bytes(damaged)),
                 ("cuts", first, second), ("cuts-revert", first, second),
                 ("swap", signed(1, 179 * 1024), signed(2, 179 * 1024))]
        for length in (1, 31, 511, len(first) - 32):
            cases.append(("reject", first[:length], b""))
        # header/TLV/서명 변조. keyhash와 hash를 통과하더라도 ECDSA 변조는 거절해야 한다.
        for offset in (0, 8, 10, 12, 16, 512 + 8192, len(first) - 8):
            changed = bytearray(first)
            changed[offset] ^= 1
            cases.append(("reject", bytes(changed), b""))
            cases.append(("bad-secondary", first, bytes(changed)))
        changed = bytearray(first)
        struct.pack_into("<I", changed, 12, 0xFFFFFFFF)
        cases.append(("reject", bytes(changed), b""))
        untrusted = signed(2, signing_key=ECDSA256P1(ec.generate_private_key(ec.SECP256R1())))
        cases.extend((("reject", untrusted, b""), ("bad-secondary", first, untrusted)))
        for index, (mode, primary, secondary) in enumerate(cases):
            p = root / "primary.bin"
            s = root / "secondary.bin"
            p.write_bytes(primary)
            s.write_bytes(secondary)
            result = subprocess.run([str(args.probe.resolve()), mode, str(public), str(p), str(s)],
                                    capture_output=True, text=True, timeout=15)
            if result.returncode != 0:
                raise AssertionError(f"case {index}/{mode}: {result.stdout}\n{result.stderr}")
            print(result.stdout.strip())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
