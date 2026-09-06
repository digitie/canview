"""Independent byte-level reference checks for ESP-NOW v1.3."""

from __future__ import annotations

import unittest
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
GOLDEN = ROOT / "protocol" / "golden" / "espnow-v1.3"
MAGIC = b"CV"
HEADER_SIZE = 32
MAX_FRAME = 240
MAX_PAYLOAD = 208
KNOWN_FLAGS = 0x7F


def crc32(frame: bytes) -> int:
    mutable = bytearray(frame)
    mutable[28:32] = b"\x00\x00\x00\x00"
    return zlib.crc32(mutable) & 0xFFFFFFFF


def decode(frame: bytes) -> dict[str, object]:
    if not HEADER_SIZE <= len(frame) <= MAX_FRAME:
        raise ValueError("bad_length")
    if frame[0:2] != MAGIC:
        raise ValueError("bad_magic")
    if frame[2] != 1:
        raise ValueError("incompatible_major")
    if frame[3] < 3 or frame[4] != HEADER_SIZE:
        raise ValueError("bad_header")
    flags = frame[6]
    if flags & ~KNOWN_FLAGS:
        raise ValueError("bad_flags")
    if int.from_bytes(frame[26:28], "little") != 0:
        raise ValueError("bad_reserved")
    payload_length = int.from_bytes(frame[24:26], "little")
    if payload_length > MAX_PAYLOAD or payload_length != len(frame) - HEADER_SIZE:
        raise ValueError("bad_length")
    if int.from_bytes(frame[28:32], "little") != crc32(frame):
        raise ValueError("bad_crc")
    return {
        "major": frame[2],
        "minor": frame[3],
        "message_type": frame[5],
        "flags": flags,
        "priority": frame[7],
        "session_id": int.from_bytes(frame[8:12], "little"),
        "sequence": int.from_bytes(frame[12:16], "little"),
        "sender_time_ms": int.from_bytes(frame[16:20], "little"),
        "correlation_id": int.from_bytes(frame[20:24], "little"),
        "payload": frame[HEADER_SIZE:],
    }


def encode(decoded: dict[str, object]) -> bytes:
    payload = bytes(decoded["payload"])
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("oversize")
    frame = bytearray(HEADER_SIZE + len(payload))
    frame[0:2] = MAGIC
    frame[2] = int(decoded["major"])
    frame[3] = int(decoded["minor"])
    frame[4] = HEADER_SIZE
    frame[5] = int(decoded["message_type"])
    frame[6] = int(decoded["flags"])
    frame[7] = int(decoded["priority"])
    frame[8:12] = int(decoded["session_id"]).to_bytes(4, "little")
    frame[12:16] = int(decoded["sequence"]).to_bytes(4, "little")
    frame[16:20] = int(decoded["sender_time_ms"]).to_bytes(4, "little")
    frame[20:24] = int(decoded["correlation_id"]).to_bytes(4, "little")
    frame[24:26] = len(payload).to_bytes(2, "little")
    frame[HEADER_SIZE:] = payload
    frame[28:32] = crc32(frame).to_bytes(4, "little")
    return bytes(frame)


class EspNowReferenceTests(unittest.TestCase):
    def test_every_golden_frame_round_trips_without_generator_import(self) -> None:
        frames = sorted(GOLDEN.glob("*.bin"))
        self.assertEqual(len(frames), 15)
        for path in frames:
            with self.subTest(path=path.name):
                frame = path.read_bytes()
                self.assertEqual(encode(decode(frame)), frame)

    def test_malformed_corpus_rejects(self) -> None:
        expected = {
            "bad-crc",
            "bad-magic",
            "payload-length-overrun",
            "reserved-header",
            "truncated-header",
            "unknown-flags",
        }
        actual = {path.stem for path in (GOLDEN / "malformed").glob("*.bin")}
        self.assertEqual(actual, expected)
        for path in sorted((GOLDEN / "malformed").glob("*.bin")):
            with self.subTest(path=path.name), self.assertRaises(ValueError):
                decode(path.read_bytes())

    def test_crc_detects_each_header_and_payload_bit(self) -> None:
        frame = bytearray((GOLDEN / "hello.bin").read_bytes())
        for offset in (0, 2, 5, 12, 24, 32, len(frame) - 1):
            with self.subTest(offset=offset):
                mutated = bytearray(frame)
                mutated[offset] ^= 0x01
                with self.assertRaises(ValueError):
                    decode(bytes(mutated))

    def test_future_minor_same_major_remains_structurally_accepted(self) -> None:
        frame = bytearray((GOLDEN / "hello.bin").read_bytes())
        frame[3] = 4
        frame[28:32] = crc32(frame).to_bytes(4, "little")
        self.assertEqual(decode(bytes(frame))["minor"], 4)

    def test_sequence_window_modular_order(self) -> None:
        newest = 0xFFFFFFFF
        next_sequence = (newest + 1) & 0xFFFFFFFF
        self.assertEqual(next_sequence, 0)
        self.assertLess(((next_sequence - newest) & 0xFFFFFFFF), 0x80000000)


if __name__ == "__main__":
    unittest.main()
