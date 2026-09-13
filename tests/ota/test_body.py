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
VERSION, CAPABILITY = 4, 5
LOCAL = (1, 1, 1, 1, 1, 2, 2, 1, 0xFFFFFFFFFFFFFFFF)


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

        def add(name, prefix, data, chunk=31, expected=OK, scenario=0, fault=0, root=public, local=LOCAL, policy=0):
            wire = (struct.pack("<7I", role, len(prefix), len(data), chunk, scenario, fault, policy) +
                    struct.pack("<8IQ", *local) + root + prefix + data)
            vectors.append((f"{role}-{name}", wire, expected, fault))

        prefix, data, manifest = prepare(blobs)
        # 실제 body 경로의 floor 실패는 첫 hash operation 전에 거절해야 한다.
        add("policy-unavailable", prefix, data, policy=1, expected=INCOMPLETE)
        add("policy-floor-conflict", prefix, data, policy=2, expected=AUTH)
        add("policy-wrong-board", prefix, data, policy=3, expected=AUTH)
        add("policy-record-missing", prefix, data, policy=4, expected=INCOMPLETE)
        add("policy-null", prefix, data, policy=5, expected=INVALID)
        older = copy.deepcopy(manifest)
        for image in older[8]:
            image[4] -= 1
        add("below-floor-u64", make_prefix(older, sign), data, policy=2, expected=STALE)
        # 서명된 후보와 trusted 로컬 snapshot을 실제 body 시작 경로에서 비교한다.
        for field in range(8):
            for value in (0, 1, 2, 3, 4, 5, 0xFFFFFFFF):
                if field == 0 and value not in (0, 1):
                    continue
                local = list(LOCAL)
                local[field] = value
                expected = OK
                if field == 0 and value == 0:
                    expected = INCOMPLETE
                elif field in (1, 2) and (field == 1 or role == 1):
                    expected = OK if value in ((1, 2) if role == 1 else (1, 2, 3, 4)) else VERSION
                elif field in (3, 4) and (field == 3 or role == 1):
                    expected = OK if value >= 1 else VERSION
                elif field in (5, 6) and (field == 5 or role == 1):
                    expected = OK if value >= 2 else VERSION
                elif field == 7:
                    expected = OK if 1 <= value <= 3 else VERSION
                add(f"runtime-{field}-{value}", prefix, data, local=local, expected=expected)
        for bit in range(64):
            local = (*LOCAL[:-1], LOCAL[-1] ^ (1 << bit))
            add(f"missing-capability-{bit}", prefix, data, local=local, expected=CAPABILITY)
            relaxed = copy.deepcopy(manifest)
            relaxed[11][2] = 1 << bit
            add(f"required-capability-{bit}", make_prefix(relaxed, sign), data,
                local=(*LOCAL[:-1], 1 << bit))
        relaxed = copy.deepcopy(manifest)
        relaxed[11] = [0, 0, 0]
        add("zero-requires", make_prefix(relaxed, sign), data, local=(1, 1, 1, 0, 0, 0, 0, 1, 0))
        # Peer ABI is not a local installation prerequisite; peers may be absent.
        relaxed[9][2] = [0xFFFFFFFF, 0xFFFFFFFF]
        add("no-peer-install-dependency", make_prefix(relaxed, sign), data)
        maximum = copy.deepcopy(manifest)
        maximum[11] = [0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF]
        add("maximum-requires", make_prefix(maximum, sign), data,
            local=(1, 1, 1, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 1, LOCAL[-1]))
        for abi in (0, 0xFFFFFFFF):
            edge = copy.deepcopy(manifest)
            edge[9][0] = [abi, abi]
            edge[9][1] = [abi, abi]
            edge[9][3] = [[abi, abi]] if role == 1 else []
            for image in edge[8]:
                image[6] = abi
            add(f"abi-edge-{abi}", make_prefix(edge, sign), data,
                local=(1, abi, abi, *LOCAL[3:]))
        if role == 1:
            reversed_images = copy.deepcopy(manifest)
            reversed_images[8].reverse()
            reversed_images[9][3].reverse()
            add("reversed-image-and-pair-order", make_prefix(reversed_images, sign), b"".join(reversed(blobs)))
            all_pairs = copy.deepcopy(manifest)
            all_pairs[9][3] = [[esp, stm] for esp in range(1, 5) for stm in range(1, 5)]
            add("maximum-pair-list", make_prefix(all_pairs, sign), data, local=(1, 4, 4, *LOCAL[3:]))
            for pair_index in range(4):
                missing = copy.deepcopy(manifest)
                del missing[9][3][pair_index]
                add(f"missing-transition-pair-{pair_index}", make_prefix(missing, sign), data, expected=VERSION)
            for image_index in (0, 1):
                single = copy.deepcopy(manifest)
                single[8] = [single[8][image_index]]
                single[9][3] = [[1, 1], [2, 1]] if image_index == 0 else [[1, 1], [1, 2]]
                add(f"single-image-retain-other-{image_index}", make_prefix(single, sign), blobs[image_index])
                del single[9][3][1]
                add(f"single-image-missing-transition-{image_index}", make_prefix(single, sign),
                    blobs[image_index], expected=VERSION)
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
