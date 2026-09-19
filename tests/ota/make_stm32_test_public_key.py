"""비배포 Arm 링크 시험용 일회 P-256 공개 DER. 개인키는 메모리에만 존재한다."""
import argparse
from pathlib import Path

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    public = ec.generate_private_key(ec.SECP256R1()).public_key()
    encoded = public.public_bytes(serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)
    # 실패/재실행에서 기존 입력을 조용히 교체하지 않는다. secret은 직렬화하지 않는다.
    with args.output.open("xb") as stream:
        stream.write(encoded)
    print("PASS: ephemeral test-only public P-256 DER; no production identity, private key not exported")


if __name__ == "__main__":
    main()
