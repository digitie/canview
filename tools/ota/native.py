"""공식 ESP/MCUboot 검증 도구와 기존 CVIMG001 계약을 연결하는 host 검사."""
from __future__ import annotations

import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec, rsa, utils

from cbor import CborError, Status

ROOT = Path(__file__).resolve().parents[2]
ESPTOOL_VERSION = "5.4.0"
METADATA_BYTES = 168
STM_HEADER_BYTES = 512
STM_PROTECTED_BYTES = 176
PUBLIC_BYTES_MAX = 4096
ESP_SIGNATURE_BLOCKS = 3


def _require(condition, status=Status.AUTH_FAILED):
    if not condition:
        raise CborError(status)


def _public(data, kind):
    _require(type(data) is bytes and 0 < len(data) <= PUBLIC_BYTES_MAX)
    key = serialization.load_pem_public_key(data)
    if kind == "esp":
        _require(isinstance(key, rsa.RSAPublicKey) and key.key_size == 3072)
    else:
        _require(isinstance(key, ec.EllipticCurvePublicKey) and isinstance(key.curve, ec.SECP256R1))
    return key


def _metadata(data, identity, entry):
    # identity/entry는 outer typed 검사 완료값. u64를 float/string으로 변환하지 않는다.
    expected = struct.pack("<8sHH5IQ", b"CVIMG001", 1, METADATA_BYTES,
                           identity[0], entry[0], identity[3], entry[6], 0, entry[4])
    expected += identity[1].encode("ascii").ljust(64, b"\0")
    expected += identity[2].encode("ascii").ljust(64, b"\0")
    _require(data == expected)


def _esp(data, public, identity, entry):
    import espsecure
    import esptool
    from esptool.bin_image import ESP32S3FirmwareImage

    _require(esptool.__version__ == ESPTOOL_VERSION, Status.UNSUPPORTED_VERSION)
    _public(public, "esp")
    _require(len(data) >= 2 * espsecure.SECTOR_SIZE and len(data) % espsecure.SECTOR_SIZE == 0 and
             1 <= data[1] <= 16 and
             struct.unpack_from("<H", data, 12)[0] == ESP32S3FirmwareImage.ROM_LOADER.IMAGE_CHIP_ID)
    try:
        with contextlib.redirect_stdout(io.StringIO()):
            matched = False
            for index in range(ESP_SIGNATURE_BLOCKS):
                block = espsecure.validate_signature_block(data, index)
                if block is None or block[1] != espsecure.SIG_BLOCK_VERSION_RSA:
                    continue
                signature = struct.unpack("<BBxx32s384sI384sI384sI16x", block)[7][::-1]
                try:
                    # 공식 helper는 서명을 실제 검증하고 supplied public key로 block을 재구성한다.
                    # 같은 block 전체 비교로 scheme/key/digest/signature/CRC를 함께 결합한다.
                    expected = espsecure.generate_signature_block_using_pre_calculated_signature(
                        [io.BytesIO(signature)], [io.BytesIO(public)], data[:-espsecure.SECTOR_SIZE])
                except esptool.FatalError:
                    continue
                if block == expected:
                    matched = True
                    break
            _require(matched)
            image = ESP32S3FirmwareImage(io.BytesIO(data))
        _require(image.checksum == image.calculate_checksum())
        if image.append_digest:
            _require(image.stored_digest == image.calc_digest)
        _require(bool(image.segments))
        first = image.segments[0].data
        _require(len(first) >= 256 + METADATA_BYTES and struct.unpack_from("<I", first)[0] == 0xABCD5432)
        _require(len(entry[3]) < 32 and first[16:48] == entry[3].encode("ascii").ljust(32, b"\0"))
        _metadata(first[256:256 + METADATA_BYTES], identity, entry)
    except (esptool.FatalError, struct.error, RuntimeError) as error:
        raise CborError(Status.AUTH_FAILED) from error


def _mcuboot_checkout(path):
    if path is None:
        raise ValueError("MCUboot checkout required")
    path = Path(path).resolve()
    expected = json.loads((ROOT / "tools/toolchain-versions.json").read_text(encoding="utf-8"))["sdk"]["mcuboot"]["gitCommit"]
    head = subprocess.run(["git", "-C", str(path), "rev-parse", "HEAD"],
                          check=True, capture_output=True, text=True, timeout=10).stdout.strip()
    dirty = subprocess.run(["git", "-C", str(path), "status", "--porcelain=v1", "--untracked-files=all"],
                           check=True, capture_output=True, text=True, timeout=10).stdout.strip()
    _require(head == expected and not dirty, Status.UNSUPPORTED_VERSION)
    return path / "scripts/imgtool.py"


def _stm(data, public, identity, entry, tool):
    key = _public(public, "stm")
    _require(len(data) >= STM_HEADER_BYTES + 8 + STM_PROTECTED_BYTES + 88, Status.INCOMPLETE)
    magic, load, header, protected, size, flags = struct.unpack_from("<IIHHII", data)
    _require((magic, load, header, protected, flags) ==
             (0x96F3B83D, 0, STM_HEADER_BYTES, STM_PROTECTED_BYTES, 0), Status.MALFORMED)
    _require(data[28:STM_HEADER_BYTES] == bytes(STM_HEADER_BYTES - 28), Status.MALFORMED)
    _require(8 <= size <= len(data) - STM_HEADER_BYTES - STM_PROTECTED_BYTES - 88, Status.MALFORMED)
    version = "{}.{}.{}+{}".format(*struct.unpack_from("<BBHI", data, 20))
    _require(version == entry[3])
    start = STM_HEADER_BYTES + size
    _require(data[start:start + 8] == struct.pack("<4H", 0x6908, STM_PROTECTED_BYTES, 0xA0, METADATA_BYTES),
             Status.MALFORMED)
    _metadata(data[start + 8:start + STM_PROTECTED_BYTES], identity, entry)
    tlv = data[start + STM_PROTECTED_BYTES:]
    _require(88 <= len(tlv) <= 152 and
             tlv[:8] == struct.pack("<4H", 0x6907, len(tlv), 0x10, 32) and
             tlv[40:44] == struct.pack("<2H", 1, 32) and
             tlv[76:80] == struct.pack("<2H", 0x22, len(tlv) - 80), Status.MALFORMED)
    r, s = utils.decode_dss_signature(tlv[80:])
    _require(utils.encode_dss_signature(r, s) == tlv[80:], Status.MALFORMED)
    spki = key.public_bytes(serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)
    _require(tlv[44:76] == hashlib.sha256(spki).digest())
    # 제한된 profile을 검사한 뒤 공식 imgtool의 SHA256/P256 검증을 반드시 실행한다.
    with tempfile.TemporaryDirectory(prefix="canview-native-check-") as directory:
        image_path, public_path = Path(directory) / "image.bin", Path(directory) / "public.pem"
        image_path.write_bytes(data)
        public_path.write_bytes(public)
        result = subprocess.run([sys.executable, "-B", str(tool), "verify", "-k", str(public_path), str(image_path)],
                                capture_output=True, timeout=60)
        _require(result.returncode == 0)


def check_native_container(data, identity, manifest_public, esp_public=None, stm_public=None, mcuboot_root=None):
    """Outer 서명·hash 뒤 native 서명/metadata를 검사한다. 로컬 정책·설치 승인은 아니다.

    신뢰 root는 package 밖 caller가 공급한다. 개인키를 읽거나 생성하지 않는다.
    고정 MCUboot checkout과 esptool5.4.0을 사용하며, 프로세스 실패/timeout도 성공이 아니다.
    """
    from container import check_container

    checked = check_container(data, identity, manifest_public)
    targets = {entry[0] for entry in checked["manifest"][8]}
    try:
        tool = _mcuboot_checkout(mcuboot_root) if 2 in targets else None
        for entry, offset in zip(checked["manifest"][8], checked["offsets"]):
            image = data[offset:offset + entry[1]]
            if entry[0] == 2:
                _stm(image, stm_public, identity, entry, tool)
            else:
                _esp(image, esp_public, identity, entry)
    except (subprocess.SubprocessError, ImportError) as error:
        raise ValueError("native verification tool unavailable or failed") from error
    return checked
