/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_queue.h
 * @brief 고정 record를 동기 복사하는 bounded queue. heap/borrowed payload 없음.
 * Producer/consumer는 동일 core에 한정한다. 모든 push/pop은 필수 critical port로
 * 직렬화하며 NMI/HardFault에서 사용하지 않는다. DMA가 queue storage를 직접 쓰면 안 된다.
 */
#ifndef CANVIEW_STM_QUEUE_H
#define CANVIEW_STM_QUEUE_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "canview_status.h"
#define CANVIEW_STM_QUEUE_RECORD_MAX (64U)
#define CANVIEW_STM_QUEUE_CAPACITY_MAX (256U)
typedef uint32_t canview_stm_lock_fn(void *context);
typedef void canview_stm_unlock_fn(void *context, uint32_t saved_mask);
typedef struct
{
    canview_stm_lock_fn *enter;
    canview_stm_unlock_fn *leave;
    void *context;
} canview_stm_critical_t;
typedef struct
{
    uint8_t *storage;
    size_t record_size;
    size_t capacity;
    size_t read_index;
    size_t write_index;
    size_t count;
    size_t high_water;
    uint32_t dropped;
    canview_stm_critical_t critical;
    bool initialized;
} canview_stm_queue_t;

/** @brief caller 소유 storage를 등록. ISR 활성화 전 한 번만 호출한다.
 * @param queue zero-init context. storage와 겹치지 않는다.
 * @param storage queue 수명 전체 유효한 전용 배열. 외부 직접 접근 금지.
 * @param bytes 배열 크기. capacity*record_size 이상.
 * @param capacity 1..256 record. full이면 새 record를 거부한다.
 * @param record_size 1..64 bytes. header/payload pointer 대신 소유 bytes만 담는다.
 * @param critical enter/leave 필수. 이전 interrupt mask를 그대로 복원해야 한다.
 * @return 입력 실패 시 queue/storage 불변. 초기화 이후 reset/flush 없음.
 */
canview_status_t canview_stm_queue_init(canview_stm_queue_t *queue, uint8_t *storage, size_t bytes,
                                        size_t capacity, size_t record_size,
                                        const canview_stm_critical_t *critical);
/** @brief record 전체를 동기 복사. full이면 drop counter만 saturating 증가.
 * @param queue initialized context.
 * @param record record_size bytes 이상, queue/storage와 겹치지 않는 입력.
 * @param bytes 입력 크기는 record_size와 같아야 한다.
 * @return OK, RESOURCE_BUSY(full) 또는 INVALID_ARGUMENT. 부분 publish 없음.
 */
canview_status_t canview_stm_queue_push(canview_stm_queue_t *queue, const void *record,
                                        size_t bytes);
/** @brief 가장 오래된 record 전체를 caller 버퍼로 복사한 뒤 dequeue.
 * @param queue initialized context.
 * @param record queue/storage와 겹치지 않는 출력 버퍼.
 * @param bytes 출력 크기는 record_size와 같아야 한다.
 * @return OK, INCOMPLETE(empty) 또는 INVALID_ARGUMENT. 실패 시 출력 불변.
 */
canview_status_t canview_stm_queue_pop(canview_stm_queue_t *queue, void *record, size_t bytes);
#endif
