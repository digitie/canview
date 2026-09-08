/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_stack.h
 * @brief static stack guard watermark 계약.
 */
#ifndef CANVIEW_STM_STACK_H
#define CANVIEW_STM_STACK_H

#include "canview_status.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CANVIEW_STM_STACK_WATERMARK_PATTERN UINT8_C(0xa5)
#define CANVIEW_STM_STACK_WATERMARK_PATTERN_INVERTED UINT8_C(0x5a)
#define CANVIEW_STM_STACK_WATERMARK_MIN_BYTES (64U)
#define CANVIEW_STM_STACK_WATERMARK_MAX_BYTES (24U * 1024U)
#define CANVIEW_STM_STACK_SAMPLE_MAX_BYTES (256U)
#define CANVIEW_STM_STACK_MIN_FREE_BYTES (128U)

/** @brief caller가 정적으로 소유하는 watermark context. zero-init 후 한 번 arm한다. */
typedef struct
{
    volatile uint8_t *region;
    size_t region_size;
    size_t minimum_free_bytes;
    bool armed;
    bool busy;
} canview_stm_stack_watermark_t;

typedef struct
{
    size_t current_free_bytes;
    size_t minimum_free_bytes;
    bool valid;
} canview_stm_stack_watermark_snapshot_t;

/**
 * @brief 사용 가능한 reserved stack prefix를 pattern으로 채운다.
 *
 * region은 현재 call frame 아래의 안전한 범위여야 한다. 이 API는 caller가
 * 제공한 memory의 유효성을 추정하지 않으며 ISR에서 호출하지 않는다.
 */
canview_status_t canview_stm_stack_watermark_arm(canview_stm_stack_watermark_t *watermark,
                                                  volatile uint8_t *region, size_t region_size);

/**
 * @brief low-address의 연속 pattern prefix를 bounded scan하고 최소 free watermark를 갱신한다.
 * scan budget보다 큰 free prefix는 budget 크기의 보수적 lower bound로 보고한다.
 * pattern 불일치나 malformed context는 free 공간으로 승격하지 않는다.
 */
canview_status_t canview_stm_stack_watermark_sample(
    canview_stm_stack_watermark_t *watermark,
    canview_stm_stack_watermark_snapshot_t *snapshot);

#endif
