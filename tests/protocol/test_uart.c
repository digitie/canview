/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_uart.h"

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

static void put_le(uint8_t *bytes, size_t width, uint64_t value)
{
    for (size_t index = 0U; index < width; ++index)
    {
        bytes[index] = (uint8_t)(value >> (index * 8U));
    }
}

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
        put_le(payload + 52U, 2U, maximum ? 136U : 0U);
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
            CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
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
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_TX_AUDIT, 18U, 2U, 0x10U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_TX_AUDIT, 11U, 1U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_TX_AUDIT, 36U, 4U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL, 0U, 8U, 0U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL, 16U, 1U, 0U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL, 17U, 1U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL, 24U, 4U, 1U) == 0);
    CHECK(expect_fixed_malformed(CANVIEW_UART_MSG_CAN_EVENT_MARKER, 22U, 2U, 1U) == 0);
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
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    size_t chunk_size = build_plan_chunk(chunk, 1U, 1U, 0U, 2U, 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    memset(commit, 0, sizeof(commit));
    commit[0] = CANVIEW_UART_PLAN_OP_COMMIT;
    put_le(commit + 4U, 8U, 1U);
    put_le(commit + 12U, 4U, 1U);
    CHECK(canview_uart_plan_apply(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 1U, 1U, 1U, 2U, 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    for (size_t index = 0U; index < 32U; ++index)
    {
        commit[16U + index] = (uint8_t)(index + 1U);
    }
    CHECK(canview_uart_plan_apply(&context, commit, sizeof(commit)) == CANVIEW_OK);
    canview_uart_observer_plan_t active;
    CHECK(canview_uart_plan_current(&context, &active) == CANVIEW_OK);
    CHECK(active.revision == 1U && active.filter_count == 2U && active.filters[1].can_id == 1U);

    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    build_plan_begin(begin, 2U, 2U, 0U, 2U, 1U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    chunk_size = build_plan_chunk(chunk, 2U, 2U, 1U, 2U, 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    CHECK(canview_uart_plan_current(&context, &active) == CANVIEW_OK && active.revision == 0U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_RESOURCE_BUSY);
    memset(chunk, 0, sizeof(chunk));
    chunk[0] = CANVIEW_UART_PLAN_OP_CHUNK;
    put_le(chunk + 4U, 8U, 2U);
    put_le(chunk + 12U, 4U, 2U);
    put_le(chunk + 16U, 2U, 0U);
    put_le(chunk + 18U, 2U, 2U);
    chunk[20] = 1U;
    CHECK(canview_uart_plan_apply(&context, chunk, 36U) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_plan_apply(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_RESOURCE_BUSY);
    build_plan_begin(begin, 3U, 3U, 0U, 1U, 0U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_RESOURCE_BUSY);
    CHECK(canview_uart_plan_apply(&context, chunk, 36U) == CANVIEW_MALFORMED);

    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    build_plan_begin(begin, 4U, 4U, 0U, 1U, 0U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    uint8_t abort_payload[20] = {0};
    abort_payload[0] = CANVIEW_UART_PLAN_OP_ABORT;
    put_le(abort_payload + 4U, 8U, 4U);
    put_le(abort_payload + 12U, 4U, 4U);
    CHECK(canview_uart_plan_apply(&context, abort_payload, sizeof(abort_payload)) == CANVIEW_OK);
    CHECK(canview_uart_plan_current(&context, &active) == CANVIEW_OK && active.revision == 0U);
    return 0;
}

static int test_plan_boundaries(void)
{
    canview_uart_plan_context_t context;
    uint8_t begin[32];
    uint8_t chunk[CANVIEW_UART_MAX_PAYLOAD_SIZE];
    uint8_t commit[48] = {0};
    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    CHECK(canview_uart_plan_apply(NULL, begin, sizeof(begin)) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_plan_apply(&context, NULL, sizeof(begin)) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_plan_current(NULL, NULL) == CANVIEW_INVALID_ARGUMENT);

    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    begin[1U] = 1U;
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 0U, 1U, 0U, 1U, 0U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 0U, 0U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, CANVIEW_UART_PLAN_MAX_CHUNKS + 1U, 0U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, CANVIEW_UART_PLAN_MAX_FILTERS + 1U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    begin[30U] = 8U;
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 0U);
    begin[31U] = 1U;
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    build_plan_begin(begin, 1U, 1U, 0U, 1U, 17U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_MALFORMED);
    CHECK(canview_uart_plan_apply(&context, begin, 31U) == CANVIEW_MALFORMED);

    context.active.revision = 7U;
    build_plan_begin(begin, 2U, 8U, 0U, 1U, 0U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_STALE);
    CHECK(!context.pending);
    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);

    build_plan_begin(begin, 3U, 1U, 0U, 1U, 1U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    size_t chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[1U] = 1U;
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 0U, 1U, 0U, 1U, 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 0U, 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, CANVIEW_UART_PLAN_MAX_CHUNKS + 1U, 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 1U, 1U, 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U,
                                  CANVIEW_UART_PLAN_CHUNK_MAX_FILTERS + 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[21U] = 1U;
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[24U] = CANVIEW_WIRE_CAN_BUS_COUNT;
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[25U] = 0x10U;
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[26U] = 1U;
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    put_le(chunk + 28U, 4U, CANVIEW_WIRE_CAN_STANDARD_ID_MAX + 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[24U + 2U] = 1U;
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    chunk[24U + 1U] = 1U;
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    build_plan_begin(begin, 3U, 1U, 0U, 1U, 1U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    chunk_size = build_plan_chunk(chunk, 3U, 1U, 0U, 1U, 1U);
    put_le(chunk + 24U + 4U, 4U, CANVIEW_WIRE_CAN_EXTENDED_ID_MAX);
    chunk[24U + 1U] = 1U;
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);

    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    build_plan_begin(begin, 6U, 1U, 0U, 2U, 1U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    chunk_size = build_plan_chunk(chunk, 6U, 1U, 0U, 2U, 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    chunk_size = build_plan_chunk(chunk, 6U, 1U, 1U, 2U, 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);

    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    build_plan_begin(begin, 4U, 1U, 0U, 1U, 1U);
    CHECK(canview_uart_plan_apply(&context, begin, sizeof(begin)) == CANVIEW_INCOMPLETE);
    chunk_size = build_plan_chunk(chunk, 4U, 1U, 0U, 1U, 1U);
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_INCOMPLETE);
    chunk[20U] = 1U;
    CHECK(canview_uart_plan_apply(&context, chunk, chunk_size) == CANVIEW_MALFORMED);
    memset(commit, 0, sizeof(commit));
    commit[0] = CANVIEW_UART_PLAN_OP_COMMIT;
    put_le(commit + 4U, 8U, 4U);
    put_le(commit + 12U, 4U, 1U);
    commit[1U] = 1U;
    CHECK(canview_uart_plan_apply(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);
    commit[1U] = 0U;
    CHECK(canview_uart_plan_apply(&context, commit, 47U) == CANVIEW_MALFORMED);
    CHECK(canview_uart_plan_apply(&context, commit, sizeof(commit)) == CANVIEW_OK);

    uint8_t unknown[20] = {0};
    CHECK(canview_uart_plan_apply(&context, unknown, sizeof(unknown)) ==
          CANVIEW_UNSUPPORTED_MESSAGE);
    CHECK(canview_uart_plan_apply(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);

    CHECK(canview_uart_plan_reset(&context) == CANVIEW_OK);
    memset(commit, 0, sizeof(commit));
    commit[0] = CANVIEW_UART_PLAN_OP_COMMIT;
    put_le(commit + 4U, 8U, 5U);
    put_le(commit + 12U, 4U, 1U);
    CHECK(canview_uart_plan_apply(&context, commit, sizeof(commit)) == CANVIEW_MALFORMED);
    uint8_t abort_payload[20] = {0};
    abort_payload[0] = CANVIEW_UART_PLAN_OP_ABORT;
    put_le(abort_payload + 4U, 8U, 5U);
    put_le(abort_payload + 12U, 4U, 1U);
    CHECK(canview_uart_plan_apply(&context, abort_payload, sizeof(abort_payload)) ==
          CANVIEW_MALFORMED);
    abort_payload[18U] = 1U;
    CHECK(canview_uart_plan_apply(&context, abort_payload, sizeof(abort_payload)) ==
          CANVIEW_MALFORMED);
    return 0;
}

static int test_command_cache(void)
{
    canview_uart_command_cache_t cache;
    uint8_t digest[CANVIEW_UART_COMMAND_DIGEST_SIZE];
    uint8_t changed[CANVIEW_UART_COMMAND_DIGEST_SIZE];
    uint8_t result[CANVIEW_UART_COMMAND_RESULT_MAX];
    memset(digest, 0x11, sizeof(digest));
    memset(changed, 0x22, sizeof(changed));
    memset(result, 0xA5, sizeof(result));
    CHECK(canview_uart_command_cache_reset(&cache) == CANVIEW_OK);
    size_t entry = 0U;
    CHECK(canview_uart_command_cache_admit(&cache, 1U, 7U, digest, 100U, &entry) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_record_result(&cache, entry, result, sizeof(result), 101U) ==
          CANVIEW_OK);
    const uint8_t *stored = NULL;
    size_t stored_size = 0U;
    size_t duplicate_entry = 0U;
    CHECK(canview_uart_command_cache_lookup(&cache, 1U, 7U, digest, 102U, &duplicate_entry,
                                            &stored, &stored_size) == CANVIEW_DUPLICATE);
    CHECK(duplicate_entry == entry && stored != NULL && stored_size == sizeof(result) &&
          memcmp(stored, result, sizeof(result)) == 0);
    CHECK(canview_uart_command_cache_mark_ack(&cache, entry) == CANVIEW_OK);
    CHECK(cache.entries[entry].terminal && cache.entries[entry].acknowledged);
    CHECK(canview_uart_command_cache_admit(&cache, 1U, 7U, digest, 103U, &duplicate_entry) ==
          CANVIEW_DUPLICATE);
    CHECK(canview_uart_command_cache_admit(&cache, 1U, 7U, changed, 103U, &duplicate_entry) ==
          CANVIEW_MALFORMED);
    CHECK(canview_uart_command_cache_lookup(&cache, 1U, 7U, changed, 103U, &duplicate_entry,
                                            &stored, &stored_size) == CANVIEW_MALFORMED);

    for (uint64_t token = 2U; token <= CANVIEW_UART_COMMAND_CACHE_CAPACITY; ++token)
    {
        CHECK(canview_uart_command_cache_admit(&cache, token, 7U, digest, 100U, &entry) ==
              CANVIEW_OK);
    }
    CHECK(canview_uart_command_cache_admit(&cache, 257U, 7U, digest, 100U, &entry) ==
          CANVIEW_RESOURCE_BUSY);
    CHECK(canview_uart_command_cache_expire(&cache, 60100U) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_admit(&cache, 257U, 7U, digest, 60101U, &entry) ==
          CANVIEW_OK);
    CHECK(canview_uart_command_cache_record_result(&cache, entry, result, sizeof(result),
                                                   60101U) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_lookup(&cache, 257U, 7U, digest, 60102U, &entry, &stored,
                                            &stored_size) == CANVIEW_DUPLICATE &&
          stored != NULL && stored_size == sizeof(result));
    CHECK(canview_uart_command_cache_expire(&cache, 120101U) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_lookup(&cache, 257U, 7U, digest, 120102U, &entry, &stored,
                                            &stored_size) == CANVIEW_STALE);
    return 0;
}

static int test_command_boundaries(void)
{
    canview_uart_command_cache_t cache;
    uint8_t digest[CANVIEW_UART_COMMAND_DIGEST_SIZE];
    uint8_t result[CANVIEW_UART_COMMAND_RESULT_MAX];
    memset(digest, 0x31, sizeof(digest));
    memset(result, 0xC3, sizeof(result));
    size_t entry = 0U;
    size_t ignored_entry = 0U;
    const uint8_t *stored = NULL;
    size_t stored_size = 0U;

    CHECK(canview_uart_command_cache_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_reset(&cache) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_admit(NULL, 1U, 1U, digest, 0U, &entry) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_admit(&cache, 0U, 1U, digest, 0U, &entry) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_admit(&cache, 1U, 1U, NULL, 0U, &entry) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_admit(&cache, 1U, 1U, digest, 0U, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_admit(&cache, 1U, 1U, digest, 0U, &entry) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_lookup(&cache, 1U, 1U, digest, 1U, &ignored_entry,
                                            &stored, &stored_size) == CANVIEW_DUPLICATE);
    CHECK(stored == NULL && stored_size == 0U);
    CHECK(canview_uart_command_cache_record_result(&cache, entry, result, sizeof(result), 2U) ==
          CANVIEW_OK);
    CHECK(canview_uart_command_cache_record_result(&cache, entry, NULL, 1U, 2U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_record_result(&cache, entry, result, 0U, 2U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_record_result(&cache, entry, result,
                                                   CANVIEW_UART_COMMAND_RESULT_MAX + 1U, 2U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_mark_ack(&cache, CANVIEW_UART_COMMAND_CACHE_CAPACITY) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_mark_ack(NULL, entry) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_mark_ack(&cache, entry) == CANVIEW_OK);

    CHECK(canview_uart_command_cache_lookup(NULL, 1U, 1U, digest, 2U, &ignored_entry, &stored,
                                            &stored_size) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_lookup(&cache, 0U, 1U, digest, 2U, &ignored_entry, &stored,
                                            &stored_size) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_lookup(&cache, 1U, 1U, NULL, 2U, &ignored_entry, &stored,
                                            &stored_size) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_lookup(&cache, 1U, 1U, digest, 2U, NULL, &stored,
                                            &stored_size) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_lookup(&cache, 1U, 1U, digest, 2U, &ignored_entry, NULL,
                                            &stored_size) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_lookup(&cache, 1U, 1U, digest, 2U, &ignored_entry, &stored,
                                            NULL) == CANVIEW_INVALID_ARGUMENT);

    CHECK(canview_uart_command_cache_reset(&cache) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_mark_ack(&cache, 0U) == CANVIEW_STALE);
    CHECK(canview_uart_command_cache_record_result(&cache, 0U, result, sizeof(result), 2U) ==
          CANVIEW_STALE);
    CHECK(canview_uart_command_cache_record_result(NULL, 0U, result, sizeof(result), 2U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_record_result(&cache,
                                                   CANVIEW_UART_COMMAND_CACHE_CAPACITY, result,
                                                   sizeof(result), 2U) == CANVIEW_INVALID_ARGUMENT);

    CHECK(canview_uart_command_cache_admit(&cache, 2U, 1U, digest, 10U, &entry) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_record_result(&cache, entry, result, sizeof(result), 60010U) ==
          CANVIEW_STALE);
    CHECK(canview_uart_command_cache_admit(&cache, 3U, 1U, digest, 60010U, &ignored_entry) ==
          CANVIEW_OK);
    CHECK(canview_uart_command_cache_admit(&cache, 4U, 1U, digest, 60010U, &ignored_entry) ==
          CANVIEW_OK);
    CHECK(canview_uart_command_cache_lookup(&cache, 3U, 1U, digest, 60011U, &ignored_entry,
                                            &stored, &stored_size) == CANVIEW_DUPLICATE);
    CHECK(canview_uart_command_cache_lookup(&cache, 2U, 1U, digest, 60010U, &ignored_entry,
                                            &stored, &stored_size) == CANVIEW_STALE);
    CHECK(canview_uart_command_cache_expire(NULL, 0U) == CANVIEW_INVALID_ARGUMENT);

    CHECK(canview_uart_command_cache_reset(&cache) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_admit(&cache, 5U, 1U, digest, UINT64_MAX - 1U, &entry) ==
          CANVIEW_OK);
    CHECK(cache.entries[entry].expires_at_ms == UINT64_MAX);
    CHECK(canview_uart_command_cache_record_result(&cache, entry, result, sizeof(result),
                                                   UINT64_MAX - 1U) == CANVIEW_OK);
    CHECK(canview_uart_command_cache_lookup(&cache, 5U, 1U, digest, UINT64_MAX, &ignored_entry,
                                            &stored, &stored_size) == CANVIEW_STALE);
    return 0;
}

static int test_link(void)
{
    canview_uart_link_t link;
    bool changed = true;
    CHECK(canview_uart_link_reset(&link) == CANVIEW_OK);
    CHECK(link.state == CANVIEW_UART_LINK_OFFLINE);
    CHECK(canview_uart_link_note_hello(&link, 1U, 0U, &changed) == CANVIEW_OK && !changed);
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 0U, &changed) == CANVIEW_OK && !changed);
    CHECK(link.state == CANVIEW_UART_LINK_ONLINE);
    CHECK(canview_uart_link_command_admission_allowed(&link, 50U));
    CHECK(canview_uart_link_tick(&link, 300U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_SUSPECT &&
          !canview_uart_link_command_admission_allowed(&link, 300U));
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 301U, &changed) == CANVIEW_OK);
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
    CHECK(canview_uart_link_note_heartbeat(&link, 2U, 1701U, &changed) == CANVIEW_OK && changed);
    CHECK(link.state == CANVIEW_UART_LINK_OFFLINE);
    CHECK(!canview_uart_link_command_admission_allowed(&link, 1701U));
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
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 10U, &changed) == CANVIEW_OK && !changed);
    CHECK(link.state == CANVIEW_UART_LINK_OFFLINE && !link.hello_complete);
    CHECK(canview_uart_link_note_hello(&link, 1U, 20U, &changed) == CANVIEW_OK && !changed);
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 20U, &changed) == CANVIEW_OK && !changed);
    CHECK(link.state == CANVIEW_UART_LINK_ONLINE);
    CHECK(canview_uart_link_note_hello(&link, 1U, 30U, &changed) == CANVIEW_OK && !changed);
    CHECK(canview_uart_link_note_heartbeat(&link, 2U, 31U, &changed) == CANVIEW_OK && changed);
    CHECK(link.state == CANVIEW_UART_LINK_OFFLINE && !link.hello_complete);

    CHECK(canview_uart_link_reset(&link) == CANVIEW_OK);
    CHECK(canview_uart_link_note_hello(&link, 1U, 100U, &changed) == CANVIEW_OK);
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 100U, &changed) == CANVIEW_OK);
    CHECK(canview_uart_link_tick(&link, 50U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_OFFLINE);

    CHECK(canview_uart_link_reset(&link) == CANVIEW_OK);
    CHECK(canview_uart_link_note_hello(&link, 1U, 0U, &changed) == CANVIEW_OK);
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 0U, &changed) == CANVIEW_OK);
    CHECK(canview_uart_link_set_cts_blocked(&link, true, 10U) == CANVIEW_OK);
    CHECK(link.state == CANVIEW_UART_LINK_ONLINE);
    CHECK(canview_uart_link_tick(&link, 109U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_ONLINE &&
          canview_uart_link_command_admission_allowed(&link, 109U));
    CHECK(canview_uart_link_tick(&link, 110U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_SUSPECT &&
          !canview_uart_link_command_admission_allowed(&link, 110U));
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 200U, &changed) == CANVIEW_OK);
    CHECK(canview_uart_link_tick(&link, 1000U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_SUSPECT);
    CHECK(canview_uart_link_note_heartbeat(&link, 1U, 1000U, &changed) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_SUSPECT);
    CHECK(canview_uart_link_tick(&link, 1011U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_OFFLINE);
    CHECK(canview_uart_link_set_cts_blocked(&link, false, 1012U) == CANVIEW_OK &&
          link.state == CANVIEW_UART_LINK_ONLINE &&
          canview_uart_link_command_admission_allowed(&link, 1012U));
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
    CHECK(canview_uart_plan_apply(&plan, payload, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_command_cache_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_link_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
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
    if (strcmp(scenario, "limits") == 0)
    {
        return test_limits();
    }
    if (strcmp(scenario, "all") == 0)
    {
        return test_message_matrix() || test_malformed() || test_stream() || test_plan() ||
               test_plan_boundaries() ||
               test_command_cache() || test_command_boundaries() || test_link() ||
               test_link_boundaries() || test_limits();
    }
    (void)fprintf(stderr, "unknown UART scenario: %s\n", scenario);
    return 2;
}

int main(int argc, char **argv)
{
    return argc == 2 ? run_scenario(argv[1]) : run_scenario("all");
}
