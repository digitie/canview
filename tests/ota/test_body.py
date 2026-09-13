"""합성 image의 순차 길이/hash·reset·오류 시험. native image 서명 검증은 아니다."""
from __future__ import annotations

import copy
import hashlib
import struct
import subprocess
import sys

from test_manifest import fixture, make_prefix
from manifest import IMAGE_LIMITS

OK, INVALID, MALFORMED, OVERSIZE, INCOMPLETE, DUPLICATE, STALE, AUTH, BUSY = 0, 1, 3, 7, 8, 9, 10, 12, 13


def main():
    crypto = len(sys.argv) == 3 and sys.argv[2] == "--crypto"
    vectors = []
    if crypto:
        import cryptography
        from cryptography.hazmat.primitives import hashes
        from cryptography.hazmat.primitives.asymmetric import ec, utils
        assert cryptography.__version__ == "48.0.0"

    def digest(data):
        if crypto:
            return hashlib.sha256(data).digest()
        # 모형 oracle만 위한 sum/length. SHA256 검증으로 집계하지 않는다.
        return struct.pack("<II", sum(data) & 0xFFFFFFFF, len(data)) + bytes(24)

    for role in (1, 2, 3):
        if crypto:
            key = ec.generate_private_key(ec.SECP256R1())
            point = key.public_key().public_numbers()
            public = point.x.to_bytes(32, "big") + point.y.to_bytes(32, "big")

            def sign(message):
                r, s = utils.decode_dss_signature(key.sign(message, ec.ECDSA(hashes.SHA256(), deterministic_signing=True)))
                return r.to_bytes(32, "big") + s.to_bytes(32, "big")
        else:
            public = bytes(64)
            sign = lambda message: bytes(64)
        base = fixture(role)
        blobs = [bytes(range(96))] + ([b"synthetic STM\x00\xff\x1a\x0d\x0a"] if role == 1 else [])

        def prepare(payloads, template=None):
            manifest = copy.deepcopy(base if template is None else template)
            assert len(payloads) == len(manifest[8])
            for image, payload in zip(manifest[8], payloads):
                image[1] = len(payload)
                image[2] = digest(payload)
            return make_prefix(manifest, sign), b"".join(payloads), manifest

        def add(name, prefix, data, chunk=31, expected=OK, scenario=0, fault=0, root=public):
            wire = struct.pack("<6I", role, len(prefix), len(data), chunk, scenario, fault) + root + prefix + data
            vectors.append((f"{role}-{name}", wire, expected, fault))

        prefix, data, manifest = prepare(blobs)
        add("truncated-prefix", prefix[:-1], data, expected=INCOMPLETE)
        wrong_identity = copy.deepcopy(manifest)
        wrong_identity[3] = "other-board"
        add("wrong-identity-before-hash", make_prefix(wrong_identity, sign), data, expected=AUTH)
        for chunk in (*range(1, 66), 95, 96, 97, 127, 128, 129, 16384):
            add(f"chunk-{chunk}", prefix, data, chunk)
        for end in range(len(data)):
            add(f"truncated-body-{end}", prefix, data[:end], expected=INCOMPLETE)
        for offset in range(len(data)):
            changed = bytearray(data)
            changed[offset] ^= 1
            add(f"body-byte-mutation-{offset}", prefix, bytes(changed), expected=AUTH)
        for target_index in range(len(blobs)):
            mutated = copy.deepcopy(manifest)
            mutated[8][target_index][2] = bytes(32)
            add(f"signed-wrong-digest-{target_index}", make_prefix(mutated, sign), data, expected=AUTH)
        add("trailing-body", prefix, data + b"x", chunk=len(data), expected=OVERSIZE)
        add("duplicate-offset", prefix, data, expected=DUPLICATE, scenario=1)
        add("missing-offset", prefix, data, expected=MALFORMED, scenario=2)
        add("reset-restart", prefix, data, scenario=3)
        add("reset-abandon-partial", prefix, data, expected=INCOMPLETE, scenario=4)
        add("null-data", prefix, data, expected=INVALID, scenario=5)
        add("size-max", prefix, data, expected=OVERSIZE, scenario=6)
        add("reset-partial-reject-old-stream", prefix, data, expected=STALE, scenario=7)
        add("explicit-reset-failure-retry", prefix, data, expected=BUSY, scenario=3, fault=4)
        add("truncated-and-cleanup-failure", prefix, blobs[0][:-1], expected=INCOMPLETE, fault=4,
            chunk=len(data))
        add("bad-hash-and-cleanup-failure", prefix, bytes([data[0] ^ 1]) + data[1:], expected=AUTH,
            fault=4, chunk=len(data))
        for fault in range(1, 5):
            add(f"provider-error-{fault}", prefix, data, expected=BUSY, fault=fault)
            if role == 1:
                add(f"second-image-provider-error-{fault}", prefix, data, expected=BUSY, fault=(1 << 8) | fault,
                    chunk=len(blobs[0]))
        small = [b"a"] * len(blobs)
        p, b, _ = prepare(small)
        add("one-byte-images-one-chunk", p, b, chunk=16384)
        p, b, _ = prepare([b"abc"] * len(blobs))
        add("sha256-abc", p, b, chunk=2)
        large = [bytes(range(256)) * (IMAGE_LIMITS[image[0]] // 256) for image in base[8]]
        p, b, _ = prepare(large)
        add("slot-maximum", p, b, chunk=16384)
        p, b, _ = prepare([b"z" * 20000 for _ in blobs])
        add("oversized-chunk", p, b, chunk=16385, expected=OVERSIZE)
        if role == 1:
            p, b, _ = prepare([b"a" * 32, b"b" * 32])
            add("swapped-images", p, b[32:] + b[:32], chunk=64, expected=AUTH)
        if crypto:
            add("manifest-bad-signature", prefix[:-1] + bytes([prefix[-1] ^ 1]), data, expected=AUTH)
            other = ec.generate_private_key(ec.SECP256R1()).public_key().public_numbers()
            add("wrong-role-root", prefix, data, expected=AUTH,
                root=other.x.to_bytes(32, "big") + other.y.to_bytes(32, "big"))

    wire = b"".join(vector[1] for vector in vectors)
    assert hashlib.sha256(b"abc").hexdigest() == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
    result = subprocess.run([sys.argv[1]], input=wire, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
    assert result.returncode == 0, (result.returncode, result.stderr.decode(errors="replace"), result.stdout[-200:])
    lines = result.stdout.decode("ascii").splitlines()
    assert len(lines) == len(vectors), (len(lines), len(vectors))
    for (name, _, expected, fault), line in zip(vectors, lines):
        status, state, error, cleanup, live, reset = map(int, line.split())
        assert status == expected, (name, line, expected)
        assert reset == OK, (name, line)
        if expected == OK:
            assert (state, error, cleanup, live) == (2, OK, OK, 0), (name, line)
        else:
            assert state == 3 and error == expected, (name, line)
            if (fault & 0xFF) == 4:
                assert cleanup == BUSY and live == 1, (name, line)
            else:
                assert cleanup == OK and live == 0, (name, line)
    print(f"body stream {'P256 + SHA256' if crypto else 'mock-only lifecycle'} cases: {len(vectors)}")


if __name__ == "__main__":
    main()
