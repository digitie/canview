/* SPDX-License-Identifier: GPL-3.0-only */
/** @file fdcan_capture.h
 * @brief STM32G474 CMSIS FDCAN 수신 adapter.
 *
 * 이 adapter는 FDCAN bus-monitoring mode와 RX FIFO0만 설정한다. TX buffer
 * register 또는 송신 callback은 존재하지 않는다. IRQ는 FIFO element를
 * bounded raw ring에 복사하고, service()의 worker context가 element decode와
 * frame_sink 호출을 수행한다. PSR/ECR 상태는 status_sink으로 전달한다.
 */
#ifndef CANVIEW_STM_FDCAN_PLATFORM_H
#define CANVIEW_STM_FDCAN_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canview_stm_fdcan_capture.h"

#define CANVIEW_STM_FDCAN_PLATFORM_RAW_RING_CAPACITY (16U)

/* High bits are adapter-owned and cannot be confused with FDCAN LEC values. */
#define CANVIEW_STM_FDCAN_PLATFORM_ERROR_FIFO_LOSS (UINT32_C(0x80000000))
#define CANVIEW_STM_FDCAN_PLATFORM_ERROR_RAW_RING_OVERFLOW (UINT32_C(0x40000000))
#define CANVIEW_STM_FDCAN_PLATFORM_ERROR_MESSAGE_RAM (UINT32_C(0x20000000))

/** @brief IRQ가 복사하고 worker가 decode하는 FDCAN FIFO element snapshot. */
typedef struct
{
    uint32_t words[4];
    uint32_t source_timestamp_us;
} canview_stm_fdcan_raw_element_t;

typedef canview_status_t canview_stm_fdcan_frame_sink_fn(
    void *context, size_t channel, const canview_stm_fdcan_rx_frame_t *frame);
typedef canview_status_t canview_stm_fdcan_drop_sink_fn(void *context, size_t channel,
                                                        uint32_t dropped);
typedef void canview_stm_fdcan_status_sink_fn(
    void *context, size_t channel, canview_stm_fdcan_bus_state_t state, uint16_t rx_error_count,
    uint16_t tx_error_count, uint32_t bus_off_count, uint32_t last_error,
    uint32_t source_timestamp_us);

typedef struct
{
    canview_stm_fdcan_profile_t profiles[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    canview_stm_fdcan_frame_sink_fn *frame_sink;
    canview_stm_fdcan_drop_sink_fn *drop_sink;
    canview_stm_fdcan_status_sink_fn *status_sink;
    void *sink_context;
} canview_stm_fdcan_platform_config_t;

typedef struct
{
    canview_stm_fdcan_platform_config_t config;
    canview_stm_fdcan_raw_element_t raw_ring[CANVIEW_STM_FDCAN_CHANNEL_COUNT]
                                            [CANVIEW_STM_FDCAN_PLATFORM_RAW_RING_CAPACITY];
    /* SPSC ownership: IRQ writes raw_write_index, service() writes raw_read_index. */
    volatile uint8_t raw_read_index[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    volatile uint8_t raw_write_index[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    volatile uint32_t raw_drops[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    uint32_t reported_raw_drops[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    volatile uint32_t pending_interrupts[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    volatile bool fifo_loss_unknown[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    volatile bool message_ram_fault[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    volatile bool raw_ring_overflow[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    uint32_t bus_off_count[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    uint32_t sink_failures[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    canview_stm_fdcan_bus_state_t previous_state[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    bool started[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    bool servicing;
    bool initialized;
} canview_stm_fdcan_platform_t;

/** @brief config/profile/callback lifetime를 확인하고 adapter context를 초기화한다.
 * @param platform caller 소유 zero-init adapter context.
 * @param config profile과 정적 sink callback 설정.
 * @return 성공, profile 오류, callback 오류 또는 이미 사용 중.
 */
canview_status_t canview_stm_fdcan_platform_init(
    canview_stm_fdcan_platform_t *platform,
    const canview_stm_fdcan_platform_config_t *config);

/** @brief 검증된 profile만 FDCAN RX FIFO0와 IRQ를 bounded하게 시작한다.
 * @param platform initialized adapter context.
 * @return 성공, clock/timestamp/profile 오류 또는 resource 오류.
 */
canview_status_t canview_stm_fdcan_platform_start(canview_stm_fdcan_platform_t *platform);

/** @brief IRQ와 FIFO를 끄고 각 peripheral을 INIT 상태로 되돌린다.
 * @param platform initialized adapter context.
 * @return safe output/INIT 복구 결과.
 */
canview_status_t canview_stm_fdcan_platform_stop(canview_stm_fdcan_platform_t *platform);

/** @brief worker context에서 raw snapshot과 pending PSR/ECR 상태를 sink으로 전달한다.
 * @param platform initialized and started adapter context.
 * @param source_timestamp_us status snapshot에 사용할 u32 monotonic timestamp.
 * @return sink/decode 결과 또는 성공.
 */
canview_status_t canview_stm_fdcan_platform_service(canview_stm_fdcan_platform_t *platform,
                                                    uint32_t source_timestamp_us);

/** @cond INTERNAL */
void FDCAN1_IT0_IRQHandler(void);
void FDCAN2_IT0_IRQHandler(void);
void FDCAN3_IT0_IRQHandler(void);
/** @endcond */

#endif
