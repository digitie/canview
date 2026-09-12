"""T-007 내부 prefix 후보. 전체 manifest 의미/이미지 검증기는 아직 아니다."""
from __future__ import annotations

import struct

from cbor import CborError, MAX_BYTES, Status, decode_document, encode_document

HEADER = struct.Struct("<8sHHIHHI")
MAGIC = b"CVOTA001"
SIGNATURE_BYTES = 64
MAX_IMAGES = 3
MAX_IMAGE_BYTES = 4 * 1024 * 1024
MAX_BUNDLE = MAX_IMAGES * MAX_IMAGE_BYTES + HEADER.size + MAX_BYTES + SIGNATURE_BYTES


def assemble(manifest: dict, images: list[bytes], sign) -> bytes:
    """합성/packager용 조립. sign은 CBOR bytes를 받고 raw P256 서명64B를 반환한다.

    개인키를 생성/로드/저장하지 않는다. 이미지 본문/manifest 의미 검증은 caller의
    후속 책임이며 이 함수 성공만으로 배포하거나 설치하면 안 된다.
    """
    if type(images) not in (list, tuple) or not 1 <= len(images) <= MAX_IMAGES:
        raise CborError(Status.MALFORMED)
    images = tuple(images)
    if any(type(blob) is not bytes or not blob for blob in images):
        raise CborError(Status.MALFORMED)
    if any(len(blob) > MAX_IMAGE_BYTES for blob in images):
        raise CborError(Status.OVERSIZE)
    manifest_bytes = encode_document(manifest)
    total = HEADER.size + len(manifest_bytes) + SIGNATURE_BYTES + sum(map(len, images))
    if total > MAX_BUNDLE:
        raise CborError(Status.OVERSIZE)
    signature = sign(manifest_bytes)
    if type(signature) is not bytes or len(signature) != SIGNATURE_BYTES:
        raise CborError(Status.MALFORMED)
    return (HEADER.pack(MAGIC, 1, HEADER.size, len(manifest_bytes), SIGNATURE_BYTES,
                        len(images), total) + manifest_bytes + signature + b"".join(images))


def prefix_length(data: bytes) -> int:
    """이미지 읽기 전에 prefix 크기만 구한다. 아직 header/서명을 신뢰하지 않는다."""
    if len(data) < HEADER.size:
        raise CborError(Status.INCOMPLETE)
    size = HEADER.unpack_from(data)[3]
    if size > MAX_BYTES:
        raise CborError(Status.OVERSIZE)
    return HEADER.size + size + SIGNATURE_BYTES


def check_prefix(data: bytes, verify) -> dict:
    """C counterpart와 같은 prefix 검사. verify 실패는 예외로 전파한다."""
    if len(data) > HEADER.size + MAX_BYTES + SIGNATURE_BYTES:
        raise CborError(Status.OVERSIZE)
    if len(data) < HEADER.size:
        raise CborError(Status.INCOMPLETE)
    magic, version, header_size, manifest_size, signature_size, count, total = HEADER.unpack_from(data)
    if magic != MAGIC:
        raise CborError(Status.MALFORMED)
    if version != 1:
        raise CborError(Status.UNSUPPORTED_VERSION)
    if header_size != HEADER.size or signature_size != SIGNATURE_BYTES or manifest_size == 0:
        raise CborError(Status.MALFORMED)
    if manifest_size > MAX_BYTES:
        raise CborError(Status.OVERSIZE)
    expected = HEADER.size + manifest_size + SIGNATURE_BYTES
    if len(data) != expected:
        raise CborError(Status.INCOMPLETE if len(data) < expected else Status.MALFORMED)
    if not 1 <= count <= MAX_IMAGES or total < expected + count:
        raise CborError(Status.MALFORMED)
    if total > MAX_BUNDLE:
        raise CborError(Status.OVERSIZE)
    manifest_bytes = data[HEADER.size:HEADER.size + manifest_size]
    manifest = decode_document(manifest_bytes)
    verify(manifest_bytes, data[-SIGNATURE_BYTES:])
    return manifest
