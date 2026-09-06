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
#define UART_COMMAND_TTL_MIN_MS (500U)
#define UART_COMMAND_TTL_MAX_MS (30000U)

static uint64_t read_le(const uint8_t *bytes, size_t width)
{
    uint64_t value = 0U;
    for (size_t index = 0U; index < width; ++index)
    {
        value |= (uint64_t)bytes[index] << (index * 8U);
    }
    return value;
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
        return size == UART_PLAN_COMMIT_SIZE && read_le(payload + 4U, 8U) != 0U
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
        read_le(payload + 10U, 2U) < UART_COMMAND_TTL_MIN_MS ||
        read_le(payload + 10U, 2U) > UART_COMMAND_TTL_MAX_MS ||
        !zero_range(payload, size, 54U, 2U))
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
    if (message_type == CANVIEW_UART_MSG_FIRMWARE_PREPARE &&
        (!zero_range(payload, size, 10U, 2U) || read_le(payload, 8U) == 0U))
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

canview_status_t canview_uart_message_validate(const canview_wire_view_t *wire,
                                               canview_uart_message_view_t *view)
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
    return CANVIEW_OK;
}

canview_status_t canview_uart_message_encode(uint8_t message_type, uint8_t flags,
                                             uint32_t sequence, uint32_t correlation_id,
                                             uint64_t sender_time_us, const uint8_t *payload,
                                             size_t payload_size, uint8_t *scratch,
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
    canview_status_t status = canview_uart_message_validate(&wire, &view);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    return canview_uart_packet_encode(&header, payload, payload_size, scratch, scratch_size, out,
                                      capacity, written);
}

canview_status_t canview_uart_codec_reset(canview_uart_codec_t *codec)
{
    if (codec == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(codec, 0, sizeof(*codec));
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
    const canview_status_t semantic = canview_uart_message_validate(&wire, view);
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

static canview_status_t plan_begin(canview_uart_plan_context_t *context, const uint8_t *payload)
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
    context->pending = true;
    context->request_token = read_le(payload + 4U, 8U);
    context->revision = (uint32_t)read_le(payload + 12U, 4U);
    context->expected_active_revision = expected_revision;
    context->total_chunks = (uint16_t)read_le(payload + 20U, 2U);
    context->next_chunk = 0U;
    context->expected_filter_count = (uint16_t)read_le(payload + 22U, 2U);
    context->received_filter_count = 0U;
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
    memcpy(context->candidate.plan_digest, payload + 16U, CANVIEW_UART_PLAN_DIGEST_SIZE);
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
                                         const uint8_t *payload, size_t payload_size)
{
    if (context == NULL || payload == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t valid = validate_plan_packet(payload, payload_size);
    if (valid != CANVIEW_OK)
    {
        return valid;
    }
    switch (payload[0])
    {
    case CANVIEW_UART_PLAN_OP_BEGIN:
        return plan_begin(context, payload);
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
    const uint64_t ttl = UINT64_C(60000);
    return now_ms > UINT64_MAX - ttl ? UINT64_MAX : now_ms + ttl;
}

static bool entry_expired(const canview_uart_command_entry_t *entry, uint64_t now_ms)
{
    return entry->valid && entry->expires_at_ms <= now_ms;
}

canview_status_t canview_uart_command_cache_reset(canview_uart_command_cache_t *cache)
{
    if (cache == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(cache, 0, sizeof(*cache));
    return CANVIEW_OK;
}

canview_status_t canview_uart_command_cache_admit(
    canview_uart_command_cache_t *cache, uint64_t request_token, uint16_t command_id,
    const uint8_t digest[CANVIEW_UART_COMMAND_DIGEST_SIZE], uint64_t now_ms, size_t *entry_index)
{
    if (cache == NULL || digest == NULL || entry_index == NULL || request_token == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *entry_index = 0U;
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
        if (entry->request_token == request_token)
        {
            *entry_index = index;
            if (entry->command_id != command_id ||
                memcmp(entry->canonical_argument_digest, digest, CANVIEW_UART_COMMAND_DIGEST_SIZE) !=
                    0)
            {
                return CANVIEW_MALFORMED;
            }
            return CANVIEW_DUPLICATE;
        }
    }
    if (free_index == CANVIEW_UART_COMMAND_CACHE_CAPACITY)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    canview_uart_command_entry_t *entry = &cache->entries[free_index];
    memset(entry, 0, sizeof(*entry));
    entry->valid = true;
    entry->request_token = request_token;
    entry->command_id = command_id;
    entry->expires_at_ms = expiry_for(now_ms);
    memcpy(entry->canonical_argument_digest, digest, CANVIEW_UART_COMMAND_DIGEST_SIZE);
    *entry_index = free_index;
    return CANVIEW_OK;
}

canview_status_t canview_uart_command_cache_mark_ack(canview_uart_command_cache_t *cache,
                                                     size_t entry_index)
{
    if (cache == NULL || entry_index >= CANVIEW_UART_COMMAND_CACHE_CAPACITY)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (!cache->entries[entry_index].valid)
    {
        return CANVIEW_STALE;
    }
    cache->entries[entry_index].acknowledged = true;
    return CANVIEW_OK;
}

canview_status_t canview_uart_command_cache_record_result(
    canview_uart_command_cache_t *cache, size_t entry_index, const uint8_t *result,
    size_t result_size, uint64_t now_ms)
{
    if (cache == NULL || entry_index >= CANVIEW_UART_COMMAND_CACHE_CAPACITY ||
        (result == NULL && result_size != 0U) || result_size == 0U ||
        result_size > CANVIEW_UART_COMMAND_RESULT_MAX)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_uart_command_entry_t *entry = &cache->entries[entry_index];
    if (!entry->valid || entry_expired(entry, now_ms))
    {
        if (entry->valid)
        {
            memset(entry, 0, sizeof(*entry));
        }
        return CANVIEW_STALE;
    }
    memcpy(entry->result, result, result_size);
    entry->result_size = (uint16_t)result_size;
    entry->terminal = true;
    entry->expires_at_ms = expiry_for(now_ms);
    return CANVIEW_OK;
}

canview_status_t canview_uart_command_cache_lookup(
    canview_uart_command_cache_t *cache, uint64_t request_token, uint16_t command_id,
    const uint8_t digest[CANVIEW_UART_COMMAND_DIGEST_SIZE], uint64_t now_ms, size_t *entry_index,
    const uint8_t **result, size_t *result_size)
{
    if (cache == NULL || digest == NULL || entry_index == NULL || result == NULL ||
        result_size == NULL || request_token == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *entry_index = 0U;
    *result = NULL;
    *result_size = 0U;
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
        if (entry->request_token != request_token)
        {
            continue;
        }
        *entry_index = index;
        if (entry->command_id != command_id ||
            memcmp(entry->canonical_argument_digest, digest, CANVIEW_UART_COMMAND_DIGEST_SIZE) != 0)
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
    if (cache == NULL)
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
    *boot_changed = link->hello_complete && link->peer_boot_id != peer_boot_id;
    link->peer_boot_id = peer_boot_id;
    link->hello_complete = true;
    link->heartbeat_seen = false;
    link->last_heartbeat_ms = now_ms;
    link->state = CANVIEW_UART_LINK_OFFLINE;
    return CANVIEW_OK;
}

canview_status_t canview_uart_link_note_heartbeat(canview_uart_link_t *link,
                                                  uint64_t peer_boot_id, uint64_t now_ms,
                                                  bool *boot_changed)
{
    if (link == NULL || boot_changed == NULL || peer_boot_id == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *boot_changed = link->heartbeat_seen && link->peer_boot_id != peer_boot_id;
    if (!link->hello_complete || link->peer_boot_id != peer_boot_id)
    {
        if (link->peer_boot_id != 0U && link->peer_boot_id != peer_boot_id)
        {
            *boot_changed = true;
        }
        link->peer_boot_id = peer_boot_id;
        link->heartbeat_seen = false;
        link->hello_complete = false;
        link->state = CANVIEW_UART_LINK_OFFLINE;
        return CANVIEW_OK;
    }
    link->heartbeat_seen = true;
    link->last_heartbeat_ms = now_ms;
    return canview_uart_link_tick(link, now_ms);
}

canview_status_t canview_uart_link_set_cts_blocked(canview_uart_link_t *link, bool blocked,
                                                   uint64_t now_ms)
{
    if (link == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
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
    if (!link->hello_complete || !link->heartbeat_seen ||
        elapsed_ms(now_ms, link->last_heartbeat_ms) >= CANVIEW_UART_HEARTBEAT_OFFLINE_MS)
    {
        link->state = CANVIEW_UART_LINK_OFFLINE;
        return CANVIEW_OK;
    }
    const bool heartbeat_suspect =
        elapsed_ms(now_ms, link->last_heartbeat_ms) >= CANVIEW_UART_HEARTBEAT_ONLINE_MS;
    const bool cts_suspect = link->cts_blocked &&
                             elapsed_ms(now_ms, link->cts_blocked_since_ms) >=
                                 CANVIEW_UART_CTS_COMMAND_STOP_MS;
    const bool cts_offline = link->cts_blocked &&
                             elapsed_ms(now_ms, link->cts_blocked_since_ms) >=
                                 CANVIEW_UART_CTS_OFFLINE_MS;
    link->state = (cts_offline ||
                   (heartbeat_suspect &&
                    elapsed_ms(now_ms, link->last_heartbeat_ms) >=
                        CANVIEW_UART_HEARTBEAT_OFFLINE_MS))
                      ? CANVIEW_UART_LINK_OFFLINE
                      : ((heartbeat_suspect || cts_suspect) ? CANVIEW_UART_LINK_SUSPECT
                                                             : CANVIEW_UART_LINK_ONLINE);
    return CANVIEW_OK;
}

bool canview_uart_link_command_admission_allowed(const canview_uart_link_t *link,
                                                 uint64_t now_ms)
{
    if (link == NULL || !link->hello_complete || !link->heartbeat_seen ||
        link->state != CANVIEW_UART_LINK_ONLINE ||
        elapsed_ms(now_ms, link->last_heartbeat_ms) >= CANVIEW_UART_HEARTBEAT_ONLINE_MS)
    {
        return false;
    }
    return !link->cts_blocked ||
           elapsed_ms(now_ms, link->cts_blocked_since_ms) < CANVIEW_UART_CTS_COMMAND_STOP_MS;
}
