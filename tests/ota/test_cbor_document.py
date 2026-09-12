"""독립 Python/C CBOR profile differential·boundary·mutation 시험."""
from __future__ import annotations

from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "ota"))
from cbor import CborError, MAX_BYTES, Status, decode_document, encode_document


def status(data: bytes) -> Status:
    try:
        decode_document(data)
    except CborError as exc:
        return exc.status
    return Status.OK


def vectors() -> list[bytes]:
    expected = [
        (b"", Status.INCOMPLETE), (b"\xa0", Status.OK),
        (b"\x00", Status.MALFORMED), (b"\xa0\x00", Status.MALFORMED),
        (b"\xa1\x60\x00", Status.MALFORMED),
        (b"\xa2\x00\x01\x00\x02", Status.DUPLICATE),
        (b"\xa2\x01\x01\x00\x02", Status.MALFORMED),
        (b"\xa1\x00\x18\x17", Status.MALFORMED),
        (b"\xa1\x00\x5b" + b"\xff" * 8, Status.OVERSIZE),
        (b"\xa1\x00\x9b" + b"\xff" * 8, Status.OVERSIZE),
        (b"\xa1\x00\xbb" + b"\xff" * 8, Status.OVERSIZE),
        (b"\xa1\x00\x99\x07\xfd" + b"\x00" * 2045, Status.OK),
        (b"\xa1\x00\x99\x07\xfe" + b"\x00" * 2046, Status.OVERSIZE),
        (b"\xa1\x00\x59\x3f\xfb" + bytes(16379), Status.OK),
        (bytes(MAX_BYTES + 1), Status.OVERSIZE),
    ]
    for depth in range(1, 11):
        expected.append((b"\xa1\x00" * depth + b"\x00",
                         Status.OK if depth <= 8 else Status.OVERSIZE))
    valid_utf8 = ("00", "7f", "c280", "dfbf", "e0a080", "ed9fbf", "ee8080",
                  "efbfbf", "f0908080", "f48fbfbf", "ed959ceab880")
    invalid_utf8 = ("80", "bf", "c0af", "c1bf", "c2", "c220", "df7f", "e08080",
                    "e0a0", "eda080", "edbfbf", "f0808080", "f4908080", "f5808080",
                    "f09080", "ff", "fe", "e228a1")
    for values, result in ((valid_utf8, Status.OK), (invalid_utf8, Status.MALFORMED)):
        for value in values:
            raw = bytes.fromhex(value)
            expected.append((b"\xa1\x00" + bytes([0x60 | len(raw)]) + raw, result))
    for encoded, result in expected:
        actual = status(encoded)
        if actual != result:
            raise AssertionError((encoded[:32].hex(), result, actual))
    output = [encoded for encoded, _ in expected]
    # root / map key / map value 각각에서 첫 byte의 전체 공간을 순회한다.
    for first in range(256):
        for prefix in (b"", b"\xa1", b"\xa1\x00"):
            for tail in (b"", bytes(8)):
                output.append(prefix + bytes([first]) + tail)
    rng = random.Random(7001)

    def value(depth=0):
        kind = rng.randrange(5 if depth < 4 else 3)
        if kind == 0:
            return rng.choice((0, 23, 24, 255, 256, 65535, 65536, (1 << 32) - 1,
                               1 << 32, (1 << 64) - 1))
        if kind == 1:
            return rng.randbytes(rng.randrange(40))
        if kind == 2:
            return rng.choice(("", "OTA", "한글", "\x00", "\U0010ffff"))
        if kind == 3:
            return [value(depth + 1) for _ in range(rng.randrange(4))]
        return {key: value(depth + 1) for key in rng.sample(range(100), rng.randrange(4))}

    for _ in range(150):
        original = {key: value() for key in rng.sample(range(200), rng.randrange(1, 6))}
        encoded = encode_document(original)
        if decode_document(encoded) != original:
            raise AssertionError("semantic roundtrip mismatch")
        if encode_document(decode_document(encoded)) != encoded:
            raise AssertionError("non-deterministic encoding")
        output.append(encoded)
        output.extend(encoded[:length] for length in range(len(encoded)))
        for _ in range(8):
            mutated = bytearray(encoded)
            position = rng.randrange(len(mutated))
            mutated[position] ^= 1 << rng.randrange(8)
            output.append(bytes(mutated))
    output.extend(rng.randbytes(rng.randrange(80)) for _ in range(1000))
    for invalid in (True, -1, 1 << 64, None, 1.2, {"x": 0}, {True: 0}, "\ud800"):
        try:
            encode_document({0: invalid})
        except CborError:
            pass
        else:
            raise AssertionError("encoder accepted unsupported value")
    cyclic = []
    cyclic.append(cyclic)
    for invalid in (bytes(MAX_BYTES + 1), "한" * MAX_BYTES,
                    [0] * 2046, cyclic):
        try:
            encode_document({0: invalid})
        except CborError as exc:
            if exc.status != Status.OVERSIZE:
                raise AssertionError("wrong resource limit status") from exc
        else:
            raise AssertionError("encoder resource limit not enforced")
    exact = {0: bytes(16379)}
    if len(encode_document(exact)) != MAX_BYTES:
        raise AssertionError("exact byte limit rejected")
    return output


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_cbor_document.py <C-probe>")
    data = vectors()
    wire = b"".join(struct.pack("<I", len(item)) + item for item in data)
    result = subprocess.run([sys.argv[1]], input=wire, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=45, check=True)
    actual = [int(line) for line in result.stdout.splitlines()]
    if len(actual) != len(data):
        raise AssertionError("C probe result count mismatch")
    for index, (item, observed) in enumerate(zip(data, actual)):
        wanted = status(item)
        if observed != wanted:
            raise AssertionError((index, item[:80].hex(), wanted, observed))
    print(f"PASS: {len(data)} C/Python CBOR cases, boundary/UTF-8/depth/items/prefix/mutation")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
