"""OTA의 bounded deterministic CBOR profile. 서명/설치 권한과 별개다."""
from __future__ import annotations

from enum import IntEnum

MAX_BYTES = 16384
MAX_DEPTH = 8
MAX_ITEMS = 2048


class Status(IntEnum):
    OK = 0
    MALFORMED = 3
    UNSUPPORTED_VERSION = 4
    UNSUPPORTED_MESSAGE = 5
    OVERSIZE = 7
    INCOMPLETE = 8
    DUPLICATE = 9
    AUTH_FAILED = 12


class CborError(ValueError):
    def __init__(self, status: Status):
        super().__init__(status.name)
        self.status = status


def decode_document(data: bytes) -> dict:
    """최대16KiB/2048 item/8 container. bytes를 보존하거나 외부 callback을 실행하지 않는다."""
    if not isinstance(data, bytes):
        raise TypeError("bytes required")
    if len(data) > MAX_BYTES:
        raise CborError(Status.OVERSIZE)
    offset = 0
    items = 0

    def item(depth: int, root: bool = False, key: bool = False):
        nonlocal offset, items
        if items >= MAX_ITEMS:
            raise CborError(Status.OVERSIZE)
        if offset == len(data):
            raise CborError(Status.INCOMPLETE)
        first = data[offset]
        offset += 1
        major, additional = first >> 5, first & 31
        if major not in (0, 2, 3, 4, 5):
            raise CborError(Status.UNSUPPORTED_MESSAGE)
        if additional >= 28:
            raise CborError(Status.MALFORMED)
        if additional < 24:
            argument = additional
        else:
            width = 1 << (additional - 24)
            if width > len(data) - offset:
                raise CborError(Status.INCOMPLETE)
            argument = int.from_bytes(data[offset:offset + width], "big")
            offset += width
            if argument < (24, 256, 65536, 4294967296)[additional - 24]:
                raise CborError(Status.MALFORMED)
        if (root and major != 5) or (key and major != 0):
            raise CborError(Status.MALFORMED)
        items += 1
        if major == 0:
            return argument
        if major in (2, 3):
            if argument > MAX_BYTES:
                raise CborError(Status.OVERSIZE)
            if argument > len(data) - offset:
                raise CborError(Status.INCOMPLETE)
            value = data[offset:offset + argument]
            offset += argument
            if major == 2:
                return value
            try:
                return value.decode("utf-8", errors="strict")
            except UnicodeDecodeError as exc:
                raise CborError(Status.MALFORMED) from exc
        factor = 2 if major == 5 else 1
        if depth >= MAX_DEPTH or argument > MAX_ITEMS // factor:
            raise CborError(Status.OVERSIZE)
        if major == 4:
            return [item(depth + 1) for _ in range(argument)]
        value = {}
        previous = None
        for _ in range(argument):
            current = item(depth + 1, key=True)
            if previous is not None and current <= previous:
                raise CborError(Status.DUPLICATE if current == previous else Status.MALFORMED)
            previous = current
            value[current] = item(depth + 1)
        return value

    value = item(0, root=True)
    if offset != len(data):
        raise CborError(Status.MALFORMED)
    return value


def encode_document(value: dict) -> bytes:
    """검증된 manifest를 동일 CBOR byte열로 인코딩한다. 부호 없는 integer key만 허용한다."""
    output = bytearray()
    items = 0

    def head(major: int, argument: int) -> None:
        if not 0 <= argument <= (1 << 64) - 1:
            raise CborError(Status.OVERSIZE)
        if argument < 24:
            output.append((major << 5) | argument)
        else:
            width = next(size for size in (1, 2, 4, 8) if argument < 1 << (8 * size))
            output.append((major << 5) | {1: 24, 2: 25, 4: 26, 8: 27}[width])
            output.extend(argument.to_bytes(width, "big"))

    def item(current, depth: int) -> None:
        nonlocal items
        items += 1
        if items > MAX_ITEMS:
            raise CborError(Status.OVERSIZE)
        if type(current) is int:
            head(0, current)
        elif type(current) in (bytes, str):
            if len(current) > MAX_BYTES:
                raise CborError(Status.OVERSIZE)
            try:
                raw = current.encode("utf-8") if type(current) is str else current
            except UnicodeEncodeError as exc:
                raise CborError(Status.MALFORMED) from exc
            head(3 if type(current) is str else 2, len(raw))
            output.extend(raw)
        elif type(current) in (list, dict):
            factor = 2 if type(current) is dict else 1
            if depth >= MAX_DEPTH or len(current) > MAX_ITEMS // factor:
                raise CborError(Status.OVERSIZE)
            head(5 if type(current) is dict else 4, len(current))
            if type(current) is dict:
                if any(type(key) is not int or not 0 <= key < 1 << 64 for key in current):
                    raise CborError(Status.MALFORMED)
                for key in sorted(current):
                    item(key, depth + 1)
                    item(current[key], depth + 1)
            else:
                for child in current:
                    item(child, depth + 1)
        else:
            raise CborError(Status.UNSUPPORTED_MESSAGE)
        if len(output) > MAX_BYTES:
            raise CborError(Status.OVERSIZE)

    if type(value) is not dict:
        raise CborError(Status.MALFORMED)
    item(value, 0)
    return bytes(output)
