"""실제 ESP-IDF fixture BIN의 고정 custom descriptor 검사. 서명/부팅 시험이 아니다."""
from pathlib import Path
import argparse
import hashlib
import struct

APP_OFFSET = 24 + 8  # SDK image header + 첫 segment header
APP_BYTES = 256
VERSION_OFFSET = 16
VERSION_BYTES = 32
METADATA_BYTES = 168
IMAGE_BYTES_MAX = 4 * 1024 * 1024


def check(data: bytes) -> None:
    if len(data) < APP_OFFSET + APP_BYTES + METADATA_BYTES or len(data) > IMAGE_BYTES_MAX:
        raise ValueError("fixture image length")
    if struct.unpack_from("<I", data, APP_OFFSET)[0] != 0xABCD5432:
        raise ValueError("SDK app descriptor offset/magic")
    version = data[APP_OFFSET + VERSION_OFFSET:APP_OFFSET + VERSION_OFFSET + VERSION_BYTES]
    if version != b"1.2.3+4".ljust(VERSION_BYTES, b"\0"):
        raise ValueError("fixture app version")
    expected = struct.pack("<8sHH5IQ", b"CVIMG001", 1, METADATA_BYTES, 1, 1, 7, 2, 0, (1 << 64) - 1)
    expected += b"synthetic-board".ljust(64, b"\0") + b"synthetic-layout".ljust(64, b"\0")
    start = APP_OFFSET + APP_BYTES
    if data[start:start + METADATA_BYTES] != expected:
        raise ValueError("native metadata bytes/offset/u64 drift")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    args = parser.parse_args()
    with args.image.open("rb") as source:
        data = source.read(IMAGE_BYTES_MAX + 1)
    check(data)
    # 실제 생성물의 metadata 전 byte 변이·절단이 reject되는지 검사한다.
    start = APP_OFFSET + APP_BYTES
    for position in range(start, start + METADATA_BYTES):
        changed = bytearray(data)
        changed[position] ^= 1
        try:
            check(bytes(changed))
        except ValueError:
            continue
        raise AssertionError(f"metadata mutation accepted: {position}")
    for size in (0, APP_OFFSET - 1, start, start + METADATA_BYTES - 1):
        try:
            check(data[:size])
        except ValueError:
            continue
        raise AssertionError(f"truncated descriptor accepted: {size}")
    print(f"PASS: SDK BIN descriptor/u64; 168 mutations + 4 truncations rejected; SHA256={hashlib.sha256(data).hexdigest()}")
    print("Native signature/boot/physical/HIL: NOT_RUN")


if __name__ == "__main__":
    main()
