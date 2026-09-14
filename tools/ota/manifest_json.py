"""이름 있는 JSON manifest를 기존 typed 검사 후 정규 CBOR로 변환한다. 서명/설치는 하지 않는다."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys

from cbor import CborError, Status, encode_document
from envelope import HEADER, MAGIC, FORMAT_VERSION, SIGNATURE_BYTES, image_offset
from manifest import IMAGE_LIMITS, check_manifest

ROOT = Path(__file__).resolve().parents[2]
JSON_BYTES_MAX = 32768
SCHEMA = json.loads((ROOT / "schema/cvota-v2.schema.json").read_text(encoding="utf-8"))


def _invalid(*_):
    raise CborError(Status.MALFORMED)


def _object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise CborError(Status.DUPLICATE)
        result[key] = value
    return result


def _fields(value, schema):
    if type(value) is not dict or set(value) != set(schema["required"]):
        _invalid()
    result = {}
    for name, field in sorted(schema["properties"].items(), key=lambda item: item[1]["x-cbor-key"]):
        item = value[name]
        if field.get("x-cbor-hex"):
            if type(item) is not str or re.fullmatch(field["pattern"], item) is None:
                _invalid()
            item = bytes.fromhex(item)
        result[field["x-cbor-key"]] = item
    return result


def load_manifest_json(data: bytes) -> dict:
    """최대32KiB JSON을 읽는다. duplicate/float/unknown field를 거부하고 u64를 정수로 보존한다.

    schema의 field map만 재사용하며 범용 JSON Schema 엔진을 구현하지 않는다.
    실제 type/range/교차 제약은 기존 check_manifest가 검사한다. 여기서 identity는
    작성 중인 manifest 자체의 값이며 장치 인증 증거가 아니다. 입력 내 경로/키는 허용하지 않는다.
    """
    if type(data) is not bytes:
        _invalid()
    if len(data) > JSON_BYTES_MAX:
        raise CborError(Status.OVERSIZE)
    try:
        source = json.loads(data.decode("utf-8"), object_pairs_hook=_object,
                            parse_float=_invalid, parse_constant=_invalid)
    except (UnicodeError, RecursionError, ValueError) as error:
        if isinstance(error, CborError):
            raise
        raise CborError(Status.MALFORMED) from error
    result = _fields(source, SCHEMA)
    if type(result[8]) is not list or not 1 <= len(result[8]) <= 2:
        _invalid()
    result[8] = [_fields(image, SCHEMA["$defs"]["image"]) for image in result[8]]
    result[9] = _fields(result[9], SCHEMA["$defs"]["compatibility"])
    encoded = encode_document(result)
    total = HEADER.size + len(encoded) + SIGNATURE_BYTES
    for image in result[8]:
        target, length = image[0], image[1]
        if type(target) is not int or target not in IMAGE_LIMITS or type(length) is not int or length <= 0:
            _invalid()
        if length > IMAGE_LIMITS[target]:
            raise CborError(Status.OVERSIZE)
        total = image_offset(total) + length
    prefix = HEADER.pack(MAGIC, FORMAT_VERSION, HEADER.size, len(encoded), SIGNATURE_BYTES,
                         len(result[8]), total) + encoded + bytes(SIGNATURE_BYTES)
    # 구조 검사만 수행한다. zero signature는 검증 증거가 아니며 출력에도 포함하지 않는다.
    identity = (result[2], result[3], result[4], result[6], result[7])
    check_manifest(prefix, identity, lambda _message, _signature: None)
    return result


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="검사한 JSON manifest를 서명 전 CBOR로 변환. OTA package/설치 승인이 아님.")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path, help="새 CBOR 파일. 기존 파일을 덮어쓰지 않음")
    args = parser.parse_args(argv)
    try:
        with args.input.open("rb") as source:
            manifest = load_manifest_json(source.read(JSON_BYTES_MAX + 1))
        encoded = encode_document(manifest)
        with args.output.open("xb") as destination:
            destination.write(encoded)
    except (OSError, CborError) as error:
        # 입력 문서/키/경로는 로그에 넣지 않는다.
        print(f"manifest encode failed: {error.status.name if isinstance(error, CborError) else type(error).__name__}", file=sys.stderr)
        return 1
    print(f"UNSIGNED_MANIFEST: {len(encoded)} CBOR bytes; native images/signature not verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
