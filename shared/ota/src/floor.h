/* SPDX-License-Identifier: GPL-3.0-only */
/** @file floor.h @brief OTA 업로드의 버전 하한 사전 검사. 영속 journal은 별도 owner다. */
#ifndef CANVIEW_OTA_FLOOR_H
#define CANVIEW_OTA_FLOOR_H

#include "manifest.h"

#define CANVIEW_OTA_FLOOR_TARGET_MAX (2U)

/** @brief 정상 앱의 실제 검사 결과. recovery 실행/과거 성공 cache는 증거가 아니다. */
typedef enum
{
    CANVIEW_OTA_INSTALLED_UNKNOWN = 0,
    CANVIEW_OTA_INSTALLED_BOOTABLE,
    CANVIEW_OTA_INSTALLED_DAMAGED
} canview_ota_installed_state_t;

/** @brief 신뢰된 영속 record와 현재 정상 앱 검사 결과의 일관된 snapshot.
 * @details BOOTABLE은 실제 전체 hash/native 서명/board/layout/epoch/부팅 선택
 * metadata를 검사한 정상 앱이다. 그 앱의 sequence/digest를 observed_*에 넣는다.
 * DAMAGED는 정상 앱 손상 또는 부팅 선택 불가를 확인한 경우다. 읽기 실패/미검사는
 * UNKNOWN이다. floor/confirmed_digest는 선택한 valid policy copy에서만 읽는다.
 */
typedef struct
{
    canview_ota_target_t target;
    uint64_t minimum_sequence;
    uint8_t confirmed_digest[CANVIEW_OTA_DIGEST_BYTES];
    canview_ota_installed_state_t installed;
    uint64_t observed_sequence;
    uint8_t observed_digest[CANVIEW_OTA_DIGEST_BYTES];
} canview_ota_floor_record_t;

/** @brief BSP/boot/journal owner가 제공하는 불변 로컬 입력. 네트워크 값 사용 금지.
 * @details ready는 valid policy copy 선택 및 CONFIRM_INTENT/실제 boot 상태 대조가
 * 끝나 새 업로드를 판정할 수 있다는 뜻이다. 두 copy 손상/미확인/미조정이면 false다.
 * {0}은 잠금 상태이며 floor0 최초 설치를 뜻하지 않는다. record는 포함 target별로
 * 필요하며 중복/다른 역할을 허용하지 않는다. key_id는 영속 floor의 일부가 아니다.
 * 이 구조는 영속 wire 형식이 아니다. CRC/generation/commit/rollback 처리를 하지 않는다.
 */
typedef struct
{
    bool ready;
    canview_ota_identity_t identity;
    uint32_t count;
    canview_ota_floor_record_t records[CANVIEW_OTA_FLOOR_TARGET_MAX];
} canview_ota_floor_t;

/** @brief OK일 때의 target별 사전 판정. 설치/erase/activation 권한은 아니다. */
typedef enum
{
    CANVIEW_OTA_FLOOR_UNCHECKED = 0,
    CANVIEW_OTA_FLOOR_UPGRADE,
    CANVIEW_OTA_FLOOR_ALREADY_INSTALLED,
    CANVIEW_OTA_FLOOR_REPAIR_REQUIRED
} canview_ota_floor_action_t;

/** @brief manifest image 순서의 판정. 실패하면 전체0이며 부분 성공을 사용하지 않는다. */
typedef struct
{
    canview_ota_floor_action_t images[CANVIEW_OTA_FLOOR_TARGET_MAX];
} canview_ota_floor_result_t;

/** @brief 서명/identity/호환성 검사를 통과한 manifest를 로컬 floor와 비교한다.
 * @param manifest preflight 성공 결과. 모든 입력은 호출 중 불변, NULL 불가.
 * @param floor 신뢰된 현재 snapshot. caller가 변경/Flash 작업과 직렬화한다.
 * @param out 모든 입력과 비중첩, NULL 불가. 실패 시 전체0.
 * @return OK, INVALID_ARGUMENT(NULL), MALFORMED(count/enum/중복),
 * INCOMPLETE(policy/설치 상태 미확인), STALE(floor 미만),
 * AUTH_FAILED(identity 불일치 또는 같은 sequence의 다른 digest인 CONFLICT).
 * @details uint64 직접 비교, 무힙/무I/O/무callback이다. 같은 sequence/digest여도
 * 실제 설치 증거가 없으면 ALREADY_INSTALLED를 반환하지 않는다. REPAIR에도 native
 * image 검증/새 activation commit/trial이 필요하다. floor를 갱신하지 않으며 자동
 * rollback 예외를 제공하지 않는다. owner는 상태가 바뀌면 erase/설치 전에 재검사한다.
 */
canview_status_t canview_ota_floor_check(const canview_ota_manifest_t *manifest,
    const canview_ota_floor_t *floor, canview_ota_floor_result_t *out);

#endif
