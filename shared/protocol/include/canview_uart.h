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
    uint32_t unsupported_messages;
    uint32_t crc_failures;
    uint32_t oversize_packets;
} canview_uart_codec_t;

/** @brief Find the generated policy for a message ID. */
canview_status_t canview_uart_message_policy(uint8_t message_type,
                                             const canview_uart_message_policy_t **policy);

/** @brief Validate UART envelope semantics after structural framing/CRC. */
canview_status_t canview_uart_message_validate(const canview_wire_view_t *wire,
                                               canview_uart_message_view_t *view);

/** @brief Return whether a decoded message is legal for the local endpoint role.
 *
 * Structural validation is deliberately separate from this direction check. This
 * function is still not an authentication, lease, safety or CAN-TX permission.
 */
typedef enum
{
    CANVIEW_UART_ENDPOINT_ESP32 = 0,
    CANVIEW_UART_ENDPOINT_STM32 = 1
} canview_uart_endpoint_t;

typedef enum
{
    CANVIEW_UART_FLOW_INBOUND = 0,
    CANVIEW_UART_FLOW_OUTBOUND = 1
} canview_uart_flow_t;

bool canview_uart_message_direction_allowed(const canview_uart_message_view_t *view,
                                            canview_uart_endpoint_t endpoint,
                                            canview_uart_flow_t flow);

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
    uint64_t pending_since_ms;
    canview_uart_observer_plan_t active;
    canview_uart_observer_plan_t candidate;
} canview_uart_plan_context_t;

/** @brief Reset active plan and discard any uncommitted candidate. */
canview_status_t canview_uart_plan_reset(canview_uart_plan_context_t *context);

/** @brief Discard only a pending candidate and retain the active plan. */
canview_status_t canview_uart_plan_discard_pending(canview_uart_plan_context_t *context);

/** @brief Calculate the SHA-256 digest of the canonical observer plan. */
canview_status_t canview_uart_plan_digest(const canview_uart_observer_plan_t *plan,
                                          uint8_t digest[CANVIEW_UART_PLAN_DIGEST_SIZE]);

/** @brief Apply BEGIN/CHUNK/COMMIT/ABORT atomically to the active plan. */
canview_status_t canview_uart_plan_apply(canview_uart_plan_context_t *context,
                                         const uint8_t *payload, size_t payload_size,
                                         uint64_t now_ms);

/** @brief Copy the last committed observer plan. */
canview_status_t canview_uart_plan_current(const canview_uart_plan_context_t *context,
                                           canview_uart_observer_plan_t *plan);

typedef struct
{
    uint64_t origin_device_id;
    uint64_t origin_boot_id;
    uint32_t wireless_session_id;
    uint32_t control_generation;
    uint64_t request_token;
    uint16_t command_id;
    uint8_t canonical_argument_digest[CANVIEW_UART_COMMAND_DIGEST_SIZE];
} canview_uart_command_key_t;

typedef struct
{
    uint16_t slot;
    uint16_t reserved;
    uint64_t generation;
} canview_uart_command_handle_t;

typedef struct
{
    bool valid;
    bool acknowledged;
    bool terminal;
    uint64_t generation;
    uint64_t expires_at_ms;
    uint16_t result_size;
    canview_uart_command_key_t key;
    uint8_t result[CANVIEW_UART_COMMAND_RESULT_MAX];
} canview_uart_command_entry_t;

typedef struct
{
    uint32_t initialized;
    uint64_t next_generation;
    canview_uart_command_entry_t entries[CANVIEW_UART_COMMAND_CACHE_CAPACITY];
} canview_uart_command_cache_t;

/** @brief Reset the bounded cache. No live entry is evicted by admission. */
canview_status_t canview_uart_command_cache_reset(canview_uart_command_cache_t *cache);

/** @brief Admit a command or identify a duplicate/conflicting token. */
canview_status_t canview_uart_command_cache_admit(
    canview_uart_command_cache_t *cache, const canview_uart_command_key_t *key, uint64_t now_ms,
    canview_uart_command_handle_t *handle);

/** @brief Mark the queue ACK without changing a terminal result. */
canview_status_t canview_uart_command_cache_mark_ack(canview_uart_command_cache_t *cache,
                                                     const canview_uart_command_handle_t *handle);

/** @brief Store a terminal result, including when it arrives before ACK. */
canview_status_t canview_uart_command_cache_record_result(
    canview_uart_command_cache_t *cache, const canview_uart_command_handle_t *handle,
    const uint8_t *result, size_t result_size, uint64_t now_ms);

/** @brief Look up a duplicate and expose its retained terminal result if present. */
canview_status_t canview_uart_command_cache_lookup(
    canview_uart_command_cache_t *cache, const canview_uart_command_key_t *key, uint64_t now_ms,
    canview_uart_command_handle_t *handle, const uint8_t **result, size_t *result_size);

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
    bool hello_ack_complete;
    bool safety_snapshot_valid;
    bool heartbeat_seen;
    bool cts_blocked;
    bool cts_known;
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

/** @brief Complete version negotiation for the current peer boot epoch. */
canview_status_t canview_uart_link_note_hello_ack(canview_uart_link_t *link,
                                                  uint64_t peer_boot_id, uint8_t selected_major,
                                                  uint8_t selected_minor, uint8_t result,
                                                  uint64_t now_ms);

/** @brief Mark the current local/peer safety snapshot as fresh for admission. */
canview_status_t canview_uart_link_note_safety_snapshot(canview_uart_link_t *link,
                                                        uint64_t peer_boot_id,
                                                        uint64_t now_ms);

/** @brief Note heartbeat and report a boot epoch replacement to the caller. */
canview_status_t canview_uart_link_note_heartbeat(canview_uart_link_t *link,
                                                  uint64_t peer_boot_id, uint64_t now_ms,
                                                  bool *boot_changed);

/** @brief Update CTS state; a block lasting at least the stop threshold halts commands. */
canview_status_t canview_uart_link_set_cts_blocked(canview_uart_link_t *link, bool blocked,
                                                   uint64_t now_ms);

/** @brief Recompute ONLINE/SUSPECT/OFFLINE using heartbeat and CTS deadlines. */
canview_status_t canview_uart_link_tick(canview_uart_link_t *link, uint64_t now_ms);

/** @brief Return true only while heartbeat, HELLO and CTS admission are healthy. */
bool canview_uart_link_command_admission_allowed(const canview_uart_link_t *link,
                                                 uint64_t now_ms);

typedef struct
{
    bool authenticated;
    bool control_lease_valid;
    bool safety_snapshot_current;
    bool build_mode_allows_tx;
    bool hardware_tx_gate_ready;
} canview_uart_command_admission_context_t;

/** @brief Combine transport readiness with STM32-owned command safety results.
 *
 * The booleans must come from the local authenticated/safety owner. UART CRC or
 * this helper never authenticates a peer and never creates a vehicle CAN frame.
 */
bool canview_uart_command_admission_allowed(
    const canview_uart_link_t *link,
    const canview_uart_command_admission_context_t *context, uint64_t now_ms);

/** @brief Atomically clear link, pending observer state and command cache. */
canview_status_t canview_uart_session_reset(canview_uart_link_t *link,
                                             canview_uart_plan_context_t *plan,
                                             canview_uart_command_cache_t *cache);

/** @brief Note HELLO and invalidate state when a new UART session is observed. */
canview_status_t canview_uart_session_note_hello(
    canview_uart_link_t *link, canview_uart_plan_context_t *plan,
    canview_uart_command_cache_t *cache, uint64_t peer_boot_id, uint64_t now_ms,
    bool *boot_changed);

/** @brief Note heartbeat and invalidate state when its boot epoch changes. */
canview_status_t canview_uart_session_note_heartbeat(
    canview_uart_link_t *link, canview_uart_plan_context_t *plan,
    canview_uart_command_cache_t *cache, uint64_t peer_boot_id, uint64_t now_ms,
    bool *boot_changed);

#ifdef __cplusplus
}
#endif
#endif
