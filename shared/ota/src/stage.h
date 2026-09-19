/* SPDX-License-Identifier: GPL-3.0-only */
/** @file stage.h @brief 인증한 컨테이너의 수신 저장 순서. 설치 상태기계가 아니다. */
#ifndef CANVIEW_OTA_STAGE_H
#define CANVIEW_OTA_STAGE_H

#include "body.h"

/** @brief 단일 BSP storage owner의 동기 수신 계약. 모든 callback은 task context다.
 * @details begin은 인증/호환성/floor 검사 뒤에만 호출된다. manifest의 enum target을
 * 고정 BSP map의 비활성 slot/staging에만 대응하고 현재 활성/bootloader/recovery/
 * provisioning/유일한 정상본을 거절한 뒤 수신용 erase와 prefix 보존을 수행한다.
 * manifest/prefix pointer는 호출 중만 빌린다. 임의 주소/경로는 입력으로 받지 않는다.
 * write의 offset은 컨테이너 절대 offset이며 padding을 포함한다. BSP가 보존한 map으로
 * 저장하고 read-back까지 성공해야 OK다. 비동기 DMA/queue에 input pointer 보존 금지.
 * verify는 저장한 모든 native image의 전체 hash/서명/metadata를 manifest와 대조한다.
 * begin/write/verify/close 모두 PREPARED/journal/boot selector/설치를 수행하면 안 된다.
 * close는 partial begin에도 유효하며 수신 자원만 해제한다. 실패하면 context를 보존해
 * 재시도한다. context와 입력을 바꾸거나 같은 stage에 재진입하면 안 된다.
 * BSP map/Flash 불변성·freshness·전원 조건의 실제 enforcement는 storage owner 책임이다.
 */
typedef struct
{
    void *context;
    canview_status_t (*begin)(void *context, const canview_ota_manifest_t *manifest,
        const uint8_t *prefix, size_t size);
    canview_status_t (*write)(void *context, uint32_t offset, const uint8_t *data, size_t size);
    canview_status_t (*verify)(void *context, const canview_ota_manifest_t *manifest);
    canview_status_t (*close)(void *context);
} canview_ota_storage_t;

/** @brief NATIVE_MATCHED도 설치 권한이나 영속 PREPARED 상태가 아니다. */
typedef enum
{
    CANVIEW_OTA_STAGE_EMPTY = 0,
    CANVIEW_OTA_STAGE_RECEIVING,
    CANVIEW_OTA_STAGE_NATIVE_MATCHED,
    CANVIEW_OTA_STAGE_FAILED
} canview_ota_stage_state_t;

/** @brief {0}으로 시작하는 단일 task 소유 고정 메모리. 멤버 직접 수정/복사 금지.
 * @details body를 별도로 호출하지 않는다. prefix/chunk는 저장하지 않으며 hash/storage
 * context는 reset 성공까지 유효해야 한다. busy는 재진입 방어이지 thread lock이 아니다.
 * reset까지 같은 owner가 storage를 독점하고 외부 write/검증을 직렬화해야 한다.
 */
typedef struct
{
    canview_ota_body_t body;
    canview_ota_storage_t storage;
    canview_ota_stage_state_t state;
    canview_status_t error;
    canview_status_t cleanup_error;
    bool storage_live;
    bool busy;
} canview_ota_stage_t;

/** @brief 완전한 prefix의 기존 사전 검증 뒤에만 storage.begin을 실행한다.
 * @param stage 초기 {0} 또는 reset 성공한 객체. 모든 입력/context와 비중첩.
 * @param prefix 호출 중 불변인 완전한 prefix. 본문/정렬 padding 제외.
 * @param size 실제 prefix 크기. 기존 bounded parser 상한을 따른다.
 * @param identity 신뢰된 BSP/provisioning identity.
 * @param runtime 신뢰된 로컬 호환성 snapshot.
 * @param floor 신뢰된 영속 정책/정상 앱 snapshot.
 * @param verify 신뢰된 manifest 서명 provider.
 * @param verify_context verify 동안 빌리는 context.
 * @param hash SDK hash provider. context는 reset 성공까지 유효.
 * @param storage 동기 BSP 저장 provider. context는 reset 성공까지 유효.
 * @return OK는 수신 개시일 뿐 설치 승인이 아니다. 실패 뒤 reset 필요.
 */
canview_status_t canview_ota_stage_open(canview_ota_stage_t *stage,
    const uint8_t *prefix, size_t size, const canview_ota_identity_t *identity,
    const canview_ota_runtime_t *runtime, const canview_ota_floor_t *floor,
    canview_ota_manifest_verify_fn verify, void *verify_context,
    const canview_ota_hash_t *hash, const canview_ota_storage_t *storage);

/** @brief body의 순서/길이/padding/hash 검증에 성공한 chunk만 storage.write에 전달한다.
 * @param stage 수신 중인 단일 owner 객체.
 * @param offset prefix 이후의 다음 절대 컨테이너 offset.
 * @param data 호출 중 불변·비중첩 입력. size0이면 NULL 허용.
 * @param size 0..CANVIEW_OTA_BODY_CHUNK_MAX. 0이면 write를 호출하지 않는다.
 * @return 오류 뒤 계속 쓰지 않으며 reset이 필요하다. 이미 쓴 후보는 설치하지 않는다.
 */
canview_status_t canview_ota_stage_feed(canview_ota_stage_t *stage, uint32_t offset,
    const uint8_t *data, size_t size);

/** @brief 전체 body와 쓰기가 끝난 뒤 저장된 native 검증을 한 번만 실행한다.
 * @param stage 수신 중 객체.
 * @return OK는 NATIVE_MATCHED. local policy/freshness 재검사·승인·설치는 별도다.
 */
canview_status_t canview_ota_stage_finish(canview_ota_stage_t *stage);

/** @brief storage close와 body reset. 실패한 cleanup은 context를 유지해 재시도한다.
 * @param stage 단일 owner 객체. NULL 불가.
 * @return 둘 다 성공해야 EMPTY. 최초 작업 오류는 cleanup 성공 전까지 보존한다.
 */
canview_status_t canview_ota_stage_reset(canview_ota_stage_t *stage);

#endif
