/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_esp_pool.h
 * @brief Task/callback용 복사 소유권 고정 pool. wire 형식이나 ISR API가 아니다.
 */
#ifndef CANVIEW_ESP_POOL_H
#define CANVIEW_ESP_POOL_H
#include "canview_status.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CANVIEW_ESP_POOL_SLOTS (16U)
#define CANVIEW_ESP_POOL_BYTES (256U)
typedef void canview_esp_pool_lock_fn_t(void *context);
typedef struct
{
    canview_esp_pool_lock_fn_t *enter;
    canview_esp_pool_lock_fn_t *leave;
    void *context;
} canview_esp_pool_port_t;
typedef struct
{
    uintptr_t owner;
    uint32_t generation;
    uint16_t slot;
} canview_esp_pool_token_t;
typedef struct
{
    uint32_t generation;
    uint16_t length;
    bool used;
    uint8_t data[CANVIEW_ESP_POOL_BYTES];
} canview_esp_pool_slot_t;
typedef struct
{
    uint32_t exhausted;
    uint32_t stale;
    uint16_t used;
    uint16_t high_water;
    uint16_t retired;
} canview_esp_pool_stats_t;
/** Zero-init 후 init 한 번. 저장소는 caller 소유이며 직접 필드 접근은 금지한다. */
typedef struct
{
    canview_esp_pool_port_t port;
    canview_esp_pool_slot_t slots[CANVIEW_ESP_POOL_SLOTS];
    canview_esp_pool_stats_t stats;
    uint16_t capacity;
    bool initialized;
} canview_esp_pool_t;

/**
 * @brief 고정 저장소를 초기화한다. 모든 사용자 시작 전에 단일 owner가 호출한다.
 * @param pool zero-init context. 이후 재초기화는 거부한다.
 * @param capacity 사용할 slot 수, 1..16. 나머지 저장소를 재사용하지 않는다.
 * @param port task 문맥의 bounded/non-reentrant critical lock. pool보다 오래 살아야 한다.
 * @return 잘못된 인자는 INVALID_ARGUMENT, 이미 초기화했으면 RESOURCE_BUSY다.
 */
canview_status_t canview_esp_pool_init(canview_esp_pool_t *pool, uint16_t capacity,
                                       const canview_esp_pool_port_t *port);
/**
 * @brief payload를 pool로 복사하고 단독 소유 token을 반환한다.
 * @param pool 초기화한 context. 모든 접근은 동일 lock으로 보호된다.
 * @param data 복사할 1..256 byte, context와 겹치면 안 된다.
 * @param length byte 수.
 * @param token 성공 시 새 token; 실패 시 변경하지 않는다. context/data 밖 저장소다.
 * @return 부족하면 RESOURCE_BUSY 및 포화 exhaustion counter 증가. generation은 wrap하지 않는다.
 *
 * Token은 pool 주소와 generation에 결합된 RAM 전용 값이다. serialize하거나 reboot 뒤 재사용하지
 * 않는다.
 */
canview_status_t canview_esp_pool_acquire(canview_esp_pool_t *pool, const void *data, size_t length,
                                          canview_esp_pool_token_t *token);
/**
 * @brief token의 payload를 caller 버퍼로 복사한다. 소유권은 해제하지 않는다.
 * @param pool 초기화한 context.
 * @param token 현재 단독 owner의 token. 다른 owner와 동시에 release하지 않는다.
 * @param data context 밖 출력 버퍼. 실패 시 보존한다.
 * @param capacity 출력 byte 용량.
 * @param length 성공 시 복사 byte 수. data/context와 겹치면 안 된다.
 * @return STALE 또는 BUFFER_TOO_SMALL을 구분한다. 내부 pointer를 반환하지 않는다.
 */
canview_status_t canview_esp_pool_copy(canview_esp_pool_t *pool, canview_esp_pool_token_t token,
                                       void *data, size_t capacity, size_t *length);
/**
 * @brief token을 한 번만 해제하고 payload를 지운다.
 * @param pool 초기화한 context.
 * @param token 단독 owner의 현재 token. 재사용·과거 token은 거부한다.
 * @return 현재 token이면 OK, double/stale release면 STALE과 포화 counter 증가다.
 */
canview_status_t canview_esp_pool_release(canview_esp_pool_t *pool, canview_esp_pool_token_t token);
/**
 * @brief lock 아래 통계 snapshot을 복사한다.
 * @param pool 초기화한 context.
 * @param stats context와 겹치지 않는 caller 출력. 실패 시 보존한다.
 * @return 성공은 OK, 잘못된 context/출력은 INVALID_ARGUMENT이다.
 */
canview_status_t canview_esp_pool_stats(canview_esp_pool_t *pool, canview_esp_pool_stats_t *stats);
#endif
