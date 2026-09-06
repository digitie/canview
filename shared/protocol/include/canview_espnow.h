/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_espnow.h
 * @brief ESP-NOW v1.3 byte-safe codec, peer session and bounded QoS API.
 *
 * This API is transport-neutral. It does not call ESP-IDF, allocate heap
 * memory, create queues, or authorize a vehicle CAN transmission.
 */
#ifndef CANVIEW_ESPNOW_H
#define CANVIEW_ESPNOW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canview_espnow_contract.h"
#include "canview_protocol.h"
#include "canview_status.h"
#include "canview_wire.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CANVIEW_ESPNOW_HASH_BYTES (32U)
#define CANVIEW_ESPNOW_LINK_ROOT_BYTES (32U)
#define CANVIEW_ESPNOW_CONTROL_TAG_BYTES (16U)
#define CANVIEW_ESPNOW_PAIRING_NONCE_BYTES (16U)
#define CANVIEW_ESPNOW_MAX_CANONICAL_BYTES (240U)
#define CANVIEW_ESPNOW_RX_RESOURCE_LIMIT (8U)
#define CANVIEW_ESPNOW_MAX_PENDING (8U)
#define CANVIEW_ESPNOW_MAX_ATTEMPTS (3U)
#define CANVIEW_ESPNOW_INITIAL_ACK_TIMEOUT_MS (80U)
#define CANVIEW_ESPNOW_MIN_ACK_TIMEOUT_MS (40U)
#define CANVIEW_ESPNOW_MAX_ACK_TIMEOUT_MS (250U)
#define CANVIEW_ESPNOW_CONTROL_TIME_MAX_AGE_MS (30000U)
#define CANVIEW_ESPNOW_CONTROL_TIME_MAX_UNCERTAINTY_US (50000U)

typedef enum
{
    CANVIEW_DECODE_DROP_SILENT = 0,
    CANVIEW_DECODE_ERROR_RATE_LIMITED,
    CANVIEW_DECODE_ACK_REQUIRED,
    CANVIEW_DECODE_DELIVER
} canview_decode_result_t;

typedef enum
{
    CANVIEW_DECODE_REASON_NONE = 0,
    CANVIEW_DECODE_REASON_INVALID_ARGUMENT,
    CANVIEW_DECODE_REASON_BAD_LENGTH,
    CANVIEW_DECODE_REASON_BAD_MAGIC,
    CANVIEW_DECODE_REASON_BAD_FLAGS,
    CANVIEW_DECODE_REASON_BAD_RESERVED,
    CANVIEW_DECODE_REASON_BAD_CRC,
    CANVIEW_DECODE_REASON_INCOMPATIBLE_MAJOR,
    CANVIEW_DECODE_REASON_UNSUPPORTED_MESSAGE,
    CANVIEW_DECODE_REASON_UNSUPPORTED_TLV,
    CANVIEW_DECODE_REASON_ROLE_MISMATCH,
    CANVIEW_DECODE_REASON_STATE_MISMATCH,
    CANVIEW_DECODE_REASON_ENCRYPTION_REQUIRED,
    CANVIEW_DECODE_REASON_SESSION_MISMATCH,
    CANVIEW_DECODE_REASON_REPLAY,
    CANVIEW_DECODE_REASON_STALE,
    CANVIEW_DECODE_REASON_SEMANTIC,
    CANVIEW_DECODE_REASON_RESOURCE_BUSY,
    CANVIEW_DECODE_REASON_RATE_LIMITED,
    CANVIEW_DECODE_REASON_TIME_SYNC_STALE,
    CANVIEW_DECODE_REASON_CONTROL_TAG,
    CANVIEW_DECODE_REASON_PAIRING_REPLAY,
    CANVIEW_DECODE_REASON_AUTH_BACKOFF
} canview_decode_reason_t;

typedef enum
{
    CANVIEW_ENCODE_OK = 0,
    CANVIEW_ENCODE_INVALID_ARGUMENT,
    CANVIEW_ENCODE_BUFFER_TOO_SMALL,
    CANVIEW_ENCODE_MALFORMED,
    CANVIEW_ENCODE_UNSUPPORTED_MESSAGE,
    CANVIEW_ENCODE_UNSUPPORTED_VERSION,
    CANVIEW_ENCODE_UNAUTHENTICATED,
    CANVIEW_ENCODE_ROLE_MISMATCH,
    CANVIEW_ENCODE_STATE_MISMATCH,
    CANVIEW_ENCODE_SESSION_MISMATCH
} canview_encode_result_t;

/** Metadata supplied by the transport owner; none of it is read from wire bytes. */
typedef struct
{
    canview_role_t sender_role;
    canview_role_t receiver_role;
    canview_link_state_t link_state;
    uint32_t expected_session_id;
    uint32_t now_ms;
    bool encrypted;
    bool peer_authenticated;
    bool peer_known;
    bool source_is_broadcast;
} canview_transport_meta_t;

/** Borrowed decoded view. payload is valid only while the caller-owned input is unchanged. */
typedef struct
{
    canview_wire_header_t header;
    const uint8_t *payload;
    size_t payload_size;
    const canview_espnow_message_contract_t *contract;
    uint8_t variant_index;
    bool duplicate;
    canview_decode_reason_t reason;
} canview_decoded_frame_t;

typedef struct
{
    canview_transport_meta_t meta;
    canview_wire_header_t header;
    uint8_t variant_index;
    const uint8_t *payload;
    size_t payload_size;
} canview_encode_request_t;

typedef struct
{
    bool valid;
    uint32_t generation;
    uint32_t uncertainty_us;
    uint32_t updated_ms;
    uint64_t controller_boot_id;
    uint64_t stm_boot_id;
} canview_control_time_sync_t;

typedef struct
{
    uint32_t window_start_ms;
    uint16_t emitted;
    uint16_t suppressed;
    uint16_t per_second_limit;
} canview_error_rate_limiter_t;

/**
 * Caller-owned session state. Reset/start, decode, and all session mutations
 * must be serialized by the owning task; this portable core is not an ISR or
 * multi-task synchronization primitive.
 */
typedef struct
{
    bool initialized;
    uint32_t lifecycle_cookie;
    uint32_t session_id;
    uint64_t peer_device_id;
    uint64_t peer_boot_id;
    canview_role_t local_role;
    canview_role_t peer_role;
    canview_link_state_t state;
    bool encrypted;
    bool authenticated;
    bool local_authorization_valid;
    canview_role_t authorized_role;
    uint16_t authorized_scope;
    uint32_t authorized_message_classes;
    uint32_t link_key_generation;
    canview_sequence_window_t rx_window;
    uint32_t next_sequence;
    uint32_t last_valid_ms;
    uint8_t heartbeat_misses;
    uint8_t time_sync_samples;
    uint8_t rx_resource_limit;
    uint8_t rx_reserved;
    uint8_t auth_failures;
    uint32_t auth_backoff_until_ms;
    bool pairing_nonce_seen;
    uint8_t pairing_nonce[CANVIEW_ESPNOW_PAIRING_NONCE_BYTES];
    bool transcript_seen;
    uint8_t transcript_hash[CANVIEW_ESPNOW_HASH_BYTES];
    canview_control_time_sync_t control_time_sync;
    canview_error_rate_limiter_t error_limiter;
} canview_peer_session_t;

typedef int (*canview_hmac_sha256_fn)(
    void *context, const uint8_t *key, size_t key_length,
    const uint8_t *data, size_t data_length, uint8_t output[CANVIEW_ESPNOW_HASH_BYTES]);

typedef int (*canview_hkdf_sha256_fn)(
    void *context, const uint8_t *salt, size_t salt_length,
    const uint8_t *input_key_material, size_t input_key_material_length,
    const uint8_t *info, size_t info_length, uint8_t *output, size_t output_length);

/** Crypto primitive adapter. Implementations are supplied by mbedTLS/host tests. */
typedef struct
{
    void *context;
    canview_hmac_sha256_fn hmac_sha256;
    canview_hkdf_sha256_fn hkdf_sha256;
} canview_crypto_adapter_t;

typedef struct
{
    bool valid;
    uint32_t generation;
    uint8_t link_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES];
} canview_link_key_record_t;

/**
 * Caller-owned two-slot model for an atomic per-pair key rotation. The
 * production NVS/flash adapter owns persistence and must serialize access.
 */
typedef struct
{
    canview_link_key_record_t active;
    canview_link_key_record_t staging;
    bool staging_ready;
} canview_link_key_store_t;

/** Caller-owned, single-owner scheduler; transport tasks must serialize calls. */
typedef struct
{
    bool in_use;
    uint8_t bytes[CANVIEW_WIRE_ESPNOW_MAX_FRAME];
} canview_frame_pool_slot_t;

typedef struct
{
    canview_frame_pool_slot_t slots[CANVIEW_ESPNOW_RX_RESOURCE_LIMIT];
} canview_frame_pool_t;

typedef struct
{
    bool in_use;
    uint8_t frame[CANVIEW_WIRE_ESPNOW_MAX_FRAME];
    size_t frame_size;
    uint64_t request_token;
    uint32_t first_sequence;
    uint32_t current_sequence;
    uint32_t issued_at_ms;
    uint32_t expires_at_ms;
    uint32_t next_deadline_ms;
    uint32_t last_sent_ms;
    uint8_t attempts;
} canview_qos_pending_t;

typedef struct
{
    canview_qos_pending_t pending[CANVIEW_ESPNOW_MAX_PENDING];
    uint32_t next_sequence;
    uint32_t srtt_ms;
    bool has_srtt;
} canview_qos_scheduler_t;

typedef enum
{
    CANVIEW_QOS_NO_EVENT = 0,
    CANVIEW_QOS_RETRY_READY,
    CANVIEW_QOS_COMPLETED,
    CANVIEW_QOS_EXPIRED
} canview_qos_event_t;

canview_decode_result_t canview_frame_decode(
    const canview_transport_meta_t *meta, const uint8_t *bytes, size_t length,
    canview_peer_session_t *session, canview_decoded_frame_t *out);

canview_encode_result_t canview_frame_encode(
    const canview_encode_request_t *request, uint8_t *out, size_t capacity, size_t *written);

canview_status_t canview_frame_recalculate_crc(uint8_t *bytes, size_t length);

canview_status_t canview_peer_session_reset(canview_peer_session_t *session);
canview_status_t canview_peer_session_start(
    canview_peer_session_t *session, canview_role_t local_role, uint32_t session_id,
    bool encrypted, bool authenticated, uint32_t now_ms);
canview_status_t canview_peer_session_authorize(
    canview_peer_session_t *session, canview_role_t peer_role, uint16_t scope,
    uint32_t message_classes, uint32_t link_key_generation);
canview_status_t canview_peer_session_on_message(
    canview_peer_session_t *session, uint8_t message_type, uint32_t now_ms);
canview_status_t canview_peer_session_reserve_rx(canview_peer_session_t *session);
canview_status_t canview_peer_session_release_rx(canview_peer_session_t *session);
bool canview_peer_session_auth_allowed(const canview_peer_session_t *session, uint32_t now_ms);
void canview_peer_session_auth_failure(canview_peer_session_t *session, uint32_t now_ms);
void canview_peer_session_auth_success(canview_peer_session_t *session);

canview_status_t canview_control_time_sync_update(
    canview_control_time_sync_t *sync, uint32_t generation, uint32_t uncertainty_us,
    uint32_t now_ms, uint64_t controller_boot_id, uint64_t stm_boot_id);
void canview_control_time_sync_invalidate(canview_control_time_sync_t *sync);
bool canview_control_time_sync_is_valid(
    const canview_control_time_sync_t *sync, uint32_t now_ms);

canview_status_t canview_pairing_transcript_encode(
    uint8_t message_type, const uint8_t *payload, size_t payload_size,
    uint8_t *output, size_t capacity, size_t *written);
canview_status_t canview_pairing_tag_compute(
    const canview_crypto_adapter_t *crypto, const uint8_t link_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES],
    uint8_t message_type, const uint8_t *payload, size_t payload_size,
    uint8_t tag[CANVIEW_ESPNOW_CONTROL_TAG_BYTES]);
canview_status_t canview_pairing_tag_verify(
    const canview_crypto_adapter_t *crypto, const uint8_t link_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES],
    uint8_t message_type, const uint8_t *payload, size_t payload_size,
    const uint8_t tag[CANVIEW_ESPNOW_CONTROL_TAG_BYTES]);
canview_status_t canview_pairing_replay_accept(
    canview_peer_session_t *session,
    const uint8_t nonce[CANVIEW_ESPNOW_PAIRING_NONCE_BYTES],
    const uint8_t transcript_hash[CANVIEW_ESPNOW_HASH_BYTES]);

canview_status_t canview_control_tag_compute(
    const canview_crypto_adapter_t *crypto,
    const uint8_t control_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES],
    uint32_t session_id, uint8_t message_type, const uint8_t *payload, size_t payload_size,
    uint8_t tag[CANVIEW_ESPNOW_CONTROL_TAG_BYTES]);
canview_status_t canview_control_tag_verify(
    const canview_crypto_adapter_t *crypto,
    const uint8_t control_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES],
    uint32_t session_id, uint8_t message_type, const uint8_t *payload, size_t payload_size,
    const uint8_t tag[CANVIEW_ESPNOW_CONTROL_TAG_BYTES]);

canview_status_t canview_link_key_store_reset(canview_link_key_store_t *store);
canview_status_t canview_link_key_store_stage(
    canview_link_key_store_t *store, uint32_t generation,
    const uint8_t link_root[CANVIEW_ESPNOW_LINK_ROOT_BYTES]);
canview_status_t canview_link_key_store_mark_ready(canview_link_key_store_t *store);
canview_status_t canview_link_key_store_commit(canview_link_key_store_t *store);
canview_status_t canview_link_key_store_recover(canview_link_key_store_t *store);

canview_status_t canview_frame_pool_reset(canview_frame_pool_t *pool);
canview_status_t canview_frame_pool_acquire(
    canview_frame_pool_t *pool, uint8_t *slot_index, uint8_t **buffer);
canview_status_t canview_frame_pool_release(canview_frame_pool_t *pool, uint8_t slot_index);

canview_status_t canview_error_rate_limiter_reset(
    canview_error_rate_limiter_t *limiter, uint16_t per_second_limit);
bool canview_error_rate_limiter_allow(
    canview_error_rate_limiter_t *limiter, uint32_t now_ms);

canview_status_t canview_qos_scheduler_reset(
    canview_qos_scheduler_t *scheduler, uint32_t first_sequence);
canview_status_t canview_qos_submit(
    canview_qos_scheduler_t *scheduler, const canview_encode_request_t *request,
    uint32_t now_ms, uint16_t ttl_ms, uint64_t request_token,
    const uint8_t **frame, size_t *frame_size);
canview_qos_event_t canview_qos_poll(
    canview_qos_scheduler_t *scheduler, uint32_t now_ms,
    const uint8_t **frame, size_t *frame_size);
canview_status_t canview_qos_ack(
    canview_qos_scheduler_t *scheduler, uint64_t request_token,
    uint32_t acknowledged_sequence, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
#endif /* CANVIEW_ESPNOW_H */
