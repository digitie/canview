/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_uart.h
 * @brief STM32 UART semantic runtime and bounded command boundary.
 *
 * The runtime is portable C99. It owns no USART/DMA registers and is called
 * by one cooperative worker. The platform adapter only supplies bytes, CTS
 * state and a DMA TX completion result. No API in this module creates a CAN
 * frame or grants vehicle-TX permission.
 */
#ifndef CANVIEW_STM_UART_H
#define CANVIEW_STM_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canview_status.h"
#include "canview_uart.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CANVIEW_STM_UART_RX_DMA_CAPACITY (2048U)
#define CANVIEW_STM_UART_TX_QUEUE_CAPACITY (8U)
#define CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY (8U)
#define CANVIEW_STM_UART_COMMAND_PAYLOAD_MAX (240U)
#define CANVIEW_STM_UART_TX_SERIAL_CAPACITY (CANVIEW_UART_MAX_SERIAL_SIZE)
#define CANVIEW_STM_UART_HEARTBEAT_PERIOD_MS (100U)
#define CANVIEW_STM_UART_TIME_SYNC_REPLY_UNCERTAINTY_US (0U)
#define CANVIEW_STM_UART_TIME_SYNC_MAX_DELAY_US (100000U)
#define CANVIEW_STM_UART_TIME_SYNC_PENDING_TIMEOUT_MS (1000U)
#define CANVIEW_STM_UART_ERROR_RATE_LIMIT (10U)
#define CANVIEW_STM_UART_ERROR_RATE_WINDOW_MS (1000U)
#define CANVIEW_STM_UART_CONTEXT_BUDGET_BYTES (88000U)
/* T-105 replaces this foundation revision with the generated safety profile. */
#define CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION (UINT32_C(1))

/** @brief Priority class. Lower numeric values are always selected first. */
typedef enum
{
    CANVIEW_STM_UART_TX_P0 = 0,
    CANVIEW_STM_UART_TX_P1,
    CANVIEW_STM_UART_TX_STATE,
    CANVIEW_STM_UART_TX_RAW,
    CANVIEW_STM_UART_TX_CLASS_COUNT
} canview_stm_uart_tx_class_t;

/** @brief Terminal/accepted stages used in COMMAND_RESULT. */
typedef enum
{
    CANVIEW_STM_UART_RESULT_STAGE_ACCEPTED = 1,
    CANVIEW_STM_UART_RESULT_STAGE_EXECUTING = 2,
    CANVIEW_STM_UART_RESULT_STAGE_COMPLETED = 3,
    CANVIEW_STM_UART_RESULT_STAGE_REJECTED = 4,
    CANVIEW_STM_UART_RESULT_STAGE_EXPIRED = 5,
    CANVIEW_STM_UART_RESULT_STAGE_BUSY = 6
} canview_stm_uart_result_stage_t;

/** @brief Stable semantic reason values; never cast from canview_status_t. */
typedef enum
{
    CANVIEW_STM_UART_REASON_NONE = 0,
    CANVIEW_STM_UART_REASON_INVALID = 1,
    CANVIEW_STM_UART_REASON_NOT_READY = 2,
    CANVIEW_STM_UART_REASON_AUTH_FAILED = 3,
    CANVIEW_STM_UART_REASON_CACHE_FULL = 4,
    CANVIEW_STM_UART_REASON_QUEUE_FULL = 5,
    CANVIEW_STM_UART_REASON_DUPLICATE = 6,
    CANVIEW_STM_UART_REASON_TOKEN_CONFLICT = 7,
    CANVIEW_STM_UART_REASON_TIME_SYNC = 8,
    CANVIEW_STM_UART_REASON_UNSUPPORTED = 9,
    CANVIEW_STM_UART_REASON_LINK_OFFLINE = 10,
    CANVIEW_STM_UART_REASON_TIMEOUT = 11
} canview_stm_uart_reason_t;

/** @brief Local lease mirror. It is not a vehicle-TX grant. */
typedef struct
{
    bool valid;
    uint64_t lease_id;
    uint16_t scope;
    uint32_t control_generation;
    uint64_t expires_at_ms;
    uint8_t control_tag[CANVIEW_UART_CONTROL_TAG_SIZE];
} canview_stm_uart_lease_t;

/** @brief Four-timestamp mapping owned by the STM32 worker. */
typedef struct
{
    bool valid;
    uint32_t generation;
    uint32_t uncertainty_us;
    int64_t offset_stm_minus_controller_us;
    uint64_t updated_ms;
    uint64_t controller_boot_id;
    uint64_t stm_boot_id;
} canview_stm_uart_time_mapping_t;

/** @brief Copy of an admitted command retained until an executor consumes it. */
typedef struct
{
    bool valid;
    uint16_t payload_size;
    uint32_t request_sequence;
    uint32_t correlation_id;
    canview_uart_command_handle_t cache_handle;
    canview_uart_command_key_t key;
    uint8_t payload[CANVIEW_STM_UART_COMMAND_PAYLOAD_MAX];
} canview_stm_uart_pending_command_t;

/** @brief Copy-owned application message held by one priority queue. */
typedef struct
{
    bool valid;
    uint8_t message_type;
    uint8_t flags;
    uint16_t payload_size;
    uint32_t sequence;
    uint32_t correlation_id;
    uint64_t sender_time_us;
    uint8_t payload[CANVIEW_STM_UART_COMMAND_PAYLOAD_MAX];
} canview_stm_uart_tx_item_t;

typedef struct
{
    canview_stm_uart_tx_item_t items[CANVIEW_STM_UART_TX_QUEUE_CAPACITY];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} canview_stm_uart_tx_queue_t;

/** @brief Saturating counters exposed to heartbeat/diagnostic code. */
typedef struct
{
    uint32_t rx_bytes;
    uint32_t rx_frames;
    uint32_t rx_incomplete;
    uint32_t rx_malformed;
    uint32_t rx_unsupported;
    uint32_t rx_crc_failures;
    uint32_t rx_oversize;
    uint32_t protocol_errors;
    uint32_t tx_enqueued;
    uint32_t tx_completed;
    uint32_t tx_dropped;
    uint32_t tx_raw_dropped;
    uint32_t tx_queue_full;
    uint32_t tx_safety_inhibited;
    uint32_t commands_admitted;
    uint32_t commands_completed;
    uint32_t commands_authorization_rejected;
    uint32_t commands_duplicate;
    uint32_t commands_conflict;
    uint32_t commands_cache_busy;
    uint32_t commands_expired;
    uint32_t pending_cancelled;
    uint32_t session_invalidations;
    uint32_t link_resets;
    uint32_t cts_stalls;
    uint32_t time_sync_requests;
    uint32_t time_sync_responses;
    uint32_t time_sync_commits;
    uint32_t time_sync_rejected;
    uint32_t error_responses_rate_limited;
    uint32_t callback_reentry;
    uint32_t execution_count;
} canview_stm_uart_stats_t;

/** @brief Local command authorizer. NULL means deny every control request. */
typedef bool canview_stm_uart_authorize_fn(const canview_uart_message_view_t *view,
                                           uint64_t now_ms, void *context);

/** @brief DMA owner가 runtime session buffer를 지우기 전에 전송을 quiesce한다. */
typedef canview_status_t canview_stm_uart_reset_hook_fn(void *context);

/** @brief Static configuration copied during init. All callback state is caller-owned. */
typedef struct
{
    uint64_t local_boot_id;
    uint64_t local_device_id;
    uint32_t local_safety_revision;
    canview_stm_uart_authorize_fn *authorize;
    void *authorize_context;
} canview_stm_uart_config_t;

/** @brief One caller-owned STM32 UART runtime context. */
typedef struct
{
    canview_uart_codec_t codec;
    canview_uart_link_t link;
    canview_uart_plan_context_t plan;
    canview_uart_command_cache_t command_cache;
    canview_uart_replay_context_t replay;
    canview_stm_uart_time_mapping_t time_mapping;
    canview_stm_uart_lease_t lease;
    canview_stm_uart_pending_command_t pending[CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY];
    uint8_t pending_head;
    uint8_t pending_count;
    canview_stm_uart_tx_queue_t tx_queues[CANVIEW_STM_UART_TX_CLASS_COUNT];
    canview_stm_uart_tx_item_t tx_current;
    canview_stm_uart_tx_class_t tx_current_class;
    uint8_t tx_serial[CANVIEW_STM_UART_TX_SERIAL_CAPACITY];
    uint8_t tx_scratch[CANVIEW_UART_MAX_FRAME_SIZE];
    uint16_t tx_serial_size;
    uint32_t next_tx_sequence;
    uint64_t last_heartbeat_tx_ms;
    uint64_t error_window_started_ms;
    uint8_t error_responses_in_window;
    uint64_t local_boot_id;
    uint64_t local_device_id;
    uint32_t local_safety_revision;
    canview_stm_uart_authorize_fn *authorize;
    void *authorize_context;
    canview_stm_uart_reset_hook_fn *reset_hook;
    void *reset_hook_context;
    canview_stm_uart_stats_t stats;
    uint64_t last_now_ms;
    uint64_t last_now_us;
    uint64_t pending_sync_token;
    uint64_t pending_sync_controller_boot_id;
    uint32_t pending_sync_generation;
    uint64_t pending_sync_t1_controller_us;
    uint64_t pending_sync_t2_stm_us;
    uint64_t pending_sync_t3_stm_us;
    uint64_t pending_sync_started_ms;
    bool pending_sync_valid;
    bool cts_blocked;
    bool cts_known;
    bool safety_inhibited;
    bool initialized;
    bool servicing;
} canview_stm_uart_context_t;

/** @cond INTERNAL */
typedef char canview_stm_uart_context_fits_budget[
    (sizeof(canview_stm_uart_context_t) <= CANVIEW_STM_UART_CONTEXT_BUDGET_BYTES) ? 1 : -1];
/** @endcond */

/** @brief Initialize all state and enqueue the local HELLO.
 * @param context caller-owned zero-init runtime context.
 * @param config local identity, safety revision and optional authorizer.
 * @param now_ms current monotonic millisecond timestamp.
 * @param now_us current monotonic microsecond timestamp.
 * @return `CANVIEW_OK` or invalid configuration/reset failure.
 */
canview_status_t canview_stm_uart_init(canview_stm_uart_context_t *context,
                                       const canview_stm_uart_config_t *config,
                                       uint64_t now_ms, uint64_t now_us);

/** @brief Reset link, cache, mapping, lease, pending commands and TX queues.
 * @param context initialized runtime context.
 * @param now_ms current monotonic millisecond timestamp.
 * @param now_us current monotonic microsecond timestamp.
 * @return `CANVIEW_OK`, `CANVIEW_RESOURCE_BUSY` during callback reentry, or reset failure.
 */
canview_status_t canview_stm_uart_reset(canview_stm_uart_context_t *context,
                                        uint64_t now_ms, uint64_t now_us);

/** @brief Register the platform hook used before runtime buffers are reset.
 * @param context initialized runtime context.
 * @param hook platform-owned DMA quiesce callback; NULL removes it.
 * @param hook_context opaque platform context passed to hook.
 * @return `CANVIEW_OK`, or busy while the worker is servicing a callback.
 */
canview_status_t canview_stm_uart_set_reset_hook(canview_stm_uart_context_t *context,
                                                 canview_stm_uart_reset_hook_fn *hook,
                                                 void *hook_context);

/** @brief Feed one DMA-owned byte in the single worker context.
 * @param context initialized runtime context owned by the UART worker.
 * @param byte one byte copied from the DMA ring.
 * @param now_ms current monotonic millisecond timestamp.
 * @param now_us current monotonic microsecond timestamp.
 * @return codec/dispatch status; no CAN frame is created by this API.
 */
canview_status_t canview_stm_uart_ingest_byte(canview_stm_uart_context_t *context, uint8_t byte,
                                              uint64_t now_ms, uint64_t now_us);

/** @brief Run heartbeat, timeout, cache expiry and mapping expiry maintenance.
 * @param context initialized runtime context.
 * @param now_ms current monotonic millisecond timestamp.
 * @param now_us current monotonic microsecond timestamp.
 * @return `CANVIEW_OK`, maintenance failure, or callback reentry rejection.
 */
canview_status_t canview_stm_uart_tick(canview_stm_uart_context_t *context, uint64_t now_ms,
                                       uint64_t now_us);

/** @brief Apply a sampled hardware CTS state; ISR must post it to the worker first.
 * @param context initialized runtime context.
 * @param blocked true when the peer's RTS/CTS input blocks transmission.
 * @param now_ms current monotonic millisecond timestamp.
 * @return `CANVIEW_OK`, timeout after offline transition, or reentry rejection.
 */
canview_status_t canview_stm_uart_set_cts_blocked(canview_stm_uart_context_t *context,
                                                  bool blocked, uint64_t now_ms);

/** @brief Copy-validate and enqueue a semantic message; payload is never retained.
 * @param context initialized runtime context.
 * @param queue_class bounded priority queue selected by the caller.
 * @param message_type generated UART message type.
 * @param flags generated UART message flags.
 * @param correlation_id opaque request correlation value.
 * @param sender_time_us sender timestamp for the encoded envelope.
 * @param payload caller-owned payload bytes.
 * @param payload_size payload length in bytes.
 * @return `CANVIEW_OK`, validation failure, queue full, or reentry rejection.
 */
canview_status_t canview_stm_uart_enqueue(canview_stm_uart_context_t *context,
                                          canview_stm_uart_tx_class_t queue_class,
                                          uint8_t message_type, uint8_t flags,
                                          uint32_t correlation_id, uint64_t sender_time_us,
                                          const uint8_t *payload, size_t payload_size);

/** @brief Select the next encoded message. Returned bytes borrow context until finish/reset.
 * @param context initialized runtime context.
 * @param data receives a borrowed encoded serial buffer, or NULL on no item.
 * @param size receives the encoded buffer length.
 * @param sequence receives the selected message sequence.
 * @return `CANVIEW_OK`, `CANVIEW_INCOMPLETE`, encoding failure, or reentry rejection.
 */
canview_status_t canview_stm_uart_tx_begin(canview_stm_uart_context_t *context,
                                           const uint8_t **data, size_t *size,
                                           uint32_t *sequence);

/** @brief Finish one TX. BUSY keeps the item; other failure drops it fail-closed.
 * @param context initialized runtime context.
 * @param transfer_status DMA transfer result.
 * @param now_ms current monotonic millisecond timestamp.
 * @return the supplied transfer status, or invalid/reentry status.
 */
canview_status_t canview_stm_uart_tx_finish(canview_stm_uart_context_t *context,
                                            canview_status_t transfer_status,
                                            uint64_t now_ms);

/** @brief Copy the oldest pending command to an executor-owned buffer.
 * @param context initialized runtime context.
 * @param command receives a copy-owned pending command.
 * @return `CANVIEW_OK`, `CANVIEW_INCOMPLETE`, or invalid argument.
 */
canview_status_t canview_stm_uart_pending_peek(const canview_stm_uart_context_t *context,
                                               canview_stm_uart_pending_command_t *command);

/** @brief Record a terminal result, cache it and enqueue COMMAND_RESULT.
 * @param context initialized runtime context.
 * @param handle cache handle copied from `pending_peek`.
 * @param stage terminal result stage.
 * @param reason stable semantic result reason.
 * @param state_revision local state revision.
 * @param feedback_revision optional feedback revision.
 * @param feedback_time_ms optional feedback timestamp.
 * @param now_ms current monotonic millisecond timestamp.
 * @param now_us current monotonic microsecond timestamp.
 * @return `CANVIEW_OK`, stale handle, queue full, or invalid argument.
 */
canview_status_t canview_stm_uart_pending_complete(canview_stm_uart_context_t *context,
                                                   const canview_uart_command_handle_t *handle,
                                                   uint8_t stage, uint16_t reason,
                                                   uint32_t state_revision, uint32_t feedback_revision,
                                                   uint32_t feedback_time_ms, uint64_t now_ms,
                                                   uint64_t now_us);

/** @brief Snapshot counters.
 * @param context initialized runtime context.
 * @param stats receives a copy of saturating runtime counters.
 * @return `CANVIEW_OK` or invalid argument.
 */
canview_status_t canview_stm_uart_get_stats(const canview_stm_uart_context_t *context,
                                            canview_stm_uart_stats_t *stats);

/** @brief Snapshot link state.
 * @param context initialized runtime context.
 * @param link receives a copy of the generated link state.
 * @return `CANVIEW_OK` or invalid argument.
 */
canview_status_t canview_stm_uart_get_link(const canview_stm_uart_context_t *context,
                                           canview_uart_link_t *link);

/** @brief Snapshot time mapping; invalid mapping is returned with valid=false.
 * @param context initialized runtime context.
 * @param mapping receives a copy of the time mapping.
 * @return `CANVIEW_OK` or invalid argument.
 */
canview_status_t canview_stm_uart_get_time_mapping(
    const canview_stm_uart_context_t *context, canview_stm_uart_time_mapping_t *mapping);

/** @brief Snapshot lease mirror; valid never means vehicle TX is allowed.
 * @param context initialized runtime context.
 * @param lease receives a copy of the local lease mirror.
 * @return `CANVIEW_OK` or invalid argument.
 */
canview_status_t canview_stm_uart_get_lease(const canview_stm_uart_context_t *context,
                                            canview_stm_uart_lease_t *lease);

#ifdef __cplusplus
}
#endif
#endif
