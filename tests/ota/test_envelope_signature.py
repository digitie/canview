"""실제 Python 서명과 Windows CNG 검증. 임시 개인키는 메모리 밖으로 쓰지 않는다."""
from pathlib import Path
import struct
import subprocess
import sys

import cryptography
from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec, utils

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "ota"))
from cbor import CborError, Status
from envelope import HEADER, MAX_BUNDLE, MAX_BYTES, MAX_IMAGE_BYTES, SIGNATURE_BYTES, assemble, check_prefix, prefix_length


def main() -> int:
    if cryptography.__version__ != "48.0.0":
        raise RuntimeError("cryptography==48.0.0 required; install tools/requirements-ota.lock")
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_envelope_signature.py <Windows-C-probe>")
    private = ec.generate_private_key(ec.SECP256R1())
    public = private.public_key()
    numbers = public.public_numbers()
    public_bytes = numbers.x.to_bytes(32, "big") + numbers.y.to_bytes(32, "big")

    def sign(message):
        der = private.sign(message, ec.ECDSA(hashes.SHA256(), deterministic_signing=True))
        r, s = utils.decode_dss_signature(der)
        return r.to_bytes(32, "big") + s.to_bytes(32, "big")

    def verify(message, signature):
        r, s = int.from_bytes(signature[:32], "big"), int.from_bytes(signature[32:], "big")
        public.verify(utils.encode_dss_signature(r, s), message, ec.ECDSA(hashes.SHA256()))

    # 합성 문서/이미지다. 아직 최종 manifest schema 또는 부팅 가능한 이미지가 아니다.
    manifest = {0: 1, 1: bytes(range(16)), 2: "synthetic-envelope-only", 3: (1 << 64) - 1}
    images = [b"CANVIEW SYNTHETIC IMAGE\x00"]
    package = assemble(manifest, images, sign)
    if package != assemble(manifest, images, sign):
        raise AssertionError("same manifest/images/key did not reproduce bytes")
    prefix = package[:prefix_length(package)]
    if check_prefix(prefix, verify) != manifest:
        raise AssertionError("manifest roundtrip mismatch")
    if HEADER.unpack_from(package)[-1] != len(package):
        raise AssertionError("declared total mismatch")
    mutable_images = [b"snapshot"]

    def mutating_sign(message):
        mutable_images[0] = b"changed after length calculation"
        return sign(message)

    snapshot = assemble(manifest, mutable_images, mutating_sign)
    if not snapshot.endswith(b"snapshot") or HEADER.unpack_from(snapshot)[-1] != len(snapshot):
        raise AssertionError("signer changed image snapshot")
    for bad_images, expected in (([], Status.MALFORMED), ([b"x"] * 4, Status.MALFORMED),
                                 ([b""], Status.MALFORMED), (["x"], Status.MALFORMED),
                                 ([bytes(MAX_IMAGE_BYTES + 1)], Status.OVERSIZE)):
        try:
            assemble(manifest, bad_images, sign)
        except CborError as exc:
            if exc.status != expected:
                raise AssertionError("wrong assembly reject") from exc
        else:
            raise AssertionError("invalid image accepted")
    try:
        assemble(manifest, images, lambda _: bytes(63))
    except CborError as exc:
        if exc.status != Status.MALFORMED:
            raise AssertionError("wrong signature-size reject") from exc
    else:
        raise AssertionError("short signature accepted")
    cases = [(public_bytes, prefix, Status.OK)]
    cases.extend((public_bytes, prefix[:size], Status.INCOMPLETE) for size in range(len(prefix)))
    cases.append((public_bytes, prefix + b"x", Status.MALFORMED))
    for offset, fmt, value, expected in (
        (0, "B", 0, Status.MALFORMED),
        (8, "H", 2, Status.UNSUPPORTED_VERSION),
        (10, "H", 23, Status.MALFORMED),
        (12, "I", 0, Status.MALFORMED),
        (12, "I", MAX_BYTES + 1, Status.OVERSIZE),
        (12, "I", 0xFFFFFFFF, Status.OVERSIZE),
        (16, "H", 63, Status.MALFORMED),
        (16, "H", 65, Status.MALFORMED),
        (18, "H", 0, Status.MALFORMED),
        (18, "H", 4, Status.MALFORMED),
        (20, "I", len(prefix), Status.MALFORMED),
        (20, "I", MAX_BUNDLE + 1, Status.OVERSIZE),
    ):
        changed = bytearray(prefix)
        struct.pack_into("<" + fmt, changed, offset, value)
        cases.append((public_bytes, bytes(changed), expected))
    for index in range(SIGNATURE_BYTES):
        changed = bytearray(prefix)
        changed[len(prefix) - SIGNATURE_BYTES + index] ^= 1
        cases.append((public_bytes, bytes(changed), Status.AUTH_FAILED))
    changed = bytearray(prefix)
    changed[HEADER.size + 6] ^= 1  # package_id byte 변경; CBOR 구조는 동일.
    cases.append((public_bytes, bytes(changed), Status.AUTH_FAILED))
    wrong = ec.generate_private_key(ec.SECP256R1()).public_key().public_numbers()
    cases.append((wrong.x.to_bytes(32, "big") + wrong.y.to_bytes(32, "big"), prefix, Status.AUTH_FAILED))
    cases.append((bytes(64), prefix, Status.AUTH_FAILED))
    cases.append((public_bytes, bytes(HEADER.size + MAX_BYTES + SIGNATURE_BYTES + 1), Status.OVERSIZE))
    for bad_cbor in (b"\x00", b"\xa1\x60\x00", b"\xa2\x00\x00\x00\x00"):
        signed = (HEADER.pack(b"CVOTA001", 1, HEADER.size, len(bad_cbor), SIGNATURE_BYTES,
                              1, HEADER.size + len(bad_cbor) + SIGNATURE_BYTES + 1)
                  + bad_cbor + sign(bad_cbor))
        expected = Status.DUPLICATE if bad_cbor[0] == 0xA2 else Status.MALFORMED
        cases.append((public_bytes, signed, expected))
    maximum = assemble({0: bytes(16379)}, images, sign)
    cases.append((public_bytes, maximum[:prefix_length(maximum)], Status.OK))
    for key, data, expected in cases:
        if key != public_bytes:
            continue
        try:
            check_prefix(data, verify)
            actual = Status.OK
        except CborError as exc:
            actual = exc.status
        except InvalidSignature:
            actual = Status.AUTH_FAILED
        if actual != expected:
            raise AssertionError(("Python status", len(data), actual, expected))
    wire = b"".join(struct.pack("<I", len(data)) + key + data for key, data, _ in cases)
    result = subprocess.run([sys.argv[1]], input=wire, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, check=True, timeout=45)
    actual = [int(line) for line in result.stdout.splitlines()]
    if actual != [int(expected) for _, _, expected in cases]:
        raise AssertionError(("C status mismatch", actual))
    print(f"PASS: {len(cases)} real P256 signature/prefix cases; ephemeral keys, no device writes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
