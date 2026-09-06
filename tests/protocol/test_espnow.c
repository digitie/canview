/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_espnow.h"

#include <stdio.h>
#include <string.h>

#ifndef CANVIEW_SOURCE_DIR
#define CANVIEW_SOURCE_DIR "."
#endif

#define TEST_FRAME_BYTES (CANVIEW_MAX_FRAME_SIZE)
#define TEST_PAYLOAD_BYTES (CANVIEW_MAX_PAYLOAD_SIZE)
#define TEST_ROOT_BYTE (0x31U)

#define CHECK(condition)                                                                          \
    do                                                                                            \
    {                                                                                             \
        if (!(condition))                                                                         \
        {                                                                                         \
            (void)fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);         \
            return 1;                                                                             \
        }                                                                                         \
    } while (0)

static bool read_fixture(const char *directory, const char *name, uint8_t *bytes, size_t capacity,
                         size_t *size)
{
    char path[512];
    const int written = snprintf(path, sizeof(path), "%s/protocol/golden/espnow-v1.3/%s/%s.bin",
                                 CANVIEW_SOURCE_DIR, directory, name);
    if (written < 0 || (size_t)written >= sizeof(path))
    {
        return false;
    }
    FILE *file = NULL;
#if defined(_MSC_VER)
    if (fopen_s(&file, path, "rb") != 0)
    {
        file = NULL;
    }
#else
    file = fopen(path, "rb");
#endif
    if (file == NULL)
    {
        return false;
    }
    const size_t count = fread(bytes, 1U, capacity, file);
    const int read_error = ferror(file);
    const int at_end = feof(file);
    const int close_result = fclose(file);
    if (close_result != 0 || read_error != 0)
    {
        return false;
    }
    *size = count;
    return count != capacity || at_end != 0;
}

static uint32_t read_le32_for_test(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) |
           ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}

static void default_meta(canview_transport_meta_t *meta)
{
    memset(meta, 0, sizeof(*meta));
    meta->sender_role = CANVIEW_ROLE_COMMUNICATOR;
    meta->receiver_role = CANVIEW_ROLE_PRIMARY_CONTROLLER;
    meta->link_state = CANVIEW_LINK_ONLINE;
    meta->expected_session_id = UINT32_C(0x01020304);
    meta->now_ms = 9000U;
    meta->encrypted = true;
    meta->peer_authenticated = true;
    meta->peer_known = true;
}

static void metadata_for(const char *name, canview_transport_meta_t *meta)
{
    default_meta(meta);
    if (strcmp(name, "hello") == 0)
    {
        meta->link_state = CANVIEW_LINK_SECURE_HELLO;
    }
    else if (strcmp(name, "capabilities") == 0)
    {
        meta->link_state = CANVIEW_LINK_NEGOTIATING;
    }
    else if (strcmp(name, "pair-discovery") == 0)
    {
        meta->sender_role = CANVIEW_ROLE_COMMUNICATOR;
        meta->receiver_role = CANVIEW_ROLE_PRIMARY_CONTROLLER;
        meta->link_state = CANVIEW_LINK_DISCOVERING;
        meta->expected_session_id = 0U;
        meta->encrypted = false;
        meta->peer_authenticated = false;
        meta->peer_known = false;
        meta->source_is_broadcast = true;
    }
    else if (strcmp(name, "command-retry") == 0 || strcmp(name, "config-get") == 0 ||
             strcmp(name, "config-set") == 0)
    {
        meta->sender_role = CANVIEW_ROLE_PRIMARY_CONTROLLER;
        meta->receiver_role = CANVIEW_ROLE_COMMUNICATOR;
    }
    else if (strcmp(name, "capture-status") == 0 || strcmp(name, "remote-config-status") == 0)
    {
        meta->sender_role = CANVIEW_ROLE_COMMUNICATOR;
        meta->receiver_role = CANVIEW_ROLE_DIAGNOSTIC_BRIDGE;
    }
    else if (strcmp(name, "config-result") == 0)
    {
        meta->sender_role = CANVIEW_ROLE_COMMUNICATOR;
        meta->receiver_role = CANVIEW_ROLE_PRIMARY_CONTROLLER;
    }
    else if (strcmp(name, "config-schema-request") == 0 ||
             strcmp(name, "remote-config-request") == 0)
    {
        meta->sender_role = CANVIEW_ROLE_DIAGNOSTIC_BRIDGE;
        meta->receiver_role = CANVIEW_ROLE_COMMUNICATOR;
    }
}

static int decode_and_round_trip(const char *name)
{
    static const char *const names[] = {
        "bulk-ack", "bulk-begin", "bulk-end", "bulk-fragment", "capabilities",
        "capture-status", "command-retry", "config-get", "config-result",
        "config-schema-request", "config-set", "hello", "pair-discovery",
        "remote-config-request", "remote-config-status",
    };
    (void)names;
    uint8_t bytes[TEST_FRAME_BYTES] = {0};
    size_t length = 0U;
    CHECK(read_fixture("", name, bytes, sizeof(bytes), &length));
    canview_transport_meta_t meta;
    metadata_for(name, &meta);
    canview_peer_session_t session;
    CHECK(canview_peer_session_reset(&session) == CANVIEW_OK);
    if (meta.expected_session_id != 0U)
    {
        CHECK(canview_peer_session_start(&session, meta.receiver_role, meta.expected_session_id,
                                         true, true, meta.now_ms) == CANVIEW_OK);
        CHECK(canview_control_time_sync_update(&session.control_time_sync, 3U, 1000U,
                                               meta.now_ms, 1U, 2U) == CANVIEW_OK);
    }
    canview_decoded_frame_t decoded;
    const canview_decode_result_t result = canview_frame_decode(
        &meta, bytes, length, &session, &decoded);
    CHECK(result == CANVIEW_DECODE_DELIVER || result == CANVIEW_DECODE_ACK_REQUIRED);
    CHECK(decoded.payload_size == length - CANVIEW_HEADER_SIZE);
    uint8_t encoded[TEST_FRAME_BYTES] = {0};
    size_t encoded_size = 0U;
    canview_encode_request_t request = {
        .meta = meta,
        .header = decoded.header,
        .variant_index = decoded.variant_index,
        .payload = decoded.payload,
        .payload_size = decoded.payload_size,
    };
    CHECK(canview_frame_encode(&request, encoded, sizeof(encoded), &encoded_size) == CANVIEW_ENCODE_OK);
    CHECK(encoded_size == length);
    CHECK(memcmp(encoded, bytes, length) == 0);
    CHECK(canview_peer_session_release_rx(&session) == CANVIEW_OK);
    return 0;
}

static int test_vectors(void)
{
    static const char *const names[] = {
        "bulk-ack", "bulk-begin", "bulk-end", "bulk-fragment", "capabilities",
        "capture-status", "command-retry", "config-get", "config-result",
        "config-schema-request", "config-set", "hello", "pair-discovery",
        "remote-config-request", "remote-config-status",
    };
    for (size_t index = 0U; index < sizeof(names) / sizeof(names[0]); ++index)
    {
        CHECK(decode_and_round_trip(names[index]) == 0);
    }
    return 0;
}

static int test_malformed(void)
{
    static const char *const names[] = {
        "bad-crc", "bad-magic", "payload-length-overrun", "reserved-header",
        "truncated-header", "unknown-flags",
    };
    for (size_t index = 0U; index < sizeof(names) / sizeof(names[0]); ++index)
    {
        uint8_t bytes[TEST_FRAME_BYTES] = {0};
        size_t length = 0U;
        CHECK(read_fixture("malformed", names[index], bytes, sizeof(bytes), &length));
        canview_transport_meta_t meta;
        default_meta(&meta);
        canview_peer_session_t session;
        CHECK(canview_peer_session_reset(&session) == CANVIEW_OK);
        CHECK(canview_peer_session_start(&session, CANVIEW_ROLE_COMMUNICATOR,
                                         UINT32_C(0x01020304), true, true, 9000U) == CANVIEW_OK);
        canview_decoded_frame_t decoded;
        const canview_decode_result_t result = canview_frame_decode(
            &meta, bytes, length, &session, &decoded);
        CHECK(result == CANVIEW_DECODE_DROP_SILENT || result == CANVIEW_DECODE_ERROR_RATE_LIMITED);
        CHECK(decoded.reason != CANVIEW_DECODE_REASON_NONE);
        CHECK(session.rx_reserved == 0U);
    }
    return 0;
}

static int test_session(void)
{
    canview_peer_session_t session;
    CHECK(canview_peer_session_start(&session, CANVIEW_ROLE_COMMUNICATOR, 7U, true, true, 0U) ==
          CANVIEW_OK);
    CHECK(session.state == CANVIEW_LINK_SECURE_HELLO);
    CHECK(canview_peer_session_on_message(&session, CANVIEW_MSG_HELLO, 1U) == CANVIEW_OK);
    CHECK(session.state == CANVIEW_LINK_NEGOTIATING);
    CHECK(canview_peer_session_on_message(&session, CANVIEW_MSG_CAPABILITIES, 2U) == CANVIEW_OK);
    CHECK(session.state == CANVIEW_LINK_TIME_SYNC);
    for (size_t index = 0U; index < 3U; ++index)
    {
        CHECK(canview_peer_session_on_message(&session, CANVIEW_MSG_TIME_SYNC_RESPONSE,
                                              (uint32_t)(3U + index)) == CANVIEW_OK);
    }
    CHECK(session.state == CANVIEW_LINK_STATE_SYNC);
    CHECK(canview_peer_session_on_message(&session, CANVIEW_MSG_STATE_SNAPSHOT, 7U) == CANVIEW_OK);
    CHECK(session.state == CANVIEW_LINK_ONLINE);
    CHECK(canview_control_time_sync_update(&session.control_time_sync, 5U, 50000U, 10U, 1U, 2U) ==
          CANVIEW_OK);
    CHECK(canview_control_time_sync_is_valid(&session.control_time_sync, 30010U));
    CHECK(!canview_control_time_sync_is_valid(&session.control_time_sync, 30011U));
    CHECK(canview_control_time_sync_update(&session.control_time_sync, 4U, 0U, 20U, 1U, 2U) ==
          CANVIEW_STALE);
    for (size_t index = 0U; index < 5U; ++index)
    {
        canview_peer_session_auth_failure(&session, 100U);
    }
    CHECK(!canview_peer_session_auth_allowed(&session, 100U));
    CHECK(canview_peer_session_auth_allowed(&session, 1100U));
    canview_sequence_window_t window;
    CHECK(canview_sequence_window_reset(&window) == CANVIEW_OK);
    CHECK(canview_sequence_window_accept(&window, UINT32_MAX) == CANVIEW_OK);
    CHECK(canview_sequence_window_accept(&window, 0U) == CANVIEW_OK);
    CHECK(canview_sequence_window_accept(&window, UINT32_MAX) == CANVIEW_DUPLICATE);
    return 0;
}

static int fake_hmac(void *context, const uint8_t *key, size_t key_length,
                     const uint8_t *data, size_t data_length,
                     uint8_t output[CANVIEW_ESPNOW_HASH_BYTES])
{
    uint32_t state = context == NULL ? 0xA5A5A5A5U : *(const uint32_t *)context;
    for (size_t index = 0U; index < key_length; ++index)
    {
        state = (state * 33U) ^ key[index];
    }
    for (size_t index = 0U; index < data_length; ++index)
    {
        state = (state * 33U) ^ data[index];
    }
    for (size_t index = 0U; index < CANVIEW_ESPNOW_HASH_BYTES; ++index)
    {
        output[index] = (uint8_t)(state >> ((index % 4U) * 8U));
        state = state * 33U + (uint32_t)index;
    }
    return 0;
}

static int test_security_and_keys(void)
{
    uint8_t bytes[TEST_FRAME_BYTES] = {0};
    size_t length = 0U;
    CHECK(read_fixture("", "pair-discovery", bytes, sizeof(bytes), &length));
    const uint8_t root_a[CANVIEW_ESPNOW_LINK_ROOT_BYTES] = {[0] = TEST_ROOT_BYTE};
    const uint8_t root_b[CANVIEW_ESPNOW_LINK_ROOT_BYTES] = {[0] = (uint8_t)(TEST_ROOT_BYTE + 1U)};
    uint32_t hmac_context = 11U;
    const canview_crypto_adapter_t crypto = {
        .context = &hmac_context,
        .hmac_sha256 = fake_hmac,
        .hkdf_sha256 = NULL,
    };
    uint8_t tag_a[CANVIEW_ESPNOW_CONTROL_TAG_BYTES] = {0};
    uint8_t tag_b[CANVIEW_ESPNOW_CONTROL_TAG_BYTES] = {0};
    CHECK(canview_pairing_tag_compute(&crypto, root_a, CANVIEW_MSG_DISCOVERY,
                                      bytes + CANVIEW_HEADER_SIZE, length - CANVIEW_HEADER_SIZE,
                                      tag_a) == CANVIEW_OK);
    CHECK(canview_pairing_tag_compute(&crypto, root_b, CANVIEW_MSG_DISCOVERY,
                                      bytes + CANVIEW_HEADER_SIZE, length - CANVIEW_HEADER_SIZE,
                                      tag_b) == CANVIEW_OK);
    CHECK(memcmp(tag_a, tag_b, sizeof(tag_a)) != 0);
    CHECK(canview_pairing_tag_verify(&crypto, root_a, CANVIEW_MSG_DISCOVERY,
                                     bytes + CANVIEW_HEADER_SIZE, length - CANVIEW_HEADER_SIZE,
                                     tag_a) == CANVIEW_OK);
    tag_a[0] ^= 1U;
    CHECK(canview_pairing_tag_verify(&crypto, root_a, CANVIEW_MSG_DISCOVERY,
                                     bytes + CANVIEW_HEADER_SIZE, length - CANVIEW_HEADER_SIZE,
                                     tag_a) == CANVIEW_AUTH_FAILED);
    canview_peer_session_t session;
    CHECK(canview_peer_session_reset(&session) == CANVIEW_OK);
    uint8_t nonce[CANVIEW_ESPNOW_PAIRING_NONCE_BYTES] = {0};
    uint8_t transcript[CANVIEW_ESPNOW_HASH_BYTES] = {0};
    CHECK(canview_pairing_replay_accept(&session, nonce, transcript) == CANVIEW_OK);
    CHECK(canview_pairing_replay_accept(&session, nonce, transcript) == CANVIEW_DUPLICATE);
    canview_link_key_store_t store;
    CHECK(canview_link_key_store_reset(&store) == CANVIEW_OK);
    CHECK(canview_link_key_store_stage(&store, 1U, root_a) == CANVIEW_OK);
    CHECK(canview_link_key_store_mark_ready(&store) == CANVIEW_OK);
    CHECK(canview_link_key_store_commit(&store) == CANVIEW_OK);
    CHECK(store.active.valid && store.active.generation == 1U);
    CHECK(canview_link_key_store_stage(&store, 2U, root_b) == CANVIEW_OK);
    CHECK(canview_link_key_store_recover(&store) == CANVIEW_OK);
    CHECK(store.active.generation == 1U);
    CHECK(canview_link_key_store_stage(&store, 2U, root_b) == CANVIEW_OK);
    CHECK(canview_link_key_store_mark_ready(&store) == CANVIEW_OK);
    CHECK(canview_link_key_store_recover(&store) == CANVIEW_OK);
    CHECK(store.active.generation == 2U);
    return 0;
}

static int test_control_tag(void)
{
    uint8_t bytes[TEST_FRAME_BYTES] = {0};
    size_t length = 0U;
    CHECK(read_fixture("", "command-retry", bytes, sizeof(bytes), &length));
    uint32_t hmac_context = 23U;
    const canview_crypto_adapter_t crypto = {
        .context = &hmac_context,
        .hmac_sha256 = fake_hmac,
        .hkdf_sha256 = NULL,
    };
    const uint8_t root[CANVIEW_ESPNOW_LINK_ROOT_BYTES] = {[0] = TEST_ROOT_BYTE};
    uint8_t *payload = bytes + CANVIEW_HEADER_SIZE;
    uint8_t expected[CANVIEW_ESPNOW_CONTROL_TAG_BYTES] = {0};
    CHECK(canview_control_tag_compute(&crypto, root, UINT32_C(0x01020304),
                                      CANVIEW_MSG_COMMAND_REQUEST, payload,
                                      length - CANVIEW_HEADER_SIZE, expected) == CANVIEW_OK);
    CHECK(canview_control_tag_verify(&crypto, root, UINT32_C(0x01020304),
                                     CANVIEW_MSG_COMMAND_REQUEST, payload,
                                     length - CANVIEW_HEADER_SIZE, expected) == CANVIEW_OK);
    payload[12U] ^= 1U;
    CHECK(canview_control_tag_verify(&crypto, root, UINT32_C(0x01020304),
                                     CANVIEW_MSG_COMMAND_REQUEST, payload,
                                     length - CANVIEW_HEADER_SIZE, expected) == CANVIEW_AUTH_FAILED);
    return 0;
}

static int test_qos(void)
{
    uint8_t bytes[TEST_FRAME_BYTES] = {0};
    size_t length = 0U;
    CHECK(read_fixture("", "command-retry", bytes, sizeof(bytes), &length));
    canview_transport_meta_t meta;
    metadata_for("command-retry", &meta);
    canview_qos_scheduler_t scheduler;
    CHECK(canview_qos_scheduler_reset(&scheduler, 100U) == CANVIEW_OK);
    canview_encode_request_t request = {
        .meta = meta,
        .header = {
            .message_type = CANVIEW_MSG_COMMAND_REQUEST,
            .flags = CANVIEW_FLAG_ACK_REQUIRED,
            .priority = CANVIEW_PRIORITY_COMMAND,
            .session_id = UINT32_C(0x01020304),
            .sequence = 0U,
            .correlation_id = 0U,
            .sender_time = 5000U,
        },
        .variant_index = 0U,
        .payload = bytes + CANVIEW_HEADER_SIZE,
        .payload_size = length - CANVIEW_HEADER_SIZE,
    };
    const uint8_t *frame = NULL;
    size_t frame_size = 0U;
    CHECK(canview_qos_submit(&scheduler, &request, 5000U, 2000U,
                             UINT64_C(0x0102030405060708), &frame, &frame_size) == CANVIEW_OK);
    CHECK(frame != NULL && frame_size == length && read_le32_for_test(frame + 12U) == 100U);
    CHECK(canview_qos_poll(&scheduler, 5079U, &frame, &frame_size) == CANVIEW_QOS_NO_EVENT);
    CHECK(canview_qos_poll(&scheduler, 5080U, &frame, &frame_size) == CANVIEW_QOS_RETRY_READY);
    CHECK(read_le32_for_test(frame + 12U) == 101U);
    CHECK(canview_qos_ack(&scheduler, UINT64_C(0x0102030405060708), 101U, 5090U) == CANVIEW_OK);
    CHECK(canview_qos_poll(&scheduler, 6000U, &frame, &frame_size) == CANVIEW_QOS_NO_EVENT);
    return 0;
}

static int test_pool_and_fuzz(void)
{
    canview_frame_pool_t pool;
    CHECK(canview_frame_pool_reset(&pool) == CANVIEW_OK);
    uint8_t slots[CANVIEW_ESPNOW_RX_RESOURCE_LIMIT] = {0};
    uint8_t *buffers[CANVIEW_ESPNOW_RX_RESOURCE_LIMIT] = {0};
    for (size_t index = 0U; index < CANVIEW_ESPNOW_RX_RESOURCE_LIMIT; ++index)
    {
        CHECK(canview_frame_pool_acquire(&pool, &slots[index], &buffers[index]) == CANVIEW_OK);
        CHECK(buffers[index] != NULL);
    }
    uint8_t extra_slot = 0U;
    uint8_t *extra_buffer = NULL;
    CHECK(canview_frame_pool_acquire(&pool, &extra_slot, &extra_buffer) == CANVIEW_BUFFER_TOO_SMALL);
    CHECK(canview_frame_pool_release(&pool, slots[3]) == CANVIEW_OK);
    CHECK(canview_frame_pool_acquire(&pool, &extra_slot, &extra_buffer) == CANVIEW_OK);
    canview_error_rate_limiter_t limiter;
    CHECK(canview_error_rate_limiter_reset(&limiter, 2U) == CANVIEW_OK);
    CHECK(canview_error_rate_limiter_allow(&limiter, 0U));
    CHECK(canview_error_rate_limiter_allow(&limiter, 0U));
    CHECK(!canview_error_rate_limiter_allow(&limiter, 0U));
    CHECK(canview_error_rate_limiter_allow(&limiter, 1000U));
    uint8_t random_bytes[1500] = {0};
    uint32_t state = 0x12345678U;
    canview_transport_meta_t meta;
    default_meta(&meta);
    canview_peer_session_t session;
    CHECK(canview_peer_session_start(&session, CANVIEW_ROLE_COMMUNICATOR,
                                     UINT32_C(0x01020304), true, true, 0U) == CANVIEW_OK);
    for (size_t iteration = 0U; iteration < 4096U; ++iteration)
    {
        state ^= state << 13U;
        state ^= state >> 17U;
        state ^= state << 5U;
        const size_t length = (size_t)(state % (sizeof(random_bytes) + 1U));
        for (size_t byte_index = 0U; byte_index < length; ++byte_index)
        {
            state = state * 1664525U + 1013904223U;
            random_bytes[byte_index] = (uint8_t)(state >> 24U);
        }
        canview_decoded_frame_t decoded;
        (void)canview_frame_decode(&meta, random_bytes, length, &session, &decoded);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        return 2;
    }
    if (strcmp(argv[1], "vectors") == 0)
    {
        return test_vectors();
    }
    if (strcmp(argv[1], "malformed") == 0)
    {
        return test_malformed();
    }
    if (strcmp(argv[1], "session") == 0)
    {
        return test_session();
    }
    if (strcmp(argv[1], "security") == 0)
    {
        return test_security_and_keys();
    }
    if (strcmp(argv[1], "control") == 0)
    {
        return test_control_tag();
    }
    if (strcmp(argv[1], "qos") == 0)
    {
        return test_qos();
    }
    if (strcmp(argv[1], "pool-fuzz") == 0)
    {
        return test_pool_and_fuzz();
    }
    return 2;
}
