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

    CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
    for (size_t index = 0U; index < CANVIEW_UART_MAX_ENCODED_SIZE + 1U; ++index)
    {
        status = canview_uart_codec_feed(&codec, UINT8_C(0xA5), &view);
    }
    CHECK(status == CANVIEW_OVERSIZE && codec.oversize_packets == 1U);
    CHECK(canview_uart_codec_feed(&codec, UINT8_C(0xA5), &view) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_codec_feed(&codec, 0U, &view) == CANVIEW_INCOMPLETE);
    CHECK(feed_frame(&codec, serial, serial_size, &view) == 0);
    CHECK(codec.oversize_packets == 1U && codec.packets_ok == 1U);
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

static int test_limits(void)
{
    canview_uart_codec_t codec;
    canview_uart_message_view_t view;
    CHECK(canview_uart_codec_reset(&codec) == CANVIEW_OK);
    CHECK(canview_uart_codec_feed(&codec, 0U, &view) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_codec_feed(NULL, 0U, &view) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_codec_feed(&codec, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_message_policy(CANVIEW_UART_MSG_LINK_HELLO, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_plan_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
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
    if (strcmp(scenario, "command") == 0)
    {
        return test_command_cache();
    }
    if (strcmp(scenario, "link") == 0)
    {
        return test_link();
    }
    if (strcmp(scenario, "limits") == 0)
    {
        return test_limits();
    }
    if (strcmp(scenario, "all") == 0)
    {
        return test_message_matrix() || test_malformed() || test_stream() || test_plan() ||
               test_command_cache() || test_link() || test_limits();
    }
    (void)fprintf(stderr, "unknown UART scenario: %s\n", scenario);
    return 2;
}

int main(int argc, char **argv)
{
    return argc == 2 ? run_scenario(argv[1]) : run_scenario("all");
}
