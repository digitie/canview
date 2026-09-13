/* SPDX-License-Identifier: GPL-3.0-only */
/** @file envelope.h @brief 구현 중인 OTA prefix의 구조와 manifest 서명 검사. */
#ifndef CANVIEW_OTA_ENVELOPE_H
#define CANVIEW_OTA_ENVELOPE_H

#include <stddef.h>
#include <stdint.h>
#include "canview_status.h"
#include "cbor_document.h"

#define CANVIEW_OTA_ENVELOPE_HEADER_BYTES (24U)
#define CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES (64U)
#define CANVIEW_OTA_ENVELOPE_MANIFEST_MAX (CANVIEW_OTA_CBOR_MAX_BYTES)
#define CANVIEW_OTA_ENVELOPE_PREFIX_MAX (CANVIEW_OTA_ENVELOPE_HEADER_BYTES + \
    CANVIEW_OTA_ENVELOPE_MANIFEST_MAX + CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES)
#define CANVIEW_OTA_ENVELOPE_IMAGE_MAX (3U)
#define CANVIEW_OTA_ENVELOPE_IMAGE_BYTES_MAX (UINT32_C(4194304))
#define CANVIEW_OTA_ENVELOPE_BUNDLE_MAX (CANVIEW_OTA_ENVELOPE_IMAGE_MAX * \
    CANVIEW_OTA_ENVELOPE_IMAGE_BYTES_MAX + CANVIEW_OTA_ENVELOPE_PREFIX_MAX)

/** @brief 신뢰된 caller가 제공하는 ECDSA-P256/SHA-256 검증 함수.
 * @details signature는 big-endian r[32] || s[32]이다. message는 정확한 CBOR
 * byte열이며 사전 hash가 아니다. 공개키는 context에서 공급한다. 입력 package의
 * key를 신뢰 root로 사용하면 안 된다. 성공만 OK; 나머지는 실패 상태를 반환한다.
 * 모든 입력은 호출 중만 빌린다. 저장/변경 금지. 별도 context로 재진입 가능하며
 * 공유 context의 thread safety와 SDK resource cleanup은 provider가 소유한다.
 */
typedef canview_status_t (*canview_ota_manifest_verify_fn)(
    void *context, const uint8_t *message, size_t message_size,
    const uint8_t signature[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES]);

/** @brief Prefix 위치와 아직 manifest에 대조하지 않은 unsigned header 주장.
 * @details pointer를 보존하지 않는다. declared 필드는 인증된 설치 정보가 아니다.
 */
typedef struct
{
    size_t manifest_offset;
    size_t manifest_size;
    size_t images_offset;
    uint32_t declared_total_size;
    uint16_t declared_image_count;
} canview_ota_envelope_t;

/** @brief 고정 header + CBOR + 서명으로 구성된 prefix만 검사한다.
 * @param prefix 호출 중 불변이며 실제 size만큼 읽을 수 있는 입력. NULL 불가.
 * @param size prefix 실제 길이. image 본문을 포함하지 않는다.
 * @param verify 신뢰된 검증 provider. NULL이면 실패한다.
 * @param context provider의 호출 수명 context. NULL 허용 여부는 provider 계약이다.
 * @param out 실패하면 모든 필드를 0으로 한다. NULL 불가. 다른 인자와 겹침 금지.
 * @return 구조/길이 오류 또는 provider 실패; 모두 통과한 경우만 OK.
 * @details 힙/전역 가변 상태 없음. 최대16KiB CBOR와 provider 1회만 처리한다.
 * callback은 입력/out을 변경하거나 동일 out으로 재진입하면 안 된다.
 * role/board/layout/key_id/epoch/compatibility/서명된 image 길이/본문은 아직
 * 검사하지 않는다. OK는 erase/write/PREPARED/boot selector 권한이 아니다.
 * 최종 public API/배포 포맷이 아닌 T-007 내부 구현 후보다.
 */
canview_status_t canview_ota_envelope_check(
    const uint8_t *prefix, size_t size, canview_ota_manifest_verify_fn verify,
    void *context, canview_ota_envelope_t *out);

#endif
