"""생성기/C 구현을 읽지 않는 독립 struct+zlib 참조. 모든 payload 길이 대조."""
import struct
import subprocess
import sys
import zlib


def cobs(data):
    result = bytearray()
    block = bytearray()
    for value in data:
        if value == 0:
            result.append(len(block) + 1)
            result.extend(block)
            block.clear()
        else:
            block.append(value)
            if len(block) == 254:
                result.append(255)
                result.extend(block)
                block.clear()
    result.append(len(block) + 1)
    result.extend(block)
    return bytes(result) + b"\0"


def main():
    actual = subprocess.check_output([sys.argv[1]], text=True).splitlines()
    expected = []
    for transport, limit in ((0, 208), (1, 992)):
        for size in range(limit + 1):
            payload = bytes(index * 37 % 256 for index in range(size))
            if transport == 0:
                header = struct.pack("<H6B4IHHI", 0x5643, 1, 3, 32, 0x20, 1, 4,
                                     0xAABBCCDD, 0x89ABCDEF, 0x11223344, 0x12345678, size, 0, 0)
            else:
                header = struct.pack("<H4B3H2IQI", 0x5543, 1, 0, 0x20, 1, 32, size, 0,
                                     0x89ABCDEF, 0x12345678, 0x1122334455667788, 0)
            packet = header[:28] + struct.pack("<I", zlib.crc32(header + payload)) + payload
            expected.append(packet.hex())
            if transport:
                expected.append(cobs(packet).hex())
    if len(actual) != 2195 or actual != expected:
        raise AssertionError("independent golden frame/COBS mismatch")
    print("PASS: 2195 independent Python/C vectors (all legal payload lengths)")


if __name__ == "__main__":
    main()
