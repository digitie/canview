/* SPDX-License-Identifier: GPL-3.0-only */
/** @file native_stm.h @brief MCUboot P256 image의 고정 CANView profile 검사. */
#ifndef CANVIEW_OTA_NATIVE_STM_H
#define CANVIEW_OTA_NATIVE_STM_H
#include "manifest.h"

#define CANVIEW_OTA_STM_IMAGE_MAX (UINT32_C(184320))
#define CANVIEW_OTA_STM_HEADER_BYTES (512U)
#define CANVIEW_OTA_NATIVE_METADATA_BYTES (168U)
#define CANVIEW_OTA_NATIVE_TLV (0x00A0U)

/** @brief 신뢰된 SDK 암호 provider. package 내부 key를 root로 사용하지 않는다.
 * @details sha256은 최대180KiB를 읽어32byte digest를 출력한다. verify는 SHA-256
 * digest와 raw big-endian r[32]||s[32]를 받는다. 다시 hash하지 않는다.
 * key_digest는 context에서 사용하는 MCUboot P256 SPKI DER 공개키의 SHA-256이다.
 * 인자/context는 호출 중만 빌리며 보존/변경 금지. 공유 context의 직렬화·SDK 정리는
 * provider 책임이다. 모든 callback은 실제 실패를 반환하며 자체 암호 구현은 금지한다.
 */
typedef struct
{
    void *context;
    uint8_t key_digest[CANVIEW_OTA_DIGEST_BYTES];
    canview_status_t (*sha256)(void *context, const uint8_t *data, size_t size,
                               uint8_t digest[CANVIEW_OTA_DIGEST_BYTES]);
    canview_status_t (*verify)(void *context, const uint8_t digest[CANVIEW_OTA_DIGEST_BYTES],
                              const uint8_t signature[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES]);
} canview_ota_native_crypto_t;

/** @brief 비암호화 MCUboot P256 image·protected metadata·manifest를 함께 대조한다.
 * @param data size만큼 유효한 불변 전체 image. memory-mapped staging도 허용한다.
 * @param size 실제 크기. 180KiB 이하이며 slot padding/trailer를 포함하지 않는다.
 * @param identity 신뢰된 로컬 identity. 고정 배열은 NUL 종단 printable ASCII다.
 * @param expected 서명 검증된 manifest의 COMM_STM descriptor. 다른 인자와 비중첩.
 * @param crypto 신뢰된 STM 전용 공개키/SDK. context 포함 NULL 불가.
 * @return OK, 입력/형식/크기/서명 오류 또는 provider 오류.
 * @details header512, flag/load address0, 단일 protected CANView TLV, SHA256/keyhash/
 * P256 DER TLV만 허용한다. version은 major.minor.revision+build의 정규 문자열이다.
 * 고정 메모리/무힙/무Flash/무ISR. 모든 입력은 호출 중 불변이며 포인터를 보존하지 않는다.
 * 별도 context로 재진입 가능하다. 성공도 floor/activation/부팅 승인은 아니다.
 * target SDK/provider와 bootloader 연결 전까지 내부 profile 후보다.
 */
canview_status_t canview_ota_stm_image_check(
    const uint8_t *data, size_t size, const canview_ota_identity_t *identity,
    const canview_ota_image_t *expected, const canview_ota_native_crypto_t *crypto);
#endif
