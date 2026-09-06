/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_uart.h
 * @brief Communicator ESP32↔STM32 UART semantic validation and bounded state.
 *
 * The generated header describes the wire ABI. This API validates a decoded
 * envelope before dispatch and owns no UART peripheral, DMA, queue or CAN TX
 * operation. All state is caller-owned and fixed-size.
 */
#ifndef CANVIEW_UART_H
#define CANVIEW_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canview_status.h"
#include "canview_uart_protocol.h"
#include "canview_wire.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CANVIEW_UART_COMMAND_DIGEST_SIZE (32U)
#define CANVIEW_UART_CONTROL_TAG_SIZE (16U)
#define CANVIEW_UART_PLAN_DIGEST_SIZE (32U)
#define CANVIEW_UART_COMMAND_TTL_MS (60000U)
#define CANVIEW_UART_HEARTBEAT_ONLINE_MS (300U)
#define CANVIEW_UART_HEARTBEAT_OFFLINE_MS (1000U)
#define CANVIEW_UART_CTS_COMMAND_STOP_MS (100U)
#define CANVIEW_UART_CTS_OFFLINE_MS (1000U)

typedef struct
{
    canview_wire_view_t wire;
    const canview_uart_message_policy_t *policy;
} canview_uart_message_view_t;

typedef struct
{
    canview_uart_stream_t stream;
    uint32_t packets_ok;
    uint32_t malformed_packets;
    uint32_t crc_failures;
    uint32_t oversize_packets;
} canview_uart_codec_t;

/** @brief Find the generated policy for a message ID. */
canview_status_t canview_uart_message_policy(uint8_t message_type,
                                             const canview_uart_message_policy_t **policy);

/** @brief Validate UART envelope semantics after structural framing/CRC. */
canview_status_t canview_uart_message_validate(const canview_wire_view_t *wire,
                                               canview_uart_message_view_t *view);

/** @brief Encode one validated semantic UART message into COBS + delimiter. */
canview_status_t canview_uart_message_encode(uint8_t message_type, uint8_t flags,
                                             uint32_t sequence, uint32_t correlation_id,
                                             uint64_t sender_time_us, const uint8_t *payload,
                                             size_t payload_size, uint8_t *scratch,
                                             size_t scratch_size, uint8_t *out, size_t capacity,
                                             size_t *written);

/** @brief Reset a caller-owned worker-context decoder and its counters. */
canview_status_t canview_uart_codec_reset(canview_uart_codec_t *codec);

/** @brief Feed one byte; successful views borrow codec storage until next feed. */
canview_status_t canview_uart_codec_feed(canview_uart_codec_t *codec, uint8_t byte,
                                          canview_uart_message_view_t *view);

typedef struct
{
    uint8_t bus_id;
    uint8_t flags;
    uint32_t can_id;
    uint32_t can_mask;
} canview_uart_observer_filter_t;

typedef struct
{
    uint32_t revision;
    uint16_t filter_count;
    uint16_t max_records_per_second;
    uint32_t max_bytes_per_second;
    uint8_t bus_mask;
    uint8_t reserved[3];
    uint8_t plan_digest[CANVIEW_UART_PLAN_DIGEST_SIZE];
    canview_uart_observer_filter_t filters[CANVIEW_UART_PLAN_MAX_FILTERS];
} canview_uart_observer_plan_t;

typedef struct
{
    bool pending;
    uint64_t request_token;
    uint32_t revision;
    uint32_t expected_active_revision;
    uint16_t total_chunks;
    uint16_t next_chunk;
    uint16_t expected_filter_count;
    uint16_t received_filter_count;
    canview_uart_observer_plan_t active;
    canview_uart_observer_plan_t candidate;
} canview_uart_plan_context_t;

/** @brief Reset active plan and discard any uncommitted candidate. */
canview_status_t canview_uart_plan_reset(canview_uart_plan_context_t *context);

/** @brief Apply BEGIN/CHUNK/COMMIT/ABORT atomically to the active plan. */
canview_status_t canview_uart_plan_apply(canview_uart_plan_context_t *context,
                                         const uint8_t *payload, size_t payload_size);

/** @brief Copy the last committed observer plan. */
canview_status_t canview_uart_plan_current(const canview_uart_plan_context_t *context,
                                           canview_uart_observer_plan_t *plan);

typedef struct
{
    bool valid;
    bool acknowledged;
    bool terminal;
    uint64_t request_token;
    uint64_t expires_at_ms;
    uint16_t command_id;
    uint16_t result_size;
    uint8_t canonical_argument_digest[CANVIEW_UART_COMMAND_DIGEST_SIZE];
    uint8_t result[CANVIEW_UART_COMMAND_RESULT_MAX];
} canview_uart_command_entry_t;

typedef struct
{
    canview_uart_command_entry_t entries[CANVIEW_UART_COMMAND_CACHE_CAPACITY];
} canview_uart_command_cache_t;

/** @brief Reset the bounded cache. No live entry is evicted by admission. */
canview_status_t canview_uart_command_cache_reset(canview_uart_command_cache_t *cache);

/** @brief Admit a command or identify a duplicate/conflicting token. */
canview_status_t canview_uart_command_cache_admit(
    canview_uart_command_cache_t *cache, uint64_t request_token, uint16_t command_id,
    const uint8_t digest[CANVIEW_UART_COMMAND_DIGEST_SIZE], uint64_t now_ms, size_t *entry_index);

/** @brief Mark the queue ACK without changing a terminal result. */
canview_status_t canview_uart_command_cache_mark_ack(canview_uart_command_cache_t *cache,
                                                     size_t entry_index);

/** @brief Store a terminal result, including when it arrives before ACK. */
canview_status_t canview_uart_command_cache_record_result(
    canview_uart_command_cache_t *cache, size_t entry_index, const uint8_t *result,
    size_t result_size, uint64_t now_ms);

/** @brief Look up a duplicate and expose its retained terminal result if present. */
canview_status_t canview_uart_command_cache_lookup(
    canview_uart_command_cache_t *cache, uint64_t request_token, uint16_t command_id,
    const uint8_t digest[CANVIEW_UART_COMMAND_DIGEST_SIZE], uint64_t now_ms, size_t *entry_index,
    const uint8_t **result, size_t *result_size);

/** @brief Expire only entries whose bounded retention window has elapsed. */
canview_status_t canview_uart_command_cache_expire(canview_uart_command_cache_t *cache,
                                                   uint64_t now_ms);

typedef enum
{
    CANVIEW_UART_LINK_OFFLINE = 0,
    CANVIEW_UART_LINK_SUSPECT = 1,
    CANVIEW_UART_LINK_ONLINE = 2
} canview_uart_link_state_t;

typedef struct
{
    bool hello_complete;
    bool heartbeat_seen;
    bool cts_blocked;
    uint64_t peer_boot_id;
    uint64_t last_heartbeat_ms;
    uint64_t cts_blocked_since_ms;
    canview_uart_link_state_t state;
} canview_uart_link_t;

/** @brief Reset link state to fail-closed OFFLINE. */
canview_status_t canview_uart_link_reset(canview_uart_link_t *link);

/** @brief Complete the HELLO side of link admission for a peer boot epoch. */
canview_status_t canview_uart_link_note_hello(canview_uart_link_t *link, uint64_t peer_boot_id,
                                              uint64_t now_ms, bool *boot_changed);

/** @brief Note heartbeat and report a boot epoch replacement to the caller. */
canview_status_t canview_uart_link_note_heartbeat(canview_uart_link_t *link,
                                                  uint64_t peer_boot_id, uint64_t now_ms,
                                                  bool *boot_changed);

/** @brief Update CTS state; blocked flow is not an online command permit. */
canview_status_t canview_uart_link_set_cts_blocked(canview_uart_link_t *link, bool blocked,
                                                   uint64_t now_ms);

/** @brief Recompute ONLINE/SUSPECT/OFFLINE using heartbeat and CTS deadlines. */
canview_status_t canview_uart_link_tick(canview_uart_link_t *link, uint64_t now_ms);

/** @brief Return true only while heartbeat, HELLO and CTS admission are healthy. */
bool canview_uart_link_command_admission_allowed(const canview_uart_link_t *link,
                                                 uint64_t now_ms);

#ifdef __cplusplus
}
#endif
#endif
