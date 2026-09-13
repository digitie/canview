/* SPDX-License-Identifier: GPL-3.0-only */
/** @file manifest.h @brief T-007 내부 typed manifest 후보. 설치 API가 아니다. */
#ifndef CANVIEW_OTA_MANIFEST_H
#define CANVIEW_OTA_MANIFEST_H

#include "envelope.h"

#define CANVIEW_OTA_TEXT_BYTES (64U)
#define CANVIEW_OTA_PACKAGE_BYTES (16U)
#define CANVIEW_OTA_DIGEST_BYTES (32U)
#define CANVIEW_OTA_COMBINATIONS_MAX (16U)

/** @brief OTA 장치 역할. 무선 peer의 control 권한 enum과 별개다. */
typedef enum
{
    CANVIEW_OTA_ROLE_COMMUNICATOR = 1,
    CANVIEW_OTA_ROLE_CONTROLLER = 2,
    CANVIEW_OTA_ROLE_BRIDGE = 3
} canview_ota_role_t;

/** @brief 일반 앱만 허용한다. recovery/bootloader/주소 target은 없다. */
typedef enum
{
    CANVIEW_OTA_TARGET_COMM_ESP = 1,
    CANVIEW_OTA_TARGET_COMM_STM = 2,
    CANVIEW_OTA_TARGET_CONTROLLER = 3,
    CANVIEW_OTA_TARGET_BRIDGE = 4
} canview_ota_target_t;

/** @brief 이미지 내부의 native signature 형식. 이 필드 자체는 검증 증거가 아니다. */
typedef enum
{
    CANVIEW_OTA_IMAGE_ESP_SECURE_BOOT_V2 = 1,
    CANVIEW_OTA_IMAGE_MCUBOOT_P256 = 2
} canview_ota_image_signature_t;

/** @brief 서명된 image descriptor. offset은 순차 길이 합으로만 계산한다. */
typedef struct
{
    canview_ota_target_t target;
    uint32_t length;
    uint32_t offset;
    uint8_t sha256[CANVIEW_OTA_DIGEST_BYTES];
    char version[CANVIEW_OTA_TEXT_BYTES];
    uint64_t release_sequence;
    canview_ota_image_signature_t signature;
    uint32_t abi;
} canview_ota_image_t;

/** @brief 포함 경계인 ABI 범위. */
typedef struct
{
    uint32_t minimum;
    uint32_t maximum;
} canview_ota_abi_range_t;

/** @brief 허용 ESP/STM ABI 조합. */
typedef struct
{
    uint32_t esp;
    uint32_t stm;
} canview_ota_abi_pair_t;

/** @brief Pointer 없는 고정 크기 해석 결과. 아직 이미지 본문은 검증하지 않았다. */
typedef struct
{
    uint8_t package_id[CANVIEW_OTA_PACKAGE_BYTES];
    canview_ota_role_t role;
    char board_revision[CANVIEW_OTA_TEXT_BYTES];
    char layout_id[CANVIEW_OTA_TEXT_BYTES];
    char release[CANVIEW_OTA_TEXT_BYTES];
    uint32_t security_epoch;
    uint32_t key_id;
    uint32_t image_count;
    canview_ota_image_t images[CANVIEW_OTA_ENVELOPE_IMAGE_MAX];
    canview_ota_abi_range_t esp;
    canview_ota_abi_range_t stm;
    canview_ota_abi_range_t peer;
    uint32_t combination_count;
    canview_ota_abi_pair_t combinations[CANVIEW_OTA_COMBINATIONS_MAX];
    uint32_t config_minimum;
    uint32_t config_maximum;
    uint32_t config_snapshot;
    uint32_t minimum_bootloader;
    uint32_t minimum_recovery;
    uint64_t hardware_capabilities;
    uint32_t total_size;
} canview_ota_manifest_t;

/** @brief Caller가 신뢰된 BSP/provisioning에서 읽은 기대 identity.
 * @details 문자열은 고정 배열 내 NUL 종단이며 1..63 printable ASCII다.
 * key_id는 verify context에 실제 선택한 역할별 root의 ID여야 한다.
 * 파일/HTTP 입력으로 이 값을 구성하거나 override하면 안 된다.
 */
typedef struct
{
    canview_ota_role_t role;
    char board_revision[CANVIEW_OTA_TEXT_BYTES];
    char layout_id[CANVIEW_OTA_TEXT_BYTES];
    uint32_t security_epoch;
    uint32_t key_id;
} canview_ota_identity_t;

/** @brief 서명 prefix와 typed 필드/identity/target/길이를 함께 검사한다.
 * @param prefix 실제 size만큼 읽을 수 있는 불변 prefix. 본문 제외, NULL 불가.
 * @param size 실제 prefix 길이.
 * @param identity 신뢰된 로컬 identity. 호출 중 불변, NULL 불가.
 * @param verify 신뢰된 root로 manifest 서명을 검증하는 provider. NULL 불가.
 * @param context verify가 소유한 context. envelope.h의 수명 계약을 따른다.
 * @param out 실패 시 전체 0. 모든 입력/context와 비중첩, NULL 불가.
 * @return prefix 오류, MALFORMED(필드/조합), OVERSIZE, AUTH_FAILED(identity) 또는 OK.
 * @details 고정 메모리, 무힙/무재귀/무ISR/무Flash. 입력 pointer를 보존하지 않는다.
 * 별도 out/context로 재진입 가능하다. provider는 입력/out/identity를 변경하지 않는다.
 * ABI 범위/조합의 내부 정합성만 검사하며 실제 실행중/새 이미지 ABI 대조, version
 * floor, native image hash/서명/metadata 대조와 streaming은 후속 단계다.
 * OK는 erase/write/PREPARED/boot selector 권한이 아니다. 배포 전 schema review 필요.
 */
canview_status_t canview_ota_manifest_check(
    const uint8_t *prefix, size_t size, const canview_ota_identity_t *identity,
    canview_ota_manifest_verify_fn verify, void *context, canview_ota_manifest_t *out);

#endif
