/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_fdcan_capture.h
 * @brief STM32 3채널 classic CAN capture 계약.
 *
 * 이 모듈은 수신 전용 fixed ring과 wire batch 변환만 제공한다. CAN 송신
 * descriptor, transmit queue, control lease 또는 raw command 경로는 없다.
 * ISR producer와 단일 worker consumer는 caller가 제공한 critical port로
 * 직렬화한다. 모든 context와 callback은 caller가 정적 수명으로 보유한다.
 */
#ifndef CANVIEW_STM_FDCAN_CAPTURE_H
#define CANVIEW_STM_FDCAN_CAPTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canview_stm_queue.h"
#include "canview_status.h"
#include "canview_wire.h"

#define CANVIEW_STM_FDCAN_CHANNEL_COUNT (3U)
#define CANVIEW_STM_FDCAN_RING_CAPACITY (64U)
#define CANVIEW_STM_FDCAN_INVENTORY_CAPACITY (64U)
#define CANVIEW_STM_FDCAN_PERIOD_SAMPLE_CAPACITY (8U)
#define CANVIEW_STM_FDCAN_MAX_DATA_BYTES (8U)
#define CANVIEW_STM_FDCAN_KERNEL_CLOCK_HZ (UINT32_C(80000000))
#define CANVIEW_STM_FDCAN_CAN3_BITRATE (UINT32_C(125000))
#define CANVIEW_STM_FDCAN_NO_DATA_TIMEOUT_US (UINT64_C(100000))
#define CANVIEW_STM_FDCAN_TIMESTAMP_HALF_RANGE (UINT32_C(0x80000000))

/* Input-only flags. FD/BRS are deliberately recognized and rejected. */
#define CANVIEW_STM_FDCAN_FRAME_IDE (UINT8_C(0x01))
#define CANVIEW_STM_FDCAN_FRAME_RTR (UINT8_C(0x02))
#define CANVIEW_STM_FDCAN_FRAME_ERROR (UINT8_C(0x04))
#define CANVIEW_STM_FDCAN_FRAME_FD (UINT8_C(0x10))
#define CANVIEW_STM_FDCAN_FRAME_BRS (UINT8_C(0x20))
#define CANVIEW_STM_FDCAN_FRAME_FLAGS_MASK                                           \
    (CANVIEW_STM_FDCAN_FRAME_IDE | CANVIEW_STM_FDCAN_FRAME_RTR |                     \
     CANVIEW_STM_FDCAN_FRAME_ERROR | CANVIEW_STM_FDCAN_FRAME_FD |                     \
     CANVIEW_STM_FDCAN_FRAME_BRS)

#define CANVIEW_STM_FDCAN_STATUS_CONFIGURED (UINT8_C(0x01))
#define CANVIEW_STM_FDCAN_STATUS_DATA_SEEN (UINT8_C(0x02))
#define CANVIEW_STM_FDCAN_STATUS_DROPPED (UINT8_C(0x04))
#define CANVIEW_STM_FDCAN_STATUS_FD_UNSUPPORTED (UINT8_C(0x08))
#define CANVIEW_STM_FDCAN_STATUS_MALFORMED (UINT8_C(0x10))
#define CANVIEW_STM_FDCAN_STATUS_TIMESTAMP_WRAP (UINT8_C(0x20))
#define CANVIEW_STM_FDCAN_STATUS_NO_DATA (UINT8_C(0x40))
#define CANVIEW_STM_FDCAN_STATUS_INVENTORY_FULL (UINT8_C(0x80))

/* Adapter-owned loss/fault bits are part of the module status contract. */
#define CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS (UINT32_C(0x80000000))
#define CANVIEW_STM_FDCAN_ERROR_RAW_RING_OVERFLOW (UINT32_C(0x40000000))
#define CANVIEW_STM_FDCAN_ERROR_MESSAGE_RAM (UINT32_C(0x20000000))
#define CANVIEW_STM_FDCAN_ERROR_STICKY_MASK                                               \
    (CANVIEW_STM_FDCAN_ERROR_FIFO_LOSS | CANVIEW_STM_FDCAN_ERROR_RAW_RING_OVERFLOW |        \
     CANVIEW_STM_FDCAN_ERROR_MESSAGE_RAM)

typedef enum
{
    CANVIEW_STM_FDCAN_BUS_UNKNOWN_BITRATE = 0,
    CANVIEW_STM_FDCAN_BUS_NO_DATA,
    CANVIEW_STM_FDCAN_BUS_ERROR_ACTIVE,
    CANVIEW_STM_FDCAN_BUS_ERROR_PASSIVE,
    CANVIEW_STM_FDCAN_BUS_OFF,
    CANVIEW_STM_FDCAN_BUS_FAULT,
    CANVIEW_STM_FDCAN_BUS_STATE_MAX
} canview_stm_fdcan_bus_state_t;

/** @brief FDCAN NBTP의 N-1 encoding 전 논리 timing 값. */
typedef struct
{
    uint16_t prescaler;
    uint16_t time_segment1;
    uint8_t time_segment2;
    uint8_t sync_jump_width;
} canview_stm_fdcan_timing_t;

typedef enum
{
    CANVIEW_STM_FDCAN_TRANSCEIVER_UNKNOWN = 0,
    CANVIEW_STM_FDCAN_TRANSCEIVER_TCAN1046,
    CANVIEW_STM_FDCAN_TRANSCEIVER_MAX3055,
    CANVIEW_STM_FDCAN_TRANSCEIVER_MAX
} canview_stm_fdcan_transceiver_t;

/** @brief 수신 profile. disabled/unknown profile은 모든 채널을 standby로 남긴다. */
typedef struct
{
    bool enabled;
    bool bitrate_known;
    bool transceiver_known;
    canview_stm_fdcan_transceiver_t transceiver;
    uint32_t nominal_bitrate;
    uint32_t data_bitrate;
    canview_stm_fdcan_timing_t nominal_timing;
    canview_stm_fdcan_timing_t data_timing;
} canview_stm_fdcan_profile_t;

/** @brief profile timing table에서 classic CAN 값을 복사한다.
 * 출력은 bitrate가 지원되는 경우에만 변경되며, transceiver_known은 false다.
 * caller가 실제 board/PHY evidence를 확인한 뒤에만 true로 바꾸고 초기화한다.
 * @param nominal_bitrate 지원할 nominal bitrate.
 * @param data_bitrate 0이어야 하며 non-zero는 FD 설정으로 거부한다.
 * @param profile caller 소유 출력 profile.
 * @return 성공, 입력 오류 또는 지원하지 않는 bitrate/FD 설정.
 */
canview_status_t canview_stm_fdcan_profile_for_bitrate(
    uint32_t nominal_bitrate, uint32_t data_bitrate, canview_stm_fdcan_profile_t *profile);

/** @brief profile의 disabled/bitrate/transceiver/timing 불변식을 검사한다.
 * @param profile 검사할 profile.
 * @return profile이 유효하면 CANVIEW_OK, 아니면 입력 오류/미지원 상태.
 */
canview_status_t canview_stm_fdcan_profile_validate(const canview_stm_fdcan_profile_t *profile);

/** @brief board channel별 PHY contract까지 검사한다. disabled channel은 standby로 허용한다.
 * @param channel 0..CANVIEW_STM_FDCAN_CHANNEL_COUNT-1 board channel.
 * @param profile channel에 적용할 profile.
 * @return 공통 profile과 board PHY contract가 유효한지 나타내는 상태.
 */
canview_status_t canview_stm_fdcan_channel_profile_validate(
    size_t channel, const canview_stm_fdcan_profile_t *profile);

/** @brief FDCAN FIFO element를 portable record로 전달하는 입력 snapshot. */
typedef struct
{
    uint32_t source_timestamp_us;
    uint32_t can_id;
    uint8_t dlc;
    uint8_t flags;
    uint8_t data[CANVIEW_STM_FDCAN_MAX_DATA_BYTES];
} canview_stm_fdcan_rx_frame_t;

/** @brief raw FDCAN RX element의 W1..W4를 input snapshot으로 변환한다.
 * FD/DLC>8은 여기서 보존하고, 상위 capture가 unsupported/malformed로 분리한다.
 * words는 register read로 caller가 복사한 4개의 little-endian host word다.
 * @param words W1..W4 snapshot.
 * @param source_timestamp_us caller가 읽은 u32 monotonic timestamp.
 * @param frame caller 소유 출력 frame.
 * @return 입력이 유효하면 CANVIEW_OK.
 */
canview_status_t canview_stm_fdcan_decode_element(const uint32_t words[4],
                                                  uint32_t source_timestamp_us,
                                                  canview_stm_fdcan_rx_frame_t *frame);

/** @brief timestamp가 확장된 내부 record. wire struct와 직접 cast하지 않는다. */
typedef struct
{
    uint64_t timestamp_us;
    uint32_t can_id;
    uint8_t bus_id;
    uint8_t flags;
    uint8_t dlc;
    uint8_t data[CANVIEW_STM_FDCAN_MAX_DATA_BYTES];
} canview_stm_fdcan_record_t;

typedef bool canview_stm_fdcan_filter_fn(const canview_stm_fdcan_record_t *record,
                                         void *context);

/** @brief worker/diagnostic용 채널 snapshot. dropped_frames는 saturate 전 exact counter다.
 * hardware_fault_count는 현재 session에서 새로 관찰한 adapter fault latch 수다.
 */
typedef struct
{
    canview_stm_fdcan_bus_state_t state;
    uint8_t status_flags;
    uint16_t rx_error_count;
    uint16_t tx_error_count;
    uint32_t bitrate;
    uint32_t bus_off_count;
    uint32_t last_error;
    uint64_t last_timestamp_us;
    uint32_t accepted_frames;
    uint32_t dropped_frames;
    uint32_t hardware_fault_count;
    uint32_t unsupported_frames;
    uint32_t malformed_frames;
    uint32_t filtered_frames;
    size_t queued_frames;
    size_t high_water_frames;
} canview_stm_fdcan_channel_stats_t;

/** @brief worker가 만든 generic bus/ID/DLC inventory snapshot. signal 의미는 없다. */
typedef struct
{
    bool valid;
    uint8_t bus_id;
    uint8_t flags;
    uint8_t dlc;
    uint32_t can_id;
    uint16_t rate_tenth_hz;
    uint32_t frame_count;
    uint32_t change_count;
    uint32_t period_p50_us;
    uint32_t period_p95_us;
    uint64_t bit_change_mask;
    uint64_t first_timestamp_us;
    uint64_t last_timestamp_us;
    uint8_t last_data[CANVIEW_STM_FDCAN_MAX_DATA_BYTES];
    uint32_t period_samples[CANVIEW_STM_FDCAN_PERIOD_SAMPLE_CAPACITY];
    uint8_t period_sample_count;
    uint8_t period_sample_index;
    uint16_t reserved;
} canview_stm_fdcan_inventory_entry_t;

typedef struct
{
    canview_stm_fdcan_record_t records[CANVIEW_STM_FDCAN_RING_CAPACITY];
    size_t read_index;
    size_t write_index;
    size_t count;
    size_t high_water;
    uint32_t dropped;
    uint32_t hardware_fault_count;
    uint32_t sticky_error_flags;
    uint32_t accepted;
    uint32_t unsupported;
    uint32_t malformed;
    uint32_t filtered;
    uint16_t rx_error_count;
    uint16_t tx_error_count;
    uint32_t bus_off_count;
    uint32_t last_error;
    uint32_t bitrate;
    uint64_t last_timestamp_us;
    uint64_t last_status_timestamp_us;
    uint32_t last_source_timestamp_us;
    uint64_t timestamp_epoch_us;
    canview_stm_fdcan_bus_state_t state;
    uint8_t status_flags;
    bool enabled;
    bool timestamp_initialized;
    bool status_timestamp_initialized;
    bool data_seen;
} canview_stm_fdcan_channel_t;

/** @brief caller-owned, zero-initialized capture context. */
typedef struct
{
    canview_stm_fdcan_channel_t channels[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    canview_stm_fdcan_inventory_entry_t inventory[CANVIEW_STM_FDCAN_INVENTORY_CAPACITY];
    size_t inventory_count;
    uint32_t inventory_dropped;
    uint32_t reported_dropped[CANVIEW_STM_FDCAN_CHANNEL_COUNT];
    uint32_t last_source_timestamp_us;
    uint64_t timestamp_epoch_us;
    uint64_t extended_timestamp_us;
    canview_stm_critical_t critical;
    canview_stm_fdcan_filter_fn *filter;
    void *filter_context;
    bool initialized;
    bool timestamp_initialized;
    bool building;
    bool reentry_requested;
} canview_stm_fdcan_capture_t;

/** @brief profile 검증 후 세 채널 ring과 timestamp state를 초기화한다.
 * @param capture caller 소유 zero-init context. 재초기화는 거부한다.
 * @param profiles 세 channel profile 배열.
 * @param critical ISR/worker shared state를 직렬화하는 정적 critical port.
 * @param filter worker에서 실행할 optional observer filter.
 * @param filter_context filter가 사용하는 정적 context.
 * @return 성공, 입력 오류, 이미 초기화됨 또는 profile 오류.
 */
canview_status_t canview_stm_fdcan_capture_init(
    canview_stm_fdcan_capture_t *capture,
    const canview_stm_fdcan_profile_t profiles[CANVIEW_STM_FDCAN_CHANNEL_COUNT],
    const canview_stm_critical_t *critical, canview_stm_fdcan_filter_fn *filter,
    void *filter_context);

/** @brief 현재 capture session의 frame·timestamp·inventory·counter를 폐기한다.
 * @param capture initialized capture context.
 * @return session reset 성공 또는 입력 오류.
 *
 * profile, critical port, filter callback 계약은 유지한다. 호출자는 worker
 * context에서만 실행해야 하며, reset 뒤 들어오는 frame만 새 session으로
 * 해석한다.
 */
canview_status_t canview_stm_fdcan_capture_reset(canview_stm_fdcan_capture_t *capture);

/** @brief ISR에서 bounded copy/validate/enqueue한다. callback/malloc/blocking은 없다.
 * @param capture initialized capture context.
 * @param channel frame이 발생한 channel.
 * @param frame caller가 준비한 단일 input snapshot.
 * @return enqueue 성공, malformed/unsupported, ring full 또는 입력 오류.
 */
canview_status_t canview_stm_fdcan_capture_ingest(canview_stm_fdcan_capture_t *capture,
                                                  size_t channel,
                                                  const canview_stm_fdcan_rx_frame_t *frame);

/** @brief platform raw-ring 포화로 잃은 frame 수를 worker에서 exact drop으로 반영한다.
 * @param capture initialized capture context.
 * @param channel drop이 발생한 channel.
 * @param dropped 누적할 drop 수.
 * @return 성공, 입력 오류 또는 disabled channel.
 */
canview_status_t canview_stm_fdcan_capture_record_drops(canview_stm_fdcan_capture_t *capture,
                                                         size_t channel, uint32_t dropped);

/** @brief worker context에서 세 ring을 timestamp 순서로 최대 12개 batch로 만든다.
 * @param capture initialized single-worker capture context.
 * @param batch caller 소유 출력 wire batch.
 * @return batch 생성, pending 없음, callback reentry 또는 데이터 오류.
 */
canview_status_t canview_stm_fdcan_capture_build_batch(canview_stm_fdcan_capture_t *capture,
                                                        canview_wire_can_batch_t *batch);

/** @brief no-data timeout을 적용한다. now_us는 같은 monotonic timestamp domain이어야 한다.
 * @param capture initialized capture context.
 * @param now_us 현재 extended monotonic timestamp.
 * @return 상태 적용 성공, timestamp 역행 또는 입력 오류.
 */
canview_status_t canview_stm_fdcan_capture_observe(canview_stm_fdcan_capture_t *capture,
                                                   uint64_t now_us);

/** @brief hardware PSR/ECR snapshot을 worker에서 반영한다.
 * @param capture initialized capture context.
 * @param channel 상태가 발생한 channel.
 * @param state PSR/ECR에서 분류한 bus state.
 * @param rx_error_count hardware receive error counter.
 * @param tx_error_count hardware transmit error counter.
 * @param bus_off_count 누적 bus-off transition count.
 * @param last_error LEC와 pending hardware error snapshot. adapter-owned high bits are
 *                   sticky for the current session.
 * @param timestamp_us status snapshot의 extended timestamp.
 * @return 상태 반영 성공, stale/역행 timestamp, disabled channel 또는 입력 오류.
 */
canview_status_t canview_stm_fdcan_capture_set_status(
    canview_stm_fdcan_capture_t *capture, size_t channel, canview_stm_fdcan_bus_state_t state,
    uint16_t rx_error_count, uint16_t tx_error_count, uint32_t bus_off_count,
    uint32_t last_error, uint64_t timestamp_us);

/** @brief channel snapshot을 caller buffer에 복사한다.
 * @param capture initialized capture context.
 * @param channel snapshot 대상 channel.
 * @param stats caller 소유 출력 snapshot.
 * @return 성공 또는 입력 오류.
 */
canview_status_t canview_stm_fdcan_capture_get_stats(
    const canview_stm_fdcan_capture_t *capture, size_t channel,
    canview_stm_fdcan_channel_stats_t *stats);

/** @brief generic ID inventory를 index 순서로 복사한다. signal decode/승격은 하지 않는다.
 * @param capture initialized capture context.
 * @param index inventory index.
 * @param entry caller 소유 출력 entry.
 * @return entry 복사 성공, 아직 없는 index 또는 입력 오류.
 */
canview_status_t canview_stm_fdcan_capture_get_inventory(
    const canview_stm_fdcan_capture_t *capture, size_t index,
    canview_stm_fdcan_inventory_entry_t *entry);

/** @brief inventory entry 수와 고정 table 포화 drop을 읽는다.
 * @param capture initialized capture context.
 * @param count caller 소유 entry count 출력.
 * @param dropped caller 소유 table saturation drop 출력.
 * @return 성공 또는 입력 오류.
 */
canview_status_t canview_stm_fdcan_capture_get_inventory_state(
    const canview_stm_fdcan_capture_t *capture, size_t *count, uint32_t *dropped);

#endif
