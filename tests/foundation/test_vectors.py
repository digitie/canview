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
    for count in range(13):
        for variant in range(16):
            packet = struct.pack("<QBBH", 0x123456789ABC0000 + variant * 0x123,
                                 count, variant * 13, 0)
            for i in range(count):
                flags, dlc = (i + variant) % 16, (i + variant) % 9
                can_id = 0x1234567 + i * 257 if flags & 1 else 0x321 + i * 7
                data = bytes((17 + i * 29 + j * 37 + variant) % 256
                             if j < dlc and not flags & 2 else 0 for j in range(8))
                packet += struct.pack("<HBBI8s", 0x102 + i * 0x101, (i + variant) % 3,
                                      flags * 16 + dlc, can_id, data)
            expected.append(packet.hex())
    if len(actual) != 2403 or actual != expected:
        raise AssertionError("independent golden frame/COBS/CAN mismatch")
    print("PASS: 2195 envelope/COBS + 208 independent CAN batch vectors")


if __name__ == "__main__":
    main()
