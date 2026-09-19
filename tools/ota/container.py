"""Revision2 컨테이너 조립·검사. native 서명/설치 정책 검사를 대체하지 않는다."""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import sys

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, utils

from cbor import CborError, Status
from envelope import MAX_BUNDLE, SIGNATURE_BYTES, assemble, prefix_length
from manifest import IMAGE_LIMITS, check_manifest
from manifest_json import JSON_BYTES_MAX, load_manifest_json

PUBLIC_KEY_BYTES_MAX = 4096


def _verifier(public_key):
    if not isinstance(public_key, ec.EllipticCurvePublicKey) or not isinstance(public_key.curve, ec.SECP256R1):
        raise CborError(Status.AUTH_FAILED)

    def verify(message, signature):
        if len(signature) != SIGNATURE_BYTES:
            raise CborError(Status.MALFORMED)
        r = int.from_bytes(signature[:32], "big")
        s = int.from_bytes(signature[32:], "big")
        try:
            public_key.verify(utils.encode_dss_signature(r, s), message, ec.ECDSA(hashes.SHA256()))
        except (InvalidSignature, ValueError) as error:
            raise CborError(Status.AUTH_FAILED) from error

    return verify


def check_container(data: bytes, identity: tuple, public_key) -> dict:
    """외부 신뢰 identity/root로 manifest, 모든 padding·길이·SHA256을 검사한다.

    입력 안의 공개키/identity를 신뢰하지 않는다. 반환은 MANIFEST_AND_HASHES_MATCHED이며
    native image 서명·로컬 호환성/floor·Flash 승인과 별개다. 단일 입력 bytes 길이 상한은
    MAX_BUNDLE이다. 임시 복사·decoded 객체·암호 provider를 포함한 peak RAM 보장은 아니다.
    """
    if type(data) is not bytes:
        raise CborError(Status.MALFORMED)
    if len(data) > MAX_BUNDLE:
        raise CborError(Status.OVERSIZE)
    prefix_size = prefix_length(data)
    checked = check_manifest(data[:prefix_size], identity, _verifier(public_key))
    if len(data) != checked["total_size"]:
        raise CborError(Status.INCOMPLETE if len(data) < checked["total_size"] else Status.MALFORMED)
    previous = prefix_size
    view = memoryview(data)
    for image, offset in zip(checked["manifest"][8], checked["offsets"]):
        if any(view[previous:offset]):
            raise CborError(Status.MALFORMED)
        previous = offset + image[1]
        if hashlib.sha256(view[offset:previous]).digest() != image[2]:
            raise CborError(Status.AUTH_FAILED)
    return checked


def assemble_container(manifest: dict, images: list[bytes], signature: bytes,
                       identity: tuple, public_key) -> bytes:
    """서명 전 JSON→CBOR 결과에 대한 detached raw P256 서명을 사용한다.

    key 생성/로드/저장은 하지 않는다. 기존 조립기를 재사용하고 출력 전에 전체 바깥
    컨테이너를 다시 검증한다. manifest의 image hash/length를 조용히 재작성하지 않는다.
    """
    if type(signature) is not bytes or len(signature) != SIGNATURE_BYTES:
        raise CborError(Status.MALFORMED)
    data = assemble(manifest, images, lambda _message: signature)
    check_container(data, identity, public_key)
    return data


def _read(path: Path, limit: int) -> bytes:
    with path.open("rb") as source:
        data = source.read(limit + 1)
    if len(data) > limit:
        raise CborError(Status.OVERSIZE)
    return data


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="CVOTA002 manifest 서명/본문 hash 검사. native 검증·설치 승인은 별도.")
    parser.add_argument("--public-key", required=True, type=Path, help="신뢰된 역할별 P256 공개키 PEM; package에서 가져오지 않음")
    parser.add_argument("--role", required=True, type=int, choices=(1, 2, 3))
    parser.add_argument("--board", required=True)
    parser.add_argument("--layout", required=True)
    parser.add_argument("--epoch", required=True, type=int)
    parser.add_argument("--key-id", required=True, type=int)
    parser.add_argument("--native", action="store_true", help="공식 도구로 native 서명/metadata까지 검사; 설치 승인은 아님")
    parser.add_argument("--esp-public-key", type=Path, help="package 밖 신뢰된 RSA3072 공개키 PEM")
    parser.add_argument("--stm-public-key", type=Path, help="package 밖 신뢰된 MCUboot P256 공개키 PEM")
    parser.add_argument("--mcuboot-root", type=Path, help="고정된 clean MCUboot checkout")
    commands = parser.add_subparsers(dest="command", required=True)
    create = commands.add_parser("assemble")
    create.add_argument("manifest", type=Path)
    create.add_argument("signature", type=Path, help="정규 CBOR의 raw r[32]||s[32] detached 서명")
    create.add_argument("output", type=Path, help="새 파일만 허용; 기존 파일 덮어쓰기 금지")
    create.add_argument("images", nargs="+", type=Path, help="manifest 순서의 native image")
    verify = commands.add_parser("check")
    verify.add_argument("input", type=Path)
    args = parser.parse_args(argv)
    try:
        identity = (args.role, args.board, args.layout, args.epoch, args.key_id)
        public_key = serialization.load_pem_public_key(_read(args.public_key, PUBLIC_KEY_BYTES_MAX))
        if args.command == "assemble":
            manifest = load_manifest_json(_read(args.manifest, JSON_BYTES_MAX))
            if len(args.images) != len(manifest[8]):
                raise CborError(Status.MALFORMED)
            images = [_read(path, IMAGE_LIMITS[image[0]]) for path, image in zip(args.images, manifest[8])]
            data = assemble_container(manifest, images, _read(args.signature, SIGNATURE_BYTES), identity, public_key)
        else:
            data = _read(args.input, MAX_BUNDLE)
            check_container(data, identity, public_key)
        if args.native:
            from native import check_native_container
            esp_public = _read(args.esp_public_key, PUBLIC_KEY_BYTES_MAX) if args.esp_public_key else None
            stm_public = _read(args.stm_public_key, PUBLIC_KEY_BYTES_MAX) if args.stm_public_key else None
            check_native_container(data, identity, public_key, esp_public, stm_public, args.mcuboot_root)
        elif args.esp_public_key or args.stm_public_key or args.mcuboot_root:
            raise ValueError("native options require --native")
        if args.command == "assemble":
            # 요청한 모든 검증 뒤에만 새 출력 생성. 기존 파일 덮어쓰기 금지.
            with args.output.open("xb") as destination:
                destination.write(data)
    except (OSError, ValueError) as error:
        print(f"container failed: {error.status.name if isinstance(error, CborError) else type(error).__name__}", file=sys.stderr)
        return 1
    if args.native:
        print(f"NATIVE_SIGNATURES_AND_METADATA_MATCHED: {len(data)} bytes; local policy/install NOT_VERIFIED")
    else:
        print(f"MANIFEST_AND_HASHES_MATCHED: {len(data)} bytes; native signatures/local policy/install NOT_VERIFIED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
