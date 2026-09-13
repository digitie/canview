/* SPDX-License-Identifier: GPL-3.0-only */
/** @file body.h @brief 검증한 manifest에 대한 순차 image SHA-256 검사. */
#ifndef CANVIEW_OTA_BODY_H
#define CANVIEW_OTA_BODY_H

#include <stdbool.h>
#include "manifest.h"

#define CANVIEW_OTA_BODY_CHUNK_MAX (16384U)

/** @brief SDK SHA-256 operation 계약. 자체 암호 구현이나 Flash writer가 아니다.
 * @details start는 새 SHA-256 operation, update는 호출 중만 빌리는 bytes,
 * finish는32-byte digest를 출력한다. reset은 partial start/finish 뒤에도 허용하며
 * 성공 시 모든 operation 자원을 해제한다. reset 실패면 context를 보존해 재시도한다.
 * provider는 같은 body로 재진입하거나 body/입력을 직접 수정하면 안 된다.
 * 각 함수는 OK 또는 실제 오류를 반환한다. context는 NULL 불가이며 reset 성공까지 유효해야 한다.
 */
typedef struct
{
    void *context;
    canview_status_t (*start)(void *context);
    canview_status_t (*update)(void *context, const uint8_t *data, size_t size);
    canview_status_t (*finish)(void *context, uint8_t digest[CANVIEW_OTA_DIGEST_BYTES]);
    canview_status_t (*reset)(void *context);
} canview_ota_hash_t;

/** @brief HASHES_MATCHED도 native image 서명/설치 검증 완료가 아니다. */
typedef enum
{
    CANVIEW_OTA_BODY_EMPTY = 0,
    CANVIEW_OTA_BODY_RECEIVING,
    CANVIEW_OTA_BODY_HASHES_MATCHED,
    CANVIEW_OTA_BODY_FAILED
} canview_ota_body_state_t;

/** @brief 단일 owner의 고정 메모리. 처음에 반드시 {0}으로 초기화한다.
 * @details 멤버는 모듈 소유다. 외부 수정/활성 객체 복사/memset 금지. hash context만
 * reset 성공까지 빌리고 prefix/chunk/identity는 보존하지 않는다. busy는 callback
 * 재진입 방어이며 thread lock이 아니다. task owner가 호출을 직렬화해야 한다.
 */
typedef struct
{
    canview_ota_manifest_t manifest;
    canview_ota_hash_t hash;
    canview_ota_body_state_t state;
    canview_status_t error;
    canview_status_t cleanup_error;
    uint32_t image_index;
    uint32_t image_bytes;
    uint32_t next_offset;
    bool hash_live;
    bool busy;
} canview_ota_body_t;

/** @brief 완전한 prefix를 검증하고 첫 image의 hash operation을 시작한다.
 * @param body {0} 또는 reset 성공 뒤 EMPTY인 객체. 다른 인자와 비중첩.
 * @param prefix 호출 중만 읽는 완전한 prefix. body image는 포함하지 않는다.
 * @param size prefix 실제 크기. prefix 조립 buffer는 caller가 최대16KiB manifest로 제한한다.
 * @param identity 신뢰된 BSP/provisioning identity. manifest.h 계약을 따른다.
 * @param runtime 신뢰된 로컬 ABI/boot/recovery/config/capability snapshot. 호출 중만 빌린다.
 * @param verify 신뢰된 manifest 서명 verifier.
 * @param verify_context verify 호출 중만 유효하면 된다.
 * @param hash SDK SHA-256 provider. 함수표는 복사하고 context만 빌린다.
 * @return OK 또는 prefix/provider 오류. 활성/완료 객체 재사용은 RESOURCE_BUSY.
 * @details 오류 후 reset이 필요하다. native signature/version floor/erase 권한은 검사하지 않는다.
 */
canview_status_t canview_ota_body_open(
    canview_ota_body_t *body, const uint8_t *prefix, size_t size,
    const canview_ota_identity_t *identity, const canview_ota_runtime_t *runtime,
    canview_ota_manifest_verify_fn verify,
    void *verify_context, const canview_ota_hash_t *hash);

/** @brief 절대 file offset 순서대로 최대16KiB chunk를 소비한다.
 * @param body 활성 단일 owner 객체.
 * @param offset 기대하는 다음 절대 file offset. 중복/누락을 거부한다.
 * @param data size만큼 읽을 수 있는 불변 입력. size0일 때만 NULL 허용.
 * @param size 0..16KiB. 이미지 경계를 넘는 chunk도 허용한다.
 * @return OK 또는 실패. 실패 시 일부 hash 처리가 있었어도 FAILED이며 재전송으로 복구하지 않는다.
 * @details chunk를 보존하지 않는다. 완료 뒤 추가 bytes도 실패시킨다. 모든 image hash
 * 일치는 HASHES_MATCHED일 뿐 PREPARED/boot selector/erase/write 허가가 아니다.
 */
canview_status_t canview_ota_body_feed(canview_ota_body_t *body, uint32_t offset,
                                     const uint8_t *data, size_t size);

/** @brief EOF 확인. 부족한 body는 INCOMPLETE와 FAILED로 종료한다.
 * @param body 검사 객체. NULL 불가.
 * @return 전체 길이/hash가 맞으면 OK. native image 검증과 별개다.
 */
canview_status_t canview_ota_body_finish(canview_ota_body_t *body);

/** @brief 부분 입력·실패·완료 객체를 정리한다. reset 실패는 재시도 가능하다.
 * @param body {0}으로 초기화된 객체. NULL 불가.
 * @return 정리 성공은 EMPTY/OK, 실패는 FAILED이며 provider/context를 계속 보존한다.
 */
canview_status_t canview_ota_body_reset(canview_ota_body_t *body);

#endif
