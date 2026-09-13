"""고정 typed manifest 내부 후보. Flash/부팅 권한과 native image 검증은 포함하지 않는다."""
from __future__ import annotations

from cbor import CborError, Status
from envelope import HEADER, check_prefix

TEXT_MAX = 63
COMBINATIONS_MAX = 16
IMAGE_LIMITS = {1: 4194304, 2: 184320, 3: 4194304, 4: 2621440}
ROLE_TARGETS = {1: {1, 2}, 2: {3}, 3: {4}}


def _reject(status=Status.MALFORMED):
    raise CborError(status)


def _uint(value, bits=32):
    if type(value) is not int or value < 0:
        _reject()
    if value >= 1 << bits:
        _reject(Status.OVERSIZE)
    return value


def _text(value):
    if type(value) is not str or not 1 <= len(value) <= TEXT_MAX:
        _reject()
    if not all(0x20 <= ord(character) <= 0x7E for character in value):
        _reject()


def _map(value, count):
    if type(value) is not dict or list(value) != list(range(count)):
        _reject()


def _array(value, count):
    if type(value) is not list or len(value) != count:
        _reject()


def check_manifest(data: bytes, identity: tuple, verify) -> dict:
    """신뢰된 (role, board, layout, epoch, key_id)와 signed manifest를 비교한다.

    key_id는 verify에 실제 설정된 역할별 root여야 한다. 입력 파일의 공개키나
    role을 identity로 쓰지 않는다. 성공 결과의 ABI는 아직 현재 장치와 대조 전이다.
    """
    manifest = check_prefix(data, verify)
    _map(manifest, 12)
    if _uint(manifest[0]) != 1:
        _reject(Status.UNSUPPORTED_VERSION)
    if type(manifest[1]) is not bytes or len(manifest[1]) != 16:
        _reject()
    role = _uint(manifest[2])
    for field in (3, 4, 5):
        _text(manifest[field])
    _uint(manifest[6])
    _uint(manifest[7])
    images = manifest[8]
    if role not in ROLE_TARGETS or type(images) is not list or not 1 <= len(images) <= 3:
        _reject()
    for image in images:
        _map(image, 7)
        target = _uint(image[0])
        _uint(image[1])
        if type(image[2]) is not bytes or len(image[2]) != 32:
            _reject()
        _text(image[3])
        _uint(image[4], 64)
        signature = _uint(image[5])
        _uint(image[6])
        if target not in IMAGE_LIMITS or signature not in (1, 2):
            _reject()
    compatibility = manifest[9]
    _map(compatibility, 4)
    for index in range(3):
        _array(compatibility[index], 2)
        low, high = map(_uint, compatibility[index])
        if low > high:
            _reject()
    pairs = compatibility[3]
    if type(pairs) is not list:
        _reject()
    if len(pairs) > COMBINATIONS_MAX:
        _reject(Status.OVERSIZE)
    for pair in pairs:
        _array(pair, 2)
        for value in pair:
            _uint(value)
    _array(manifest[10], 3)
    for value in manifest[10]:
        _uint(value)
    _array(manifest[11], 3)
    _uint(manifest[11][0])
    _uint(manifest[11][1])
    _uint(manifest[11][2], 64)
    if (role, manifest[3], manifest[4], manifest[6], manifest[7]) != identity:
        _reject(Status.AUTH_FAILED)
    _, _, _, _, _, count, total = HEADER.unpack_from(data)
    low, high, snapshot = manifest[10]
    if len(images) != count or len(images) > (2 if role == 1 else 1) or not low <= snapshot <= high:
        _reject()
    if (role == 1 and not pairs) or (role != 1 and pairs):
        _reject()
    offset = len(data)
    offsets = []
    seen = set()
    for image in images:
        target, length = image[0], image[1]
        abi_low, abi_high = compatibility[1 if target == 2 else 0]
        if (target not in ROLE_TARGETS[role] or length == 0 or
                not abi_low <= image[6] <= abi_high or image[5] != (2 if target == 2 else 1)):
            _reject()
        if target in seen:
            _reject(Status.DUPLICATE)
        seen.add(target)
        if length > IMAGE_LIMITS[target] or offset + length > 0xFFFFFFFF:
            _reject(Status.OVERSIZE)
        offsets.append(offset)
        offset += length
    seen_pairs = set()
    for esp, stm in pairs:
        if not compatibility[0][0] <= esp <= compatibility[0][1] or not compatibility[1][0] <= stm <= compatibility[1][1]:
            _reject()
        if (esp, stm) in seen_pairs:
            _reject(Status.DUPLICATE)
        seen_pairs.add((esp, stm))
    if offset != total:
        _reject()
    return {"manifest": manifest, "offsets": offsets, "total_size": offset}
