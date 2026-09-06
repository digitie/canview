#!/usr/bin/env python3
"""Deterministic duplex UART framing fault simulator.

This is a virtual-time host test. It exercises COBS delimiter recovery and
CRC/length rejection; it is not a physical 4 Mbps, RTS/CTS or board soak test.
"""

from __future__ import annotations

import argparse
import random
import struct
import zlib
from dataclasses import dataclass


HEADER_SIZE = 32
MAX_FRAME = 1024
MAX_PAYLOAD = MAX_FRAME - HEADER_SIZE
MAX_ENCODED = 1029
DELIMITER = 0
MAGIC = b"CU"


def cobs_encode(data: bytes) -> bytes:
    output = bytearray((1,))
    code_at = 0
    code = 1
    for byte in data:
        if byte == 0:
            output[code_at] = code
            code_at = len(output)
            output.append(0)
            code = 1
        else:
            output.append(byte)
            code += 1
            if code == 0xFF:
                output[code_at] = code
                code_at = len(output)
                output.append(0)
                code = 1
    output[code_at] = code
    return bytes(output)


def cobs_decode(encoded: bytes) -> bytes:
    if not encoded or len(encoded) > MAX_ENCODED:
        raise ValueError("encoded boundary")
    output = bytearray()
    cursor = 0
    while cursor < len(encoded):
        code = encoded[cursor]
        cursor += 1
        if code == 0 or cursor + code - 1 > len(encoded):
            raise ValueError("malformed COBS")
        output.extend(encoded[cursor:cursor + code - 1])
        cursor += code - 1
        if code != 0xFF and cursor < len(encoded):
            output.append(0)
    if len(output) > MAX_FRAME:
        raise ValueError("decoded boundary")
    return bytes(output)


def packet(sequence: int, payload: bytes, message_type: int = 3) -> bytes:
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("payload boundary")
    header = bytearray(HEADER_SIZE)
    header[0:2] = MAGIC
    header[2:4] = b"\x01\x00"
    header[4] = message_type
    header[5] = 0x10 if message_type == 3 else 0
    struct.pack_into("<H", header, 6, HEADER_SIZE)
    struct.pack_into("<H", header, 8, len(payload))
    struct.pack_into("<I", header, 12, sequence & 0xFFFFFFFF)
    struct.pack_into("<I", header, 16, (sequence + 1) & 0xFFFFFFFF)
    struct.pack_into("<Q", header, 20, sequence * 100_000)
    body = header + payload
    crc = zlib.crc32(body) & 0xFFFFFFFF
    struct.pack_into("<I", body, 28, crc)
    return cobs_encode(bytes(body)) + bytes((DELIMITER,))


@dataclass
class Stream:
    encoded: bytearray
    discarding: bool = False
    accepted: int = 0
    malformed: int = 0
    crc_errors: int = 0
    oversize: int = 0

    def __init__(self) -> None:
        self.encoded = bytearray()

    def feed(self, byte: int) -> bool:
        if self.discarding:
            if byte == DELIMITER:
                self.discarding = False
                self.encoded.clear()
            return False
        if byte != DELIMITER:
            if len(self.encoded) >= MAX_ENCODED:
                self.encoded.clear()
                self.discarding = True
                self.oversize += 1
                return False
            self.encoded.append(byte)
            return False
        frame = bytes(self.encoded)
        self.encoded.clear()
        if not frame:
            return False
        try:
            raw = cobs_decode(frame)
        except ValueError:
            self.malformed += 1
            return False
        if len(raw) < HEADER_SIZE or raw[:2] != MAGIC or raw[2:4] != b"\x01\x00":
            self.malformed += 1
            return False
        payload_size = struct.unpack_from("<H", raw, 8)[0]
        if struct.unpack_from("<H", raw, 6)[0] != HEADER_SIZE or payload_size != len(raw) - HEADER_SIZE:
            self.malformed += 1
            return False
        expected = struct.unpack_from("<I", raw, 28)[0]
        check = bytearray(raw)
        struct.pack_into("<I", check, 28, 0)
        if zlib.crc32(check) & 0xFFFFFFFF != expected:
            self.crc_errors += 1
            return False
        self.accepted += 1
        return True


def mutate(frame: bytes, rng: random.Random) -> bytes:
    body = bytearray(frame[:-1])
    kind = rng.randrange(3)
    if kind == 0:
        body.insert(rng.randrange(len(body) + 1), rng.randrange(1, 256))
    elif kind == 1 and body:
        del body[rng.randrange(len(body))]
    elif body:
        index = rng.randrange(len(body))
        body[index] ^= 1 << rng.randrange(8)
    return bytes(body) + bytes((DELIMITER,))


def run(seed: int, duration_seconds: int) -> int:
    if duration_seconds <= 0:
        raise ValueError("duration must be positive")
    rng = random.Random(seed)
    streams = (Stream(), Stream())
    expected_good = 0
    corrupted = 0
    frames = duration_seconds * 10
    for sequence in range(frames):
        for direction, stream in enumerate(streams):
            payload = bytes(rng.randrange(256) for _ in range((sequence + direction) % 241))
            good = packet(sequence, payload)
            if rng.randrange(20) == 0:
                corrupted += 1
                damaged = mutate(good, rng)
                for byte in damaged:
                    stream.feed(byte)
            else:
                expected_good += 1
                for byte in good:
                    stream.feed(byte)
            # A good delimiter-framed packet must be accepted after every fault.
            recovery = packet(sequence + 1_000_000, b"\x00\x01\x00\x02", 3)
            expected_good += 1
            if not any(stream.feed(byte) for byte in recovery):
                raise AssertionError("good packet did not recover at the next delimiter")
        if sequence % 997 == 0:
            oversized = bytes((0xA5,)) * (MAX_ENCODED + 1) + bytes((DELIMITER,))
            for stream in streams:
                for byte in oversized:
                    stream.feed(byte)
                if stream.oversize == 0 or stream.discarding:
                    raise AssertionError("oversize resync failed")
    accepted = sum(stream.accepted for stream in streams)
    if accepted != expected_good:
        raise AssertionError(f"accepted={accepted}, expected={expected_good}")
    if sum(stream.oversize for stream in streams) != 2 * ((frames - 1) // 997 + 1):
        raise AssertionError("oversize counter did not increment once per burst")
    if any(stream.encoded or stream.discarding for stream in streams):
        raise AssertionError("stream retained bytes after simulation")
    print(
        f"PASS: UART duplex virtual stream seed={seed} virtual_seconds={duration_seconds} "
        f"frames={frames * 2} corrupted={corrupted} accepted={accepted} "
        f"load=10_primary_frames_per_second_per_direction malformed={sum(s.malformed for s in streams)} "
        f"crc_errors={sum(s.crc_errors for s in streams)} oversize={sum(s.oversize for s in streams)}"
    )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--duration-seconds", type=int, default=3600)
    args = parser.parse_args()
    return run(args.seed, args.duration_seconds)


if __name__ == "__main__":
    raise SystemExit(main())
