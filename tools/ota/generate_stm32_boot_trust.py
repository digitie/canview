"""신뢰된 빌드 입력의 P-256 공개 DER와 명시적 제조 값을 C header로 변환한다."""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

from cryptography.exceptions import UnsupportedAlgorithm
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec

PUBLIC_DER_BYTES = 91
UINT32_MAX = (1 << 32) - 1


def u32(text: str) -> int:
    if not text.isascii() or not text.isdecimal() or len(text) > 10:
        raise argparse.ArgumentTypeError("명시적 unsigned decimal u32가 필요하다")
    value = int(text)
    if value > UINT32_MAX:
        raise argparse.ArgumentTypeError("u32 범위를 초과했다")
    return value


def render(public_der: bytes, epoch: int, key_id: int, abi: int) -> str:
    for value in (epoch, key_id, abi):
        if type(value) is not int or not 0 <= value <= UINT32_MAX:
            raise ValueError("제조 epoch/key ID/ABI는 명시적 u32여야 한다")
    if len(public_der) != PUBLIC_DER_BYTES:
        raise ValueError("91B P-256 SubjectPublicKeyInfo DER가 필요하다")
    try:
        public = serialization.load_der_public_key(public_der)
    except (ValueError, TypeError, UnsupportedAlgorithm) as error:
        raise ValueError("유효한 공개 DER가 아니다") from error
    if not isinstance(public, ec.EllipticCurvePublicKey) or not isinstance(public.curve, ec.SECP256R1):
        raise ValueError("ECDSA P-256 공개키만 허용한다")
    canonical = public.public_bytes(serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)
    if canonical != public_der:
        raise ValueError("canonical uncompressed P-256 공개 DER가 필요하다")
    values = ", ".join(f"0x{byte:02x}U" for byte in public_der)
    return ("/* 생성물: 공개키만 포함. production provisioning 승인이 아니다. */\n"
            "#ifndef CANVIEW_BOOT_TRUST_GENERATED_H\n#define CANVIEW_BOOT_TRUST_GENERATED_H\n#include <stdint.h>\n"
            f"/* 공개 DER SHA256: {hashlib.sha256(public_der).hexdigest()} */\n"
            f"#define CANVIEW_BOOT_SECURITY_EPOCH UINT32_C({epoch})\n"
            f"#define CANVIEW_BOOT_MANIFEST_KEY_ID UINT32_C({key_id})\n"
            f"#define CANVIEW_BOOT_STM_ABI UINT32_C({abi})\n"
            f"#define CANVIEW_BOOT_PUBLIC_DER_BYTES ({PUBLIC_DER_BYTES}U)\n"
            f"#define CANVIEW_BOOT_PUBLIC_DER {{{values}}}\n#endif\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--public-der", type=Path, required=True)
    parser.add_argument("--security-epoch", type=u32, required=True)
    parser.add_argument("--manifest-key-id", type=u32, required=True)
    parser.add_argument("--stm-abi", type=u32, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    # private DER/큰 입력 전체를 읽거나 로그에 노출하지 않는다.
    with args.public_der.open("rb") as stream:
        data = stream.read(PUBLIC_DER_BYTES + 1)
    source = render(data, args.security_epoch, args.manifest_key_id, args.stm_abi)
    args.output.write_text(source, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
