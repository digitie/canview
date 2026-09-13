"""Typed manifest C/Python 교차 시험. --crypto일 때만 실제 서명을 사용한다."""
from __future__ import annotations

import copy
import random
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools" / "ota"))
from cbor import CborError, Status, encode_document
from envelope import HEADER, MAGIC
from manifest import IMAGE_LIMITS, check_manifest


def fixture(role=1):
    targets = {1: (1, 2), 2: (3,), 3: (4,)}[role]
    return {
        0: 1, 1: bytes(range(16)), 2: role, 3: "synthetic-board", 4: "synthetic-layout",
        5: "synthetic-only", 6: 7, 7: 11,
        8: [{0: target, 1: 32, 2: bytes(range(32)), 3: "test", 4: 0xFFFFFFFFFFFFFFFF,
             5: 2 if target == 2 else 1, 6: 2} for target in targets],
        9: {0: [1, 4], 1: [1, 4], 2: [0, 0xFFFFFFFF], 3: [[1, 1], [2, 1], [1, 2], [2, 2]] if role == 1 else []},
        10: [1, 3, 2], 11: [1, 2, 0xFFFFFFFFFFFFFFFF],
    }


def make_prefix(manifest, sign, *, count=None, total=None):
    encoded = encode_document(manifest)
    images = manifest.get(8, [])
    if count is None:
        count = len(images) if type(images) is list else 1
    if total is None:
        total = HEADER.size + len(encoded) + 64 + sum(
            image.get(1, 32) if type(image) is dict and type(image.get(1, 32)) is int and
            0 <= image.get(1, 32) <= 0xFFFFFFFF else 32 for image in images
        ) if type(images) is list else HEADER.size + len(encoded) + 96
    # 총길이 자체 overflow 사례는 header의 표현 범위 안에서 별도로 검증한다.
    total = min(total, 0xFFFFFFFF)
    return HEADER.pack(MAGIC, 1, HEADER.size, len(encoded), 64, count, total) + encoded + sign(encoded)


def cases(role, sign):
    result = []
    base = fixture(role)

    def add(name, manifest, expected=None, **kwargs):
        result.append((name, make_prefix(manifest, sign, **kwargs), expected))

    def mutate(name, path, value, expected=None, **kwargs):
        manifest = copy.deepcopy(base)
        parent = manifest
        for part in path[:-1]:
            parent = parent[part]
        parent[path[-1]] = value
        add(name, manifest, expected, **kwargs)

    add("valid", base, Status.OK)
    for field in range(12):
        manifest = copy.deepcopy(base)
        del manifest[field]
        add(f"missing-{field}", manifest, Status.MALFORMED, count=len(base[8]))
    mutate("unknown-root", [12], 0, Status.MALFORMED)
    for field in (0, 2, 6, 7):
        mutate(f"integer-overflow-{field}", [field], 1 << 32)
        mutate(f"integer-type-{field}", [field], "1", Status.MALFORMED)
    mutate("wrong-version", [0], 2, Status.UNSUPPORTED_VERSION)
    for value in (0, 4, 0xFFFFFFFF):
        mutate(f"invalid-role-{value}", [2], value, Status.MALFORMED)
    for field, value in ((2, 2 if role == 1 else 1), (3, "other"), (4, "other"), (6, 8), (7, 12)):
        mutate(f"identity-{field}", [field], value, Status.AUTH_FAILED)
    for length in (0, 15, 17):
        mutate(f"package-size-{length}", [1], bytes(length), Status.MALFORMED)
    for field in (3, 4, 5):
        for value in ("", "a" * 64, "\x00", "\x1f", "\x7f", "한글", b"text"):
            mutate(f"text-{field}-{value!r}", [field], value, Status.MALFORMED)
    mutate("release63", [5], "x" * 63, Status.OK)
    mutate("empty-images", [8], [], count=1)
    mutate("images-map", [8], {}, Status.MALFORMED)
    mutate("too-many-role-images", [8], base[8] * 2, count=len(base[8]) * 2)
    for index, image in enumerate(base[8]):
        for field in range(7):
            manifest = copy.deepcopy(base)
            del manifest[8][index][field]
            add(f"image-{index}-missing-{field}", manifest)
        mutate(f"unknown-image-{index}", [8, index, 7], 0, Status.MALFORMED)
        for target in (0, 5, 0xFFFFFFFF):
            mutate(f"target-{index}-{target}", [8, index, 0], target, Status.MALFORMED)
        wrong_target = 4 if role != 3 else 3
        mutate(f"wrong-target-{index}", [8, index, 0], wrong_target, Status.MALFORMED)
        for length in (0, 1, IMAGE_LIMITS[image[0]], IMAGE_LIMITS[image[0]] + 1, 0xFFFFFFFF, 1 << 32):
            mutate(f"length-{index}-{length}", [8, index, 1], length)
        for length in (0, 31, 33):
            mutate(f"digest-{index}-{length}", [8, index, 2], bytes(length), Status.MALFORMED)
        for value in (0, 1, (1 << 53) - 1, 1 << 53, (1 << 63), (1 << 64) - 1):
            mutate(f"sequence-{index}-{value}", [8, index, 4], value, Status.OK)
        mutate(f"sequence-text-{index}", [8, index, 4], "18446744073709551615", Status.MALFORMED)
        for value in (0, 3, 2 if image[5] == 1 else 1):
            mutate(f"signature-kind-{index}-{value}", [8, index, 5], value, Status.MALFORMED)
        mutate(f"abi-overflow-{index}", [8, index, 6], 1 << 32, Status.OVERSIZE)
        for value in (0, 5):
            mutate(f"abi-outside-range-{index}-{value}", [8, index, 6], value, Status.MALFORMED)
    if role == 1:
        mutate("duplicate-target", [8, 1], base[8][0], Status.DUPLICATE)
        for target in base[8]:
            mutate(f"single-target-{target[0]}", [8], [target], Status.OK)
        mutate("missing-combinations", [9, 3], [], Status.MALFORMED)
        mutate("duplicate-combination", [9, 3], [[1, 1], [1, 1]], Status.DUPLICATE)
        mutate("combination-outside-range", [9, 3], [[5, 1]], Status.MALFORMED)
        mutate("max-combinations", [9, 3], [[a, b] for a in range(1, 5) for b in range(1, 5)], Status.OK)
        mutate("too-many-combinations", [9, 3], [[1, 1]] * 17, Status.OVERSIZE)
    else:
        mutate("forbidden-combinations", [9, 3], [[1, 1]], Status.MALFORMED)
    for index in range(3):
        for value in ([2, 1], [0], [0, 1, 2], "range"):
            mutate(f"range-{index}-{value}", [9, index], value, Status.MALFORMED)
    for value in ([2, 1, 2], [1, 3, 4], [1, 3, 0], [1, 3], [1, 3, 1 << 32]):
        mutate(f"config-{value}", [10], value)
    mutate("requires-extra", [11], [1, 2, 3, 4], Status.MALFORMED)
    mutate("requires-boot-overflow", [11, 0], 1 << 32, Status.OVERSIZE)
    mutate("requires-capability-type", [11, 2], "1", Status.MALFORMED)
    add("header-count-mismatch", base, Status.MALFORMED, count=3 if role == 1 else 2)
    valid = make_prefix(base, sign)
    encoded = encode_document(base)
    duplicate = encoded.replace(b"\xa7\x00", b"\xa7\x01", 1)
    assert duplicate != encoded
    result.append(("duplicate-image-key", valid[:HEADER.size] + duplicate + sign(duplicate), Status.DUPLICATE))
    for delta in (-1, 1):
        add(f"header-total-mismatch-{delta}", base, Status.MALFORMED, total=HEADER.unpack_from(valid)[-1] + delta)
    for size in range(len(valid)):
        result.append((f"truncated-{size}", valid[:size], Status.INCOMPLETE))
    result.append(("trailing-byte", valid + b"\x00", Status.MALFORMED))
    rng = random.Random(7002 + role)
    for number in range(50):
        manifest = copy.deepcopy(base)
        for image in manifest[8]:
            image[1] = rng.randint(1, IMAGE_LIMITS[image[0]])
            image[4] = rng.getrandbits(64)
        add(f"random-valid-{number}", manifest, Status.OK)
    return result


def main():
    crypto = len(sys.argv) == 3 and sys.argv[2] == "--crypto"
    public = b""
    sign = lambda message: bytes(64)
    verify = lambda message, signature: None
    if crypto:
        import cryptography
        from cryptography.exceptions import InvalidSignature
        from cryptography.hazmat.primitives import hashes
        from cryptography.hazmat.primitives.asymmetric import ec, utils
        assert cryptography.__version__ == "48.0.0"
        key = ec.generate_private_key(ec.SECP256R1())
        point = key.public_key().public_numbers()
        public = point.x.to_bytes(32, "big") + point.y.to_bytes(32, "big")

        def sign(message):
            r, s = utils.decode_dss_signature(key.sign(message, ec.ECDSA(hashes.SHA256(), deterministic_signing=True)))
            return r.to_bytes(32, "big") + s.to_bytes(32, "big")

        def verify(message, signature):
            r, s = int.from_bytes(signature[:32], "big"), int.from_bytes(signature[32:], "big")
            try:
                key.public_key().verify(utils.encode_dss_signature(r, s), message, ec.ECDSA(hashes.SHA256()))
            except InvalidSignature:
                raise CborError(Status.AUTH_FAILED) from None

    total_cases = 0
    for role in (1, 2, 3):
        vectors = cases(role, sign)
        if crypto:
            good = make_prefix(fixture(role), sign)
            vectors.append(("invalid-signature", good[:-1] + bytes([good[-1] ^ 1]), Status.AUTH_FAILED))
        wire = b"".join(struct.pack("<I", len(data)) + public + data for _, data, _ in vectors)
        run = subprocess.run([sys.argv[1], str(role)], input=wire, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
        assert run.returncode == 0, run.stderr.decode(errors="replace")
        lines = run.stdout.decode("ascii").splitlines()
        assert len(lines) == len(vectors), (len(lines), len(vectors), run.stderr)
        for (name, data, expected), line in zip(vectors, lines):
            try:
                view = check_manifest(data, (role, "synthetic-board", "synthetic-layout", 7, 11), verify)
                status = Status.OK
            except CborError as error:
                status = error.status
            if expected is not None:
                assert status == expected, (role, name, status, expected)
            values = list(map(int, line.split()))
            assert values[0] == status, (role, name, values[0], status)
            if not crypto:
                if status == Status.OK:
                    images = view["manifest"][8]
                    expected_values = [0, len(images), view["total_size"]]
                    for image, offset in zip(images, view["offsets"]):
                        expected_values += [image[0], offset, image[1], image[4]]
                    assert values == expected_values, (name, values, expected_values)
                else:
                    assert values == [int(status), 0, 0], (name, values)
        total_cases += len(vectors)
    print(f"typed manifest {'real P256' if crypto else 'mock signature'} C/Python cases: {total_cases}")


if __name__ == "__main__":
    main()
