/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_uart.h"

#include <string.h>

#define UART_CAN_FLAGS_MASK (UINT8_C(0x0F))
#define UART_CAN_IDE (UINT8_C(0x01))
#define UART_CAN_EXTENDED_ID_MAX UINT32_C(0x1FFFFFFF)
#define UART_CAN_STANDARD_ID_MAX UINT32_C(0x7FF)
#define UART_PLAN_HEADER_SIZE (24U)
#define UART_PLAN_BEGIN_SIZE (32U)
#define UART_PLAN_COMMIT_SIZE (48U)
#define UART_PLAN_ABORT_SIZE (20U)
#define UART_PLAN_FILTER_SIZE (12U)
#define UART_COMMAND_PREFIX_SIZE (104U)
#define UART_COMMAND_SUFFIX_MAX (136U)
#define UART_COMMAND_CACHE_MAGIC UINT32_C(0x43564348)
#define UART_PLAN_DIGEST_DOMAIN_SIZE (20U)
#define UART_PLAN_CANONICAL_SIZE \
    (UART_PLAN_DIGEST_DOMAIN_SIZE + CANVIEW_UART_PLAN_MAX_FILTERS * UART_PLAN_FILTER_SIZE)

static uint64_t read_le(const uint8_t *bytes, size_t width)
{
    uint64_t value = 0U;
    for (size_t index = 0U; index < width; ++index)
    {
        value |= (uint64_t)bytes[index] << (index * 8U);
    }
    return value;
}

static void write_le(uint8_t *bytes, size_t width, uint64_t value)
{
    for (size_t index = 0U; index < width; ++index)
    {
        bytes[index] = (uint8_t)(value >> (index * 8U));
    }
}

static bool zero_range(const uint8_t *bytes, size_t size, size_t offset, size_t length)
{
    if (bytes == NULL || offset > size || length > size - offset)
    {
        return false;
    }
    for (size_t index = 0U; index < length; ++index)
    {
        if (bytes[offset + index] != 0U)
        {
            return false;
        }
    }
    return true;
}

static bool nonzero_range(const uint8_t *bytes, size_t size, size_t offset, size_t length)
{
    if (bytes == NULL || offset > size || length > size - offset)
    {
        return false;
    }
    for (size_t index = 0U; index < length; ++index)
    {
        if (bytes[offset + index] != 0U)
        {
            return true;
        }
    }
    return false;
}

static void increment_counter(uint32_t *counter)
{
    if (*counter != UINT32_MAX)
    {
        *counter += UINT32_C(1);
    }
}

static bool valid_can_id(uint32_t can_id, uint8_t flags)
{
    if ((flags & (uint8_t)~UART_CAN_FLAGS_MASK) != 0U)
    {
        return false;
    }
    return can_id <= ((flags & UART_CAN_IDE) != 0U ? UART_CAN_EXTENDED_ID_MAX
                                                  : UART_CAN_STANDARD_ID_MAX);
}

static const canview_uart_message_policy_t *find_policy(uint8_t message_type)
{
    for (size_t index = 0U; index < CANVIEW_UART_MESSAGE_POLICY_COUNT; ++index)
    {
        if (CANVIEW_UART_MESSAGE_POLICIES[index].message_type == message_type)
        {
            return &CANVIEW_UART_MESSAGE_POLICIES[index];
        }
    }
    return NULL;
}

canview_status_t canview_uart_message_policy(uint8_t message_type,
                                             const canview_uart_message_policy_t **policy)
{
    if (policy == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *policy = NULL;
    const canview_uart_message_policy_t *found = find_policy(message_type);
    if (found == NULL)
    {
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
    *policy = found;
    return CANVIEW_OK;
}

static canview_status_t validate_plan_packet(const uint8_t *payload, size_t size)
{
    if (payload == NULL || size == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (!zero_range(payload, size, 1U, 3U))
    {
        return CANVIEW_MALFORMED;
    }
    const uint8_t operation = payload[0];
    if (operation == CANVIEW_UART_PLAN_OP_BEGIN)
    {
        if (size != UART_PLAN_BEGIN_SIZE || read_le(payload + 4U, 8U) == 0U ||
            read_le(payload + 20U, 2U) == 0U ||
            read_le(payload + 20U, 2U) > CANVIEW_UART_PLAN_MAX_CHUNKS ||
            read_le(payload + 22U, 2U) > CANVIEW_UART_PLAN_MAX_FILTERS ||
            read_le(payload + 24U, 2U) < CANVIEW_UART_PLAN_MIN_RECORDS_PER_SECOND ||
            read_le(payload + 24U, 2U) > CANVIEW_UART_PLAN_MAX_RECORDS_PER_SECOND ||
            read_le(payload + 26U, 4U) < CANVIEW_UART_PLAN_MIN_BYTES_PER_SECOND ||
            read_le(payload + 26U, 4U) > CANVIEW_UART_PLAN_MAX_BYTES_PER_SECOND ||
            (payload[30] & (uint8_t)~UINT8_C(0x07)) != 0U || !zero_range(payload, size, 31U, 1U))
        {
            return CANVIEW_MALFORMED;
        }
        const size_t minimum_chunks =
            ((size_t)read_le(payload + 22U, 2U) + CANVIEW_UART_PLAN_CHUNK_MAX_FILTERS - 1U) /
            CANVIEW_UART_PLAN_CHUNK_MAX_FILTERS;
        if ((size_t)read_le(payload + 20U, 2U) < minimum_chunks)
        {
            return CANVIEW_MALFORMED;
        }
        return CANVIEW_OK;
    }
    if (operation == CANVIEW_UART_PLAN_OP_CHUNK)
    {
        if (size < UART_PLAN_HEADER_SIZE || read_le(payload + 4U, 8U) == 0U ||
            read_le(payload + 18U, 2U) == 0U ||
            read_le(payload + 18U, 2U) > CANVIEW_UART_PLAN_MAX_CHUNKS ||
            read_le(payload + 16U, 2U) >= read_le(payload + 18U, 2U) || payload[20] >
                CANVIEW_UART_PLAN_CHUNK_MAX_FILTERS ||
            size != UART_PLAN_HEADER_SIZE + (size_t)payload[20] * UART_PLAN_FILTER_SIZE ||
            !zero_range(payload, size, 21U, 3U))
        {
            return CANVIEW_MALFORMED;
        }
        for (size_t index = 0U; index < payload[20]; ++index)
        {
            const uint8_t *record = payload + UART_PLAN_HEADER_SIZE + index * UART_PLAN_FILTER_SIZE;
            const uint8_t flags = record[1];
            if (record[0] >= CANVIEW_WIRE_CAN_BUS_COUNT ||
                (flags & (uint8_t)~UART_CAN_FLAGS_MASK) != 0U ||
                !zero_range(record, UART_PLAN_FILTER_SIZE, 2U, 2U) ||
                !valid_can_id((uint32_t)read_le(record + 4U, 4U), flags))
            {
                return CANVIEW_MALFORMED;
            }
        }
        return CANVIEW_OK;
    }
    if (operation == CANVIEW_UART_PLAN_OP_COMMIT)
    {
        return size == UART_PLAN_COMMIT_SIZE && read_le(payload + 4U, 8U) != 0U &&
                       zero_range(payload, size, 1U, 3U)
                   ? CANVIEW_OK
                   : CANVIEW_MALFORMED;
    }
    if (operation == CANVIEW_UART_PLAN_OP_ABORT)
    {
        if (size != UART_PLAN_ABORT_SIZE || read_le(payload + 4U, 8U) == 0U ||
            !zero_range(payload, size, 18U, 2U))
        {
            return CANVIEW_MALFORMED;
        }
        return CANVIEW_OK;
    }
    return CANVIEW_UNSUPPORTED_MESSAGE;
}

static canview_status_t validate_id_stats(const uint8_t *payload, size_t size)
{
    if (size < 8U || payload[4] > CANVIEW_UART_CAN_ID_STATS_MAX_RECORDS ||
        size != 8U + (size_t)payload[4] * 40U || payload[6] == 0U || payload[5] >= payload[6] ||
        !zero_range(payload, size, 7U, 1U))
    {
        return CANVIEW_MALFORMED;
    }
    for (size_t index = 0U; index < payload[4]; ++index)
    {
        const uint8_t *record = payload + 8U + index * 40U;
        const uint8_t flags = record[1] >> 4U;
        const uint8_t dlc = record[1] & UINT8_C(0x0F);
        if (record[0] >= CANVIEW_WIRE_CAN_BUS_COUNT || dlc > CANVIEW_WIRE_CAN_MAX_DLC ||
            !valid_can_id((uint32_t)read_le(record + 4U, 4U), flags))
        {
            return CANVIEW_MALFORMED;
        }
    }
    return CANVIEW_OK;
}

static canview_status_t validate_config_set(const uint8_t *payload, size_t size)
{
    if (size < 4U || payload[2] > CANVIEW_UART_CONFIG_MAX_RECORDS ||
        size != 4U + (size_t)payload[2] * 8U || !zero_range(payload, size, 3U, 1U))
    {
        return CANVIEW_MALFORMED;
    }
    for (size_t index = 0U; index < payload[2]; ++index)
    {
        const uint8_t *record = payload + 4U + index * 8U;
        if (!zero_range(record, 8U, 3U, 1U))
        {
            return CANVIEW_MALFORMED;
        }
    }
    return CANVIEW_OK;
}

static canview_status_t validate_diagnostic_counters(const uint8_t *payload, size_t size)
{
    if (size < 16U || payload[12] > CANVIEW_UART_DIAGNOSTIC_COUNTER_MAX_RECORDS ||
        size != 16U + (size_t)payload[12] * 16U || !zero_range(payload, size, 13U, 3U))
    {
        return CANVIEW_MALFORMED;
    }
    for (size_t index = 0U; index < payload[12]; ++index)
    {
        const uint8_t *record = payload + 16U + index * 16U;
        if (!zero_range(record, 16U, 3U, 1U) || !zero_range(record, 16U, 13U, 3U))
        {
            return CANVIEW_MALFORMED;
        }
    }
    return CANVIEW_OK;
}

static canview_status_t validate_command_request(const uint8_t *payload, size_t size)
{
    if (size < UART_COMMAND_PREFIX_SIZE ||
        read_le(payload + 52U, 2U) > UART_COMMAND_SUFFIX_MAX ||
        size != UART_COMMAND_PREFIX_SIZE + (size_t)read_le(payload + 52U, 2U) ||
        read_le(payload, 8U) == 0U ||
        read_le(payload + 10U, 2U) < CANVIEW_UART_COMMAND_TTL_MIN_MS ||
        read_le(payload + 10U, 2U) > CANVIEW_UART_COMMAND_TTL_MAX_MS ||
        read_le(payload + 12U, 8U) == 0U || read_le(payload + 20U, 8U) == 0U ||
        read_le(payload + 28U, 4U) == 0U || read_le(payload + 32U, 4U) == 0U ||
        !zero_range(payload, size, 54U, 2U) ||
        !nonzero_range(payload, size, 56U, CANVIEW_UART_COMMAND_DIGEST_SIZE) ||
        !nonzero_range(payload, size, 88U, CANVIEW_UART_CONTROL_TAG_SIZE))
    {
        return CANVIEW_MALFORMED;
    }
    return CANVIEW_OK;
}

static canview_status_t validate_fixed_payload(uint8_t message_type, const uint8_t *payload,
                                               size_t size)
{
    if (message_type == CANVIEW_UART_MSG_LINK_HELLO &&
        (read_le(payload, 8U) == 0U || payload[8] > payload[10] ||
         (payload[8] == payload[10] && payload[9] > payload[11]) ||
         read_le(payload + 28U, 2U) > CANVIEW_UART_MAX_FRAME_SIZE ||
         !zero_range(payload, size, 30U, 2U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_LINK_HELLO_ACK &&
        (payload[16] != CANVIEW_UART_PROTOCOL_MAJOR || payload[17] != CANVIEW_UART_PROTOCOL_MINOR ||
         read_le(payload + 28U, 2U) > CANVIEW_UART_MAX_FRAME_SIZE ||
         !zero_range(payload, size, 19U, 1U) || !zero_range(payload, size, 30U, 2U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_HEARTBEAT &&
        (read_le(payload, 8U) == 0U || !zero_range(payload, size, 44U, 4U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_ERROR &&
        (!zero_range(payload, size, 5U, 1U) || !zero_range(payload, size, 16U, 4U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_CAN_BUS_STATUS &&
        (payload[0] >= CANVIEW_WIRE_CAN_BUS_COUNT || !zero_range(payload, size, 18U, 2U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_SAFETY_SNAPSHOT &&
        (!zero_range(payload, size, 26U, 2U) || read_le(payload, 8U) == 0U))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_CAN_TX_AUDIT &&
        (payload[16] >= CANVIEW_WIRE_CAN_BUS_COUNT || payload[17] > CANVIEW_WIRE_CAN_MAX_DLC ||
         read_le(payload + 18U, 2U) > UINT8_MAX ||
         !valid_can_id((uint32_t)read_le(payload + 12U, 4U), (uint8_t)read_le(payload + 18U, 2U)) ||
         !zero_range(payload, size, 11U, 1U) || !zero_range(payload, size, 36U, 4U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL &&
        (read_le(payload, 8U) == 0U || payload[16] < CANVIEW_UART_CAPTURE_ARM ||
         payload[16] > CANVIEW_UART_CAPTURE_CANCEL || !zero_range(payload, size, 17U, 1U) ||
         !zero_range(payload, size, 24U, 4U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_CAN_CAPTURE_STATUS &&
        !zero_range(payload, size, 40U, 4U))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_CAN_EVENT_MARKER &&
        (read_le(payload + 8U, 4U) == 0U || !zero_range(payload, size, 22U, 2U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_COMMAND_RESULT &&
        (read_le(payload, 8U) == 0U || payload[10] > UINT8_C(6) ||
         !zero_range(payload, size, 11U, 1U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_CONTROL_LEASE &&
        (read_le(payload, 8U) == 0U || !zero_range(payload, size, 9U, 3U) ||
         !zero_range(payload, size, 26U, 2U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_CONFIG_GET &&
        (!zero_range(payload, size, 14U, 2U) || !zero_range(payload, size, 18U, 2U)))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_type == CANVIEW_UART_MSG_CONFIG_RESULT &&
        (!zero_range(payload, size, 11U, 1U) || !zero_range(payload, size, 14U, 2U)))
    {
        return CANVIEW_MALFORMED;
    }
    return CANVIEW_OK;
}

static canview_status_t validate_payload(uint8_t message_type, uint8_t payload_kind,
                                         const uint8_t *payload, size_t size)
{
    if (payload == NULL && size != 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (message_type == CANVIEW_UART_MSG_FIRMWARE_PREPARE)
    {
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
    switch (payload_kind)
    {
    case CANVIEW_UART_PAYLOAD_FIXED:
    case CANVIEW_UART_PAYLOAD_SUFFIX:
        if (message_type == CANVIEW_UART_MSG_COMMAND_REQUEST)
        {
            return validate_command_request(payload, size);
        }
        return validate_fixed_payload(message_type, payload, size);
    case CANVIEW_UART_PAYLOAD_CAN_BATCH:
    {
        canview_wire_can_batch_t batch;
        return canview_wire_can_batch_decode(payload, size, &batch);
    }
    case CANVIEW_UART_PAYLOAD_BOUNDED:
        if (message_type == CANVIEW_UART_MSG_CAN_ID_STATS)
        {
            return validate_id_stats(payload, size);
        }
        if (message_type == CANVIEW_UART_MSG_CONFIG_SET)
        {
            return validate_config_set(payload, size);
        }
        if (message_type == CANVIEW_UART_MSG_DIAGNOSTIC_COUNTERS)
        {
            return validate_diagnostic_counters(payload, size);
        }
        return CANVIEW_MALFORMED;
    case CANVIEW_UART_PAYLOAD_VARIANTS:
        return validate_plan_packet(payload, size);
    default:
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
}

canview_status_t canview_uart_message_validate_for_endpoint(
    const canview_wire_view_t *wire, canview_uart_endpoint_t endpoint,
    canview_uart_flow_t flow, canview_uart_message_view_t *view)
{
    if (view == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(view, 0, sizeof(*view));
    if (wire == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (endpoint > CANVIEW_UART_ENDPOINT_STM32 || flow > CANVIEW_UART_FLOW_OUTBOUND)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_uart_message_policy_t *policy = NULL;
    canview_status_t status = canview_uart_message_policy(wire->header.message_type, &policy);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    if (!policy->supported)
    {
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
    if (wire->header.priority != 0U || wire->header.session_id != 0U ||
        (wire->header.flags & (uint8_t)~policy->allowed_flags) != 0U ||
        (wire->header.flags & policy->required_flags) != policy->required_flags ||
        wire->payload_size < policy->min_payload || wire->payload_size > policy->max_payload ||
        (wire->payload == NULL && wire->payload_size != 0U))
    {
        return CANVIEW_MALFORMED;
    }
    status = validate_payload(wire->header.message_type, policy->payload_kind, wire->payload,
                              wire->payload_size);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    view->wire = *wire;
    view->policy = policy;
    return canview_uart_message_direction_allowed(view, endpoint, flow) ? CANVIEW_OK
                                                                          : CANVIEW_MALFORMED;
}

bool canview_uart_message_direction_allowed(const canview_uart_message_view_t *view,
                                            canview_uart_endpoint_t endpoint,
                                            canview_uart_flow_t flow)
{
    if (view == NULL || view->policy == NULL || endpoint > CANVIEW_UART_ENDPOINT_STM32 ||
        flow > CANVIEW_UART_FLOW_OUTBOUND ||
        view->policy->direction > CANVIEW_UART_DIRECTION_STM_TO_ESP)
    {
        return false;
    }
    if (view->policy->direction == CANVIEW_UART_DIRECTION_BOTH)
    {
        return true;
    }
    const bool esp_to_stm = view->policy->direction == CANVIEW_UART_DIRECTION_ESP_TO_STM;
    const bool endpoint_is_esp = endpoint == CANVIEW_UART_ENDPOINT_ESP32;
    return flow == CANVIEW_UART_FLOW_OUTBOUND ? (endpoint_is_esp == esp_to_stm)
                                              : (endpoint_is_esp != esp_to_stm);
}

static bool message_requires_authorization(uint8_t message_type)
{
    switch (message_type)
    {
    case CANVIEW_UART_MSG_CAN_OBSERVER_PLAN:
    case CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL:
    case CANVIEW_UART_MSG_CAN_EVENT_MARKER:
    case CANVIEW_UART_MSG_COMMAND_REQUEST:
    case CANVIEW_UART_MSG_CONTROL_LEASE:
    case CANVIEW_UART_MSG_CONFIG_GET:
    case CANVIEW_UART_MSG_CONFIG_SET:
        return true;
    default:
        return false;
    }
}

canview_status_t canview_uart_replay_reset(canview_uart_replay_context_t *context)
{
    if (context == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(context, 0, sizeof(*context));
    context->configured = true;
    return canview_sequence_window_reset(&context->sequence);
}

canview_status_t canview_uart_message_admit(
    const canview_uart_message_view_t *view, canview_uart_endpoint_t endpoint,
    canview_uart_flow_t flow, canview_uart_replay_context_t *replay,
    const canview_uart_command_admission_context_t *authorization, uint64_t now_ms)
{
    if (view == NULL || view->policy == NULL || replay == NULL || !replay->configured ||
        endpoint > CANVIEW_UART_ENDPOINT_STM32 || flow > CANVIEW_UART_FLOW_OUTBOUND)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (!canview_uart_message_direction_allowed(view, endpoint, flow))
    {
        return CANVIEW_MALFORMED;
    }
    if (message_requires_authorization(view->wire.header.message_type) &&
        (authorization == NULL || authorization->authorize == NULL ||
         !authorization->authorize(view, now_ms, authorization->context)))
    {
        return CANVIEW_AUTH_FAILED;
    }
    canview_uart_replay_context_t candidate = *replay;
    const canview_status_t sequence_status =
        canview_sequence_window_accept(&candidate.sequence, view->wire.header.sequence);
    if (sequence_status != CANVIEW_OK)
    {
        return sequence_status;
    }
    *replay = candidate;
    return CANVIEW_OK;
}

canview_status_t canview_uart_message_encode(uint8_t message_type, uint8_t flags,
                                             uint32_t sequence, uint32_t correlation_id,
                                             uint64_t sender_time_us, const uint8_t *payload,
                                             size_t payload_size, canview_uart_endpoint_t endpoint,
                                             canview_uart_flow_t flow, uint8_t *scratch,
                                             size_t scratch_size, uint8_t *out, size_t capacity,
                                             size_t *written)
{
    if (written == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *written = 0U;
    if (scratch == NULL || out == NULL || (payload == NULL && payload_size != 0U))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_wire_header_t header = {0};
    header.message_type = message_type;
    header.flags = flags;
    header.sequence = sequence;
    header.correlation_id = correlation_id;
    header.sender_time = sender_time_us;
    canview_wire_view_t wire = {0};
    wire.header = header;
    wire.payload = payload;
    wire.payload_size = payload_size;
    canview_uart_message_view_t view;
    canview_status_t status =
        canview_uart_message_validate_for_endpoint(&wire, endpoint, flow, &view);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    return canview_uart_packet_encode(&header, payload, payload_size, scratch, scratch_size, out,
                                      capacity, written);
}

canview_status_t canview_uart_codec_reset(canview_uart_codec_t *codec,
                                          canview_uart_endpoint_t endpoint,
                                          canview_uart_flow_t flow)
{
    if (codec == NULL || endpoint > CANVIEW_UART_ENDPOINT_STM32 ||
        flow > CANVIEW_UART_FLOW_OUTBOUND)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(codec, 0, sizeof(*codec));
    codec->endpoint = endpoint;
    codec->flow = flow;
    return canview_uart_stream_reset(&codec->stream);
}

canview_status_t canview_uart_codec_feed(canview_uart_codec_t *codec, uint8_t byte,
                                          canview_uart_message_view_t *view)
{
    if (view == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(view, 0, sizeof(*view));
    if (codec == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_wire_view_t wire;
    const canview_status_t framing = canview_uart_stream_feed(&codec->stream, byte, &wire);
    if (framing == CANVIEW_OVERSIZE)
    {
        increment_counter(&codec->oversize_packets);
    }
    else if (framing == CANVIEW_CRC_MISMATCH)
    {
        increment_counter(&codec->crc_failures);
    }
    else if (framing == CANVIEW_MALFORMED)
    {
        increment_counter(&codec->malformed_packets);
    }
    if (framing != CANVIEW_OK)
    {
        return framing;
    }
    const canview_status_t semantic =
        canview_uart_message_validate_for_endpoint(&wire, codec->endpoint, codec->flow, view);
    if (semantic != CANVIEW_OK)
    {
        if (semantic == CANVIEW_MALFORMED)
        {
            increment_counter(&codec->malformed_packets);
        }
        else if (semantic == CANVIEW_UNSUPPORTED_MESSAGE)
        {
            increment_counter(&codec->unsupported_messages);
        }
        return semantic;
    }
    increment_counter(&codec->packets_ok);
    return CANVIEW_OK;
}

typedef struct
{
    uint32_t state[8];
    uint64_t bit_count;
    size_t block_size;
    uint8_t block[64];
} uart_sha256_context_t;

static uint32_t rotate_right32(uint32_t value, uint32_t amount)
{
    return (value >> amount) | (value << (32U - amount));
}

static void uart_sha256_transform(uart_sha256_context_t *context, const uint8_t block[64])
{
    static const uint32_t constants[64] = {
        UINT32_C(0x428A2F98), UINT32_C(0x71374491), UINT32_C(0xB5C0FBCF), UINT32_C(0xE9B5DBA5),
        UINT32_C(0x3956C25B), UINT32_C(0x59F111F1), UINT32_C(0x923F82A4), UINT32_C(0xAB1C5ED5),
        UINT32_C(0xD807AA98), UINT32_C(0x12835B01), UINT32_C(0x243185BE), UINT32_C(0x550C7DC3),
        UINT32_C(0x72BE5D74), UINT32_C(0x80DEB1FE), UINT32_C(0x9BDC06A7), UINT32_C(0xC19BF174),
        UINT32_C(0xE49B69C1), UINT32_C(0xEFBE4786), UINT32_C(0x0FC19DC6), UINT32_C(0x240CA1CC),
        UINT32_C(0x2DE92C6F), UINT32_C(0x4A7484AA), UINT32_C(0x5CB0A9DC), UINT32_C(0x76F988DA),
        UINT32_C(0x983E5152), UINT32_C(0xA831C66D), UINT32_C(0xB00327C8), UINT32_C(0xBF597FC7),
        UINT32_C(0xC6E00BF3), UINT32_C(0xD5A79147), UINT32_C(0x06CA6351), UINT32_C(0x14292967),
        UINT32_C(0x27B70A85), UINT32_C(0x2E1B2138), UINT32_C(0x4D2C6DFC), UINT32_C(0x53380D13),
        UINT32_C(0x650A7354), UINT32_C(0x766A0ABB), UINT32_C(0x81C2C92E), UINT32_C(0x92722C85),
        UINT32_C(0xA2BFE8A1), UINT32_C(0xA81A664B), UINT32_C(0xC24B8B70), UINT32_C(0xC76C51A3),
        UINT32_C(0xD192E819), UINT32_C(0xD6990624), UINT32_C(0xF40E3585), UINT32_C(0x106AA070),
        UINT32_C(0x19A4C116), UINT32_C(0x1E376C08), UINT32_C(0x2748774C), UINT32_C(0x34B0BCB5),
        UINT32_C(0x391C0CB3), UINT32_C(0x4ED8AA4A), UINT32_C(0x5B9CCA4F), UINT32_C(0x682E6FF3),
        UINT32_C(0x748F82EE), UINT32_C(0x78A5636F), UINT32_C(0x84C87814), UINT32_C(0x8CC70208),
        UINT32_C(0x90BEFFFA), UINT32_C(0xA4506CEB), UINT32_C(0xBEF9A3F7), UINT32_C(0xC67178F2)};
    uint32_t words[64];
    for (size_t index = 0U; index < 16U; ++index)
    {
        words[index] = ((uint32_t)block[index * 4U] << 24U) |
                       ((uint32_t)block[index * 4U + 1U] << 16U) |
                       ((uint32_t)block[index * 4U + 2U] << 8U) |
                       (uint32_t)block[index * 4U + 3U];
    }
    for (size_t index = 16U; index < 64U; ++index)
    {
        const uint32_t small_sigma0 = rotate_right32(words[index - 15U], 7U) ^
                                      rotate_right32(words[index - 15U], 18U) ^
                                      (words[index - 15U] >> 3U);
        const uint32_t small_sigma1 = rotate_right32(words[index - 2U], 17U) ^
                                      rotate_right32(words[index - 2U], 19U) ^
                                      (words[index - 2U] >> 10U);
        words[index] = words[index - 16U] + small_sigma0 + words[index - 7U] + small_sigma1;
    }

    uint32_t a = context->state[0];
    uint32_t b = context->state[1];
    uint32_t c = context->state[2];
    uint32_t d = context->state[3];
    uint32_t e = context->state[4];
    uint32_t f = context->state[5];
    uint32_t g = context->state[6];
    uint32_t h = context->state[7];
    for (size_t index = 0U; index < 64U; ++index)
    {
        const uint32_t big_sigma1 = rotate_right32(e, 6U) ^ rotate_right32(e, 11U) ^
                                    rotate_right32(e, 25U);
        const uint32_t choose = (e & f) ^ ((~e) & g);
        const uint32_t temporary1 = h + big_sigma1 + choose + constants[index] + words[index];
        const uint32_t big_sigma0 = rotate_right32(a, 2U) ^ rotate_right32(a, 13U) ^
                                    rotate_right32(a, 22U);
        const uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t temporary2 = big_sigma0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + temporary1;
        d = c;
        c = b;
        b = a;
        a = temporary1 + temporary2;
    }
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

static void uart_sha256_init(uart_sha256_context_t *context)
{
    *context = (uart_sha256_context_t){
        .state = {UINT32_C(0x6A09E667), UINT32_C(0xBB67AE85), UINT32_C(0x3C6EF372),
                  UINT32_C(0xA54FF53A), UINT32_C(0x510E527F), UINT32_C(0x9B05688C),
                  UINT32_C(0x1F83D9AB), UINT32_C(0x5BE0CD19)},
        .bit_count = 0U,
        .block_size = 0U,
        .block = {0U}};
}

static void uart_sha256_update(uart_sha256_context_t *context, const uint8_t *bytes, size_t size)
{
    context->bit_count += (uint64_t)size * UINT64_C(8);
    while (size > 0U)
    {
        const size_t available = sizeof(context->block) - context->block_size;
        const size_t copied = size < available ? size : available;
        memcpy(context->block + context->block_size, bytes, copied);
        context->block_size += copied;
        bytes += copied;
        size -= copied;
        if (context->block_size == sizeof(context->block))
        {
            uart_sha256_transform(context, context->block);
            context->block_size = 0U;
        }
    }
}

static void uart_sha256_final(uart_sha256_context_t *context, uint8_t digest[32])
{
    const uint64_t bit_count = context->bit_count;
    context->block[context->block_size++] = UINT8_C(0x80);
    if (context->block_size > 56U)
    {
        memset(context->block + context->block_size, 0, sizeof(context->block) - context->block_size);
        uart_sha256_transform(context, context->block);
        context->block_size = 0U;
    }
    memset(context->block + context->block_size, 0, 56U - context->block_size);
    for (size_t index = 0U; index < 8U; ++index)
    {
        context->block[56U + index] = (uint8_t)(bit_count >> (56U - index * 8U));
    }
    uart_sha256_transform(context, context->block);
    for (size_t index = 0U; index < 8U; ++index)
    {
        write_le(digest + index * 4U, 4U,
                 ((uint32_t)context->state[index] >> 24U) |
                     (((uint32_t)context->state[index] >> 8U) & UINT32_C(0x0000FF00)) |
                     (((uint32_t)context->state[index] << 8U) & UINT32_C(0x00FF0000)) |
                     ((uint32_t)context->state[index] << 24U));
    }
}

static canview_status_t build_plan_canonical(const canview_uart_observer_plan_t *plan,
                                             uint8_t canonical[UART_PLAN_CANONICAL_SIZE],
                                             size_t *canonical_size)
{
    if (plan == NULL || canonical == NULL || canonical_size == NULL ||
        plan->filter_count > CANVIEW_UART_PLAN_MAX_FILTERS ||
        plan->max_records_per_second < CANVIEW_UART_PLAN_MIN_RECORDS_PER_SECOND ||
        plan->max_records_per_second > CANVIEW_UART_PLAN_MAX_RECORDS_PER_SECOND ||
        plan->max_bytes_per_second < CANVIEW_UART_PLAN_MIN_BYTES_PER_SECOND ||
        plan->max_bytes_per_second > CANVIEW_UART_PLAN_MAX_BYTES_PER_SECOND)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(canonical, 0, UART_PLAN_CANONICAL_SIZE);
    canonical[0] = (uint8_t)'C';
    canonical[1] = (uint8_t)'U';
    canonical[2] = (uint8_t)'P';
    canonical[3] = UINT8_C(1);
    write_le(canonical + 4U, 4U, plan->revision);
    write_le(canonical + 8U, 2U, plan->filter_count);
    write_le(canonical + 10U, 2U, plan->max_records_per_second);
    write_le(canonical + 12U, 4U, plan->max_bytes_per_second);
    canonical[16U] = plan->bus_mask;
    size_t offset = UART_PLAN_DIGEST_DOMAIN_SIZE;
    for (size_t index = 0U; index < plan->filter_count; ++index)
    {
        const canview_uart_observer_filter_t *filter = &plan->filters[index];
        if (filter->bus_id >= CANVIEW_WIRE_CAN_BUS_COUNT ||
            !valid_can_id(filter->can_id, filter->flags))
        {
            return CANVIEW_MALFORMED;
        }
        canonical[offset] = filter->bus_id;
        canonical[offset + 1U] = filter->flags;
        write_le(canonical + offset + 4U, 4U, filter->can_id);
        write_le(canonical + offset + 8U, 4U, filter->can_mask);
        offset += UART_PLAN_FILTER_SIZE;
    }
    *canonical_size = offset;
    return CANVIEW_OK;
}

canview_status_t canview_uart_plan_digest(const canview_uart_observer_plan_t *plan,
                                          uint8_t digest[CANVIEW_UART_PLAN_DIGEST_SIZE])
{
    if (digest == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint8_t canonical[UART_PLAN_CANONICAL_SIZE];
    size_t canonical_size = 0U;
    const canview_status_t status = build_plan_canonical(plan, canonical, &canonical_size);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    uart_sha256_context_t sha256;
    uart_sha256_init(&sha256);
    uart_sha256_update(&sha256, canonical, canonical_size);
    uart_sha256_final(&sha256, digest);
    return CANVIEW_OK;
}

static void clear_plan_candidate(canview_uart_plan_context_t *context)
{
    context->pending = false;
    context->request_token = 0U;
    context->revision = 0U;
    context->expected_active_revision = 0U;
    context->total_chunks = 0U;
    context->next_chunk = 0U;
    context->expected_filter_count = 0U;
    context->received_filter_count = 0U;
    context->pending_since_ms = 0U;
    memset(&context->candidate, 0, sizeof(context->candidate));
}

canview_status_t canview_uart_plan_reset(canview_uart_plan_context_t *context)
{
    if (context == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(context, 0, sizeof(*context));
    return CANVIEW_OK;
}

canview_status_t canview_uart_plan_discard_pending(canview_uart_plan_context_t *context)
{
    if (context == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    clear_plan_candidate(context);
    return CANVIEW_OK;
}

static canview_status_t plan_begin(canview_uart_plan_context_t *context, const uint8_t *payload,
                                   uint64_t now_ms)
{
    if (context->pending)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    const uint32_t expected_revision = (uint32_t)read_le(payload + 16U, 4U);
    if (expected_revision != context->active.revision)
    {
        return CANVIEW_STALE;
    }
    const uint32_t revision = (uint32_t)read_le(payload + 12U, 4U);
    if (context->active.revision == UINT32_MAX || revision != context->active.revision + 1U)
    {
        return CANVIEW_STALE;
    }
    context->pending = true;
    context->request_token = read_le(payload + 4U, 8U);
    context->revision = revision;
    context->expected_active_revision = expected_revision;
    context->total_chunks = (uint16_t)read_le(payload + 20U, 2U);
    context->next_chunk = 0U;
    context->expected_filter_count = (uint16_t)read_le(payload + 22U, 2U);
    context->received_filter_count = 0U;
    context->pending_since_ms = now_ms;
    memset(&context->candidate, 0, sizeof(context->candidate));
    context->candidate.revision = context->revision;
    context->candidate.filter_count = context->expected_filter_count;
    context->candidate.max_records_per_second = (uint16_t)read_le(payload + 24U, 2U);
    context->candidate.max_bytes_per_second = (uint32_t)read_le(payload + 26U, 4U);
    context->candidate.bus_mask = payload[30];
    return CANVIEW_INCOMPLETE;
}

static canview_status_t plan_chunk(canview_uart_plan_context_t *context, const uint8_t *payload)
{
    if (!context->pending || read_le(payload + 4U, 8U) != context->request_token ||
        read_le(payload + 12U, 4U) != context->revision ||
        read_le(payload + 16U, 2U) != context->next_chunk ||
        read_le(payload + 18U, 2U) != context->total_chunks)
    {
        return CANVIEW_MALFORMED;
    }
    const size_t count = payload[20];
    if (context->received_filter_count > context->expected_filter_count ||
        count > (size_t)context->expected_filter_count - context->received_filter_count)
    {
        return CANVIEW_MALFORMED;
    }
    for (size_t index = 0U; index < count; ++index)
    {
        const uint8_t *source = payload + UART_PLAN_HEADER_SIZE + index * UART_PLAN_FILTER_SIZE;
        canview_uart_observer_filter_t *destination =
            &context->candidate.filters[context->received_filter_count + index];
        destination->bus_id = source[0];
        destination->flags = source[1];
        destination->can_id = (uint32_t)read_le(source + 4U, 4U);
        destination->can_mask = (uint32_t)read_le(source + 8U, 4U);
    }
    context->received_filter_count = (uint16_t)(context->received_filter_count + count);
    context->next_chunk = (uint16_t)(context->next_chunk + 1U);
    return CANVIEW_INCOMPLETE;
}

static canview_status_t plan_commit(canview_uart_plan_context_t *context, const uint8_t *payload)
{
    if (!context->pending || read_le(payload + 4U, 8U) != context->request_token ||
        read_le(payload + 12U, 4U) != context->revision ||
        context->next_chunk != context->total_chunks ||
        context->received_filter_count != context->expected_filter_count)
    {
        return CANVIEW_MALFORMED;
    }
    uint8_t digest[CANVIEW_UART_PLAN_DIGEST_SIZE];
    const canview_status_t digest_status = canview_uart_plan_digest(&context->candidate, digest);
    if (digest_status != CANVIEW_OK ||
        memcmp(digest, payload + 16U, CANVIEW_UART_PLAN_DIGEST_SIZE) != 0)
    {
        return CANVIEW_MALFORMED;
    }
    memcpy(context->candidate.plan_digest, digest, CANVIEW_UART_PLAN_DIGEST_SIZE);
    context->active = context->candidate;
    clear_plan_candidate(context);
    return CANVIEW_OK;
}

static canview_status_t plan_abort(canview_uart_plan_context_t *context, const uint8_t *payload)
{
    if (!context->pending || read_le(payload + 4U, 8U) != context->request_token ||
        read_le(payload + 12U, 4U) != context->revision)
    {
        return CANVIEW_MALFORMED;
    }
    clear_plan_candidate(context);
    return CANVIEW_OK;
}

canview_status_t canview_uart_plan_apply(canview_uart_plan_context_t *context,
                                         const uint8_t *payload, size_t payload_size,
                                         uint64_t now_ms)
{
    if (context == NULL || payload == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (context->pending &&
        (now_ms < context->pending_since_ms ||
         now_ms - context->pending_since_ms >= CANVIEW_UART_PLAN_STAGING_TIMEOUT_MS))
    {
        clear_plan_candidate(context);
    }
    const canview_status_t valid = validate_plan_packet(payload, payload_size);
    if (valid != CANVIEW_OK)
    {
        return valid;
    }
    switch (payload[0])
    {
    case CANVIEW_UART_PLAN_OP_BEGIN:
        return plan_begin(context, payload, now_ms);
    case CANVIEW_UART_PLAN_OP_CHUNK:
        return plan_chunk(context, payload);
    case CANVIEW_UART_PLAN_OP_COMMIT:
        return plan_commit(context, payload);
    case CANVIEW_UART_PLAN_OP_ABORT:
        return plan_abort(context, payload);
    default:
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
}

canview_status_t canview_uart_plan_current(const canview_uart_plan_context_t *context,
                                           canview_uart_observer_plan_t *plan)
{
    if (context == NULL || plan == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *plan = context->active;
    return CANVIEW_OK;
}

static uint64_t expiry_for(uint64_t now_ms)
{
    const uint64_t ttl = CANVIEW_UART_COMMAND_CACHE_RETENTION_MS;
    return now_ms > UINT64_MAX - ttl ? UINT64_MAX : now_ms + ttl;
}

static bool entry_expired(const canview_uart_command_entry_t *entry, uint64_t now_ms)
{
    return entry->valid && entry->expires_at_ms <= now_ms;
}

static bool request_expired(const canview_uart_command_entry_t *entry, uint64_t now_ms)
{
    return entry->valid && entry->request_expires_at_ms <= now_ms;
}

static uint64_t deadline_for(uint64_t now_ms, uint16_t ttl_ms)
{
    return now_ms > UINT64_MAX - ttl_ms ? UINT64_MAX : now_ms + ttl_ms;
}

static void command_handle_clear(canview_uart_command_handle_t *handle)
{
    if (handle != NULL)
    {
        *handle = (canview_uart_command_handle_t){
            .slot = CANVIEW_UART_COMMAND_CACHE_CAPACITY, .reserved = 0U, .generation = 0U};
    }
}

static bool command_key_valid(const canview_uart_command_key_t *key)
{
    return key != NULL && key->origin_device_id != 0U && key->origin_boot_id != 0U &&
           key->wireless_session_id != 0U && key->control_generation != 0U &&
           key->request_token != 0U;
}

static bool command_key_equal(const canview_uart_command_key_t *left,
                              const canview_uart_command_key_t *right)
{
    return left->origin_device_id == right->origin_device_id &&
           left->origin_boot_id == right->origin_boot_id &&
           left->wireless_session_id == right->wireless_session_id &&
           left->control_generation == right->control_generation &&
           left->request_token == right->request_token && left->command_id == right->command_id &&
           memcmp(left->canonical_argument_digest, right->canonical_argument_digest,
                  CANVIEW_UART_COMMAND_DIGEST_SIZE) == 0;
}

static bool command_key_identity_equal(const canview_uart_command_key_t *left,
                                       const canview_uart_command_key_t *right)
{
    return left->origin_device_id == right->origin_device_id &&
           left->origin_boot_id == right->origin_boot_id &&
           left->wireless_session_id == right->wireless_session_id &&
           left->control_generation == right->control_generation &&
           left->request_token == right->request_token && left->command_id == right->command_id;
}

static canview_status_t command_cache_entry_from_handle(
    canview_uart_command_cache_t *cache, const canview_uart_command_handle_t *handle,
    canview_uart_command_entry_t **entry)
{
    if (cache == NULL || handle == NULL || entry == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (cache->initialized != UART_COMMAND_CACHE_MAGIC || handle->reserved != 0U ||
        handle->slot >= CANVIEW_UART_COMMAND_CACHE_CAPACITY)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *entry = &cache->entries[handle->slot];
    if (!(*entry)->valid || (*entry)->generation != handle->generation)
    {
        *entry = NULL;
        return CANVIEW_STALE;
    }
    return CANVIEW_OK;
}

canview_status_t canview_uart_command_cache_reset(canview_uart_command_cache_t *cache)
{
    if (cache == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint64_t next_generation = 1U;
    if (cache->initialized == UART_COMMAND_CACHE_MAGIC)
    {
        if (cache->next_generation == UINT64_MAX)
        {
            return CANVIEW_RESOURCE_BUSY;
        }
        next_generation = cache->next_generation == 0U ? 1U : cache->next_generation;
    }
    memset(cache, 0, sizeof(*cache));
    cache->initialized = UART_COMMAND_CACHE_MAGIC;
    cache->next_generation = next_generation;
    return CANVIEW_OK;
}

canview_status_t canview_uart_command_cache_admit(
    canview_uart_command_cache_t *cache, const canview_uart_command_key_t *key,
    uint16_t request_ttl_ms, uint64_t now_ms, canview_uart_command_handle_t *handle)
{
    if (cache == NULL || !command_key_valid(key) || handle == NULL ||
        request_ttl_ms < CANVIEW_UART_COMMAND_TTL_MIN_MS ||
        request_ttl_ms > CANVIEW_UART_COMMAND_TTL_MAX_MS)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    command_handle_clear(handle);
    if (cache->initialized != UART_COMMAND_CACHE_MAGIC)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    size_t free_index = CANVIEW_UART_COMMAND_CACHE_CAPACITY;
    for (size_t index = 0U; index < CANVIEW_UART_COMMAND_CACHE_CAPACITY; ++index)
    {
        canview_uart_command_entry_t *entry = &cache->entries[index];
        if (entry_expired(entry, now_ms))
        {
            memset(entry, 0, sizeof(*entry));
        }
        if (!entry->valid)
        {
            if (free_index == CANVIEW_UART_COMMAND_CACHE_CAPACITY)
            {
                free_index = index;
            }
            continue;
        }
        if (command_key_identity_equal(&entry->key, key))
        {
            handle->slot = (uint16_t)index;
            handle->generation = entry->generation;
            if (!command_key_equal(&entry->key, key))
            {
                return CANVIEW_MALFORMED;
            }
            if (request_expired(entry, now_ms))
            {
                return CANVIEW_TIMEOUT;
            }
            return CANVIEW_DUPLICATE;
        }
    }
    if (free_index == CANVIEW_UART_COMMAND_CACHE_CAPACITY)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    if (cache->next_generation == UINT64_MAX)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    canview_uart_command_entry_t *entry = &cache->entries[free_index];
    memset(entry, 0, sizeof(*entry));
    entry->valid = true;
    entry->generation = cache->next_generation;
    cache->next_generation += 1U;
    entry->key = *key;
    entry->request_expires_at_ms = deadline_for(now_ms, request_ttl_ms);
    entry->expires_at_ms = expiry_for(now_ms);
    handle->slot = (uint16_t)free_index;
    handle->generation = entry->generation;
    return CANVIEW_OK;
}

canview_status_t canview_uart_command_cache_mark_ack(canview_uart_command_cache_t *cache,
                                                     const canview_uart_command_handle_t *handle,
                                                     uint64_t now_ms)
{
    canview_uart_command_entry_t *entry = NULL;
    const canview_status_t status = command_cache_entry_from_handle(cache, handle, &entry);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    if (request_expired(entry, now_ms) || entry_expired(entry, now_ms))
    {
        return CANVIEW_TIMEOUT;
    }
    entry->acknowledged = true;
    return CANVIEW_OK;
}

canview_status_t canview_uart_command_cache_record_result(
    canview_uart_command_cache_t *cache, const canview_uart_command_handle_t *handle,
    const uint8_t *result,
    size_t result_size, uint64_t now_ms)
{
    if (cache == NULL || handle == NULL || (result == NULL && result_size != 0U) ||
        result_size == 0U ||
        result_size > CANVIEW_UART_COMMAND_RESULT_MAX)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_uart_command_entry_t *entry = NULL;
    const canview_status_t status = command_cache_entry_from_handle(cache, handle, &entry);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    if (entry_expired(entry, now_ms))
    {
        memset(entry, 0, sizeof(*entry));
        return CANVIEW_STALE;
    }
    if (request_expired(entry, now_ms))
    {
        return CANVIEW_TIMEOUT;
    }
    if (entry->terminal)
    {
        if (entry->result_size != result_size || memcmp(entry->result, result, result_size) != 0)
        {
            return CANVIEW_MALFORMED;
        }
        return CANVIEW_OK;
    }
    memcpy(entry->result, result, result_size);
    entry->result_size = (uint16_t)result_size;
    entry->terminal = true;
    return CANVIEW_OK;
}

canview_status_t canview_uart_command_cache_lookup(
    canview_uart_command_cache_t *cache, const canview_uart_command_key_t *key, uint64_t now_ms,
    canview_uart_command_handle_t *handle, const uint8_t **result, size_t *result_size)
{
    if (cache == NULL || !command_key_valid(key) || handle == NULL || result == NULL ||
        result_size == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    command_handle_clear(handle);
    *result = NULL;
    *result_size = 0U;
    if (cache->initialized != UART_COMMAND_CACHE_MAGIC)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    for (size_t index = 0U; index < CANVIEW_UART_COMMAND_CACHE_CAPACITY; ++index)
    {
        canview_uart_command_entry_t *entry = &cache->entries[index];
        if (!entry->valid)
        {
            continue;
        }
        if (entry_expired(entry, now_ms))
        {
            memset(entry, 0, sizeof(*entry));
            continue;
        }
        if (!command_key_identity_equal(&entry->key, key))
        {
            continue;
        }
        if (request_expired(entry, now_ms))
        {
            handle->slot = (uint16_t)index;
            handle->generation = entry->generation;
            return CANVIEW_TIMEOUT;
        }
        handle->slot = (uint16_t)index;
        handle->generation = entry->generation;
        if (!command_key_equal(&entry->key, key))
        {
            return CANVIEW_MALFORMED;
        }
        if (entry->terminal)
        {
            *result = entry->result;
            *result_size = entry->result_size;
        }
        return CANVIEW_DUPLICATE;
    }
    return CANVIEW_STALE;
}

canview_status_t canview_uart_command_cache_expire(canview_uart_command_cache_t *cache,
                                                   uint64_t now_ms)
{
    if (cache == NULL || cache->initialized != UART_COMMAND_CACHE_MAGIC)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    for (size_t index = 0U; index < CANVIEW_UART_COMMAND_CACHE_CAPACITY; ++index)
    {
        if (entry_expired(&cache->entries[index], now_ms))
        {
            memset(&cache->entries[index], 0, sizeof(cache->entries[index]));
        }
    }
    return CANVIEW_OK;
}

static uint64_t elapsed_ms(uint64_t now_ms, uint64_t then_ms)
{
    return now_ms < then_ms ? UINT64_MAX : now_ms - then_ms;
}

static void link_require_hello(canview_uart_link_t *link)
{
    link->hello_complete = false;
    link->hello_ack_complete = false;
    link->safety_snapshot_valid = false;
    link->heartbeat_seen = false;
    link->safety_snapshot_time_ms = 0U;
    link->safety_revision = 0U;
    link->cts_known = false;
    link->cts_blocked = false;
    link->cts_blocked_since_ms = 0U;
    link->state = CANVIEW_UART_LINK_OFFLINE;
}

canview_status_t canview_uart_link_reset(canview_uart_link_t *link)
{
    if (link == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(link, 0, sizeof(*link));
    link->state = CANVIEW_UART_LINK_OFFLINE;
    return CANVIEW_OK;
}

canview_status_t canview_uart_link_note_hello(canview_uart_link_t *link, uint64_t peer_boot_id,
                                              uint64_t now_ms, bool *boot_changed)
{
    if (link == NULL || boot_changed == NULL || peer_boot_id == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *boot_changed = link->peer_boot_id != 0U && link->peer_boot_id != peer_boot_id;
    link->peer_boot_id = peer_boot_id;
    link->hello_complete = true;
    link->hello_ack_complete = false;
    link->safety_snapshot_valid = false;
    link->heartbeat_seen = false;
    link->safety_snapshot_time_ms = 0U;
    link->safety_revision = 0U;
    link->cts_known = false;
    link->cts_blocked = false;
    link->cts_blocked_since_ms = 0U;
    link->last_heartbeat_ms = now_ms;
    link->state = CANVIEW_UART_LINK_OFFLINE;
    return CANVIEW_OK;
}

canview_status_t canview_uart_link_note_hello_ack(canview_uart_link_t *link,
                                                  uint64_t peer_boot_id, uint8_t selected_major,
                                                  uint8_t selected_minor, uint8_t result,
                                                  uint64_t now_ms)
{
    if (link == NULL || !link->hello_complete || peer_boot_id == 0U ||
        peer_boot_id != link->peer_boot_id)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (selected_major != CANVIEW_UART_PROTOCOL_MAJOR ||
        selected_minor != CANVIEW_UART_PROTOCOL_MINOR || result != 0U)
    {
        return CANVIEW_UNSUPPORTED_VERSION;
    }
    link->hello_ack_complete = true;
    return canview_uart_link_tick(link, now_ms);
}

canview_status_t canview_uart_link_note_safety_snapshot(canview_uart_link_t *link,
                                                        uint64_t peer_boot_id,
                                                        uint32_t safety_revision,
                                                        uint64_t now_ms)
{
    if (link == NULL || !link->hello_complete || !link->hello_ack_complete ||
        peer_boot_id == 0U || peer_boot_id != link->peer_boot_id)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    link->safety_snapshot_valid = true;
    link->safety_snapshot_time_ms = now_ms;
    link->safety_revision = safety_revision;
    return canview_uart_link_tick(link, now_ms);
}

canview_status_t canview_uart_link_note_heartbeat(canview_uart_link_t *link,
                                                  uint64_t peer_boot_id,
                                                  uint32_t safety_revision, uint64_t now_ms,
                                                  bool *boot_changed)
{
    if (link == NULL || boot_changed == NULL || peer_boot_id == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *boot_changed = link->peer_boot_id != 0U && link->peer_boot_id != peer_boot_id;
    if (!link->hello_complete || link->peer_boot_id != peer_boot_id)
    {
        link->peer_boot_id = peer_boot_id;
        link_require_hello(link);
        return CANVIEW_OK;
    }
    const bool heartbeat_offline =
        link->heartbeat_seen &&
        elapsed_ms(now_ms, link->last_heartbeat_ms) >= CANVIEW_UART_HEARTBEAT_OFFLINE_MS;
    const bool cts_offline = link->cts_known && link->cts_blocked &&
                             elapsed_ms(now_ms, link->cts_blocked_since_ms) >=
                                 CANVIEW_UART_CTS_OFFLINE_MS;
    if (heartbeat_offline || cts_offline)
    {
        link_require_hello(link);
        return CANVIEW_TIMEOUT;
    }
    link->heartbeat_seen = true;
    link->last_heartbeat_ms = now_ms;
    if (link->safety_snapshot_valid && link->safety_revision != safety_revision)
    {
        link->safety_snapshot_valid = false;
    }
    return canview_uart_link_tick(link, now_ms);
}

canview_status_t canview_uart_link_set_cts_blocked(canview_uart_link_t *link, bool blocked,
                                                   uint64_t now_ms)
{
    if (link == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (link->cts_known && link->cts_blocked &&
        elapsed_ms(now_ms, link->cts_blocked_since_ms) >= CANVIEW_UART_CTS_OFFLINE_MS)
    {
        link_require_hello(link);
        return CANVIEW_TIMEOUT;
    }
    link->cts_known = true;
    if (blocked && !link->cts_blocked)
    {
        link->cts_blocked_since_ms = now_ms;
    }
    link->cts_blocked = blocked;
    return canview_uart_link_tick(link, now_ms);
}

canview_status_t canview_uart_link_tick(canview_uart_link_t *link, uint64_t now_ms)
{
    if (link == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const bool heartbeat_offline =
        link->heartbeat_seen &&
        elapsed_ms(now_ms, link->last_heartbeat_ms) >= CANVIEW_UART_HEARTBEAT_OFFLINE_MS;
    const bool cts_offline = link->cts_known && link->cts_blocked &&
                             elapsed_ms(now_ms, link->cts_blocked_since_ms) >=
                                 CANVIEW_UART_CTS_OFFLINE_MS;
    const bool safety_stale =
        link->safety_snapshot_valid &&
        elapsed_ms(now_ms, link->safety_snapshot_time_ms) >=
            CANVIEW_UART_SAFETY_SNAPSHOT_MAX_AGE_MS;
    if (safety_stale)
    {
        link->safety_snapshot_valid = false;
    }
    if (!link->hello_complete || heartbeat_offline || cts_offline)
    {
        link_require_hello(link);
        return CANVIEW_OK;
    }
    if (!link->heartbeat_seen)
    {
        link->state = CANVIEW_UART_LINK_OFFLINE;
        return CANVIEW_OK;
    }
    const bool heartbeat_suspect =
        elapsed_ms(now_ms, link->last_heartbeat_ms) >= CANVIEW_UART_HEARTBEAT_ONLINE_MS;
    const bool cts_suspect = link->cts_blocked &&
                             elapsed_ms(now_ms, link->cts_blocked_since_ms) >=
                                 CANVIEW_UART_CTS_COMMAND_STOP_MS;
    link->state = (heartbeat_suspect || cts_suspect || safety_stale ||
                   !link->hello_ack_complete || !link->safety_snapshot_valid || !link->cts_known)
                      ? CANVIEW_UART_LINK_SUSPECT
                      : CANVIEW_UART_LINK_ONLINE;
    return CANVIEW_OK;
}

bool canview_uart_link_command_admission_allowed(const canview_uart_link_t *link,
                                                 uint64_t now_ms)
{
    if (link == NULL || !link->hello_complete || !link->hello_ack_complete ||
        !link->safety_snapshot_valid || !link->heartbeat_seen || !link->cts_known ||
        link->state != CANVIEW_UART_LINK_ONLINE ||
        elapsed_ms(now_ms, link->last_heartbeat_ms) >= CANVIEW_UART_HEARTBEAT_ONLINE_MS ||
        elapsed_ms(now_ms, link->safety_snapshot_time_ms) >=
            CANVIEW_UART_SAFETY_SNAPSHOT_MAX_AGE_MS)
    {
        return false;
    }
    return !link->cts_blocked ||
           elapsed_ms(now_ms, link->cts_blocked_since_ms) < CANVIEW_UART_CTS_COMMAND_STOP_MS;
}

bool canview_uart_command_admission_allowed(
    const canview_uart_link_t *link,
    const canview_uart_message_view_t *request,
    const canview_uart_command_admission_context_t *context, uint64_t now_ms)
{
    if (request == NULL || request->policy == NULL ||
        request->wire.header.message_type != CANVIEW_UART_MSG_COMMAND_REQUEST ||
        context == NULL || context->authorize == NULL ||
        !canview_uart_link_command_admission_allowed(link, now_ms))
    {
        return false;
    }
    return context->authorize(request, now_ms, context->context);
}

canview_status_t canview_uart_command_dispatch_admit(
    const canview_uart_link_t *link, const canview_uart_message_view_t *request,
    const canview_uart_command_admission_context_t *authorization,
    canview_uart_replay_context_t *replay, uint64_t now_ms)
{
    if (link == NULL || request == NULL || authorization == NULL || replay == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (!canview_uart_link_command_admission_allowed(link, now_ms))
    {
        return CANVIEW_TIMEOUT;
    }
    return canview_uart_message_admit(request, CANVIEW_UART_ENDPOINT_STM32,
                                      CANVIEW_UART_FLOW_INBOUND, replay, authorization, now_ms);
}

static canview_status_t session_invalidate_state(canview_uart_plan_context_t *plan,
                                                 canview_uart_command_cache_t *cache)
{
    if (plan == NULL || cache == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t cache_status = canview_uart_command_cache_reset(cache);
    if (cache_status != CANVIEW_OK)
    {
        return cache_status;
    }
    return canview_uart_plan_reset(plan);
}

static bool link_session_expired(const canview_uart_link_t *link, uint64_t now_ms)
{
    if (link == NULL)
    {
        return true;
    }
    const bool heartbeat_offline =
        link->heartbeat_seen &&
        elapsed_ms(now_ms, link->last_heartbeat_ms) >= CANVIEW_UART_HEARTBEAT_OFFLINE_MS;
    const bool cts_offline = link->cts_known && link->cts_blocked &&
                             elapsed_ms(now_ms, link->cts_blocked_since_ms) >=
                                 CANVIEW_UART_CTS_OFFLINE_MS;
    return heartbeat_offline || cts_offline;
}

static bool link_has_stale_session(const canview_uart_link_t *link)
{
    return link != NULL && !link->hello_complete && link->peer_boot_id != 0U;
}

canview_status_t canview_uart_session_reset(canview_uart_link_t *link,
                                             canview_uart_plan_context_t *plan,
                                             canview_uart_command_cache_t *cache)
{
    if (link == NULL || plan == NULL || cache == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t state_status = session_invalidate_state(plan, cache);
    if (state_status != CANVIEW_OK)
    {
        return state_status;
    }
    const canview_status_t link_status = canview_uart_link_reset(link);
    return link_status;
}

canview_status_t canview_uart_session_note_hello(
    canview_uart_link_t *link, canview_uart_plan_context_t *plan,
    canview_uart_command_cache_t *cache, uint64_t peer_boot_id, uint64_t now_ms,
    bool *boot_changed)
{
    if (link == NULL || plan == NULL || cache == NULL || boot_changed == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (peer_boot_id == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const bool new_session = link->peer_boot_id == 0U || !link->hello_complete ||
                             link->peer_boot_id != peer_boot_id;
    if (new_session)
    {
        const canview_status_t state_status = session_invalidate_state(plan, cache);
        if (state_status != CANVIEW_OK)
        {
            return state_status;
        }
    }
    return canview_uart_link_note_hello(link, peer_boot_id, now_ms, boot_changed);
}

canview_status_t canview_uart_session_note_heartbeat(
    canview_uart_link_t *link, canview_uart_plan_context_t *plan,
    canview_uart_command_cache_t *cache, uint64_t peer_boot_id, uint32_t safety_revision,
    uint64_t now_ms, bool *boot_changed)
{
    if (link == NULL || plan == NULL || cache == NULL || boot_changed == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (peer_boot_id == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const bool peer_epoch_changed = link->peer_boot_id != peer_boot_id;
    if (peer_epoch_changed || link_session_expired(link, now_ms) ||
        link_has_stale_session(link))
    {
        const canview_status_t state_status = session_invalidate_state(plan, cache);
        if (state_status != CANVIEW_OK)
        {
            return state_status;
        }
    }
    return canview_uart_link_note_heartbeat(link, peer_boot_id, safety_revision, now_ms,
                                            boot_changed);
}

canview_status_t canview_uart_session_tick(canview_uart_link_t *link,
                                            canview_uart_plan_context_t *plan,
                                            canview_uart_command_cache_t *cache,
                                            uint64_t now_ms)
{
    if (link == NULL || plan == NULL || cache == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (link_session_expired(link, now_ms) || link_has_stale_session(link))
    {
        const canview_status_t state_status = session_invalidate_state(plan, cache);
        if (state_status != CANVIEW_OK)
        {
            return state_status;
        }
        link_require_hello(link);
        return CANVIEW_OK;
    }
    return canview_uart_link_tick(link, now_ms);
}
