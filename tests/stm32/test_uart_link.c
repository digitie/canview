/* SPDX-License-Identifier: GPL-3.0-only */
/** @file test_uart_link.c
 * @brief Host regression tests for the STM32 UART owner and its safety boundary.
 */
#include "canview_stm_uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expression)                                                                        \
    do                                                                                          \
    {                                                                                           \
        if (!(expression))                                                                       \
        {                                                                                        \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression);              \
            return 1;                                                                            \
        }                                                                                        \
    } while (0)

#define TEST_STM_BOOT_ID (UINT64_C(0x1111222233334444))
#define TEST_STM_DEVICE_ID (UINT64_C(0x5555666677778888))
#define TEST_ESP_BOOT_ID (UINT64_C(0x9999AAAABBBBCCCC))
#define TEST_ESP_DEVICE_ID (UINT64_C(0xDDDDEEEEFFFF0001))
#define TEST_CONTROLLER_BOOT_ID (UINT64_C(0x123456789ABCDEF0))
#define TEST_COMMAND_ORIGIN_ID (UINT64_C(0x0102030405060708))
#define TEST_COMMAND_ORIGIN_BOOT_ID (TEST_CONTROLLER_BOOT_ID)
#define TEST_COMMAND_SESSION_ID (UINT32_C(0x20212223))
#define TEST_COMMAND_GENERATION (UINT32_C(7))
#define TEST_SYNC_GENERATION (UINT32_C(1))
#define TEST_COMMAND_ID (UINT16_C(0x0101))
#define TEST_COMMAND_TTL_MS (UINT16_C(500))
#define TEST_MAX_DRAIN (64U)

typedef struct
{
    uint8_t message_type;
    uint8_t flags;
    uint32_t sequence;
    uint32_t correlation_id;
    size_t payload_size;
    uint8_t payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
} test_message_t;

typedef struct
{
    canview_stm_uart_context_t *runtime;
    bool allow;
    uint32_t calls;
    canview_status_t nested_status;
} test_authorizer_t;

typedef struct
{
    uint32_t calls;
    canview_status_t status;
} test_reset_hook_t;

static void put_le(uint8_t *bytes, size_t width, uint64_t value)
{
    for (size_t index = 0U; index < width; ++index)
    {
        bytes[index] = (uint8_t)(value >> (index * 8U));
    }
}

static uint64_t get_le(const uint8_t *bytes, size_t width)
{
    uint64_t value = 0U;
    for (size_t index = 0U; index < width; ++index)
    {
        value |= (uint64_t)bytes[index] << (index * 8U);
    }
    return value;
}

static bool allow_command(const canview_uart_message_view_t *view, uint64_t now_ms,
                          void *opaque)
{
    test_authorizer_t *authorizer = (test_authorizer_t *)opaque;
    if (authorizer == NULL || view == NULL ||
        view->wire.header.message_type != CANVIEW_UART_MSG_COMMAND_REQUEST)
    {
        return false;
    }
    ++authorizer->calls;
    if (authorizer->runtime != NULL)
    {
        authorizer->nested_status =
            canview_stm_uart_tick(authorizer->runtime, now_ms, now_ms * UINT64_C(1000));
    }
    return authorizer->allow;
}

static bool allow_any_message(const canview_uart_message_view_t *view, uint64_t now_ms,
                              void *opaque)
{
    (void)now_ms;
    test_authorizer_t *authorizer = (test_authorizer_t *)opaque;
    if (authorizer == NULL || view == NULL)
    {
        return false;
    }
    ++authorizer->calls;
    return authorizer->allow;
}

static canview_status_t test_reset_hook(void *opaque)
{
    test_reset_hook_t *hook = (test_reset_hook_t *)opaque;
    if (hook == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    ++hook->calls;
    return hook->status;
}

static int init_runtime(canview_stm_uart_context_t *runtime, test_authorizer_t *authorizer)
{
    canview_stm_uart_config_t config = {0};
    config.local_boot_id = TEST_STM_BOOT_ID;
    config.local_device_id = TEST_STM_DEVICE_ID;
    config.local_safety_revision = CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION;
    for (size_t index = 0U; index < sizeof(config.build_id_digest); ++index)
    {
        config.build_id_digest[index] = (uint8_t)(index + 1U);
    }
    if (authorizer != NULL)
    {
        config.authorize = allow_command;
        config.authorize_context = authorizer;
        authorizer->runtime = runtime;
        authorizer->nested_status = CANVIEW_OK;
    }
    CHECK(canview_stm_uart_init(runtime, &config, 0U, 0U) == CANVIEW_OK);
    return 0;
}

static canview_status_t feed_inbound(canview_stm_uart_context_t *runtime, uint8_t message_type,
                                     uint8_t flags, uint32_t sequence, uint32_t correlation_id,
                                     uint64_t sender_time_us, const uint8_t *payload,
                                     size_t payload_size, uint64_t now_ms, uint64_t now_us)
{
    uint8_t scratch[CANVIEW_UART_MAX_FRAME_SIZE];
    uint8_t serial[CANVIEW_UART_MAX_SERIAL_SIZE];
    size_t serial_size = 0U;
    const canview_status_t encode_status = canview_uart_message_encode(
        message_type, flags, sequence, correlation_id, sender_time_us, payload, payload_size,
        CANVIEW_UART_ENDPOINT_STM32, CANVIEW_UART_FLOW_INBOUND, scratch, sizeof(scratch), serial,
        sizeof(serial), &serial_size);
    if (encode_status != CANVIEW_OK)
    {
        return encode_status;
    }
    canview_status_t status = CANVIEW_INCOMPLETE;
    for (size_t index = 0U; index < serial_size; ++index)
    {
        status = canview_stm_uart_ingest_byte(runtime, serial[index], now_ms, now_us);
        if (index + 1U != serial_size && status != CANVIEW_INCOMPLETE)
        {
            return status;
        }
    }
    return status;
}

static canview_status_t take_outbound(canview_stm_uart_context_t *runtime,
                                      test_message_t *message)
{
    if (runtime == NULL || message == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const uint8_t *serial = NULL;
    size_t serial_size = 0U;
    uint32_t sequence = 0U;
    const canview_status_t begin_status =
        canview_stm_uart_tx_begin(runtime, &serial, &serial_size, &sequence);
    if (begin_status != CANVIEW_OK)
    {
        return begin_status;
    }
    canview_uart_codec_t codec;
    canview_status_t status = canview_uart_codec_reset(
        &codec, CANVIEW_UART_ENDPOINT_ESP32, CANVIEW_UART_FLOW_INBOUND);
    if (status != CANVIEW_OK)
    {
        (void)canview_stm_uart_tx_finish(runtime, CANVIEW_MALFORMED, 0U);
        return status;
    }
    canview_uart_message_view_t view;
    for (size_t index = 0U; index < serial_size; ++index)
    {
        status = canview_uart_codec_feed(&codec, serial[index], &view);
    }
    if (status != CANVIEW_OK || view.wire.payload_size > sizeof(message->payload))
    {
        (void)canview_stm_uart_tx_finish(runtime, CANVIEW_MALFORMED, 0U);
        return status == CANVIEW_OK ? CANVIEW_OVERSIZE : status;
    }
    message->message_type = view.wire.header.message_type;
    message->flags = view.wire.header.flags;
    message->sequence = view.wire.header.sequence;
    message->correlation_id = view.wire.header.correlation_id;
    message->payload_size = view.wire.payload_size;
    memcpy(message->payload, view.wire.payload, view.wire.payload_size);
    if (sequence != message->sequence)
    {
        (void)canview_stm_uart_tx_finish(runtime, CANVIEW_MALFORMED, 0U);
        return CANVIEW_MALFORMED;
    }
    CHECK(canview_stm_uart_tx_finish(runtime, CANVIEW_OK, 0U) == CANVIEW_OK);
    return CANVIEW_OK;
}

static int expect_outbound(canview_stm_uart_context_t *runtime, uint8_t message_type,
                           test_message_t *message)
{
    CHECK(take_outbound(runtime, message) == CANVIEW_OK);
    CHECK(message->message_type == message_type);
    return 0;
}

static int drain_outbound(canview_stm_uart_context_t *runtime)
{
    test_message_t message;
    for (size_t count = 0U; count < TEST_MAX_DRAIN; ++count)
    {
        const canview_status_t status = take_outbound(runtime, &message);
        if (status == CANVIEW_INCOMPLETE)
        {
            return 0;
        }
        CHECK(status == CANVIEW_OK);
    }
    return 1;
}

static void build_hello(uint8_t payload[72U], uint64_t boot_id, uint64_t device_id)
{
    memset(payload, 0, 72U);
    put_le(payload, 8U, boot_id);
    payload[8U] = CANVIEW_UART_PROTOCOL_MAJOR;
    payload[9U] = CANVIEW_UART_PROTOCOL_MINOR;
    payload[10U] = CANVIEW_UART_PROTOCOL_MAJOR;
    payload[11U] = CANVIEW_UART_PROTOCOL_MINOR;
    put_le(payload + 28U, 2U, CANVIEW_UART_MAX_FRAME_SIZE);
    put_le(payload + 48U, 8U, device_id);
}

static void build_hello_ack(uint8_t payload[40U], uint64_t peer_boot_id,
                            uint64_t local_boot_id)
{
    memset(payload, 0, 40U);
    put_le(payload, 8U, peer_boot_id);
    put_le(payload + 8U, 8U, local_boot_id);
    payload[16U] = CANVIEW_UART_PROTOCOL_MAJOR;
    payload[17U] = CANVIEW_UART_PROTOCOL_MINOR;
    put_le(payload + 28U, 2U, CANVIEW_UART_MAX_FRAME_SIZE);
}

static void build_heartbeat(uint8_t payload[48U], uint64_t boot_id, uint32_t state_revision)
{
    memset(payload, 0, 48U);
    put_le(payload, 8U, boot_id);
    put_le(payload + 8U, 8U, UINT64_C(2000));
    put_le(payload + 40U, 4U, state_revision);
}

static void build_time_sync(uint8_t payload[80U], uint8_t phase, uint64_t request_token,
                            uint64_t controller_boot_id, uint64_t stm_boot_id,
                            uint32_t generation, uint64_t t1, uint64_t t2, uint64_t t3,
                            uint64_t t4)
{
    memset(payload, 0, 80U);
    put_le(payload, 8U, request_token);
    put_le(payload + 8U, 8U, controller_boot_id);
    put_le(payload + 16U, 8U, stm_boot_id);
    put_le(payload + 24U, 4U, generation);
    payload[28U] = phase;
    put_le(payload + 32U, 8U, t1);
    put_le(payload + 40U, 8U, t2);
    put_le(payload + 48U, 8U, t3);
    put_le(payload + 56U, 8U, t4);
}

static void build_command(uint8_t payload[104U], uint64_t request_token, uint8_t digest_byte,
                          uint32_t sync_generation, uint32_t issued_at_ms)
{
    memset(payload, 0, 104U);
    put_le(payload, 8U, request_token);
    put_le(payload + 8U, 2U, TEST_COMMAND_ID);
    put_le(payload + 10U, 2U, TEST_COMMAND_TTL_MS);
    put_le(payload + 12U, 8U, TEST_COMMAND_ORIGIN_ID);
    put_le(payload + 20U, 8U, TEST_COMMAND_ORIGIN_BOOT_ID);
    put_le(payload + 28U, 4U, TEST_COMMAND_SESSION_ID);
    put_le(payload + 32U, 4U, TEST_COMMAND_GENERATION);
    put_le(payload + 36U, 4U, issued_at_ms);
    put_le(payload + 40U, 4U, sync_generation);
    put_le(payload + 44U, 4U, CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION);
    memset(payload + 56U, digest_byte, CANVIEW_UART_COMMAND_DIGEST_SIZE);
    memset(payload + 88U, 0xB2, CANVIEW_UART_CONTROL_TAG_SIZE);
}

static void build_lease(uint8_t payload[52U], uint64_t request_token, uint8_t action,
                        uint32_t requested_ms, uint64_t lease_id, uint16_t scope,
                        uint32_t expected_state_revision, uint32_t control_generation)
{
    memset(payload, 0, 52U);
    put_le(payload, 8U, request_token);
    payload[8U] = action;
    put_le(payload + 12U, 4U, requested_ms);
    put_le(payload + 16U, 8U, lease_id);
    put_le(payload + 24U, 2U, scope);
    put_le(payload + 28U, 4U, expected_state_revision);
    put_le(payload + 32U, 4U, control_generation);
    memset(payload + 36U, 0xC4, CANVIEW_UART_CONTROL_TAG_SIZE);
}

static void build_plan_begin(uint8_t payload[32U], uint64_t request_token,
                             uint32_t revision, uint32_t expected_active_revision,
                             uint16_t total_chunks, uint16_t filter_count,
                             uint16_t max_records_per_second, uint32_t max_bytes_per_second,
                             uint8_t bus_mask)
{
    memset(payload, 0, 32U);
    payload[0U] = CANVIEW_UART_PLAN_OP_BEGIN;
    put_le(payload + 4U, 8U, request_token);
    put_le(payload + 12U, 4U, revision);
    put_le(payload + 16U, 4U, expected_active_revision);
    put_le(payload + 20U, 2U, total_chunks);
    put_le(payload + 22U, 2U, filter_count);
    put_le(payload + 24U, 2U, max_records_per_second);
    put_le(payload + 26U, 4U, max_bytes_per_second);
    payload[30U] = bus_mask;
}

static void build_plan_chunk(uint8_t payload[36U], uint64_t request_token,
                             uint32_t revision, uint16_t chunk_index, uint16_t chunk_count,
                             uint8_t bus_id, uint8_t filter_flags, uint32_t can_id,
                             uint32_t can_mask)
{
    memset(payload, 0, 36U);
    payload[0U] = CANVIEW_UART_PLAN_OP_CHUNK;
    put_le(payload + 4U, 8U, request_token);
    put_le(payload + 12U, 4U, revision);
    put_le(payload + 16U, 2U, chunk_index);
    put_le(payload + 18U, 2U, chunk_count);
    payload[20U] = 1U;
    payload[24U] = bus_id;
    payload[25U] = filter_flags;
    put_le(payload + 28U, 4U, can_id);
    put_le(payload + 32U, 4U, can_mask);
}

static void build_plan_commit(uint8_t payload[48U], uint64_t request_token,
                              uint32_t revision, const uint8_t digest[32U])
{
    memset(payload, 0, 48U);
    payload[0U] = CANVIEW_UART_PLAN_OP_COMMIT;
    put_le(payload + 4U, 8U, request_token);
    put_le(payload + 12U, 4U, revision);
    memcpy(payload + 16U, digest, 32U);
}

static void build_capture_control(uint8_t payload[28U], uint64_t request_token,
                                  uint64_t capture_id)
{
    memset(payload, 0, 28U);
    put_le(payload, 8U, request_token);
    put_le(payload + 8U, 8U, capture_id);
    payload[16U] = CANVIEW_UART_CAPTURE_START;
}

static void build_diagnostic_counters(uint8_t payload[16U], uint64_t boot_id,
                                      uint32_t state_revision)
{
    memset(payload, 0, 16U);
    put_le(payload, 8U, boot_id);
    put_le(payload + 8U, 4U, state_revision);
}

static int establish_link(canview_stm_uart_context_t *runtime, test_authorizer_t *authorizer)
{
    CHECK(init_runtime(runtime, authorizer) == 0);
    test_message_t message;
    CHECK(expect_outbound(runtime, CANVIEW_UART_MSG_LINK_HELLO, &message) == 0);
    for (size_t index = 0U; index < CANVIEW_STM_UART_BUILD_ID_DIGEST_SIZE; ++index)
    {
        CHECK(message.payload[32U + index] == (uint8_t)(index + 1U));
    }

    uint8_t hello[72U];
    build_hello(hello, TEST_ESP_BOOT_ID, TEST_ESP_DEVICE_ID);
    CHECK(feed_inbound(runtime, CANVIEW_UART_MSG_LINK_HELLO, 0U, 1U, 0U, 1000U, hello,
                       sizeof(hello), 10U, 1000U) == CANVIEW_OK);
    CHECK(expect_outbound(runtime, CANVIEW_UART_MSG_LINK_HELLO, &message) == 0);
    CHECK(expect_outbound(runtime, CANVIEW_UART_MSG_LINK_HELLO_ACK, &message) == 0);

    uint8_t hello_ack[40U];
    build_hello_ack(hello_ack, TEST_ESP_BOOT_ID, TEST_STM_BOOT_ID);
    CHECK(feed_inbound(runtime, CANVIEW_UART_MSG_LINK_HELLO_ACK, CANVIEW_UART_FLAG_RESPONSE, 2U,
                       0U, 2000U, hello_ack, sizeof(hello_ack), 20U, 2000U) == CANVIEW_OK);
    CHECK(expect_outbound(runtime, CANVIEW_UART_MSG_SAFETY_SNAPSHOT, &message) == 0);
    CHECK(get_le(message.payload, 8U) == TEST_STM_BOOT_ID);
    CHECK(get_le(message.payload + 8U, 4U) == CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION);
    CHECK((message.flags & CANVIEW_UART_FLAG_SNAPSHOT) != 0U);

    CHECK(canview_stm_uart_set_cts_blocked(runtime, false, 20U) == CANVIEW_OK);
    uint8_t heartbeat[48U];
    build_heartbeat(heartbeat, TEST_ESP_BOOT_ID,
                    CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION);
    CHECK(feed_inbound(runtime, CANVIEW_UART_MSG_HEARTBEAT, 0U, 3U, 0U, 3000U, heartbeat,
                       sizeof(heartbeat), 30U, 3000U) == CANVIEW_OK);
    CHECK(expect_outbound(runtime, CANVIEW_UART_MSG_SAFETY_SNAPSHOT, &message) == 0);
    canview_uart_link_t link;
    CHECK(canview_stm_uart_get_link(runtime, &link) == CANVIEW_OK);
    CHECK(link.hello_complete && link.hello_ack_complete && link.heartbeat_seen &&
          link.safety_snapshot_valid && link.cts_known && !link.cts_blocked &&
          link.state == CANVIEW_UART_LINK_ONLINE);
    return 0;
}

static int establish_time_mapping(canview_stm_uart_context_t *runtime, uint32_t sequence_base)
{
    uint8_t request[80U];
    build_time_sync(request, CANVIEW_UART_TIME_SYNC_REQUEST, UINT64_C(0x1234),
                    TEST_CONTROLLER_BOOT_ID, 0U, TEST_SYNC_GENERATION, 1000U, 0U, 0U, 0U);
    CHECK(feed_inbound(runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       sequence_base, 0U, 1000U, request, sizeof(request), 40U, 2000U) ==
          CANVIEW_OK);
    test_message_t response;
    CHECK(expect_outbound(runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC, &response) == 0);
    CHECK(response.payload[28U] == CANVIEW_UART_TIME_SYNC_RESPONSE);
    CHECK(get_le(response.payload + 32U, 8U) == 1000U);
    CHECK(get_le(response.payload + 40U, 8U) == 2000U);
    CHECK(get_le(response.payload + 48U, 8U) == 2000U);

    uint8_t commit[80U];
    build_time_sync(commit, CANVIEW_UART_TIME_SYNC_COMMIT, UINT64_C(0x1234),
                    TEST_CONTROLLER_BOOT_ID, TEST_STM_BOOT_ID, TEST_SYNC_GENERATION, 1000U,
                    2000U, 2000U, 3000U);
    CHECK(feed_inbound(runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       sequence_base + 1U, 0U, 3000U, commit, sizeof(commit), 50U, 3000U) ==
          CANVIEW_OK);
    canview_stm_uart_time_mapping_t mapping;
    CHECK(canview_stm_uart_get_time_mapping(runtime, &mapping) == CANVIEW_OK);
    CHECK(mapping.valid && mapping.generation == TEST_SYNC_GENERATION &&
          mapping.controller_boot_id == TEST_CONTROLLER_BOOT_ID &&
          mapping.stm_boot_id == TEST_STM_BOOT_ID && mapping.offset_stm_minus_controller_us == 0 &&
          mapping.uncertainty_us <= CANVIEW_UART_CONTROL_TIME_MAX_UNCERTAINTY_US);
    return 0;
}

static int test_handshake_and_time_sync(void)
{
    static canview_stm_uart_context_t runtime;
    CHECK(establish_link(&runtime, NULL) == 0);
    CHECK(establish_time_mapping(&runtime, 4U) == 0);
    CHECK(drain_outbound(&runtime) == 0);
    return 0;
}

static int test_time_sync_cross_clock_and_pending_timeout(void)
{
    static canview_stm_uart_context_t runtime;
    CHECK(establish_link(&runtime, NULL) == 0);

    uint8_t request[80U];
    build_time_sync(request, CANVIEW_UART_TIME_SYNC_REQUEST, UINT64_C(0x2201),
                    TEST_CONTROLLER_BOOT_ID, 0U, TEST_SYNC_GENERATION, 1000000U, 0U, 0U, 0U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       4U, 0x220U, 1000000U, request, sizeof(request), 40U, 990100U) ==
          CANVIEW_OK);
    test_message_t message;
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC, &message) == 0);
    CHECK(get_le(message.payload + 40U, 8U) == 990100U &&
          get_le(message.payload + 48U, 8U) == 990100U);

    uint8_t commit[80U];
    build_time_sync(commit, CANVIEW_UART_TIME_SYNC_COMMIT, UINT64_C(0x2201),
                    TEST_CONTROLLER_BOOT_ID, TEST_STM_BOOT_ID, TEST_SYNC_GENERATION, 1000000U,
                    990100U, 990100U, 1000210U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       5U, 0x221U, 1000210U, commit, sizeof(commit), 50U, 1000210U) ==
          CANVIEW_OK);
    canview_stm_uart_time_mapping_t mapping;
    CHECK(canview_stm_uart_get_time_mapping(&runtime, &mapping) == CANVIEW_OK);
    CHECK(mapping.valid && mapping.offset_stm_minus_controller_us < 0);

    static canview_stm_uart_context_t pending_runtime;
    CHECK(establish_link(&pending_runtime, NULL) == 0);
    build_time_sync(request, CANVIEW_UART_TIME_SYNC_REQUEST, UINT64_C(0x2202),
                    TEST_CONTROLLER_BOOT_ID, 0U, TEST_SYNC_GENERATION, 2000000U, 0U, 0U, 0U);
    CHECK(feed_inbound(&pending_runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       4U, 0x222U, 2000000U, request, sizeof(request), 40U, 2000000U) ==
          CANVIEW_OK);
    CHECK(expect_outbound(&pending_runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC, &message) == 0);
    CHECK(pending_runtime.pending_sync_valid);
    uint8_t heartbeat[48U];
    build_heartbeat(heartbeat, TEST_ESP_BOOT_ID,
                    CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION);
    CHECK(feed_inbound(&pending_runtime, CANVIEW_UART_MSG_HEARTBEAT, 0U, 5U, 0U, 900000U,
                       heartbeat, sizeof(heartbeat), 900U, 900000U) == CANVIEW_OK);
    CHECK(expect_outbound(&pending_runtime, CANVIEW_UART_MSG_SAFETY_SNAPSHOT, &message) == 0);
    CHECK(canview_stm_uart_tick(
              &pending_runtime, 40U + CANVIEW_STM_UART_TIME_SYNC_PENDING_TIMEOUT_MS + 1U,
              2001000U) == CANVIEW_OK);
    CHECK(!pending_runtime.pending_sync_valid);
    build_time_sync(commit, CANVIEW_UART_TIME_SYNC_COMMIT, UINT64_C(0x2202),
                    TEST_CONTROLLER_BOOT_ID, TEST_STM_BOOT_ID, TEST_SYNC_GENERATION, 2000000U,
                    2000000U, 2000000U, 2000100U);
    const canview_status_t stale_status = feed_inbound(
        &pending_runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
        (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY), 6U, 0x223U,
        2000100U, commit, sizeof(commit), 1042U, 2000100U);
    CHECK(stale_status == CANVIEW_STALE);
    return 0;
}

static int test_reset_hook_and_safety_inhibit(void)
{
    static canview_stm_uart_context_t runtime;
    test_authorizer_t authorizer = {.allow = true};
    CHECK(establish_link(&runtime, &authorizer) == 0);
    test_reset_hook_t hook = {.status = CANVIEW_OK};
    CHECK(canview_stm_uart_set_reset_hook(&runtime, test_reset_hook, &hook) == CANVIEW_OK);

    runtime.safety_inhibited = true;
    uint8_t command[104U];
    build_command(command, UINT64_C(0x2301), 0xA1U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       4U, 0x230U, 3000U, command, sizeof(command), 40U, 3000U) ==
          CANVIEW_AUTH_FAILED);
    CHECK(authorizer.calls == 0U);
    test_message_t message;
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);

    uint8_t hello[72U];
    build_hello(hello, TEST_ESP_BOOT_ID + 1U, TEST_ESP_DEVICE_ID);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_LINK_HELLO, 0U, 5U, 0U, 4000U, hello,
                       sizeof(hello), 50U, 4000U) == CANVIEW_OK);
    CHECK(hook.calls == 1U);
    hook.status = CANVIEW_TIMEOUT;
    CHECK(canview_stm_uart_reset(&runtime, 60U, 5000U) == CANVIEW_TIMEOUT);
    CHECK(hook.calls == 2U);
    hook.status = CANVIEW_OK;
    CHECK(canview_stm_uart_reset(&runtime, 61U, 6000U) == CANVIEW_OK);
    CHECK(hook.calls == 3U);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_LINK_HELLO, &message) == 0);
    for (size_t index = 0U; index < CANVIEW_STM_UART_BUILD_ID_DIGEST_SIZE; ++index)
    {
        CHECK(message.payload[32U + index] == (uint8_t)(index + 1U));
    }
    return 0;
}

static int test_time_sync_commit_expiry_before_tick(void)
{
    static const uint64_t ages[] = {
        CANVIEW_STM_UART_TIME_SYNC_PENDING_TIMEOUT_MS - 1U,
        CANVIEW_STM_UART_TIME_SYNC_PENDING_TIMEOUT_MS,
        CANVIEW_STM_UART_TIME_SYNC_PENDING_TIMEOUT_MS + 1U};
    for (size_t index = 0U; index < sizeof(ages) / sizeof(ages[0]); ++index)
    {
        static canview_stm_uart_context_t runtime;
        CHECK(establish_link(&runtime, NULL) == 0);
        uint8_t payload[80U];
        build_time_sync(payload, CANVIEW_UART_TIME_SYNC_REQUEST, 0x9001U,
                        TEST_CONTROLLER_BOOT_ID, 0U, TEST_SYNC_GENERATION,
                        2000000U, 0U, 0U, 0U);
        CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                           CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY,
                           4U, 0U, 2000000U, payload, sizeof(payload), 40U, 2000000U) ==
              CANVIEW_OK);
        test_message_t message;
        CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC, &message) == 0);
        uint8_t heartbeat[48U];
        build_heartbeat(heartbeat, TEST_ESP_BOOT_ID,
                        CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION);
        CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_HEARTBEAT, 0U, 5U, 0U,
                           900000U, heartbeat, sizeof(heartbeat), 900U, 900000U) == CANVIEW_OK);
        build_time_sync(payload, CANVIEW_UART_TIME_SYNC_COMMIT, 0x9001U,
                        TEST_CONTROLLER_BOOT_ID, TEST_STM_BOOT_ID, TEST_SYNC_GENERATION,
                        2000000U, 2000000U, 2000000U, 2000100U);
        const canview_status_t status = feed_inbound(
            &runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
            CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY,
            6U, 0U, 2000100U, payload, sizeof(payload), 40U + ages[index], 2000100U);
        CHECK(status == (index == 0U ? CANVIEW_OK : CANVIEW_STALE));
        CHECK(runtime.time_mapping.valid == (index == 0U));
        CHECK(!runtime.pending_sync_valid);
    }
    return 0;
}

static int test_command_idempotency_and_reentry(void)
{
    static canview_stm_uart_context_t runtime;
    test_authorizer_t authorizer = {.allow = true};
    CHECK(establish_link(&runtime, &authorizer) == 0);
    CHECK(establish_time_mapping(&runtime, 4U) == 0);

    uint8_t command[104U];
    build_command(command, UINT64_C(0xABC1), 0xA1U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(
              &runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
              (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY), 6U,
              0x600U, 3000U, command, sizeof(command), 60U, 3000U) == CANVIEW_OK);
    canview_stm_uart_pending_command_t pending;
    CHECK(canview_stm_uart_pending_peek(&runtime, &pending) == CANVIEW_OK);
    CHECK(pending.valid && pending.payload_size == sizeof(command) &&
          pending.key.request_token == UINT64_C(0xABC1));
    CHECK(authorizer.calls == 1U && authorizer.nested_status == CANVIEW_RESOURCE_BUSY);

    test_message_t message;
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_OK &&
          get_le(message.payload + 8U, 8U) == UINT64_C(0xABC1));
    CHECK(canview_stm_uart_pending_complete(
              &runtime, &pending.cache_handle, CANVIEW_STM_UART_RESULT_STAGE_COMPLETED, 0U,
              CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION,
              CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION, 60U, 61U, 4000U) == CANVIEW_OK);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_COMMAND_RESULT, &message) == 0);
    CHECK(message.payload[10U] == CANVIEW_STM_UART_RESULT_STAGE_COMPLETED);
    CHECK(get_le(message.payload + 22U, 4U) == 6U);

    authorizer.allow = false;
    build_command(command, UINT64_C(0xABC1), 0xA1U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       6U, 0x603U, 3500U, command, sizeof(command), 65U, 3500U) ==
          CANVIEW_DUPLICATE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_COMMAND_RESULT, &message) == 0);
    CHECK(message.correlation_id == 0x603U &&
          message.payload[10U] == CANVIEW_STM_UART_RESULT_STAGE_COMPLETED);

    authorizer.allow = true;
    build_command(command, UINT64_C(0xABC1), 0xA1U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       7U, 0x601U, 5000U, command, sizeof(command), 70U, 5000U) ==
          CANVIEW_DUPLICATE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_COMMAND_RESULT, &message) == 0);
    CHECK(message.payload[10U] == CANVIEW_STM_UART_RESULT_STAGE_COMPLETED);

    build_command(command, UINT64_C(0xABC1), 0xC3U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       7U, 0x604U, 5500U, command, sizeof(command), 75U, 5500U) ==
          CANVIEW_MALFORMED);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_STM_UART_REASON_TOKEN_CONFLICT);

    build_command(command, UINT64_C(0xABC1), 0xC3U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       8U, 0x602U, 6000U, command, sizeof(command), 80U, 6000U) ==
          CANVIEW_MALFORMED);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_STM_UART_REASON_TOKEN_CONFLICT);

    canview_stm_uart_stats_t stats;
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.commands_admitted == 1U && stats.execution_count == 1U &&
          stats.commands_duplicate == 2U && stats.commands_conflict == 2U &&
          stats.callback_reentry >= 4U);
    return 0;
}

static int test_capture_only_denies_control(void)
{
    static canview_stm_uart_context_t runtime;
    CHECK(establish_link(&runtime, NULL) == 0);
    uint8_t command[104U];
    build_command(command, UINT64_C(0xBAD1), 0xA1U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       4U, 0x401U, 3000U, command, sizeof(command), 40U, 3000U) ==
          CANVIEW_AUTH_FAILED);
    canview_stm_uart_pending_command_t pending;
    CHECK(canview_stm_uart_pending_peek(&runtime, &pending) == CANVIEW_INCOMPLETE);
    test_message_t message;
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_STM_UART_REASON_AUTH_FAILED);
    return 0;
}

static int test_lease_plan_and_observer_dispatch(void)
{
    static canview_stm_uart_context_t runtime;
    test_authorizer_t authorizer = {.allow = true};
    CHECK(establish_link(&runtime, &authorizer) == 0);
    runtime.authorize = allow_any_message;

    test_message_t message;
    uint8_t lease[52U];
    build_lease(lease, UINT64_C(0xA001), UINT8_C(1), 1000U, UINT64_C(0x1001), 1U, 1U, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_LEASE,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 4U, 0x410U, 4000U, lease, sizeof(lease),
                       40U, 4000U) == CANVIEW_NOT_IMPLEMENTED);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_NOT_IMPLEMENTED &&
          get_le(message.payload + 4U, 2U) == CANVIEW_STM_UART_REASON_NOT_READY);

    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_LEASE,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 4U, 0x411U, 4001U, lease, sizeof(lease),
                       40U, 4001U) == CANVIEW_DUPLICATE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_DUPLICATE &&
          get_le(message.payload + 4U, 2U) == CANVIEW_STM_UART_REASON_DUPLICATE);

    build_lease(lease, UINT64_C(0xA002), UINT8_C(3), 0U, UINT64_C(0x1001), 0U, 0U, 0U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_LEASE,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 5U, 0x412U, 5000U, lease, sizeof(lease),
                       50U, 5000U) == CANVIEW_STALE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_STALE &&
          get_le(message.payload + 4U, 2U) == CANVIEW_STM_UART_REASON_TOKEN_CONFLICT);

    runtime.lease.valid = true;
    runtime.lease.lease_id = UINT64_C(0x1001);
    runtime.lease.scope = 1U;
    runtime.lease.control_generation = 1U;
    runtime.lease.expires_at_ms = 1000U;
    memset(runtime.lease.control_tag, 0xC4, sizeof(runtime.lease.control_tag));
    canview_stm_uart_lease_t lease_snapshot;
    CHECK(canview_stm_uart_get_lease(&runtime, &lease_snapshot) == CANVIEW_OK);
    CHECK(lease_snapshot.valid && lease_snapshot.lease_id == UINT64_C(0x1001));

    build_lease(lease, UINT64_C(0xA003), UINT8_C(3), 0U, UINT64_C(0x1001), 0U, 0U, 0U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_LEASE,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 6U, 0x413U, 6000U, lease, sizeof(lease),
                       60U, 6000U) == CANVIEW_OK);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_OK);
    CHECK(canview_stm_uart_get_lease(&runtime, &lease_snapshot) == CANVIEW_OK);
    CHECK(!lease_snapshot.valid);

    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_LEASE,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 6U, 0x414U, 6001U, lease, sizeof(lease),
                       60U, 6001U) == CANVIEW_DUPLICATE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_DUPLICATE);

    uint8_t plan_chunk[36U];
    build_plan_chunk(plan_chunk, UINT64_C(0xB000), 1U, 0U, 1U, 0U, 0U, 0x123U, 0x7FFU);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CAN_OBSERVER_PLAN,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 7U, 0x420U, 7000U, plan_chunk,
                       sizeof(plan_chunk), 70U, 7000U) == CANVIEW_MALFORMED);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_STM_UART_REASON_INVALID);

    uint8_t plan_begin[32U];
    build_plan_begin(plan_begin, UINT64_C(0xB001), 1U, 0U, 1U, 1U, 10U, 1024U, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CAN_OBSERVER_PLAN,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 8U, 0x421U, 8000U, plan_begin,
                       sizeof(plan_begin), 80U, 8000U) == CANVIEW_INCOMPLETE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_INCOMPLETE);

    build_plan_chunk(plan_chunk, UINT64_C(0xB001), 1U, 0U, 1U, 0U, 0U, 0x123U, 0x7FFU);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CAN_OBSERVER_PLAN,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 9U, 0x422U, 9000U, plan_chunk,
                       sizeof(plan_chunk), 90U, 9000U) == CANVIEW_INCOMPLETE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_INCOMPLETE);

    canview_uart_observer_plan_t expected_plan = {0};
    expected_plan.revision = 1U;
    expected_plan.filter_count = 1U;
    expected_plan.max_records_per_second = 10U;
    expected_plan.max_bytes_per_second = 1024U;
    expected_plan.bus_mask = 1U;
    expected_plan.filters[0].bus_id = 0U;
    expected_plan.filters[0].can_id = 0x123U;
    expected_plan.filters[0].can_mask = 0x7FFU;
    uint8_t plan_digest[CANVIEW_UART_PLAN_DIGEST_SIZE];
    CHECK(canview_uart_plan_digest(&expected_plan, plan_digest) == CANVIEW_OK);

    uint8_t plan_commit[48U];
    build_plan_commit(plan_commit, UINT64_C(0xB001), 1U, plan_digest);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CAN_OBSERVER_PLAN,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 10U, 0x423U, 10000U, plan_commit,
                       sizeof(plan_commit), 100U, 10000U) == CANVIEW_OK);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_OK);
    canview_uart_observer_plan_t current_plan;
    CHECK(canview_uart_plan_current(&runtime.plan, &current_plan) == CANVIEW_OK);
    CHECK(current_plan.revision == expected_plan.revision &&
          current_plan.filter_count == expected_plan.filter_count &&
          current_plan.filters[0].can_id == expected_plan.filters[0].can_id);

    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CAN_OBSERVER_PLAN,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 10U, 0x424U, 10001U, plan_commit,
                       sizeof(plan_commit), 100U, 10001U) == CANVIEW_DUPLICATE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_DUPLICATE &&
          get_le(message.payload + 4U, 2U) == CANVIEW_STM_UART_REASON_DUPLICATE);

    build_plan_begin(plan_begin, UINT64_C(0xB002), 2U, 1U, 1U, 0U, 1U, 512U, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CAN_OBSERVER_PLAN,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 11U, 0x425U, 11000U, plan_begin,
                       sizeof(plan_begin), 110U, 11000U) == CANVIEW_INCOMPLETE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    uint8_t plan_abort[20U] = {0};
    plan_abort[0U] = CANVIEW_UART_PLAN_OP_ABORT;
    put_le(plan_abort + 4U, 8U, UINT64_C(0xB002));
    put_le(plan_abort + 12U, 4U, 2U);
    put_le(plan_abort + 16U, 2U, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CAN_OBSERVER_PLAN,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 12U, 0x426U, 12000U, plan_abort,
                       sizeof(plan_abort), 120U, 12000U) == CANVIEW_OK);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_OK);
    CHECK(canview_uart_plan_current(&runtime.plan, &current_plan) == CANVIEW_OK &&
          current_plan.revision == 1U);

    uint8_t capture[28U];
    build_capture_control(capture, UINT64_C(0xC001), UINT64_C(0xCAFE));
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 13U, 0x430U, 13000U, capture,
                       sizeof(capture), 130U, 13000U) == CANVIEW_NOT_IMPLEMENTED);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_STM_UART_REASON_UNSUPPORTED);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL,
                       CANVIEW_UART_FLAG_ACK_REQUIRED, 13U, 0x431U, 13001U, capture,
                       sizeof(capture), 130U, 13001U) == CANVIEW_DUPLICATE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_STM_UART_REASON_UNSUPPORTED);

    uint8_t diagnostics[16U];
    build_diagnostic_counters(diagnostics, TEST_STM_BOOT_ID, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_DIAGNOSTIC_COUNTERS, 0U, 14U, 0x440U,
                       14000U, diagnostics, sizeof(diagnostics), 140U, 14000U) == CANVIEW_OK);
    CHECK(take_outbound(&runtime, &message) == CANVIEW_INCOMPLETE);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_DIAGNOSTIC_COUNTERS, 0U, 14U, 0x441U,
                       14001U, diagnostics, sizeof(diagnostics), 140U, 14001U) ==
          CANVIEW_DUPLICATE);
    CHECK(take_outbound(&runtime, &message) == CANVIEW_INCOMPLETE);
    CHECK(authorizer.calls >= 8U);
    return 0;
}

static int test_error_response_rate_limit(void)
{
    static canview_stm_uart_context_t runtime;
    CHECK(establish_link(&runtime, NULL) == 0);

    uint8_t command[104U];
    test_message_t message;
    for (uint32_t index = 0U; index < CANVIEW_STM_UART_ERROR_RATE_LIMIT + 1U; ++index)
    {
        build_command(command, UINT64_C(0xBA00) + index, (uint8_t)(0xA0U + index),
                      TEST_SYNC_GENERATION, 1U);
        CHECK(feed_inbound(
                  &runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                  (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                  10U + index, 0xA10U + index, 1000U + index, command, sizeof(command), 100U,
                  100000U + index) == CANVIEW_AUTH_FAILED);
        if (index < CANVIEW_STM_UART_ERROR_RATE_LIMIT)
        {
            CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);
        }
        else
        {
            CHECK(take_outbound(&runtime, &message) == CANVIEW_INCOMPLETE);
        }
    }

    canview_stm_uart_stats_t stats;
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.error_responses_rate_limited == 1U);

    CHECK(canview_stm_uart_tick(&runtime, 1100U, 1100000U) == CANVIEW_OK);
    CHECK(drain_outbound(&runtime) == 0);

    build_command(command, UINT64_C(0xBA20), 0xB0U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       30U, 0xA30U, 2000U, command, sizeof(command), 1100U, 1100000U) ==
          CANVIEW_TIMEOUT);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.error_responses_rate_limited == 1U);
    return 0;
}

static int test_cache_full_busy_without_pending(void)
{
    static canview_stm_uart_context_t runtime;
    test_authorizer_t authorizer = {.allow = true};
    CHECK(establish_link(&runtime, &authorizer) == 0);
    CHECK(establish_time_mapping(&runtime, 4U) == 0);
    for (uint32_t index = 0U; index < CANVIEW_UART_COMMAND_CACHE_CAPACITY; ++index)
    {
        canview_uart_command_key_t key = {0};
        key.origin_device_id = TEST_COMMAND_ORIGIN_ID;
        key.origin_boot_id = TEST_COMMAND_ORIGIN_BOOT_ID;
        key.wireless_session_id = TEST_COMMAND_SESSION_ID;
        key.control_generation = TEST_COMMAND_GENERATION;
        key.request_token = UINT64_C(0x10000) + index;
        key.command_id = TEST_COMMAND_ID;
        memset(key.canonical_argument_digest, (int)(index + 1U),
               sizeof(key.canonical_argument_digest));
        canview_uart_command_handle_t handle;
        CHECK(canview_uart_command_cache_admit(
                  &runtime.command_cache, &key, TEST_COMMAND_TTL_MS, 100U, &handle) ==
              CANVIEW_OK);
    }

    uint8_t command[104U];
    build_command(command, UINT64_C(0xFFFF), 0xE1U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       6U, 0x606U, 100000U, command, sizeof(command), 100U, 100000U) ==
          CANVIEW_RESOURCE_BUSY);
    canview_stm_uart_pending_command_t pending;
    CHECK(canview_stm_uart_pending_peek(&runtime, &pending) == CANVIEW_INCOMPLETE);
    test_message_t message;
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_RESOURCE_BUSY &&
          get_le(message.payload + 4U, 2U) == CANVIEW_STM_UART_REASON_CACHE_FULL);
    canview_stm_uart_stats_t stats;
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.commands_cache_busy == 1U && stats.commands_admitted == 0U);
    return 0;
}

static int test_queue_priority_coalesce_and_drop(void)
{
    static canview_stm_uart_context_t runtime;
    CHECK(init_runtime(&runtime, NULL) == 0);
    CHECK(drain_outbound(&runtime) == 0);

    uint8_t ack[20U] = {0};
    uint8_t error[20U] = {0};
    uint8_t heartbeat[48U];
    uint8_t snapshot[48U] = {0};
    uint8_t batch[12U] = {0};
    put_le(ack + 8U, 8U, 1U);
    put_le(error, 2U, CANVIEW_OK);
    build_heartbeat(heartbeat, TEST_STM_BOOT_ID, 1U);
    put_le(heartbeat + 8U, 8U, 111U);
    put_le(snapshot, 8U, TEST_STM_BOOT_ID);
    put_le(snapshot + 8U, 4U, 1U);
    put_le(snapshot + 12U, 4U, 1U);
    put_le(snapshot + 16U, 4U, 1U);
    put_le(snapshot + 20U, 2U, 1U);
    snapshot[23U] = 0U;
    put_le(snapshot + 32U, 8U, 1U);

    CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_RAW,
                                   CANVIEW_UART_MSG_CAN_RX_BATCH, 0U, 0U, 1U, batch,
                                   sizeof(batch)) == CANVIEW_OK);
    CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_STATE,
                                   CANVIEW_UART_MSG_SAFETY_SNAPSHOT, CANVIEW_UART_FLAG_SNAPSHOT,
                                   0U, 2U, snapshot, sizeof(snapshot)) == CANVIEW_OK);
    CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_P1, CANVIEW_UART_MSG_ERROR,
                                   (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_ERROR),
                                   0U, 3U, error, sizeof(error)) == CANVIEW_OK);
    CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_P0, CANVIEW_UART_MSG_ACK,
                                   CANVIEW_UART_FLAG_RESPONSE, 0U, 4U, ack, sizeof(ack)) ==
          CANVIEW_OK);
    test_message_t message;
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ACK, &message) == 0);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_SAFETY_SNAPSHOT, &message) == 0);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_CAN_RX_BATCH, &message) == 0);

    put_le(heartbeat + 8U, 8U, 222U);
    CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_STATE,
                                   CANVIEW_UART_MSG_HEARTBEAT, 0U, 0U, 5U, heartbeat,
                                   sizeof(heartbeat)) == CANVIEW_OK);
    put_le(heartbeat + 8U, 8U, 333U);
    CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_STATE,
                                   CANVIEW_UART_MSG_HEARTBEAT, 0U, 0U, 6U, heartbeat,
                                   sizeof(heartbeat)) == CANVIEW_OK);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_HEARTBEAT, &message) == 0);
    CHECK(get_le(message.payload + 8U, 8U) == 333U);

    for (uint32_t index = 0U; index < CANVIEW_STM_UART_TX_QUEUE_CAPACITY + 1U; ++index)
    {
        put_le(batch, 8U, index + 1U);
        CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_RAW,
                                       CANVIEW_UART_MSG_CAN_RX_BATCH, 0U, index + 1U, index + 7U,
                                       batch, sizeof(batch)) == CANVIEW_OK);
    }
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_CAN_RX_BATCH, &message) == 0);
    CHECK(get_le(message.payload, 8U) == 2U);
    CHECK(drain_outbound(&runtime) == 0);

    for (size_t index = 0U; index < CANVIEW_STM_UART_TX_QUEUE_CAPACITY; ++index)
    {
        put_le(ack + 8U, 8U, 100U + index);
        CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_P0,
                                       CANVIEW_UART_MSG_ACK, CANVIEW_UART_FLAG_RESPONSE,
                                       (uint32_t)index, index + 20U, ack, sizeof(ack)) ==
              CANVIEW_OK);
    }
    CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_P0, CANVIEW_UART_MSG_ACK,
                                   CANVIEW_UART_FLAG_RESPONSE, 99U, 99U, ack, sizeof(ack)) ==
          CANVIEW_RESOURCE_BUSY);
    canview_stm_uart_stats_t stats;
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.tx_raw_dropped == 1U && stats.tx_queue_full == 1U &&
          stats.tx_safety_inhibited == 1U && runtime.safety_inhibited);
    return 0;
}

static int test_fault_resync_and_epoch_invalidation(void)
{
    static canview_stm_uart_context_t runtime;
    test_authorizer_t authorizer = {.allow = true};
    CHECK(establish_link(&runtime, &authorizer) == 0);
    CHECK(establish_time_mapping(&runtime, 4U) == 0);

    uint8_t heartbeat[48U];
    build_heartbeat(heartbeat, TEST_ESP_BOOT_ID,
                    CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION);
    uint8_t scratch[CANVIEW_UART_MAX_FRAME_SIZE];
    uint8_t serial[CANVIEW_UART_MAX_SERIAL_SIZE];
    size_t serial_size = 0U;
    CHECK(canview_uart_message_encode(
              CANVIEW_UART_MSG_HEARTBEAT, 0U, 6U, 0U, 6000U, heartbeat, sizeof(heartbeat),
              CANVIEW_UART_ENDPOINT_STM32, CANVIEW_UART_FLOW_INBOUND, scratch, sizeof(scratch),
              serial, sizeof(serial), &serial_size) == CANVIEW_OK);
    CHECK(serial_size > 3U);
    serial[serial_size / 2U] ^= 1U;
    canview_status_t status = CANVIEW_INCOMPLETE;
    for (size_t index = 0U; index < serial_size; ++index)
    {
        status = canview_stm_uart_ingest_byte(&runtime, serial[index], 60U, 6000U);
    }
    CHECK(status != CANVIEW_OK);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_HEARTBEAT, 0U, 7U, 0U, 7000U, heartbeat,
                       sizeof(heartbeat), 70U, 7000U) == CANVIEW_OK);
    test_message_t message;
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_SAFETY_SNAPSHOT, &message) == 0);

    uint8_t command[104U];
    build_command(command, UINT64_C(0xEEE1), 0xD1U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       8U, 0x808U, 8000U, command, sizeof(command), 80U, 8000U) == CANVIEW_OK);
    canview_stm_uart_pending_command_t pending;
    CHECK(canview_stm_uart_pending_peek(&runtime, &pending) == CANVIEW_OK);
    CHECK(drain_outbound(&runtime) == 0);

    uint8_t new_hello[72U];
    build_hello(new_hello, TEST_ESP_BOOT_ID + 1U, TEST_ESP_DEVICE_ID);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_LINK_HELLO, 0U, 9U, 0U, 9000U, new_hello,
                       sizeof(new_hello), 90U, 9000U) == CANVIEW_OK);
    CHECK(canview_stm_uart_pending_peek(&runtime, &pending) == CANVIEW_INCOMPLETE);
    canview_uart_link_t link;
    CHECK(canview_stm_uart_get_link(&runtime, &link) == CANVIEW_OK);
    CHECK(link.peer_boot_id == TEST_ESP_BOOT_ID + 1U && link.hello_complete &&
          !link.hello_ack_complete);
    canview_stm_uart_time_mapping_t mapping;
    CHECK(canview_stm_uart_get_time_mapping(&runtime, &mapping) == CANVIEW_OK);
    CHECK(!mapping.valid);
    canview_stm_uart_stats_t stats;
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.pending_cancelled >= 1U);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_LINK_HELLO, &message) == 0);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_LINK_HELLO_ACK, &message) == 0);
    return 0;
}

static int test_cts_offline_cancels_pending(void)
{
    static canview_stm_uart_context_t runtime;
    test_authorizer_t authorizer = {.allow = true};
    CHECK(establish_link(&runtime, &authorizer) == 0);
    CHECK(establish_time_mapping(&runtime, 4U) == 0);
    uint8_t command[104U];
    build_command(command, UINT64_C(0xCCC1), 0xA1U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       6U, 0x606U, 6000U, command, sizeof(command), 60U, 6000U) == CANVIEW_OK);
    canview_stm_uart_pending_command_t pending;
    CHECK(canview_stm_uart_pending_peek(&runtime, &pending) == CANVIEW_OK);
    CHECK(drain_outbound(&runtime) == 0);
    CHECK(canview_stm_uart_set_cts_blocked(&runtime, true, 100U) == CANVIEW_OK);
    CHECK(canview_stm_uart_tick(&runtime, 1101U, 1101000U) == CANVIEW_OK);
    CHECK(canview_stm_uart_pending_peek(&runtime, &pending) == CANVIEW_INCOMPLETE);
    canview_uart_link_t link;
    CHECK(canview_stm_uart_get_link(&runtime, &link) == CANVIEW_OK);
    CHECK(!link.hello_complete && link.state == CANVIEW_UART_LINK_OFFLINE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_LINK_HELLO, &(test_message_t){0}) == 0);
    return 0;
}

static int test_time_mapping_expiry_expires_pending(void)
{
    static canview_stm_uart_context_t runtime;
    test_authorizer_t authorizer = {.allow = true};
    CHECK(establish_link(&runtime, &authorizer) == 0);
    CHECK(establish_time_mapping(&runtime, 4U) == 0);

    uint8_t command[104U];
    build_command(command, UINT64_C(0xDDD1), 0xA1U, TEST_SYNC_GENERATION, 1U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_COMMAND_REQUEST,
                       (uint8_t)(CANVIEW_UART_FLAG_ACK_REQUIRED | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       6U, 0x607U, 6000U, command, sizeof(command), 60U, 6000U) == CANVIEW_OK);
    canview_stm_uart_pending_command_t pending;
    CHECK(canview_stm_uart_pending_peek(&runtime, &pending) == CANVIEW_OK);
    CHECK(drain_outbound(&runtime) == 0);

    uint8_t heartbeat[48U];
    build_heartbeat(heartbeat, TEST_ESP_BOOT_ID,
                    CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION);
    test_message_t message;
    uint32_t heartbeat_sequence = 7U;
    for (uint64_t heartbeat_ms = 900U; heartbeat_ms < 30000U; heartbeat_ms += 900U)
    {
        CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_HEARTBEAT, 0U, heartbeat_sequence, 0U,
                           heartbeat_ms * UINT64_C(1000), heartbeat, sizeof(heartbeat),
                           heartbeat_ms, heartbeat_ms * UINT64_C(1000)) == CANVIEW_OK);
        ++heartbeat_sequence;
        CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_SAFETY_SNAPSHOT, &message) == 0);
    }

    CHECK(canview_stm_uart_tick(&runtime, 30050U, 30050000U) == CANVIEW_OK);
    CHECK(canview_stm_uart_pending_peek(&runtime, &pending) == CANVIEW_INCOMPLETE);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_COMMAND_RESULT, &message) == 0);
    CHECK(message.payload[10U] == CANVIEW_STM_UART_RESULT_STAGE_EXPIRED &&
          get_le(message.payload + 12U, 2U) == CANVIEW_STM_UART_REASON_TIMEOUT);
    canview_stm_uart_time_mapping_t mapping;
    CHECK(canview_stm_uart_get_time_mapping(&runtime, &mapping) == CANVIEW_OK);
    CHECK(!mapping.valid);
    canview_stm_uart_stats_t stats;
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.commands_expired >= 1U);
    return 0;
}

static int test_public_reentry_guards(void)
{
    static canview_stm_uart_context_t runtime;
    CHECK(init_runtime(&runtime, NULL) == 0);
    CHECK(drain_outbound(&runtime) == 0);
    CHECK(canview_stm_uart_set_reset_hook(NULL, NULL, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_uart_set_reset_hook(&runtime, NULL, &runtime) ==
          CANVIEW_INVALID_ARGUMENT);
    uint8_t payload[12U] = {0};
    const uint8_t *data = NULL;
    size_t size = 0U;
    uint32_t sequence = 0U;
    runtime.servicing = true;
    test_reset_hook_t hook = {.status = CANVIEW_OK};
    CHECK(canview_stm_uart_set_reset_hook(&runtime, test_reset_hook, &hook) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(canview_stm_uart_reset(&runtime, 1U, 1000U) == CANVIEW_RESOURCE_BUSY);
    CHECK(canview_stm_uart_set_cts_blocked(&runtime, false, 1U) == CANVIEW_RESOURCE_BUSY);
    CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_RAW,
                                   CANVIEW_UART_MSG_CAN_RX_BATCH, 0U, 0U, 1000U, payload,
                                   sizeof(payload)) == CANVIEW_RESOURCE_BUSY);
    CHECK(canview_stm_uart_tx_begin(&runtime, &data, &size, &sequence) == CANVIEW_RESOURCE_BUSY);
    CHECK(data == NULL && size == 0U && sequence == 0U);
    CHECK(canview_stm_uart_pending_complete(&runtime, NULL,
                                            CANVIEW_STM_UART_RESULT_STAGE_COMPLETED, 0U, 1U, 1U,
                                            1U, 1U, 1000U) == CANVIEW_RESOURCE_BUSY);
    runtime.servicing = false;
    canview_stm_uart_stats_t stats;
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.callback_reentry >= 6U);
    return 0;
}

static int test_tx_lifecycle_and_reset(void)
{
    static canview_stm_uart_context_t runtime;
    CHECK(init_runtime(&runtime, NULL) == 0);
    CHECK(drain_outbound(&runtime) == 0);

    uint8_t ack[20U] = {0};
    put_le(ack + 8U, 8U, UINT64_C(0x7001));
    CHECK(canview_stm_uart_tx_finish(&runtime, CANVIEW_OK, 1U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_P0, CANVIEW_UART_MSG_ACK,
                                   CANVIEW_UART_FLAG_RESPONSE, 1U, 1000U, ack, sizeof(ack)) ==
          CANVIEW_OK);

    const uint8_t *data = NULL;
    size_t size = 0U;
    uint32_t sequence = 0U;
    CHECK(canview_stm_uart_tx_begin(&runtime, &data, &size, &sequence) == CANVIEW_OK);
    CHECK(data != NULL && size != 0U && sequence != 0U);
    const uint8_t *first_data = data;
    const size_t first_size = size;
    const uint32_t first_sequence = sequence;
    CHECK(canview_stm_uart_tx_begin(&runtime, &data, &size, &sequence) == CANVIEW_OK);
    CHECK(data == first_data && size == first_size && sequence == first_sequence);
    CHECK(canview_stm_uart_tx_finish(&runtime, CANVIEW_RESOURCE_BUSY, 10U) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(canview_stm_uart_tx_begin(&runtime, &data, &size, &sequence) == CANVIEW_OK);
    CHECK(data == first_data && size == first_size && sequence == first_sequence);
    CHECK(canview_stm_uart_tx_finish(&runtime, CANVIEW_MALFORMED, 11U) == CANVIEW_MALFORMED);
    CHECK(canview_stm_uart_tx_begin(&runtime, &data, &size, &sequence) == CANVIEW_INCOMPLETE);

    canview_stm_uart_stats_t stats;
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.tx_dropped == 1U && stats.tx_completed >= 1U &&
          stats.tx_safety_inhibited == 1U && runtime.safety_inhibited);

    CHECK(canview_stm_uart_enqueue(&runtime, CANVIEW_STM_UART_TX_P0, CANVIEW_UART_MSG_ACK,
                                   CANVIEW_UART_FLAG_RESPONSE, 2U, 2000U, ack, sizeof(ack)) ==
          CANVIEW_OK);
    CHECK(canview_stm_uart_tx_begin(&runtime, &data, &size, &sequence) == CANVIEW_OK);
    CHECK(canview_stm_uart_reset(&runtime, 20U, 20000U) == CANVIEW_OK);
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.link_resets == 1U && stats.tx_dropped == 2U &&
          stats.tx_safety_inhibited == 1U);
    canview_stm_uart_lease_t lease;
    CHECK(canview_stm_uart_get_lease(&runtime, &lease) == CANVIEW_OK && !lease.valid);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_LINK_HELLO, &(test_message_t){0}) == 0);
    return 0;
}

static int test_time_sync_rejection_and_argument_guards(void)
{
    canview_stm_uart_context_t uninitialized = {0};
    canview_stm_uart_config_t invalid_config = {0};
    CHECK(canview_stm_uart_init(NULL, &invalid_config, 0U, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_uart_init(&uninitialized, &invalid_config, 0U, 0U) ==
          CANVIEW_INVALID_ARGUMENT);

    static canview_stm_uart_context_t runtime;
    CHECK(init_runtime(&runtime, NULL) == 0);
    CHECK(drain_outbound(&runtime) == 0);
    uint8_t request[80U];
    build_time_sync(request, CANVIEW_UART_TIME_SYNC_REQUEST, UINT64_C(0x8101),
                    TEST_CONTROLLER_BOOT_ID, 0U, TEST_SYNC_GENERATION, 1000U, 0U, 0U, 0U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       1U, 0x810U, 1000U, request, sizeof(request), 10U, 1000U) ==
          CANVIEW_TIMEOUT);
    test_message_t message;
    CHECK(take_outbound(&runtime, &message) == CANVIEW_INCOMPLETE);

    static canview_stm_uart_context_t bad_hello_runtime;
    CHECK(init_runtime(&bad_hello_runtime, NULL) == 0);
    CHECK(drain_outbound(&bad_hello_runtime) == 0);
    uint8_t bad_hello[72U];
    build_hello(bad_hello, TEST_ESP_BOOT_ID, TEST_ESP_DEVICE_ID);
    bad_hello[8U] = 2U;
    bad_hello[10U] = 2U;
    CHECK(feed_inbound(&bad_hello_runtime, CANVIEW_UART_MSG_LINK_HELLO, 0U, 1U, 0U, 1000U,
                       bad_hello, sizeof(bad_hello), 10U, 1000U) == CANVIEW_OK);
    CHECK(expect_outbound(&bad_hello_runtime, CANVIEW_UART_MSG_ERROR, &message) == 0);
    CHECK(get_le(message.payload + 6U, 2U) == CANVIEW_STM_UART_REASON_UNSUPPORTED);

    CHECK(establish_link(&runtime, NULL) == 0);
    uint8_t response[80U];
    build_time_sync(response, CANVIEW_UART_TIME_SYNC_RESPONSE, UINT64_C(0x8102),
                    TEST_CONTROLLER_BOOT_ID, TEST_STM_BOOT_ID, TEST_SYNC_GENERATION, 1000U,
                    2000U, 2000U, 0U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       4U, 0x811U, 4000U, response, sizeof(response), 40U, 4000U) ==
          CANVIEW_UNSUPPORTED_MESSAGE);
    CHECK(take_outbound(&runtime, &message) == CANVIEW_INCOMPLETE);

    build_time_sync(request, CANVIEW_UART_TIME_SYNC_REQUEST, UINT64_C(0x8103),
                    TEST_CONTROLLER_BOOT_ID, 0U, TEST_SYNC_GENERATION, 5000U, 0U, 0U, 0U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       5U, 0x812U, 5000U, request, sizeof(request), 50U, 5000U) ==
          CANVIEW_OK);
    CHECK(expect_outbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC, &message) == 0);

    build_time_sync(request, CANVIEW_UART_TIME_SYNC_REQUEST, UINT64_C(0x8104),
                    TEST_CONTROLLER_BOOT_ID, 0U, TEST_SYNC_GENERATION, 6000U, 0U, 0U, 0U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       6U, 0x813U, 6000U, request, sizeof(request), 51U, 6000U) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(take_outbound(&runtime, &message) == CANVIEW_INCOMPLETE);

    uint8_t commit[80U];
    build_time_sync(commit, CANVIEW_UART_TIME_SYNC_COMMIT, UINT64_C(0x8104),
                    TEST_CONTROLLER_BOOT_ID, TEST_STM_BOOT_ID, TEST_SYNC_GENERATION, 6000U,
                    7000U, 7000U, 8000U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       7U, 0x814U, 8000U, commit, sizeof(commit), 52U, 8000U) == CANVIEW_STALE);

    build_time_sync(commit, CANVIEW_UART_TIME_SYNC_COMMIT, UINT64_C(0x8103),
                    TEST_CONTROLLER_BOOT_ID, TEST_STM_BOOT_ID, TEST_SYNC_GENERATION, 5000U,
                    5000U, 5000U, 200000U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       8U, 0x815U, 200000U, commit, sizeof(commit), 53U, 200000U) ==
          CANVIEW_TIMEOUT);

    build_time_sync(commit, CANVIEW_UART_TIME_SYNC_COMMIT, UINT64_C(0x8103),
                    TEST_CONTROLLER_BOOT_ID, TEST_STM_BOOT_ID, TEST_SYNC_GENERATION, 5000U,
                    5000U, 5000U, 7000U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
                       (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
                       9U, 0x816U, 7000U, commit, sizeof(commit), 54U, 7000U) == CANVIEW_OK);
    CHECK(canview_stm_uart_get_time_mapping(&runtime, NULL) == CANVIEW_INVALID_ARGUMENT);

    uint8_t heartbeat[48U];
    build_heartbeat(heartbeat, TEST_ESP_BOOT_ID, 999U);
    CHECK(feed_inbound(&runtime, CANVIEW_UART_MSG_HEARTBEAT, 0U, 10U, 0U, 10000U, heartbeat,
                       sizeof(heartbeat), 60U, 10000U) == CANVIEW_OK);
    CHECK(take_outbound(&runtime, &message) == CANVIEW_INCOMPLETE);

    canview_stm_uart_stats_t stats;
    canview_stm_uart_time_mapping_t mapping;
    canview_uart_link_t link;
    CHECK(canview_stm_uart_get_stats(&runtime, &stats) == CANVIEW_OK);
    CHECK(stats.time_sync_rejected >= 4U);
    CHECK(canview_stm_uart_get_stats(NULL, &stats) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_uart_get_stats(&runtime, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_uart_get_link(&runtime, &link) == CANVIEW_OK);
    CHECK(canview_stm_uart_get_link(NULL, &link) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_uart_get_link(&runtime, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_uart_get_time_mapping(&runtime, &mapping) == CANVIEW_OK);
    CHECK(canview_stm_uart_get_lease(&runtime, NULL) == CANVIEW_INVALID_ARGUMENT);
    return 0;
}

int main(void)
{
    CHECK(test_handshake_and_time_sync() == 0);
    CHECK(test_time_sync_cross_clock_and_pending_timeout() == 0);
    CHECK(test_time_sync_commit_expiry_before_tick() == 0);
    CHECK(test_reset_hook_and_safety_inhibit() == 0);
    CHECK(test_command_idempotency_and_reentry() == 0);
    CHECK(test_capture_only_denies_control() == 0);
    CHECK(test_lease_plan_and_observer_dispatch() == 0);
    CHECK(test_error_response_rate_limit() == 0);
    CHECK(test_cache_full_busy_without_pending() == 0);
    CHECK(test_queue_priority_coalesce_and_drop() == 0);
    CHECK(test_fault_resync_and_epoch_invalidation() == 0);
    CHECK(test_cts_offline_cancels_pending() == 0);
    CHECK(test_time_mapping_expiry_expires_pending() == 0);
    CHECK(test_public_reentry_guards() == 0);
    CHECK(test_tx_lifecycle_and_reset() == 0);
    CHECK(test_time_sync_rejection_and_argument_guards() == 0);
    (void)puts("STM32 UART link tests passed");
    return EXIT_SUCCESS;
}
