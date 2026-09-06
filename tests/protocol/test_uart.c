/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_uart.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expression)                                                                        \
    do                                                                                           \
    {                                                                                            \
        if (!(expression))                                                                       \
        {                                                                                        \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression);              \
            return 1;                                                                            \
        }                                                                                        \
    } while (0)

static canview_status_t test_plan_apply_default(canview_uart_plan_context_t *context,
                                                const uint8_t *payload, size_t payload_size)
{
    return canview_uart_plan_apply(context, payload, payload_size, 0U);
}

#define TEST_PLAN_APPLY(context, payload, payload_size)                                         \
    test_plan_apply_default((context), (payload), (payload_size))

static void put_le(uint8_t *bytes, size_t width, uint64_t value)
{
    for (size_t index = 0U; index < width; ++index)
    {
        bytes[index] = (uint8_t)(value >> (index * 8U));
    }
}

static canview_uart_endpoint_t test_endpoint_for_message(uint8_t message_type)
{
    const canview_uart_message_policy_t *policy = NULL;
    if (canview_uart_message_policy(message_type, &policy) != CANVIEW_OK || policy == NULL)
    {
        return CANVIEW_UART_ENDPOINT_ESP32;
    }
    return policy->direction == CANVIEW_UART_DIRECTION_STM_TO_ESP
               ? CANVIEW_UART_ENDPOINT_STM32
               : CANVIEW_UART_ENDPOINT_ESP32;
}

static canview_status_t test_message_validate(const canview_wire_view_t *wire,
                                              canview_uart_message_view_t *view)
{
    const uint8_t message_type = wire == NULL ? 0U : wire->header.message_type;
    return canview_uart_message_validate_for_endpoint(
        wire, test_endpoint_for_message(message_type), CANVIEW_UART_FLOW_OUTBOUND, view);
}

static canview_status_t test_message_encode(uint8_t message_type, uint8_t flags,
                                            uint32_t sequence, uint32_t correlation_id,
                                            uint64_t sender_time_us, const uint8_t *payload,
                                            size_t payload_size, uint8_t *scratch,
                                            size_t scratch_size, uint8_t *out, size_t capacity,
                                            size_t *written)
{
    return canview_uart_message_encode(
        message_type, flags, sequence, correlation_id, sender_time_us, payload, payload_size,
        test_endpoint_for_message(message_type), CANVIEW_UART_FLOW_OUTBOUND, scratch, scratch_size,
        out, capacity, written);
}

static canview_status_t test_codec_reset(canview_uart_codec_t *codec)
{
    return canview_uart_codec_reset(codec, CANVIEW_UART_ENDPOINT_ESP32,
                                    CANVIEW_UART_FLOW_OUTBOUND);
}

static canview_status_t test_codec_reset_for_message(canview_uart_codec_t *codec,
                                                      uint8_t message_type)
{
    return canview_uart_codec_reset(codec, test_endpoint_for_message(message_type),
                                    CANVIEW_UART_FLOW_OUTBOUND);
}

static canview_status_t test_command_cache_admit_default(
    canview_uart_command_cache_t *cache, const canview_uart_command_key_t *key, uint64_t now_ms,
    canview_uart_command_handle_t *handle)
{
    return canview_uart_command_cache_admit(cache, key, CANVIEW_UART_COMMAND_TTL_MIN_MS, now_ms,
                                            handle);
}

static canview_status_t test_command_cache_mark_ack_default(
    canview_uart_command_cache_t *cache, const canview_uart_command_handle_t *handle)
{
    return canview_uart_command_cache_mark_ack(cache, handle, 0U);
}

static canview_status_t test_command_cache_admit_with_ttl(
    canview_uart_command_cache_t *cache, const canview_uart_command_key_t *key,
    uint16_t request_ttl_ms, uint64_t now_ms, canview_uart_command_handle_t *handle)
{
    return canview_uart_command_cache_admit(cache, key, request_ttl_ms, now_ms, handle);
}

static canview_status_t test_command_cache_mark_ack_at(
    canview_uart_command_cache_t *cache, const canview_uart_command_handle_t *handle,
    uint64_t now_ms)
{
    return canview_uart_command_cache_mark_ack(cache, handle, now_ms);
}

static canview_status_t test_link_note_safety_snapshot_default(canview_uart_link_t *link,
                                                               uint64_t peer_boot_id,
                                                               uint64_t now_ms)
{
    return canview_uart_link_note_safety_snapshot(link, peer_boot_id, 1U, now_ms);
}

static canview_status_t test_link_note_safety_snapshot_with_revision(
    canview_uart_link_t *link, uint64_t peer_boot_id, uint32_t revision, uint64_t now_ms)
{
    return canview_uart_link_note_safety_snapshot(link, peer_boot_id, revision, now_ms);
}

static canview_status_t test_link_note_heartbeat_default(canview_uart_link_t *link,
                                                         uint64_t peer_boot_id, uint64_t now_ms,
                                                         bool *boot_changed)
{
    const uint32_t revision = link != NULL && link->safety_snapshot_valid
                                  ? link->safety_revision
                                  : 1U;
    return canview_uart_link_note_heartbeat(link, peer_boot_id, revision, now_ms, boot_changed);
}

static canview_status_t test_link_note_heartbeat_with_revision(
    canview_uart_link_t *link, uint64_t peer_boot_id, uint32_t revision, uint64_t now_ms,
    bool *boot_changed)
{
    return canview_uart_link_note_heartbeat(link, peer_boot_id, revision, now_ms, boot_changed);
}

static canview_status_t test_session_note_heartbeat_default(
    canview_uart_link_t *link, canview_uart_plan_context_t *plan,
    canview_uart_command_cache_t *cache, canview_uart_replay_context_t *replay,
    uint64_t peer_boot_id, uint64_t now_ms, bool *boot_changed)
{
    const uint32_t revision = link != NULL && link->safety_snapshot_valid
                                  ? link->safety_revision
                                  : 1U;
    return canview_uart_session_note_heartbeat(link, plan, cache, replay, peer_boot_id,
                                               revision, now_ms, boot_changed);
}

#define canview_uart_message_validate test_message_validate
#define canview_uart_message_encode test_message_encode
#define canview_uart_codec_reset test_codec_reset
#define canview_uart_command_cache_admit test_command_cache_admit_default
#define canview_uart_command_cache_mark_ack test_command_cache_mark_ack_default
#define canview_uart_link_note_safety_snapshot test_link_note_safety_snapshot_default
#define canview_uart_link_note_heartbeat test_link_note_heartbeat_default
#define canview_uart_session_note_heartbeat test_session_note_heartbeat_default

static size_t fill_valid_payload(uint8_t message_type, uint8_t *payload, size_t capacity,
                                 bool maximum)
{
    const canview_uart_message_policy_t *policy = NULL;
    if (canview_uart_message_policy(message_type, &policy) != CANVIEW_OK ||
        policy == NULL || !policy->supported)
    {
        return 0U;
    }
    const size_t size = maximum ? policy->max_payload : policy->min_payload;
    if (size > capacity)
    {
        return 0U;
    }
    memset(payload, 0, size);
    switch (message_type)
    {
    case CANVIEW_UART_MSG_LINK_HELLO:
        put_le(payload, 8U, 1U);
        payload[8] = CANVIEW_UART_PROTOCOL_MAJOR;
        payload[9] = CANVIEW_UART_PROTOCOL_MINOR;
        payload[10] = CANVIEW_UART_PROTOCOL_MAJOR;
        payload[11] = CANVIEW_UART_PROTOCOL_MINOR;
        put_le(payload + 28U, 2U, CANVIEW_UART_MAX_FRAME_SIZE);
        break;
    case CANVIEW_UART_MSG_LINK_HELLO_ACK:
        put_le(payload, 8U, 1U);
        put_le(payload + 8U, 8U, 2U);
        payload[16] = CANVIEW_UART_PROTOCOL_MAJOR;
        payload[17] = CANVIEW_UART_PROTOCOL_MINOR;
        put_le(payload + 28U, 2U, CANVIEW_UART_MAX_FRAME_SIZE);
        break;
    case CANVIEW_UART_MSG_HEARTBEAT:
        put_le(payload, 8U, 1U);
        break;
    case CANVIEW_UART_MSG_CAN_RX_BATCH:
        payload[8] = maximum ? CANVIEW_UART_CAN_BATCH_MAX_RECORDS : 0U;
        for (size_t index = 0U; index < payload[8]; ++index)
        {
            memset(payload + 12U + index * 16U, 0, 16U);
        }
        break;
    case CANVIEW_UART_MSG_CAN_BUS_STATUS:
        break;
    case CANVIEW_UART_MSG_SAFETY_SNAPSHOT:
        put_le(payload, 8U, 1U);
        break;
    case CANVIEW_UART_MSG_CAN_TX_AUDIT:
        break;
    case CANVIEW_UART_MSG_CAN_ID_STATS:
        payload[4] = maximum ? CANVIEW_UART_CAN_ID_STATS_MAX_RECORDS : 0U;
        payload[6] = 1U;
        break;
    case CANVIEW_UART_MSG_CAN_OBSERVER_PLAN:
        if (maximum)
        {
            payload[0] = CANVIEW_UART_PLAN_OP_CHUNK;
            put_le(payload + 4U, 8U, 1U);
            put_le(payload + 12U, 4U, 1U);
            put_le(payload + 16U, 2U, 0U);
            put_le(payload + 18U, 2U, 1U);
            payload[20] = CANVIEW_UART_PLAN_CHUNK_MAX_FILTERS;
            for (size_t index = 0U; index < payload[20]; ++index)
            {
                uint8_t *record = payload + 24U + index * 12U;
                record[0] = 0U;
                record[1] = 0U;
            }
        }
        else
        {
            payload[0] = CANVIEW_UART_PLAN_OP_ABORT;
            put_le(payload + 4U, 8U, 1U);
            put_le(payload + 12U, 4U, 1U);
        }
        break;
    case CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL:
        put_le(payload, 8U, 1U);
        put_le(payload + 8U, 8U, 1U);
        payload[16] = CANVIEW_UART_CAPTURE_ARM;
        break;
    case CANVIEW_UART_MSG_CAN_CAPTURE_STATUS:
        put_le(payload, 8U, 1U);
        put_le(payload + 8U, 8U, 1U);
        break;
    case CANVIEW_UART_MSG_CAN_EVENT_MARKER:
        put_le(payload + 8U, 4U, 1U);
        break;
    case CANVIEW_UART_MSG_COMMAND_REQUEST:
        put_le(payload, 8U, 1U);
        put_le(payload + 10U, 2U, 500U);
        put_le(payload + 12U, 8U, 2U);
        put_le(payload + 20U, 8U, 3U);
        put_le(payload + 28U, 4U, 4U);
        put_le(payload + 32U, 4U, 5U);
        put_le(payload + 52U, 2U, maximum ? 136U : 0U);
        memset(payload + 56U, 0xA1, CANVIEW_UART_COMMAND_DIGEST_SIZE);
        memset(payload + 88U, 0xB2, CANVIEW_UART_CONTROL_TAG_SIZE);
        break;
    case CANVIEW_UART_MSG_COMMAND_RESULT:
        put_le(payload, 8U, 1U);
        break;
    case CANVIEW_UART_MSG_CONTROL_LEASE:
        put_le(payload, 8U, 1U);
        break;
    case CANVIEW_UART_MSG_CONFIG_SET:
        payload[2] = maximum ? CANVIEW_UART_CONFIG_MAX_RECORDS : 0U;
        break;
    case CANVIEW_UART_MSG_DIAGNOSTIC_COUNTERS:
        payload[12] = maximum ? CANVIEW_UART_DIAGNOSTIC_COUNTER_MAX_RECORDS : 0U;
        break;
    default:
        break;
    }
    return size;
}

static uint8_t flags_for(uint8_t message_type)
{
    const canview_uart_message_policy_t *policy = NULL;
    (void)canview_uart_message_policy(message_type, &policy);
    return policy == NULL ? 0U : policy->required_flags;
}

static int expect_fixed_malformed(uint8_t message_type, size_t offset, size_t width,
                                  uint64_t value)
{
    uint8_t payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    const size_t payload_size = fill_valid_payload(message_type, payload, sizeof(payload), false);
    CHECK(payload_size > offset && width <= payload_size - offset);
    put_le(payload + offset, width, value);
    canview_wire_view_t wire = {0};
    wire.header.message_type = message_type;
    wire.header.flags = flags_for(message_type);
    wire.payload = payload;
    wire.payload_size = payload_size;
    canview_uart_message_view_t view;
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_MALFORMED);
    return 0;
}

static int test_message_matrix(void)
{
    uint8_t payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    uint8_t scratch[CANVIEW_UART_MAX_FRAME_SIZE];
    uint8_t serial[CANVIEW_UART_MAX_SERIAL_SIZE];
    for (size_t index = 0U; index < CANVIEW_UART_MESSAGE_POLICY_COUNT; ++index)
    {
        const canview_uart_message_policy_t *policy = &CANVIEW_UART_MESSAGE_POLICIES[index];
        if (policy->supported == 0U)
        {
            continue;
        }
        for (size_t maximum = 0U; maximum < 2U; ++maximum)
        {
            const size_t payload_size =
                fill_valid_payload(policy->message_type, payload, sizeof(payload), maximum != 0U);
            CHECK(payload_size == (maximum != 0U ? policy->max_payload : policy->min_payload));
            canview_wire_view_t wire = {0};
            wire.header.message_type = policy->message_type;
            wire.header.flags = flags_for(policy->message_type);
            wire.header.sequence = (uint32_t)(index + maximum + 1U);
            wire.header.correlation_id = UINT32_C(0x10203040);
            wire.header.sender_time = UINT64_C(0x0102030405060708);
            wire.payload = payload;
            wire.payload_size = payload_size;
            canview_uart_message_view_t view;
            CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_OK);
            size_t written = 0U;
            CHECK(canview_uart_message_encode(
                      policy->message_type, wire.header.flags, wire.header.sequence,
                      wire.header.correlation_id, wire.header.sender_time, payload, payload_size,
                      scratch, sizeof(scratch), serial, sizeof(serial), &written) == CANVIEW_OK);
            CHECK(written > 0U && serial[written - 1U] == 0U);
            canview_uart_codec_t codec;
            CHECK(test_codec_reset_for_message(&codec, policy->message_type) == CANVIEW_OK);
            canview_uart_message_view_t decoded;
            canview_status_t status = CANVIEW_INCOMPLETE;
            for (size_t byte_index = 0U; byte_index < written; ++byte_index)
            {
                status = canview_uart_codec_feed(&codec, serial[byte_index], &decoded);
            }
            CHECK(status == CANVIEW_OK);
            CHECK(decoded.wire.header.message_type == policy->message_type);
            CHECK(decoded.wire.payload_size == payload_size);
            CHECK(memcmp(decoded.wire.payload, payload, payload_size) == 0);
            CHECK(codec.packets_ok == 1U && codec.malformed_packets == 0U &&
                  codec.crc_failures == 0U && codec.oversize_packets == 0U);

            canview_wire_view_t too_large = wire;
            too_large.payload_size = policy->max_payload + 1U;
            CHECK(canview_uart_message_validate(&too_large, &view) == CANVIEW_MALFORMED);
        }
    }
    return 0;
}

static int test_direction(void)
{
    uint8_t payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    canview_uart_message_view_t view;
    canview_uart_message_view_t empty = {0};
    CHECK(!canview_uart_message_direction_allowed(NULL, CANVIEW_UART_ENDPOINT_ESP32,
                                                  CANVIEW_UART_FLOW_INBOUND));
    CHECK(!canview_uart_message_direction_allowed(&empty, CANVIEW_UART_ENDPOINT_ESP32,
                                                  CANVIEW_UART_FLOW_INBOUND));
    CHECK(!canview_uart_message_direction_allowed(&empty,
                                                  (canview_uart_endpoint_t)2,
                                                  CANVIEW_UART_FLOW_INBOUND));
    CHECK(!canview_uart_message_direction_allowed(&empty, CANVIEW_UART_ENDPOINT_ESP32,
                                                  (canview_uart_flow_t)2));

    for (size_t index = 0U; index < CANVIEW_UART_MESSAGE_POLICY_COUNT; ++index)
    {
        const canview_uart_message_policy_t *policy = &CANVIEW_UART_MESSAGE_POLICIES[index];
        if (policy->supported == 0U)
        {
            continue;
        }
        const size_t payload_size =
            fill_valid_payload(policy->message_type, payload, sizeof(payload), false);
        canview_wire_view_t wire = {0};
        wire.header.message_type = policy->message_type;
        wire.header.flags = flags_for(policy->message_type);
        wire.payload = payload;
        wire.payload_size = payload_size;
        CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_OK);
        const bool both = policy->direction == CANVIEW_UART_DIRECTION_BOTH;
        const bool esp_to_stm = policy->direction == CANVIEW_UART_DIRECTION_ESP_TO_STM;
        CHECK(canview_uart_message_direction_allowed(&view, CANVIEW_UART_ENDPOINT_ESP32,
                                                     CANVIEW_UART_FLOW_OUTBOUND) ==
              (both || esp_to_stm));
        CHECK(canview_uart_message_direction_allowed(&view, CANVIEW_UART_ENDPOINT_ESP32,
                                                     CANVIEW_UART_FLOW_INBOUND) ==
              (both || !esp_to_stm));
        CHECK(canview_uart_message_direction_allowed(&view, CANVIEW_UART_ENDPOINT_STM32,
                                                     CANVIEW_UART_FLOW_OUTBOUND) ==
              (both || !esp_to_stm));
        CHECK(canview_uart_message_direction_allowed(&view, CANVIEW_UART_ENDPOINT_STM32,
                                                     CANVIEW_UART_FLOW_INBOUND) ==
              (both || esp_to_stm));
    }
    return 0;
}

static int test_malformed(void)
{
    uint8_t payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    const size_t size = fill_valid_payload(CANVIEW_UART_MSG_LINK_HELLO, payload, sizeof(payload), false);
    canview_wire_view_t wire = {0};
    wire.header.message_type = CANVIEW_UART_MSG_LINK_HELLO;
    wire.payload = payload;
    wire.payload_size = size;
    canview_uart_message_view_t view;
    payload[30] = 1U;
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_MALFORMED);
    payload[30] = 0U;
    wire.header.flags = CANVIEW_UART_FLAG_ERROR;
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_MALFORMED);
    wire.header.flags = 0U;
    wire.header.priority = 1U;
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_MALFORMED);
    wire.header.priority = 0U;
    wire.header.session_id = 1U;
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_MALFORMED);
    wire.header.session_id = 0U;
    wire.header.message_type = UINT8_C(0x7F);
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_UNSUPPORTED_MESSAGE);

    wire.header.message_type = CANVIEW_UART_MSG_FIRMWARE_PREPARE;
    wire.payload_size = fill_valid_payload(wire.header.message_type, payload, sizeof(payload), false);
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_UNSUPPORTED_MESSAGE);

    wire.header.message_type = CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL;
    wire.header.flags = CANVIEW_UART_FLAG_ACK_REQUIRED;
    wire.payload_size = fill_valid_payload(wire.header.message_type, payload, sizeof(payload), false);
    payload[16] = 5U;
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_MALFORMED);
    payload[16] = CANVIEW_UART_CAPTURE_ARM;
    wire.header.message_type = CANVIEW_UART_MSG_CAN_EVENT_MARKER;
    wire.payload_size = fill_valid_payload(wire.header.message_type, payload, sizeof(payload), false);
    put_le(payload + 8U, 4U, 0U);
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_MALFORMED);
    CHECK(CANVIEW_UART_MSG_CAN_EVENT_MARKER != CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL);

    CHECK(canview_uart_message_validate(NULL, &view) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_message_validate(&wire, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_message_policy(UINT8_C(0x7F), &view.policy) == CANVIEW_UNSUPPORTED_MESSAGE);

    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_LINK_HELLO, 0U, 8U, 0U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_LINK_HELLO, 10U, 1U, 0U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_LINK_HELLO, 9U, 1U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_LINK_HELLO, 28U, 2U,
                                 (uint64_t)CANVIEW_UART_MAX_FRAME_SIZE + 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_LINK_HELLO, 30U, 2U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_LINK_HELLO_ACK, 16U, 1U,
                                 CANVIEW_UART_PROTOCOL_MAJOR + 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_LINK_HELLO_ACK, 17U, 1U,
                                 CANVIEW_UART_PROTOCOL_MINOR + 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_LINK_HELLO_ACK, 28U, 2U,
                                 (uint64_t)CANVIEW_UART_MAX_FRAME_SIZE + 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_LINK_HELLO_ACK, 19U, 1U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_LINK_HELLO_ACK, 30U, 2U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_HEARTBEAT, 0U, 8U, 0U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_HEARTBEAT, 44U, 4U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_ERROR, 5U, 1U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_ERROR, 16U, 4U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_BUS_STATUS, 0U, 1U,
                                 CANVIEW_WIRE_CAN_BUS_COUNT) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_BUS_STATUS, 18U, 2U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_SAFETY_SNAPSHOT, 0U, 8U, 0U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_SAFETY_SNAPSHOT, 26U, 2U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_TX_AUDIT, 16U, 1U,
                                 CANVIEW_WIRE_CAN_BUS_COUNT) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_TX_AUDIT, 17U, 1U,
                                 CANVIEW_WIRE_CAN_MAX_DLC + 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_TX_AUDIT, 12U, 4U,
                                 CANVIEW_WIRE_CAN_STANDARD_ID_MAX + 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_TX_AUDIT, 18U, 2U, UINT16_C(0x0100)) ==
          0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_TX_AUDIT, 18U, 2U, 0x10U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_TX_AUDIT, 11U, 1U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_TX_AUDIT, 36U, 4U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL, 0U, 8U, 0U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL, 16U, 1U, 0U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL, 17U, 1U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL, 24U, 4U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_EVENT_MARKER, 22U, 2U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_CAPTURE_STATUS, 40U, 4U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_COMMAND_RESULT, 0U, 8U, 0U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_COMMAND_RESULT, 10U, 1U, 7U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_COMMAND_RESULT, 11U, 1U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CONTROL_LEASE, 0U, 8U, 0U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CONTROL_LEASE, 9U, 1U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CONTROL_LEASE, 26U, 2U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CONFIG_GET, 14U, 2U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CONFIG_GET, 18U, 2U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CONFIG_RESULT, 11U, 1U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CONFIG_RESULT, 14U, 2U, 1U) == 0);

    uint8_t variable[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    canview_wire_view_t variable_wire = {0};
    canview_uart_message_view_t variable_view;
    size_t variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_RX_BATCH, variable,
                                              sizeof(variable), true);
    variable_wire.header.message_type = CANVIEW_UART_MSG_CAN_RX_BATCH;
    variable_wire.header.flags = flags_for(variable_wire.header.message_type);
    variable_wire.payload = variable;
    variable_wire.payload_size = variable_size;
    variable[8U] = CANVIEW_UART_CAN_BATCH_MAX_RECORDS + 1U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_RX_BATCH, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    variable[14U] = CANVIEW_WIRE_CAN_BUS_COUNT;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_RX_BATCH, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    variable[15U] = 0x09U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_RX_BATCH, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    put_le(variable + 16U, 4U, CANVIEW_WIRE_CAN_STANDARD_ID_MAX + 1U);
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_RX_BATCH, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    variable[20U] = 1U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_RX_BATCH, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    memset(variable, 0, 12U);
    variable[8U] = 1U;
    put_le(variable, 8U, UINT64_MAX);
    put_le(variable + 12U, 2U, 1U);
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);

    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_ID_STATS, variable, sizeof(variable), true);
    variable_wire.header.message_type = CANVIEW_UART_MSG_CAN_ID_STATS;
    variable_wire.header.flags = flags_for(variable_wire.header.message_type);
    variable_wire.payload = variable;
    variable_wire.payload_size = variable_size;
    variable[6U] = 0U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_ID_STATS, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    variable[5U] = 1U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_ID_STATS, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    variable[7U] = 1U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_ID_STATS, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    variable[8U] = CANVIEW_WIRE_CAN_BUS_COUNT;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_ID_STATS, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    variable[9U] = 0x09U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CAN_ID_STATS, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    put_le(variable + 12U, 4U, CANVIEW_WIRE_CAN_STANDARD_ID_MAX + 1U);
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);

    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CONFIG_SET, variable, sizeof(variable), true);
    variable_wire.header.message_type = CANVIEW_UART_MSG_CONFIG_SET;
    variable_wire.header.flags = flags_for(variable_wire.header.message_type);
    variable_wire.payload = variable;
    variable_wire.payload_size = variable_size;
    variable[3U] = 1U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_CONFIG_SET, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    variable[7U] = 1U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);

    variable_size = fill_valid_payload(CANVIEW_UART_MSG_DIAGNOSTIC_COUNTERS, variable, sizeof(variable), true);
    variable_wire.header.message_type = CANVIEW_UART_MSG_DIAGNOSTIC_COUNTERS;
    variable_wire.header.flags = flags_for(variable_wire.header.message_type);
    variable_wire.payload = variable;
    variable_wire.payload_size = variable_size;
    variable[13U] = 1U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_DIAGNOSTIC_COUNTERS, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    variable[19U] = 1U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_DIAGNOSTIC_COUNTERS, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    variable[29U] = 1U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);

    variable_size = fill_valid_payload(CANVIEW_UART_MSG_COMMAND_REQUEST, variable, sizeof(variable), false);
    variable_wire.header.message_type = CANVIEW_UART_MSG_COMMAND_REQUEST;
    variable_wire.header.flags = flags_for(variable_wire.header.message_type);
    variable_wire.payload = variable;
    variable_wire.payload_size = variable_size;
    put_le(variable, 8U, 0U);
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_COMMAND_REQUEST, variable, sizeof(variable), false);
    variable_wire.payload_size = variable_size;
    put_le(variable + 10U, 2U, 499U);
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_COMMAND_REQUEST, variable, sizeof(variable), false);
    variable_wire.payload_size = variable_size;
    put_le(variable + 10U, 2U, 30001U);
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_COMMAND_REQUEST, variable, sizeof(variable), true);
    variable_wire.payload_size = variable_size;
    put_le(variable + 52U, 2U, 137U);
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    variable_size = fill_valid_payload(CANVIEW_UART_MSG_COMMAND_REQUEST, variable, sizeof(variable), false);
    variable_wire.payload_size = variable_size;
    variable[54U] = 1U;
    CHECK(canview_uart_message_validate(&variable_wire, &variable_view) == CANVIEW_MALFORMED);
    return 0;
}

static int make_serial(uint8_t message_type, uint8_t *serial, size_t capacity, uint8_t *scratch,
                       size_t *written)
{
    uint8_t payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    const size_t payload_size = fill_valid_payload(message_type, payload, sizeof(payload), false);
    return canview_uart_message_encode(message_type, flags_for(message_type), 7U, 8U, 9U, payload,
                                       payload_size, scratch, CANVIEW_UART_MAX_FRAME_SIZE, serial,
                                       capacity, written) == CANVIEW_OK
               ? 0
               : 1;
}

static int feed_frame(canview_uart_codec_t *codec, const uint8_t *serial, size_t size,
                      canview_uart_message_view_t *view)
{
    canview_status_t status = CANVIEW_INCOMPLETE;
    for (size_t index = 0U; index < size; ++index)
    {
        status = canview_uart_codec_feed(codec, serial[index], view);
    }
    return status == CANVIEW_OK ? 0 : 1;
}

static int test_stream(void)
{
    uint8_t serial[CANVIEW_UART_MAX_SERIAL_SIZE];
    uint8_t scratch[CANVIEW_UART_MAX_FRAME_SIZE];
    size_t serial_size = 0U;
    CHECK(make_serial(CANVIEW_UART_MSG_LINK_HELLO, serial, sizeof(serial), scratch, &serial_size) == 0);
    uint8_t raw[CANVIEW_UART_MAX_FRAME_SIZE];
    size_t raw_size = 0U;
    CHECK(canview_wire_cobs_decode(serial, serial_size - 1U, raw, sizeof(raw), &raw_size) == CANVIEW_OK);
    raw[32] ^= 1U;
    uint8_t corrupt[CANVIEW_UART_MAX_SERIAL_SIZE];
    size_t corrupt_size = 0U;
    CHECK(canview_wire_cobs_encode(raw, raw_size, corrupt, sizeof(corrupt) - 1U, &corrupt_size) ==
          CANVIEW_OK);
    corrupt[corrupt_size++] = 0U;
    canview_uart_codec_t codec;
    CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
    canview_uart_message_view_t view;
    canview_status_t status = CANVIEW_INCOMPLETE;
    for (size_t index = 0U; index < corrupt_size; ++index)
    {
        status = canview_uart_codec_feed(&codec, corrupt[index], &view);
    }
    CHECK(status == CANVIEW_CRC_MISMATCH && codec.crc_failures == 1U);
    CHECK(feed_frame(&codec, serial, serial_size, &view) == 0);
    CHECK(codec.packets_ok == 1U);

    codec.crc_failures = UINT32_MAX;
    for (size_t index = 0U; index < corrupt_size; ++index)
    {
        status = canview_uart_codec_feed(&codec, corrupt[index], &view);
    }
    CHECK(status == CANVIEW_CRC_MISMATCH && codec.crc_failures == UINT32_MAX);

    CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
    codec.malformed_packets = UINT32_MAX;
    CHECK(canview_uart_codec_feed(&codec, UINT8_C(0x01), &view) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_codec_feed(&codec, 0U, &view) == CANVIEW_MALFORMED);
    CHECK(codec.malformed_packets == UINT32_MAX);

    CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
    uint8_t unsupported_payload[36] = {0};
    put_le(unsupported_payload, 8U, 1U);
    canview_wire_header_t unsupported_header = {0};
    unsupported_header.message_type = CANVIEW_UART_MSG_FIRMWARE_PREPARE;
    unsupported_header.flags = CANVIEW_UART_FLAG_ACK_REQUIRED;
    uint8_t unsupported_serial[CANVIEW_UART_MAX_SERIAL_SIZE];
    size_t unsupported_size = 0U;
    CHECK(canview_uart_packet_encode(&unsupported_header, unsupported_payload,
                                     sizeof(unsupported_payload), scratch, sizeof(scratch),
                                     unsupported_serial, sizeof(unsupported_serial),
                                     &unsupported_size) == CANVIEW_OK);
    CHECK(feed_frame(&codec, unsupported_serial, unsupported_size, &view) == 1);
    CHECK(codec.unsupported_messages == 1U && codec.packets_ok == 0U);

    CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
    uint8_t malformed_payload[28] = {0};
    put_le(malformed_payload, 8U, 1U);
    put_le(malformed_payload + 8U, 8U, 1U);
    malformed_payload[16U] = 5U;
    canview_wire_header_t malformed_header = {0};
    malformed_header.message_type = CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL;
    malformed_header.flags = CANVIEW_UART_FLAG_ACK_REQUIRED;
    uint8_t malformed_serial[CANVIEW_UART_MAX_SERIAL_SIZE];
    size_t malformed_size = 0U;
    CHECK(canview_uart_packet_encode(&malformed_header, malformed_payload,
                                     sizeof(malformed_payload), scratch, sizeof(scratch),
                                     malformed_serial, sizeof(malformed_serial),
                                     &malformed_size) == CANVIEW_OK);
    CHECK(feed_frame(&codec, malformed_serial, malformed_size, &view) == 1);
    CHECK(codec.malformed_packets == 1U && codec.unsupported_messages == 0U);

    CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
    codec.packets_ok = UINT32_MAX;
    CHECK(feed_frame(&codec, serial, serial_size, &view) == 0);
    CHECK(codec.packets_ok == UINT32_MAX);

    CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
    codec.oversize_packets = UINT32_MAX;
    for (size_t index = 0U; index < CANVIEW_UART_MAX_ENCODED_SIZE + 1U; ++index)
    {
        status = canview_uart_codec_feed(&codec, UINT8_C(0xA5), &view);
    }
    CHECK(status == CANVIEW_OVERSIZE && codec.oversize_packets == UINT32_MAX);
    CHECK(canview_uart_codec_feed(&codec, UINT8_C(0xA5), &view) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_codec_feed(&codec, 0U, &view) == CANVIEW_INCOMPLETE);
    CHECK(feed_frame(&codec, serial, serial_size, &view) == 0);
    CHECK(codec.oversize_packets == UINT32_MAX && codec.packets_ok == 1U);
    CHECK(canview_uart_codec_feed(&codec, 0U, &view) == CANVIEW_INCOMPLETE);
    return 0;
}

static int test_long_stream(void)
{
    enum
    {
        LONG_STREAM_FRAMES = 1728000U,
        LONG_STREAM_FAULT_PERIOD = 97U
    };
    uint8_t serial[CANVIEW_UART_MAX_SERIAL_SIZE];
    uint8_t scratch[CANVIEW_UART_MAX_FRAME_SIZE];
    uint8_t payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    canview_uart_codec_t codec;
    canview_uart_message_view_t view;
    CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
    uint32_t valid_frames = 0U;
    uint32_t corrupted_frames = 0U;
    for (uint32_t frame = 0U; frame < LONG_STREAM_FRAMES; ++frame)
    {
        const size_t payload_size = fill_valid_payload(CANVIEW_UART_MSG_HEARTBEAT, payload,
                                                       sizeof(payload), false);
        put_le(payload, 8U, UINT64_C(1) + frame / 100U);
        size_t serial_size = 0U;
        CHECK(canview_uart_message_encode(
                  CANVIEW_UART_MSG_HEARTBEAT, flags_for(CANVIEW_UART_MSG_HEARTBEAT), frame,
                  frame ^ UINT32_C(0x5A5A0000), UINT64_C(1000000) * frame, payload, payload_size,
                  scratch, sizeof(scratch), serial, sizeof(serial), &serial_size) == CANVIEW_OK);

        switch (frame % LONG_STREAM_FAULT_PERIOD)
        {
        case 1U:
        {
            uint8_t *byte = &serial[serial_size - 2U];
            *byte = *byte == UINT8_C(0xFF) ? UINT8_C(0xFE) : (uint8_t)(*byte ^ UINT8_C(0x01));
            if (*byte == 0U)
            {
                *byte = UINT8_C(0x02);
            }
            (void)feed_frame(&codec, serial, serial_size, &view);
            ++corrupted_frames;
            break;
        }
        case 2U:
        {
            const size_t dropped = serial_size / 2U;
            for (size_t index = 0U; index < serial_size; ++index)
            {
                if (index != dropped)
                {
                    (void)canview_uart_codec_feed(&codec, serial[index], &view);
                }
            }
            ++corrupted_frames;
            break;
        }
        case 3U:
            for (size_t index = 0U; index + 1U < serial_size; ++index)
            {
                (void)canview_uart_codec_feed(&codec, serial[index], &view);
            }
            (void)canview_uart_codec_feed(&codec, UINT8_C(0xA5), &view);
            (void)canview_uart_codec_feed(&codec, 0U, &view);
            ++corrupted_frames;
            break;
        case 4U:
            (void)canview_uart_codec_feed(&codec, UINT8_C(0xA5), &view);
            (void)canview_uart_codec_feed(&codec, UINT8_C(0x5A), &view);
            (void)canview_uart_codec_feed(&codec, 0U, &view);
            CHECK(feed_frame(&codec, serial, serial_size, &view) == 0);
            ++valid_frames;
            ++corrupted_frames;
            break;
        default:
            CHECK(feed_frame(&codec, serial, serial_size, &view) == 0);
            ++valid_frames;
            break;
        }
    }
    CHECK(codec.packets_ok == valid_frames);
    CHECK(corrupted_frames > 0U);
    CHECK(codec.crc_failures + codec.malformed_packets + codec.unsupported_messages > 0U);
    CHECK(codec.oversize_packets == 0U);
    return 0;
}

/* Count bytes actually passed through the production C decoder. The ordinary
 * virtual-time fault test does not represent a saturated 4 Mbps line.
 * This fixture exercises framing/semantics only; it does not dispatch CAN. */
static int test_byte_soak(uint32_t seconds)
{
    enum { SOAK_DIRECTIONS = 2, SOAK_BANKS = 8, SOAK_FAULT_PERIOD = 997 };
    const uint64_t required_bytes = (uint64_t)seconds * UINT64_C(4000000) / UINT64_C(10);
    const uint32_t guard = UINT32_C(0xA5963CC3);
    struct
    {
        uint32_t before;
        canview_uart_codec_t codec;
        uint32_t after;
    } guarded[SOAK_DIRECTIONS];
    uint8_t serial[SOAK_DIRECTIONS][SOAK_BANKS][CANVIEW_UART_MAX_SERIAL_SIZE];
    size_t lengths[SOAK_DIRECTIONS][SOAK_BANKS];
    uint8_t scratch[CANVIEW_UART_MAX_FRAME_SIZE];
    uint8_t payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    uint64_t fed[SOAK_DIRECTIONS] = {0U};
    uint32_t accepted[SOAK_DIRECTIONS] = {0U};
    uint32_t faults[SOAK_DIRECTIONS] = {0U};
    canview_uart_message_view_t view;
    const uint8_t messages[SOAK_DIRECTIONS] = {
        CANVIEW_UART_MSG_CAN_RX_BATCH, CANVIEW_UART_MSG_CAN_OBSERVER_PLAN};
    for (size_t direction = 0U; direction < SOAK_DIRECTIONS; ++direction)
    {
        guarded[direction].before = guard;
        guarded[direction].after = guard;
        CHECK(test_codec_reset_for_message(&guarded[direction].codec, messages[direction]) ==
              CANVIEW_OK);
        for (size_t bank = 0U; bank < SOAK_BANKS; ++bank)
        {
            const size_t size = fill_valid_payload(messages[direction], payload, sizeof(payload),
                                                  bank != 0U);
            /* Vary data/IDs while preserving the independent semantic layout. */
            if (bank != 0U)
            {
                if (direction == 0U)
                {
                    for (size_t record = 0U; record < payload[8]; ++record)
                    {
                        uint8_t *item = payload + 12U + record * 16U;
                        item[2] = (uint8_t)(record % CANVIEW_WIRE_CAN_BUS_COUNT);
                        item[3] = 8U;
                        put_le(item + 4U, 4U, bank * 16U + record);
                        for (size_t byte = 0U; byte < 8U; ++byte)
                        {
                            item[8U + byte] = (uint8_t)(bank * 31U + record * 8U + byte);
                        }
                    }
                }
                else
                {
                    for (size_t record = 0U; record < payload[20]; ++record)
                    {
                        uint8_t *item = payload + 24U + record * 12U;
                        item[0] = (uint8_t)(record % CANVIEW_WIRE_CAN_BUS_COUNT);
                        put_le(item + 4U, 4U, bank * 16U + record);
                        put_le(item + 8U, 4U, UINT32_C(0x7FF));
                    }
                }
            }
            CHECK(canview_uart_message_encode(
                      messages[direction], flags_for(messages[direction]), (uint32_t)bank,
                      (uint32_t)bank + 1U, (uint64_t)bank * UINT64_C(1000000), payload, size,
                      scratch, sizeof(scratch), serial[direction][bank],
                      sizeof(serial[direction][bank]), &lengths[direction][bank]) == CANVIEW_OK);
        }
    }
    uint32_t round = 0U;
    uint64_t next_progress = UINT64_C(1440000000); /* one hour at 4 Mbps, 8-N-1 */
    while (fed[0] < required_bytes || fed[1] < required_bytes)
    {
        for (size_t direction = 0U; direction < SOAK_DIRECTIONS; ++direction)
        {
            if (fed[direction] >= required_bytes)
            {
                continue;
            }
            canview_uart_codec_t *codec = &guarded[direction].codec;
            const size_t bank = round % SOAK_BANKS;
            const uint8_t *good = serial[direction][bank];
            const size_t size = lengths[direction][bank];
            if (round % SOAK_FAULT_PERIOD == 0U)
            {
                uint8_t damaged[CANVIEW_UART_MAX_SERIAL_SIZE + 1U];
                size_t damaged_size = size;
                memcpy(damaged, good, size);
                switch ((round / SOAK_FAULT_PERIOD) % 4U)
                {
                case 0U:
                    /* Corrupt a data byte without introducing another delimiter. */
                    damaged[size - 2U] = good[size - 2U] == 1U ? 2U : 1U;
                    break;
                case 1U:
                    damaged[size - 2U] = 0U; /* delete the final data byte */
                    --damaged_size;
                    break;
                case 2U:
                    damaged[size - 1U] = UINT8_C(0xA5); /* insert before delimiter */
                    damaged[size] = 0U;
                    ++damaged_size;
                    break;
                default:
                    damaged_size = CANVIEW_UART_MAX_ENCODED_SIZE + 2U;
                    memset(damaged, 0xA5, damaged_size);
                    damaged[damaged_size - 1U] = 0U;
                    break;
                }
                for (size_t byte = 0U; byte < damaged_size; ++byte)
                {
                    CHECK(canview_uart_codec_feed(codec, damaged[byte], &view) != CANVIEW_OK);
                }
                fed[direction] += damaged_size;
                ++faults[direction];
            }
            CHECK(feed_frame(codec, good, size, &view) == 0);
            fed[direction] += size;
            ++accepted[direction];
            CHECK(codec->packets_ok == accepted[direction]);
            CHECK(codec->stream.used == 0U && !codec->stream.discarding);
            CHECK(guarded[direction].before == guard && guarded[direction].after == guard);
        }
        ++round;
        if (fed[0] >= next_progress && fed[1] >= next_progress)
        {
            (void)printf("SOAK progress actual_bytes=%" PRIu64 ",%" PRIu64 "\n", fed[0], fed[1]);
            (void)fflush(stdout);
            next_progress += UINT64_C(1440000000);
        }
    }
    for (size_t direction = 0U; direction < SOAK_DIRECTIONS; ++direction)
    {
        const canview_uart_codec_t *codec = &guarded[direction].codec;
        CHECK(fed[direction] >= required_bytes && faults[direction] > 0U);
        CHECK(codec->crc_failures + codec->malformed_packets + codec->oversize_packets ==
              faults[direction]);
        (void)printf("PASS: C UART soak direction=%zu seconds=%" PRIu32
                     " baud=4000000 bits_per_byte=10 required_bytes=%" PRIu64
                     " actual_bytes=%" PRIu64 " accepted=%" PRIu32 " faults=%" PRIu32 "\n",
                     direction, seconds, required_bytes, fed[direction], accepted[direction],
                     faults[direction]);
    }
    return 0;
}

static void build_plan_begin(uint8_t *payload, uint64_t token, uint32_t revision,
                             uint32_t expected_revision, uint16_t total_chunks,
                             uint16_t filter_count)
{
    memset(payload, 0, 32U);
    payload[0] = CANVIEW_UART_PLAN_OP_BEGIN;
    put_le(payload + 4U, 8U, token);
    put_le(payload + 12U, 4U, revision);
    put_le(payload + 16U, 4U, expected_revision);
    put_le(payload + 20U, 2U, total_chunks);
    put_le(payload + 22U, 2U, filter_count);
    put_le(payload + 24U, 2U, 100U);
    put_le(payload + 26U, 4U, 20000U);
    payload[30] = 1U;
}

static size_t build_plan_chunk(uint8_t *payload, uint64_t token, uint32_t revision,
                               uint16_t chunk_index, uint16_t chunk_count, uint8_t count)
{
    const size_t size = 24U + (size_t)count * 12U;
    memset(payload, 0, size);
    payload[0] = CANVIEW_UART_PLAN_OP_CHUNK;
    put_le(payload + 4U, 8U, token);
    put_le(payload + 12U, 4U, revision);
    put_le(payload + 16U, 2U, chunk_index);
    put_le(payload + 18U, 2U, chunk_count);
    payload[20] = count;
    for (size_t index = 0U; index < count; ++index)
    {
        uint8_t *record = payload + 24U + index * 12U;
        record[0] = 0U;
        record[1] = 0U;
        put_le(record + 4U, 4U, (uint32_t)(index + 1U));
        put_le(record + 8U, 4U, UINT32_C(0x7FF));
    }
    return size;
}

static int test_plan(void)
{
    canview_uart_plan_context_t context;
    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    uint8_t begin[32];
    uint8_t chunk[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    uint8_t commit[48];
    build_plan_begin(begin, 1U, 1U, 0U, 2U, 2U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    size_t chunk_size = build_plan_chunk(chunk, 1U, 1U, 0U, 2U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    memset(commit, 0, sizeof(commit));
    commit[0] = CANVIEW_UART_PLAN_OP_COMMIT;
    put_le(commit + 4U, 8U, 1U);
    put_le(commit + 12U, 4U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 1U, 1U, 1U, 2U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    uint8_t plan_digest[CANVIEW_UART_PLAN_DIGEST_SIZE];
    CHECK(canview_uart_plan_digest(&context.candidate, plan_digest) == CANVIEW_OK);
    memcpy(commit + 16U, plan_digest, sizeof(plan_digest));
    CHECK(TEST_PLAN_APPLY(&context, commit, sizeof(commit)) == CANVIEW_OK);
    canview_uart_observer_plan_t active;
    CHECK(canview_uart_plan_current(&context, &active) == CANVIEW_OK);
    CHECK(active.revision == 1U && active.filter_count == 2U && active.filters[1].can_id == 1U);
    static const uint8_t expected_digest[CANVIEW_UART_PLAN_DIGEST_SIZE] = {
        0xF1U, 0x71U, 0x27U, 0x0FU, 0x56U, 0xA5U, 0xABU, 0xFAU,
        0x41U, 0x37U, 0x08U, 0x8FU, 0x2BU, 0x9BU, 0x8DU, 0xB6U,
        0xD7U, 0x42U, 0x96U, 0xB2U, 0xBFU, 0xC8U, 0x18U, 0x57U,
        0x1DU, 0x9FU, 0xB5U, 0xD9U, 0x04U, 0x6DU, 0x20U, 0x4EU};
    CHECK(memcmp(active.plan_digest, expected_digest, sizeof(expected_digest)) == 0);
    uint8_t digest_again[CANVIEW_UART_PLAN_DIGEST_SIZE];
    CHECK(canview_uart_plan_digest(&active, digest_again) == CANVIEW_OK);
    CHECK(memcmp(digest_again, active.plan_digest, sizeof(digest_again)) == 0);
    active.max_records_per_second += 1U;
    CHECK(canview_uart_plan_digest(&active, digest_again) == CANVIEW_OK);
    CHECK(memcmp(digest_again, active.plan_digest, sizeof(digest_again)) != 0);

    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    build_plan_begin(begin, 2U, 1U, 0U, 2U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    chunk_size = build_plan_chunk(chunk, 2U, 1U, 1U, 2U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    CHECK(canview_uart_plan_current(&context, &active) == CANVIEW_OK && active.revision == 0U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_RESOURCE_BUSY);
    memset(chunk, 0, sizeof(chunk));
    chunk[0] = CANVIEW_UART_PLAN_OP_CHUNK;
    put_le(chunk + 4U, 8U, 2U);
    put_le(chunk + 12U, 4U, 1U);
    put_le(chunk + 16U, 2U, 0U);
    put_le(chunk + 18U, 2U, 2U);
    chunk[20] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, chunk, 36U) == CANVIEW_INCOMPLETE);
    CHECK(TEST_PLAN_APPLY(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_RESOURCE_BUSY);
    build_plan_begin(begin, 3U, 1U, 0U, 1U, 0U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_RESOURCE_BUSY);
    CHECK(TEST_PLAN_APPLY(&context, chunk, 36U) == CANVIEW_MALFORMED);

    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    build_plan_begin(begin, 4U, 1U, 0U, 1U, 0U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    uint8_t abort_payload[20] = {0};
    abort_payload[0] = CANVIEW_UART_PLAN_OP_ABORT;
    put_le(abort_payload + 4U, 8U, 4U);
    put_le(abort_payload + 12U, 4U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, abort_payload, sizeof(abort_payload)) == CANVIEW_OK);
    CHECK(canview_uart_plan_current(&context, &active) == CANVIEW_OK && active.revision == 0U);
    build_plan_begin(begin, 5U, 1U, 0U, 1U, 0U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin), 10U) == CANVIEW_INCOMPLETE);
    build_plan_begin(begin, 6U, 1U, 0U, 1U, 0U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin),
                                  10U + CANVIEW_UART_PLAN_STAGING_TIMEOUT_MS) ==
          CANVIEW_INCOMPLETE);
    CHECK(!context.pending || context.request_token == 6U);
    return 0;
}

static int test_plan_boundaries(void)
{
    canview_uart_plan_context_t context;
    uint8_t begin[32];
    uint8_t chunk[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    uint8_t commit[48] = {0};
    uint8_t plan_digest[CANVIEW_UART_PLAN_DIGEST_SIZE];
    canview_uart_observer_plan_t active;
    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    CHECK(TEST_PLAN_APPLY(NULL, begin, sizeof(begin)) == CANVIEW_INVALID_ARGUMENT);
    CHECK(TEST_PLAN_APPLY(&context, NULL, sizeof(begin)) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_plan_current(NULL, NULL) == CANVIEW_INVALID_ARGUMENT);

    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    begin[1U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 0U, 1U, 0U, 1U, 0U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 0U, 0U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, CANVIEW_UART_PLAN_MAX_CHUNKS + 1U, 0U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, CANVIEW_UART_PLAN_MAX_FILTERS + 1U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    put_le(begin + 24U, 2U, CANVIEW_UART_PLAN_MIN_RECORDS_PER_SECOND - 1U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    put_le(begin + 24U, 2U, CANVIEW_UART_PLAN_MAX_RECORDS_PER_SECOND + 1U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    put_le(begin + 26U, 4U, CANVIEW_UART_PLAN_MIN_BYTES_PER_SECOND - 1U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    put_le(begin + 26U, 4U, CANVIEW_UART_PLAN_MAX_BYTES_PER_SECOND + 1U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    begin[30U] = 8U;
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    begin[31U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 17U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    CHECK(TEST_PLAN_APPLY(&context, begin, 31U) == CANVIEW_MALFORMED);

    context.active.revision = 7U;
    build_plan_begin(begin, 2U, 8U, 0U, 1U, 0U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_STALE);
    CHECK(!context.pending);
    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);

    build_plan_begin(begin, 3U, 1U, 0U, 1U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    size_t chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[1U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 0U, 1U, 0U, 1U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 0U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, CANVIEW_UART_PLAN_MAX_CHUNKS + 1U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 1U, 1U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U,
                                  CANVIEW_UART_PLAN_CHUNK_MAX_FILTERS + 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[21U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[24U] = CANVIEW_WIRE_CAN_BUS_COUNT;
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[25U] = 0x10U;
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[26U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    put_le(chunk + 28U, 4U, CANVIEW_WIRE_CAN_STANDARD_ID_MAX + 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[24U + 2U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[24U + 1U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    build_plan_begin(begin, 3U, 1U, 0U, 1U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    put_le(chunk + 24U + 4U, 4U, CANVIEW_WIRE_CAN_EXTENDED_ID_MAX);
    chunk[24U + 1U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);

    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    build_plan_begin(begin, 6U, 1U, 0U, 2U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    chunk_size = build_plan_chunk(chunk, 6U, 1U, 0U, 2U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    chunk_size = build_plan_chunk(chunk, 6U, 1U, 1U, 2U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);

    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    build_plan_begin(begin, 4U, 1U, 0U, 1U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    chunk_size = build_plan_chunk(chunk, 4U, 1U, 0U, 1U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    chunk[20U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    memset(commit, 0, sizeof(commit));
    commit[0] = CANVIEW_UART_PLAN_OP_COMMIT;
    put_le(commit + 4U, 8U, 4U);
    put_le(commit + 12U, 4U, 1U);
    commit[1U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);
    commit[1U] = 0U;
    CHECK(TEST_PLAN_APPLY(&context, commit, 47U) == CANVIEW_MALFORMED);
    CHECK(canview_uart_plan_digest(&context.candidate, plan_digest) == CANVIEW_OK);
    memcpy(commit + 16U, plan_digest, sizeof(plan_digest));
    commit[16U] ^= 1U;
    CHECK(TEST_PLAN_APPLY(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);
    CHECK(canview_uart_plan_current(&context, &active) == CANVIEW_OK && active.revision == 0U);
    commit[16U] ^= 1U;
    CHECK(TEST_PLAN_APPLY(&context, commit, sizeof(commit)) == CANVIEW_OK);

    uint8_t unknown[20] = {0};
    CHECK(TEST_PLAN_APPLY(&context, unknown, sizeof(unknown)) ==
          CANVIEW_UNSUPPORTED_MESSAGE);
    CHECK(TEST_PLAN_APPLY(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);

    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    memset(commit, 0, sizeof(commit));
    commit[0] = CANVIEW_UART_PLAN_OP_COMMIT;
    put_le(commit + 4U, 8U, 5U);
    put_le(commit + 12U, 4U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);
    uint8_t abort_payload[20] = {0};
    abort_payload[0] = CANVIEW_UART_PLAN_OP_ABORT;
    put_le(abort_payload + 4U, 8U, 5U);
    put_le(abort_payload + 12U, 4U, 1U);
    CHECK(TEST_PLAN_APPLY(&context, abort_payload, sizeof(abort_payload)) ==
          CANVIEW_MALFORMED);
    abort_payload[18U] = 1U;
    CHECK(TEST_PLAN_APPLY(&context, abort_payload, sizeof(abort_payload)) ==
          CANVIEW_MALFORMED);
    return 0;
}

static canview_uart_command_key_t test_command_key(uint64_t token, uint16_t command_id,
                                                   const uint8_t digest[CANVIEW_UART_COMMAND_DIGEST_SIZE])
{
    canview_uart_command_key_t key = {
        .origin_device_id = 1U,
        .origin_boot_id = 2U,
        .wireless_session_id = 3U,
        .control_generation = 4U,
        .request_token = token,
        .command_id = command_id};
    memcpy(key.canonical_argument_digest, digest, CANVIEW_UART_COMMAND_DIGEST_SIZE);
    return key;
}

static int test_command_cache(void)
{
    canview_uart_command_cache_t cache = {0};
    uint8_t digest[CANVIEW_UART_COMMAND_DIGEST_SIZE];
    uint8_t changed[CANVIEW_UART_COMMAND_DIGEST_SIZE];
    uint8_t result[CANVIEW_UART_COMMAND_RESULT_MAX];
    memset(digest, 0x11, sizeof(digest));
    memset(changed, 0x22, sizeof(changed));
    memset(result, 0xA5, sizeof(result));
    const canview_uart_command_key_t key = test_command_key(1U, 7U, digest);
    canview_uart_command_handle_t handle;
    CHECK(canview_uart_command_cache_reset(&cache) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_admit(&cache, &key, 100U, &handle) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_record_result(&cache, &handle, result, sizeof(result), 101U) ==
          CANVIEW_OK);
    const uint8_t *stored = NULL;
    size_t stored_size = 0U;
    canview_uart_command_handle_t duplicate_handle;
    CHECK(canview_uart_command_cache_lookup(&cache, &key, 102U, &duplicate_handle, &stored,
                                            &stored_size) == CANVIEW_DUPLICATE);
    CHECK(duplicate_handle.slot == handle.slot && duplicate_handle.generation == handle.generation &&
          stored != NULL && stored_size == sizeof(result) &&
          memcmp(stored, result, sizeof(result)) == 0);
    CHECK(canview_uart_command_cache_mark_ack(&cache, &handle) == CANVIEW_OK);
    CHECK(cache.entries[handle.slot].terminal && cache.entries[handle.slot].acknowledged);
    CHECK(canview_uart_command_cache_admit(&cache, &key, 103U, &duplicate_handle) ==
          CANVIEW_DUPLICATE);

    canview_uart_command_key_t changed_key = key;
    memcpy(changed_key.canonical_argument_digest, changed, sizeof(changed));
    CHECK(canview_uart_command_cache_admit(&cache, &changed_key, 103U, &duplicate_handle) ==
          CANVIEW_MALFORMED);
    CHECK(canview_uart_command_cache_lookup(&cache, &changed_key, 103U, &duplicate_handle, &stored,
                                            &stored_size) == CANVIEW_MALFORMED);
    CHECK(canview_uart_command_cache_record_result(&cache, &handle, result, sizeof(result), 104U) ==
          CANVIEW_OK);
    result[0] ^= 1U;
    CHECK(canview_uart_command_cache_record_result(&cache, &handle, result, sizeof(result), 104U) ==
          CANVIEW_MALFORMED);
    result[0] ^= 1U;

    canview_uart_command_key_t other_origin = key;
    other_origin.origin_device_id = 9U;
    CHECK(canview_uart_command_cache_admit(&cache, &other_origin, 105U, &duplicate_handle) ==
          CANVIEW_OK);
    CHECK(duplicate_handle.slot != handle.slot);
    CHECK(canview_uart_command_cache_lookup(&cache, &other_origin, 106U, &duplicate_handle,
                                            &stored, &stored_size) == CANVIEW_DUPLICATE);
    CHECK(canview_uart_command_cache_reset(&cache) == CANVIEW_OK);

    for (uint64_t token = 1U; token <= CANVIEW_UART_COMMAND_CACHE_CAPACITY; ++token)
    {
        const canview_uart_command_key_t fill_key = test_command_key(token, 7U, digest);
        CHECK(canview_uart_command_cache_admit(&cache, &fill_key, 100U, &handle) == CANVIEW_OK);
    }
    const canview_uart_command_key_t full_key = test_command_key(257U, 7U, digest);
    CHECK(canview_uart_command_cache_admit(&cache, &full_key, 100U, &handle) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(canview_uart_command_cache_expire(&cache, CANVIEW_UART_COMMAND_CACHE_RETENTION_MS + 100U) ==
          CANVIEW_OK);
    CHECK(canview_uart_command_cache_admit(&cache, &full_key,
                                           CANVIEW_UART_COMMAND_CACHE_RETENTION_MS + 101U,
                                           &handle) == CANVIEW_OK);
    const canview_uart_command_handle_t stale_handle = handle;
    CHECK(canview_uart_command_cache_record_result(&cache, &handle, result, sizeof(result),
                                                   CANVIEW_UART_COMMAND_CACHE_RETENTION_MS + 101U) ==
          CANVIEW_OK);
    CHECK(canview_uart_command_cache_expire(&cache,
                                            2U * CANVIEW_UART_COMMAND_CACHE_RETENTION_MS + 102U) ==
          CANVIEW_OK);
    const canview_uart_command_key_t replacement_key = test_command_key(258U, 7U, digest);
    CHECK(canview_uart_command_cache_admit(&cache, &replacement_key,
                                           2U * CANVIEW_UART_COMMAND_CACHE_RETENTION_MS + 103U,
                                           &handle) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_record_result(&cache, &stale_handle, result, sizeof(result),
                                                   2U * CANVIEW_UART_COMMAND_CACHE_RETENTION_MS + 104U) ==
          CANVIEW_STALE);
    CHECK(canview_uart_command_cache_reset(&cache) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_record_result(&cache, &handle, result, sizeof(result), 200U) ==
          CANVIEW_STALE);
    return 0;
}

static int test_command_boundaries(void)
{
    canview_uart_command_cache_t cache = {0};
    uint8_t digest[CANVIEW_UART_COMMAND_DIGEST_SIZE];
    uint8_t result[CANVIEW_UART_COMMAND_RESULT_MAX];
    memset(digest, 0x31, sizeof(digest));
    memset(result, 0xC3, sizeof(result));
    canview_uart_command_key_t key = test_command_key(1U, 1U, digest);
    canview_uart_command_handle_t handle = {0};
    canview_uart_command_handle_t ignored_handle;
    const uint8_t *stored = NULL;
    size_t stored_size = 0U;
    CHECK(canview_uart_command_cache_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_reset(&cache) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_admit(NULL, &key, 0U, &handle) == CANVIEW_INVALID_ARGUMENT);
    key.request_token = 0U;
    CHECK(canview_uart_command_cache_admit(&cache, &key, 0U, &handle) == CANVIEW_INVALID_ARGUMENT);
    key.request_token = 1U;
    CHECK(canview_uart_command_cache_admit(&cache, &key, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_admit(&cache, &key, 0U, &handle) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_lookup(&cache, &key, 1U, &ignored_handle, &stored,
                                            &stored_size) == CANVIEW_DUPLICATE);
    CHECK(stored == NULL && stored_size == 0U);
    CHECK(canview_uart_command_cache_record_result(&cache, &handle, result, sizeof(result), 2U) ==
          CANVIEW_OK);
    CHECK(canview_uart_command_cache_record_result(&cache, &handle, NULL, 1U, 2U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_record_result(&cache, &handle, result, 0U, 2U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_record_result(&cache, &handle, result,
                                                   CANVIEW_UART_COMMAND_RESULT_MAX + 1U, 2U) ==
          CANVIEW_INVALID_ARGUMENT);
    handle.slot = CANVIEW_UART_COMMAND_CACHE_CAPACITY;
    CHECK(canview_uart_command_cache_mark_ack(&cache, &handle) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_mark_ack(NULL, &handle) == CANVIEW_INVALID_ARGUMENT);
    handle.slot = 0U;
    CHECK(canview_uart_command_cache_mark_ack(&cache, &handle) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_lookup(NULL, &key, 2U, &ignored_handle, &stored,
                                            &stored_size) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_lookup(&cache, &key, 2U, NULL, &stored,
                                            &stored_size) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_lookup(&cache, &key, 2U, &ignored_handle, NULL,
                                            &stored_size) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_lookup(&cache, &key, 2U, &ignored_handle, &stored,
                                            NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_reset(&cache) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_mark_ack(&cache, &handle) == CANVIEW_STALE);
    CHECK(canview_uart_command_cache_expire(NULL, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_admit(&cache, &key, UINT64_MAX - 1U, &handle) == CANVIEW_OK);
    CHECK(cache.entries[handle.slot].expires_at_ms == UINT64_MAX);

    canview_uart_command_cache_t ttl_cache = {0};
    canview_uart_command_handle_t ttl_handle;
    CHECK(canview_uart_command_cache_reset(&ttl_cache) == CANVIEW_OK);
    CHECK(test_command_cache_admit_with_ttl(&ttl_cache, &key,
                                            CANVIEW_UART_COMMAND_TTL_MIN_MS - 1U, 100U,
                                            &ttl_handle) == CANVIEW_INVALID_ARGUMENT);
    CHECK(test_command_cache_admit_with_ttl(&ttl_cache, &key,
                                            CANVIEW_UART_COMMAND_TTL_MAX_MS + 1U, 100U,
                                            &ttl_handle) == CANVIEW_INVALID_ARGUMENT);
    CHECK(test_command_cache_admit_with_ttl(&ttl_cache, &key,
                                            CANVIEW_UART_COMMAND_TTL_MIN_MS, 100U,
                                            &ttl_handle) == CANVIEW_OK);
    CHECK(ttl_cache.entries[ttl_handle.slot].request_expires_at_ms ==
          100U + CANVIEW_UART_COMMAND_TTL_MIN_MS);
    CHECK(test_command_cache_mark_ack_at(&ttl_cache, &ttl_handle, 599U) == CANVIEW_OK);
    CHECK(test_command_cache_mark_ack_at(&ttl_cache, &ttl_handle, 600U) == CANVIEW_TIMEOUT);
    CHECK(canview_uart_command_cache_record_result(&ttl_cache, &ttl_handle, result,
                                                   sizeof(result), 600U) == CANVIEW_TIMEOUT);
    CHECK(canview_uart_command_cache_lookup(&ttl_cache, &key, 600U, &ignored_handle, &stored,
                                            &stored_size) == CANVIEW_TIMEOUT);
    CHECK(test_command_cache_admit_with_ttl(&ttl_cache, &key,
                                            CANVIEW_UART_COMMAND_TTL_MIN_MS, 600U,
                                            &ttl_handle) == CANVIEW_TIMEOUT);
    return 0;
}

static int link_complete_for_test(canview_uart_link_t *link, uint64_t boot_id,
                                  uint64_t now_ms)
{
    bool changed = false;
    CHECK(canview_uart_link_note_hello(link, boot_id, now_ms, &changed) == CANVIEW_OK);
    CHECK(canview_uart_link_note_hello_ack(link, boot_id, CANVIEW_UART_PROTOCOL_MAJOR,
                                           CANVIEW_UART_PROTOCOL_MINOR, 0U, now_ms) == CANVIEW_OK);
    CHECK(canview_uart_link_note_safety_snapshot(link, boot_id, now_ms) == CANVIEW_OK);
    CHECK(canview_uart_link_set_cts_blocked(link, false, now_ms) == CANVIEW_OK);
    CHECK(canview_uart_link_note_heartbeat(link, boot_id, now_ms, &changed) == CANVIEW_OK);
    return 0;
}

static bool test_authorize_command(const canview_uart_message_view_t *view, uint64_t now_ms,
                                   void *context)
{
    (void)now_ms;
    return view != NULL && context != NULL && *(const bool *)context;
}

static canview_status_t test_enqueue_command(const canview_uart_message_view_t *request,
                                             void *context)
{
    const bool *accept = (const bool *)context;
    return request != NULL && accept != NULL && *accept ? CANVIEW_OK : CANVIEW_RESOURCE_BUSY;
}

static int test_link(void)
{
    canview_uart_link_t link;
    bool changed = true;
    CHECK(canview_uart_link_reset(&link) == CANVIEW_OK);
    CHECK(link_complete_for_test(&link, 1U, 0U) == 0);
    CHECK(link.state == CANVIEW_UART_LINK_ONLINE);
    CHECK(canview_uart_link_command_admission_allowed(&link, 50U));
    CHECK(canview_uart_link_tick(&link, 300U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_SUSPECT &&
          !canview_uart_link_command_admission_allowed(&link, 300U));
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 301U, &changed) == CANVIEW_OK);
    CHECK(canview_uart_link_note_safety_snapshot(&link, 1U, 400U) == CANVIEW_OK);
    CHECK(canview_uart_link_set_cts_blocked(&link, true, 400U) == CANVIEW_OK);
    CHECK(canview_uart_link_command_admission_allowed(&link, 499U));
    CHECK(canview_uart_link_tick(&link, 500U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_SUSPECT &&
          !canview_uart_link_command_admission_allowed(&link, 500U));
    CHECK(canview_uart_link_set_cts_blocked(&link, false, 501U) == CANVIEW_OK);
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 600U, &changed) == CANVIEW_OK);
    CHECK(link.state == CANVIEW_UART_LINK_ONLINE);
    CHECK(canview_uart_link_set_cts_blocked(&link, true, 700U) == CANVIEW_OK);
    CHECK(canview_uart_link_tick(&link, 1700U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_OFFLINE);
    CHECK(test_link_note_heartbeat_with_revision(&link, 1U, 1U, 1701U, &changed) == CANVIEW_OK);
    CHECK(!link.hello_complete && !changed);
    CHECK(!canview_uart_link_command_admission_allowed(&link, 1701U));
    CHECK(link_complete_for_test(&link, 1U, 1800U) == 0);
    CHECK(link.state == CANVIEW_UART_LINK_ONLINE);
    CHECK(test_link_note_heartbeat_with_revision(&link, 1U, 1U, 2800U, &changed) ==
          CANVIEW_TIMEOUT);
    CHECK(!link.hello_complete && !canview_uart_link_command_admission_allowed(&link, 2800U));

    canview_uart_link_t revision_link;
    CHECK(canview_uart_link_reset(&revision_link) == CANVIEW_OK);
    CHECK(link_complete_for_test(&revision_link, 7U, 0U) == 0);
    CHECK(test_link_note_heartbeat_with_revision(&revision_link, 7U, 2U, 100U, &changed) ==
          CANVIEW_OK);
    CHECK(!revision_link.safety_snapshot_valid &&
          !canview_uart_link_command_admission_allowed(&revision_link, 100U));
    CHECK(test_link_note_safety_snapshot_with_revision(&revision_link, 7U, 2U, 100U) ==
          CANVIEW_OK);
    CHECK(test_link_note_heartbeat_with_revision(&revision_link, 7U, 2U, 100U, &changed) ==
          CANVIEW_OK);
    CHECK(revision_link.state == CANVIEW_UART_LINK_ONLINE &&
          canview_uart_link_command_admission_allowed(&revision_link, 100U));
    return 0;
}

static int test_link_boundaries(void)
{
    canview_uart_link_t link;
    bool changed = false;
    CHECK(canview_uart_link_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_note_hello(NULL, 1U, 0U, &changed) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_note_hello(&link, 0U, 0U, &changed) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_note_hello(&link, 1U, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_note_hello_ack(NULL, 1U, CANVIEW_UART_PROTOCOL_MAJOR,
                                           CANVIEW_UART_PROTOCOL_MINOR, 0U, 0U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_note_safety_snapshot(NULL, 1U, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_note_heartbeat(NULL, 1U, 0U, &changed) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_note_heartbeat(&link, 0U, 0U, &changed) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_set_cts_blocked(NULL, true, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_tick(NULL, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(!canview_uart_link_command_admission_allowed(NULL, 0U));

    CHECK(canview_uart_link_reset(&link) == CANVIEW_OK);
    CHECK(canview_uart_link_tick(&link, 0U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_OFFLINE);
    CHECK(!canview_uart_link_command_admission_allowed(&link, 0U));
    CHECK(canview_uart_link_note_hello(&link, 1U, 10U, &changed) == CANVIEW_OK);
    CHECK(canview_uart_link_note_hello_ack(&link, 1U, CANVIEW_UART_PROTOCOL_MAJOR + 1U,
                                           CANVIEW_UART_PROTOCOL_MINOR, 0U, 10U) ==
          CANVIEW_UNSUPPORTED_VERSION);
    CHECK(canview_uart_link_note_hello_ack(&link, 1U, CANVIEW_UART_PROTOCOL_MAJOR,
                                           CANVIEW_UART_PROTOCOL_MINOR, 0U, 10U) == CANVIEW_OK);
    CHECK(canview_uart_link_note_safety_snapshot(&link, 1U, 10U) == CANVIEW_OK);
    CHECK(!canview_uart_link_command_admission_allowed(&link, 10U));
    CHECK(canview_uart_link_set_cts_blocked(&link, false, 10U) == CANVIEW_OK);
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 10U, &changed) == CANVIEW_OK);
    CHECK(canview_uart_link_command_admission_allowed(&link, 10U));
    CHECK(canview_uart_link_tick(&link, 1011U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_OFFLINE && !link.hello_complete);
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 1012U, &changed) == CANVIEW_OK &&
          !link.hello_complete);

    CHECK(canview_uart_link_reset(&link) == CANVIEW_OK);
    CHECK(link_complete_for_test(&link, 1U, 100U) == 0);
    CHECK(canview_uart_link_note_hello(&link, 2U, 200U, &changed) == CANVIEW_OK && changed);
    CHECK(!link.hello_ack_complete && !link.safety_snapshot_valid && !link.cts_known);
    CHECK(canview_uart_link_note_hello_ack(&link, 2U, CANVIEW_UART_PROTOCOL_MAJOR,
                                           CANVIEW_UART_PROTOCOL_MINOR, 0U, 200U) == CANVIEW_OK);
    CHECK(canview_uart_link_note_safety_snapshot(&link, 2U, 200U) == CANVIEW_OK);
    CHECK(canview_uart_link_set_cts_blocked(&link, false, 200U) == CANVIEW_OK);
    CHECK(canview_uart_link_note_heartbeat(&link, 2U, 200U, &changed) == CANVIEW_OK);
    CHECK(link.state == CANVIEW_UART_LINK_ONLINE);

    uint8_t command_payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    canview_wire_view_t command_wire = {0};
    command_wire.header.message_type = CANVIEW_UART_MSG_COMMAND_REQUEST;
    command_wire.header.flags = flags_for(CANVIEW_UART_MSG_COMMAND_REQUEST);
    command_wire.payload = command_payload;
    command_wire.payload_size = fill_valid_payload(CANVIEW_UART_MSG_COMMAND_REQUEST,
                                                    command_payload, sizeof(command_payload), false);
    canview_uart_message_view_t command_view;
    CHECK(canview_uart_message_validate(&command_wire, &command_view) == CANVIEW_OK);
    bool authorized = true;
    canview_uart_command_admission_context_t admission = {
        .authorize = test_authorize_command, .context = &authorized};
    CHECK(canview_uart_command_admission_allowed(&link, &command_view, &admission, 200U));
    authorized = false;
    CHECK(!canview_uart_command_admission_allowed(&link, &command_view, &admission, 200U));
    CHECK(!canview_uart_command_admission_allowed(&link, &command_view, NULL, 200U));
    CHECK(!canview_uart_command_admission_allowed(&link, NULL, &admission, 200U));
    return 0;
}

static int test_session(void)
{
    canview_uart_link_t link;
    canview_uart_plan_context_t plan;
    canview_uart_command_cache_t cache = {0};
    canview_uart_replay_context_t replay = {0};
    bool boot_changed = true;
    uint8_t begin[32];
    uint8_t digest[CANVIEW_UART_COMMAND_DIGEST_SIZE] = {0x41U};
    canview_uart_command_key_t key = test_command_key(1U, 7U, digest);
    canview_uart_command_handle_t handle;
    canview_uart_command_handle_t ignored_handle;
    const uint8_t *stored = NULL;
    size_t stored_size = 0U;

    CHECK(canview_uart_session_reset(&link, &plan, &cache, &replay) == CANVIEW_OK);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    CHECK(TEST_PLAN_APPLY(&plan, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_command_cache_admit(&cache, &key, 10U, &handle) == CANVIEW_OK);
    CHECK(canview_uart_session_note_hello(&link, &plan, &cache, &replay, 1U, 20U, &boot_changed) ==
          CANVIEW_OK);
    CHECK(!boot_changed && !plan.pending && !link.hello_ack_complete);
    CHECK(canview_uart_command_cache_lookup(&cache, &key, 20U, &ignored_handle, &stored,
                                            &stored_size) == CANVIEW_STALE);

    build_plan_begin(begin, 2U, 1U, 0U, 1U, 0U);
    CHECK(TEST_PLAN_APPLY(&plan, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_command_cache_admit(&cache, &key, 25U, &handle) == CANVIEW_OK);
    replay.sequence.initialized = true;
    replay.sequence.newest = 200U;
    replay.sequence.seen = 1U;
    CHECK(canview_uart_session_note_hello(&link, &plan, &cache, &replay, 1U, 30U, &boot_changed) ==
          CANVIEW_OK);
    CHECK(!boot_changed && plan.pending && !link.hello_ack_complete);
    CHECK(replay.sequence.initialized && replay.sequence.newest == 200U);
    CHECK(canview_uart_command_cache_lookup(&cache, &key, 30U, &ignored_handle, &stored,
                                            &stored_size) == CANVIEW_DUPLICATE);

    key.request_token = 2U;
    CHECK(canview_uart_command_cache_admit(&cache, &key, 35U, &handle) == CANVIEW_OK);
    CHECK(canview_uart_session_note_heartbeat(&link, &plan, &cache, &replay, 1U, 40U,
                                              &boot_changed) ==
          CANVIEW_OK);
    CHECK(!boot_changed);
    CHECK(canview_uart_command_cache_lookup(&cache, &key, 40U, &ignored_handle, &stored,
                                            &stored_size) == CANVIEW_DUPLICATE);

    CHECK(canview_uart_plan_discard_pending(&plan) == CANVIEW_OK);
    build_plan_begin(begin, 3U, 1U, 0U, 1U, 0U);
    CHECK(TEST_PLAN_APPLY(&plan, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_session_note_heartbeat(&link, &plan, &cache, &replay, 2U, 50U,
                                              &boot_changed) ==
          CANVIEW_OK);
    CHECK(boot_changed && !plan.pending && !link.hello_complete);
    CHECK(!replay.sequence.initialized);
    CHECK(canview_uart_command_cache_lookup(&cache, &key, 50U, &ignored_handle, &stored,
                                            &stored_size) == CANVIEW_STALE);

    CHECK(canview_uart_link_note_hello(&link, 2U, 60U, &boot_changed) == CANVIEW_OK);
    CHECK(canview_uart_plan_reset(&plan) == CANVIEW_OK);
    CHECK(canview_uart_link_note_hello_ack(&link, 2U, CANVIEW_UART_PROTOCOL_MAJOR,
                                           CANVIEW_UART_PROTOCOL_MINOR, 0U, 60U) == CANVIEW_OK);
    CHECK(canview_uart_link_note_safety_snapshot(&link, 2U, 60U) == CANVIEW_OK);
    CHECK(canview_uart_link_set_cts_blocked(&link, false, 60U) == CANVIEW_OK);
    CHECK(test_link_note_heartbeat_with_revision(&link, 2U, 1U, 60U, &boot_changed) ==
          CANVIEW_OK);
    build_plan_begin(begin, 4U, 1U, 0U, 1U, 0U);
    CHECK(TEST_PLAN_APPLY(&plan, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    key.request_token = 4U;
    CHECK(canview_uart_command_cache_admit(&cache, &key, 70U, &handle) == CANVIEW_OK);
    CHECK(canview_uart_session_tick(&link, &plan, &cache, &replay, 1060U) == CANVIEW_OK);
    CHECK(!link.hello_complete && !plan.pending);
    CHECK(canview_uart_command_cache_lookup(&cache, &key, 1060U, &ignored_handle, &stored,
                                            &stored_size) == CANVIEW_STALE);

    CHECK(canview_uart_link_note_hello(&link, 2U, 1100U, &boot_changed) == CANVIEW_OK);
    CHECK(canview_uart_plan_reset(&plan) == CANVIEW_OK);
    build_plan_begin(begin, 5U, 1U, 0U, 1U, 0U);
    CHECK(TEST_PLAN_APPLY(&plan, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    cache.next_generation = UINT64_MAX;
    const uint64_t peer_boot_before = link.peer_boot_id;
    const bool pending_before = plan.pending;
    CHECK(canview_uart_session_note_hello(&link, &plan, &cache, &replay, 3U, 70U, &boot_changed) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(link.peer_boot_id == peer_boot_before && plan.pending == pending_before);
    CHECK(canview_uart_session_note_hello(&link, &plan, &cache, &replay, 0U, 70U, &boot_changed) ==
          CANVIEW_INVALID_ARGUMENT);
    return 0;
}

static int test_limits(void)
{
    canview_uart_codec_t codec;
    canview_uart_message_view_t view;
    CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
    CHECK(canview_uart_codec_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_codec_feed(&codec, 0U, &view) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_codec_feed(NULL, 0U, &view) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_codec_feed(&codec, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_message_policy(CANVIEW_UART_MSG_LINK_HELLO, NULL) == CANVIEW_INVALID_ARGUMENT);
    const canview_uart_message_policy_t *policy = NULL;
    CHECK(canview_uart_message_policy(UINT8_C(0x7F), &policy) == CANVIEW_UNSUPPORTED_MESSAGE &&
          policy == NULL);
    uint8_t payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    const size_t payload_size = fill_valid_payload(CANVIEW_UART_MSG_LINK_HELLO, payload,
                                                    sizeof(payload), false);
    uint8_t scratch[CANVIEW_UART_MAX_FRAME_SIZE];
    uint8_t serial[CANVIEW_UART_MAX_SERIAL_SIZE];
    size_t written = 0U;
    CHECK(canview_uart_message_encode(CANVIEW_UART_MSG_LINK_HELLO, 0U, 1U, 2U, 3U, payload,
                                      payload_size, scratch, sizeof(scratch), serial,
                                      sizeof(serial), NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_message_encode(CANVIEW_UART_MSG_LINK_HELLO, 0U, 1U, 2U, 3U, payload,
                                      payload_size, NULL, sizeof(scratch), serial,
                                      sizeof(serial), &written) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_message_encode(CANVIEW_UART_MSG_LINK_HELLO, 0U, 1U, 2U, 3U, payload,
                                      payload_size, scratch, sizeof(scratch), NULL,
                                      sizeof(serial), &written) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_message_encode(CANVIEW_UART_MSG_LINK_HELLO, 0U, 1U, 2U, 3U, NULL, 1U,
                                      scratch, sizeof(scratch), serial, sizeof(serial),
                                      &written) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_message_encode(CANVIEW_UART_MSG_LINK_HELLO, 0U, 1U, 2U, 3U, payload,
                                      payload_size, scratch, sizeof(scratch), serial, 0U,
                                      &written) == CANVIEW_BUFFER_TOO_SMALL);
    CHECK(canview_uart_message_encode(CANVIEW_UART_MSG_LINK_HELLO, 0U, 1U, 2U, 3U, payload,
                                      payload_size, scratch, 0U, serial, sizeof(serial),
                                      &written) == CANVIEW_BUFFER_TOO_SMALL);
    uint8_t unsupported_payload[36] = {0};
    put_le(unsupported_payload, 8U, 1U);
    CHECK(canview_uart_message_encode(CANVIEW_UART_MSG_FIRMWARE_PREPARE,
                                      CANVIEW_UART_FLAG_ACK_REQUIRED, 1U, 2U, 3U,
                                      unsupported_payload, sizeof(unsupported_payload), scratch,
                                      sizeof(scratch), serial, sizeof(serial), &written) ==
          CANVIEW_UNSUPPORTED_MESSAGE);
    uint8_t malformed_payload[28] = {0};
    put_le(malformed_payload, 8U, 1U);
    put_le(malformed_payload + 8U, 8U, 1U);
    malformed_payload[16U] = 5U;
    CHECK(canview_uart_message_encode(CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL,
                                      CANVIEW_UART_FLAG_ACK_REQUIRED, 1U, 2U, 3U,
                                      malformed_payload, sizeof(malformed_payload), scratch,
                                      sizeof(scratch), serial, sizeof(serial), &written) ==
          CANVIEW_MALFORMED);
    canview_wire_view_t wire = {0};
    wire.header.message_type = CANVIEW_UART_MSG_LINK_HELLO;
    wire.payload = NULL;
    wire.payload_size = payload_size;
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_MALFORMED);
    CHECK(canview_uart_plan_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    canview_uart_plan_context_t plan;
    CHECK(TEST_PLAN_APPLY(&plan, payload, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    return 0;
}

static int test_replay(void)
{
    uint8_t payload[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    canview_wire_view_t wire = {0};
    wire.header.message_type = CANVIEW_UART_MSG_COMMAND_REQUEST;
    wire.header.flags = flags_for(CANVIEW_UART_MSG_COMMAND_REQUEST);
    wire.header.sequence = 41U;
    wire.payload = payload;
    wire.payload_size = fill_valid_payload(CANVIEW_UART_MSG_COMMAND_REQUEST, payload,
                                           sizeof(payload), false);
    canview_uart_message_view_t view;
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_OK);

    bool authorized = true;
    const canview_uart_command_admission_context_t authorization = {
        .authorize = test_authorize_command, .context = &authorized};
    canview_uart_replay_context_t replay = {0};
    CHECK(canview_uart_replay_reset(&replay) == CANVIEW_OK);
    CHECK(canview_uart_message_admit(&view, CANVIEW_UART_ENDPOINT_STM32,
                                     CANVIEW_UART_FLOW_INBOUND, &replay, &authorization, 0U) ==
          CANVIEW_OK);
    CHECK(canview_uart_message_admit(&view, CANVIEW_UART_ENDPOINT_STM32,
                                     CANVIEW_UART_FLOW_INBOUND, &replay, &authorization, 1U) ==
          CANVIEW_DUPLICATE);
    CHECK(canview_uart_message_admit(&view, CANVIEW_UART_ENDPOINT_ESP32,
                                     CANVIEW_UART_FLOW_INBOUND, &replay, &authorization, 1U) ==
          CANVIEW_MALFORMED);

    wire.header.sequence = 42U;
    CHECK(canview_uart_message_validate(&wire, &view) == CANVIEW_OK);
    authorized = false;
    CHECK(canview_uart_message_admit(&view, CANVIEW_UART_ENDPOINT_STM32,
                                     CANVIEW_UART_FLOW_INBOUND, &replay, &authorization, 2U) ==
          CANVIEW_AUTH_FAILED);
    authorized = true;
    CHECK(canview_uart_message_admit(&view, CANVIEW_UART_ENDPOINT_STM32,
                                     CANVIEW_UART_FLOW_INBOUND, &replay, &authorization, 2U) ==
          CANVIEW_OK);

    canview_uart_message_view_t forged_view = view;
    forged_view.wire.header.flags |= UINT8_C(0x80);
    CHECK(canview_uart_message_admit(&forged_view, CANVIEW_UART_ENDPOINT_STM32,
                                     CANVIEW_UART_FLOW_INBOUND, &replay, &authorization, 3U) ==
          CANVIEW_MALFORMED);

    canview_uart_link_t link;
    CHECK(canview_uart_link_reset(&link) == CANVIEW_OK);
    CHECK(link_complete_for_test(&link, 9U, 0U) == 0);
    CHECK(!canview_uart_command_admission_allowed(&link, &forged_view, &authorization, 10U));
    canview_uart_replay_context_t dispatch_replay = {0};
    CHECK(canview_uart_replay_reset(&dispatch_replay) == CANVIEW_OK);
    bool queue_accept = false;
    CHECK(canview_uart_command_dispatch_admit(&link, &view, &authorization,
                                              test_enqueue_command, &queue_accept,
                                              &dispatch_replay, 10U) == CANVIEW_RESOURCE_BUSY);
    CHECK(!dispatch_replay.sequence.initialized);
    queue_accept = true;
    CHECK(canview_uart_command_dispatch_admit(&link, &view, &authorization,
                                              test_enqueue_command, &queue_accept,
                                              &dispatch_replay, 10U) == CANVIEW_OK);
    CHECK(canview_uart_command_dispatch_admit(&link, &view, &authorization,
                                              test_enqueue_command, &queue_accept,
                                              &dispatch_replay, 10U) == CANVIEW_DUPLICATE);
    CHECK(canview_uart_link_tick(&link, CANVIEW_UART_HEARTBEAT_ONLINE_MS) == CANVIEW_OK);
    CHECK(canview_uart_command_dispatch_admit(&link, &view, &authorization,
                                              test_enqueue_command, &queue_accept,
                                              &dispatch_replay,
                                              CANVIEW_UART_HEARTBEAT_ONLINE_MS) ==
          CANVIEW_TIMEOUT);
    return 0;
}

static int run_scenario(const char *scenario)
{
    if (strcmp(scenario, "matrix") == 0)
    {
        return test_message_matrix();
    }
    if (strcmp(scenario, "malformed") == 0)
    {
        return test_malformed();
    }
    if (strcmp(scenario, "stream") == 0)
    {
        return test_stream();
    }
    if (strcmp(scenario, "long-stream") == 0)
    {
        return test_long_stream();
    }
    if (strcmp(scenario, "soak-smoke") == 0)
    {
        return test_byte_soak(1U);
    }
    if (strcmp(scenario, "soak-24h") == 0)
    {
        return test_byte_soak(86400U);
    }
    if (strcmp(scenario, "direction") == 0)
    {
        return test_direction();
    }
    if (strcmp(scenario, "plan") == 0)
    {
        return test_plan();
    }
    if (strcmp(scenario, "plan-boundaries") == 0)
    {
        return test_plan_boundaries();
    }
    if (strcmp(scenario, "command") == 0)
    {
        return test_command_cache();
    }
    if (strcmp(scenario, "command-boundaries") == 0)
    {
        return test_command_boundaries();
    }
    if (strcmp(scenario, "link") == 0)
    {
        return test_link();
    }
    if (strcmp(scenario, "link-boundaries") == 0)
    {
        return test_link_boundaries();
    }
    if (strcmp(scenario, "session") == 0)
    {
        return test_session();
    }
    if (strcmp(scenario, "replay") == 0)
    {
        return test_replay();
    }
    if (strcmp(scenario, "limits") == 0)
    {
        return test_limits();
    }
    if (strcmp(scenario, "all") == 0)
    {
        return test_message_matrix() || test_direction() || test_malformed() || test_stream() ||
               test_long_stream() || test_byte_soak(1U) || test_plan() ||
               test_plan_boundaries() ||
               test_command_cache() || test_command_boundaries() || test_link() ||
               test_link_boundaries() || test_session() || test_replay() || test_limits();
    }
    (void)fprintf(stderr, "unknown UART scenario: %s\n", scenario);
    return 2;
}

int main(int argc, char **argv)
{
    return argc == 2 ? run_scenario(argv[1]) : run_scenario("all");
}
