/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_wire.h"
#include "canview_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr)                                                                                \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr);                       \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static int test_crc(void)
{
    uint32_t crc = 123U;
    CHECK(canview_wire_crc32((const uint8_t *)"123456789", 9U, &crc) == CANVIEW_OK);
    CHECK(crc == CANVIEW_WIRE_CRC_CHECK_123456789);
    CHECK(canview_wire_crc32(NULL, 0U, &crc) == CANVIEW_OK && crc == 0U);
    CHECK(canview_wire_crc32(NULL, 1U, &crc) == CANVIEW_INVALID_ARGUMENT && crc == 0U);
    CHECK(canview_wire_crc32(NULL, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);
    return 0;
}

static int test_envelope(void)
{
    uint8_t payload[992];
    uint8_t bytes[1025];
    canview_wire_view_t view;
    for (size_t i = 0U; i < sizeof(payload); ++i)
    {
        payload[i] = (uint8_t)i;
    }
    for (uint32_t kind = 0U; kind < 2U; ++kind)
    {
        const canview_wire_transport_t transport = (canview_wire_transport_t)kind;
        const size_t maximum = kind == 0U ? 208U : 992U;
        canview_wire_header_t header = {0};
        header.message_type = kind == 0U ? 0x20U : 0x10U;
        header.flags = kind == 0U ? 0x40U : 0x10U;
        header.sequence = UINT32_MAX;
        header.correlation_id = UINT32_C(0x12345678);
        header.sender_time = kind == 0U ? UINT32_MAX : UINT64_MAX;
        header.session_id = kind == 0U ? 17U : 0U;
        header.priority = kind == 0U ? 4U : 0U;
        for (size_t size = 0U; size <= maximum; ++size)
        {
            size_t written = 0U;
            CHECK(canview_wire_envelope_encode(transport, &header, payload, size, bytes,
                                               sizeof(bytes), &written) == CANVIEW_OK);
            CHECK(written == 32U + size);
            CHECK(canview_wire_envelope_decode(transport, bytes, written, &view) == CANVIEW_OK);
            CHECK(view.payload_size == size && memcmp(view.payload, payload, size) == 0);
            CHECK(view.header.sequence == UINT32_MAX &&
                  view.header.sender_time == header.sender_time);
            CHECK(view.header.session_id == header.session_id && view.header.flags == header.flags);
            CHECK(view.header.priority == header.priority &&
                  view.header.message_type == header.message_type);
            CHECK(view.header.correlation_id == header.correlation_id);
            CHECK(canview_wire_envelope_encode(transport, &header, payload, size, bytes,
                                               written - 1U,
                                               &written) == CANVIEW_BUFFER_TOO_SMALL &&
                  written == 0U);
        }
        size_t written = 99U;
        CHECK(canview_wire_envelope_encode(transport, &header, payload, maximum + 1U, bytes,
                                           sizeof(bytes), &written) == CANVIEW_OVERSIZE &&
              written == 0U);
        CHECK(canview_wire_envelope_encode(transport, &header, NULL, 0U, bytes, sizeof(bytes),
                                           &written) == CANVIEW_OK &&
              written == 32U);
        CHECK(canview_wire_envelope_decode(transport, bytes, 31U, &view) == CANVIEW_MALFORMED);
        CHECK(view.payload == NULL && view.payload_size == 0U);
        CHECK(canview_wire_envelope_decode(transport, bytes, maximum + 33U, &view) ==
              CANVIEW_OVERSIZE);
        CHECK(canview_wire_envelope_decode(transport, bytes, 33U, &view) == CANVIEW_MALFORMED);
        bytes[0] ^= 1U;
        CHECK(canview_wire_envelope_decode(transport, bytes, 32U, &view) == CANVIEW_MALFORMED);
        bytes[0] ^= 1U;
        bytes[2] ^= 0x80U;
        CHECK(canview_wire_envelope_decode(transport, bytes, 32U, &view) ==
              CANVIEW_UNSUPPORTED_VERSION);
        bytes[2] ^= 0x80U;
        bytes[3] ^= 0x80U;
        CHECK(canview_wire_envelope_decode(transport, bytes, 32U, &view) ==
              CANVIEW_UNSUPPORTED_VERSION);
        bytes[3] ^= 0x80U;
        const size_t reserved = kind == 0U ? 26U : 10U;
        bytes[reserved] = 1U;
        CHECK(canview_wire_envelope_decode(transport, bytes, 32U, &view) == CANVIEW_MALFORMED);
        bytes[reserved] = 0U;
        bytes[kind == 0U ? 4U : 6U] = 31U;
        CHECK(canview_wire_envelope_decode(transport, bytes, 32U, &view) == CANVIEW_MALFORMED);
        bytes[kind == 0U ? 4U : 6U] = 32U;
        bytes[kind == 0U ? 6U : 5U] |= 0x80U;
        CHECK(canview_wire_envelope_decode(transport, bytes, 32U, &view) == CANVIEW_MALFORMED);
        bytes[kind == 0U ? 6U : 5U] &= 0x7FU;
        if (kind == 0U)
        {
            bytes[7] = 5U;
            CHECK(canview_wire_envelope_decode(transport, bytes, 32U, &view) == CANVIEW_MALFORMED);
            bytes[7] = 4U;
        }
        bytes[28] ^= 1U;
        CHECK(canview_wire_envelope_decode(transport, bytes, 32U, &view) == CANVIEW_CRC_MISMATCH);
        header.flags = 0x80U;
        CHECK(canview_wire_envelope_encode(transport, &header, NULL, 0U, bytes, sizeof(bytes),
                                           &written) == CANVIEW_MALFORMED);
        header.flags = 0U;
        header.priority = 5U;
        CHECK(canview_wire_envelope_encode(transport, &header, NULL, 0U, bytes, sizeof(bytes),
                                           &written) == CANVIEW_MALFORMED);
        header.priority = 0U;
        if (kind == 0U)
        {
            header.sender_time = UINT64_MAX;
        }
        else
        {
            header.session_id = 1U;
        }
        CHECK(canview_wire_envelope_encode(transport, &header, NULL, 0U, bytes, sizeof(bytes),
                                           &written) == CANVIEW_MALFORMED);
    }
    /* Unaligned input and every single-bit corruption are rejected or parsed safely. */
    canview_wire_header_t header = {0};
    size_t written = 0U;
    header.message_type = 0x20U;
    CHECK(canview_wire_envelope_encode(CANVIEW_WIRE_ESPNOW, &header, payload, 208U, bytes + 1U,
                                       1024U, &written) == CANVIEW_OK);
    CHECK(canview_wire_envelope_decode(CANVIEW_WIRE_ESPNOW, bytes + 1U, written, &view) ==
          CANVIEW_OK);
    for (size_t i = 1U; i <= written; ++i)
    {
        for (uint32_t bit = 0U; bit < 8U; ++bit)
        {
            bytes[i] ^= (uint8_t)(1U << bit);
            CHECK(canview_wire_envelope_decode(CANVIEW_WIRE_ESPNOW, bytes + 1U, written, &view) !=
                  CANVIEW_OK);
            CHECK(view.payload == NULL);
            bytes[i] ^= (uint8_t)(1U << bit);
        }
    }
    return 0;
}

static int test_cobs(void)
{
    uint8_t input[1025], encoded[1031], decoded[1026];
    for (uint32_t pattern = 0U; pattern < 3U; ++pattern)
    {
        for (size_t i = 0U; i < sizeof(input); ++i)
        {
            input[i] = pattern == 0U ? 0U : pattern == 1U ? 255U : (uint8_t)i;
        }
        for (size_t size = 0U; size <= 1024U; ++size)
        {
            size_t encoded_size = 0U, decoded_size = 0U;
            encoded[0] = 0xAAU;
            decoded[0] = 0xBBU;
            encoded[1030] = 0xCCU;
            decoded[1025] = 0xDDU;
            CHECK(canview_wire_cobs_encode(input, size, encoded + 1U, 1029U, &encoded_size) ==
                  CANVIEW_OK);
            CHECK(encoded_size <= 1029U);
            CHECK(canview_wire_cobs_decode(encoded + 1U, encoded_size, decoded + 1U, 1024U,
                                           &decoded_size) == CANVIEW_OK);
            CHECK(decoded_size == size && memcmp(input, decoded + 1U, size) == 0);
            CHECK(encoded[0] == 0xAAU && encoded[1030] == 0xCCU && decoded[0] == 0xBBU &&
                  decoded[1025] == 0xDDU);
            if (size != 0U)
            {
                CHECK(canview_wire_cobs_decode(encoded + 1U, encoded_size, decoded + 1U, size - 1U,
                                               &decoded_size) == CANVIEW_BUFFER_TOO_SMALL);
                CHECK(decoded_size == 0U);
            }
        }
    }
    size_t n = 1U;
    CHECK(canview_wire_cobs_encode(NULL, 0U, encoded, 1U, &n) == CANVIEW_OK && n == 1U &&
          encoded[0] == 1U);
    CHECK(canview_wire_cobs_encode(input, 1025U, encoded, sizeof(encoded), &n) == CANVIEW_OVERSIZE);
    CHECK(canview_wire_cobs_encode(input, 1U, encoded, 0U, &n) == CANVIEW_BUFFER_TOO_SMALL);
    CHECK(canview_wire_cobs_decode(encoded, 0U, decoded, sizeof(decoded), &n) == CANVIEW_MALFORMED);
    CHECK(canview_wire_cobs_decode(encoded, 1030U, decoded, sizeof(decoded), &n) ==
          CANVIEW_OVERSIZE);
    encoded[0] = 0U;
    CHECK(canview_wire_cobs_decode(encoded, 1U, decoded, sizeof(decoded), &n) == CANVIEW_MALFORMED);
    encoded[0] = 3U;
    CHECK(canview_wire_cobs_decode(encoded, 2U, decoded, sizeof(decoded), &n) == CANVIEW_MALFORMED);
    encoded[0] = 2U;
    encoded[1] = 0U;
    CHECK(canview_wire_cobs_decode(encoded, 2U, decoded, sizeof(decoded), &n) == CANVIEW_MALFORMED);
    memset(encoded, 1, 1029U);
    CHECK(canview_wire_cobs_decode(encoded, 1029U, decoded, sizeof(decoded), &n) ==
          CANVIEW_BUFFER_TOO_SMALL);
    return 0;
}

static int test_stream(void)
{
    canview_uart_stream_t stream;
    canview_wire_view_t view;
    canview_wire_header_t header = {0};
    uint8_t serial[1030], scratch[1024], payload[992];
    size_t size = 0U;
    memset(payload, 0xA5, sizeof(payload));
    header.message_type = 0x10U;
    CHECK(canview_uart_stream_reset(&stream) == CANVIEW_OK);
    CHECK(canview_uart_stream_feed(&stream, 0U, &view) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_packet_encode(&header, payload, sizeof(payload), scratch, sizeof(scratch),
                                     serial, sizeof(serial), &size) == CANVIEW_OK);
    for (size_t i = 0U; i < size; ++i)
    {
        CHECK(canview_uart_stream_feed(&stream, serial[i], &view) ==
              (i + 1U == size ? CANVIEW_OK : CANVIEW_INCOMPLETE));
    }
    CHECK(view.payload_size == sizeof(payload) &&
          memcmp(view.payload, payload, sizeof(payload)) == 0);
    /* Overflow once, no trailing suffix admitted, next delimiter resynchronizes. */
    for (size_t i = 0U; i < 1035U; ++i)
    {
        CHECK(canview_uart_stream_feed(&stream, 1U, &view) ==
              (i == 1029U ? CANVIEW_OVERSIZE : CANVIEW_INCOMPLETE));
    }
    CHECK(canview_uart_stream_feed(&stream, 0U, &view) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_stream_feed(&stream, 3U, &view) == CANVIEW_INCOMPLETE);
    CHECK(canview_uart_stream_feed(&stream, 0U, &view) == CANVIEW_MALFORMED);
    for (size_t i = 0U; i < size; ++i)
    {
        CHECK(canview_uart_stream_feed(&stream, serial[i], &view) ==
              (i + 1U == size ? CANVIEW_OK : CANVIEW_INCOMPLETE));
    }
    serial[10] ^= 1U;
    canview_status_t status = CANVIEW_INCOMPLETE;
    for (size_t i = 0U; i < size; ++i)
    {
        status = canview_uart_stream_feed(&stream, serial[i], &view);
    }
    CHECK(status != CANVIEW_OK && view.payload == NULL);
    serial[10] ^= 1U;
    CHECK(canview_uart_packet_encode(&header, payload, 992U, scratch, 1023U, serial, sizeof(serial),
                                     &size) == CANVIEW_BUFFER_TOO_SMALL);
    CHECK(canview_uart_packet_encode(&header, payload, 992U, scratch, sizeof(scratch), serial, 1U,
                                     &size) == CANVIEW_BUFFER_TOO_SMALL);
    CHECK(canview_uart_packet_encode(&header, payload, 992U, scratch, sizeof(scratch), serial, 0U,
                                     &size) == CANVIEW_BUFFER_TOO_SMALL);
    return 0;
}

static int test_can(void)
{
    canview_wire_can_batch_t batch = {0}, decoded;
    uint8_t bytes[205];
    size_t size = 0U;
    batch.base_time_us = UINT64_C(0x0102030405060708);
    batch.dropped_since_last = 255U;
    for (uint8_t count = 0U; count <= 12U; ++count)
    {
        batch.count = count;
        for (size_t i = 0U; i < count; ++i)
        {
            batch.records[i].bus_id = (uint8_t)(i % 3U);
            batch.records[i].delta_us = (uint16_t)(i * 100U);
            batch.records[i].can_id = 0x7FFU;
            batch.records[i].dlc = 8U;
            memset(batch.records[i].data, (int)i, 8U);
        }
        CHECK(canview_wire_can_batch_encode(&batch, bytes, sizeof(bytes), &size) == CANVIEW_OK);
        CHECK(size == 12U + 16U * count);
        CHECK(canview_wire_can_batch_decode(bytes, size, &decoded) == CANVIEW_OK);
        CHECK(decoded.count == count && decoded.base_time_us == batch.base_time_us &&
              decoded.dropped_since_last == 255U);
        for (size_t i = 0U; i < count; ++i)
        {
            CHECK(decoded.records[i].can_id == 0x7FFU && decoded.records[i].delta_us == i * 100U);
            CHECK(memcmp(decoded.records[i].data, batch.records[i].data, 8U) == 0);
        }
        CHECK(canview_wire_can_batch_decode(bytes, size - 1U, &decoded) == CANVIEW_MALFORMED &&
              decoded.count == 0U);
        CHECK(canview_wire_can_batch_encode(&batch, bytes, size - 1U, &size) ==
              CANVIEW_BUFFER_TOO_SMALL);
    }
    batch.count = 13U;
    CHECK(canview_wire_can_batch_encode(&batch, bytes, sizeof(bytes), &size) == CANVIEW_MALFORMED);
    batch.count = 1U;
    canview_wire_can_record_t *r = &batch.records[0];
    r->flags = 1U;
    r->can_id = 0x1FFFFFFFU;
    r->delta_us = UINT16_MAX;
    CHECK(canview_wire_can_batch_encode(&batch, bytes, sizeof(bytes), &size) == CANVIEW_OK);
    CHECK(canview_wire_can_batch_decode(bytes, size, &decoded) == CANVIEW_OK);
    for (uint32_t invalid = 0U; invalid < 7U; ++invalid)
    {
        canview_wire_can_batch_t wrong = batch;
        if (invalid == 0U)
        {
            wrong.records[0].bus_id = 3U;
        }
        if (invalid == 1U)
        {
            wrong.records[0].dlc = 9U;
        }
        if (invalid == 2U)
        {
            wrong.records[0].flags = 16U;
        }
        if (invalid == 3U)
        {
            wrong.records[0].can_id = 0x20000000U;
        }
        if (invalid == 4U)
        {
            wrong.records[0].flags = 0U;
        }
        if (invalid == 5U)
        {
            wrong.records[0].dlc = 1U;
            wrong.records[0].data[1] = 1U;
        }
        if (invalid == 6U)
        {
            wrong.base_time_us = UINT64_MAX;
        }
        CHECK(canview_wire_can_batch_encode(&wrong, bytes, sizeof(bytes), &size) ==
                  CANVIEW_MALFORMED &&
              size == 0U);
    }
    r->flags = 2U;
    r->can_id = 1U;
    memset(r->data, 0, 8U);
    CHECK(canview_wire_can_batch_encode(&batch, bytes, sizeof(bytes), &size) == CANVIEW_OK);
    CHECK(canview_wire_can_batch_decode(bytes, size, &decoded) == CANVIEW_OK);
    r->data[0] = 1U;
    CHECK(canview_wire_can_batch_encode(&batch, bytes, sizeof(bytes), &size) == CANVIEW_MALFORMED);
    r->data[0] = 0U;
    r->flags = 0U;
    r->dlc = 0U;
    CHECK(canview_wire_can_batch_encode(&batch, bytes, sizeof(bytes), &size) == CANVIEW_OK);
    bytes[8] = 13U;
    CHECK(canview_wire_can_batch_decode(bytes, size, &decoded) == CANVIEW_MALFORMED);
    bytes[8] = 1U;
    bytes[10] = 1U;
    CHECK(canview_wire_can_batch_decode(bytes, size, &decoded) == CANVIEW_MALFORMED);
    bytes[10] = 0U;
    bytes[14] = 255U;
    CHECK(canview_wire_can_batch_decode(bytes, size, &decoded) == CANVIEW_MALFORMED);
    bytes[14] = 0U;
    memset(bytes, 255, 8U);
    CHECK(canview_wire_can_batch_decode(bytes, size, &decoded) == CANVIEW_MALFORMED);
    return 0;
}

static int test_sequence(void)
{
    canview_sequence_window_t window;
    CHECK(canview_sequence_window_reset(&window) == CANVIEW_OK);
    CHECK(canview_sequence_window_accept(&window, UINT32_MAX - 1U) == CANVIEW_OK);
    CHECK(canview_sequence_window_accept(&window, 0U) == CANVIEW_OK);
    CHECK(canview_sequence_window_accept(&window, UINT32_MAX) == CANVIEW_OK);
    CHECK(canview_sequence_window_accept(&window, UINT32_MAX) == CANVIEW_DUPLICATE);
    CHECK(canview_sequence_window_accept(&window, 0U) == CANVIEW_DUPLICATE);
    CHECK(canview_sequence_window_accept(&window, 100U) == CANVIEW_OK);
    CHECK(canview_sequence_window_accept(&window, 37U) == CANVIEW_OK);
    CHECK(canview_sequence_window_accept(&window, 36U) == CANVIEW_STALE);
    CHECK(canview_sequence_window_accept(&window, 100U + UINT32_C(0x80000000)) == CANVIEW_STALE);
    const canview_sequence_window_t before = window;
    CHECK(canview_sequence_window_accept(&window, 36U) == CANVIEW_STALE);
    CHECK(memcmp(&window, &before, sizeof(window)) == 0);
    CHECK(canview_sequence_window_reset(&window) == CANVIEW_OK);
    CHECK(canview_sequence_window_accept(&window, 0U) == CANVIEW_OK);
    for (uint32_t i = 1U; i < 10000U; ++i)
    {
        CHECK(canview_sequence_window_accept(&window, i) == CANVIEW_OK);
    }
    return 0;
}

static int test_nulls(void)
{
    canview_wire_header_t header = {0};
    canview_wire_view_t view;
    canview_wire_can_batch_t batch = {0};
    uint8_t bytes[1030] = {0}, scratch[1024];
    size_t n = 0U;
    CHECK(canview_wire_envelope_encode(CANVIEW_WIRE_ESPNOW, &header, NULL, 0U, bytes, sizeof(bytes),
                                       NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_envelope_encode((canview_wire_transport_t)99, &header, NULL, 0U, bytes,
                                       sizeof(bytes), &n) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_envelope_encode(CANVIEW_WIRE_ESPNOW, NULL, NULL, 0U, bytes, sizeof(bytes),
                                       &n) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_envelope_encode(CANVIEW_WIRE_ESPNOW, &header, NULL, 0U, NULL, 0U, &n) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_envelope_encode(CANVIEW_WIRE_ESPNOW, &header, NULL, 1U, bytes, sizeof(bytes),
                                       &n) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_envelope_decode(CANVIEW_WIRE_ESPNOW, bytes, 32U, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_envelope_decode((canview_wire_transport_t)99, bytes, 32U, &view) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_envelope_decode(CANVIEW_WIRE_ESPNOW, NULL, 32U, &view) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_cobs_encode(NULL, 1U, bytes, sizeof(bytes), &n) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_cobs_encode(bytes, 1U, NULL, 0U, &n) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_cobs_encode(bytes, 1U, scratch, sizeof(scratch), NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_cobs_decode(NULL, 1U, bytes, sizeof(bytes), &n) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_cobs_decode(bytes, 1U, NULL, 0U, &n) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_cobs_decode(bytes, 1U, scratch, sizeof(scratch), NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_stream_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_stream_feed(NULL, 0U, &view) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_stream_feed(NULL, 0U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_packet_encode(&header, bytes, 1U, scratch, sizeof(scratch), bytes,
                                     sizeof(bytes), NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_packet_encode(&header, NULL, 0U, NULL, 0U, bytes, sizeof(bytes), &n) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_uart_packet_encode(&header, NULL, 0U, scratch, sizeof(scratch), NULL, 0U, &n) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_can_batch_decode(bytes, 12U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_can_batch_decode(NULL, 12U, &batch) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_can_batch_encode(&batch, bytes, sizeof(bytes), NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_can_batch_encode(NULL, bytes, sizeof(bytes), &n) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_wire_can_batch_encode(&batch, NULL, 0U, &n) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_sequence_window_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_sequence_window_accept(NULL, 0U) == CANVIEW_INVALID_ARGUMENT);
    return 0;
}

static int test_noise(void)
{
    uint32_t state = 1234567U;
    uint8_t bytes[1501];
    canview_wire_view_t view;
    canview_uart_stream_t stream;
    canview_wire_can_batch_t batch;
    CHECK(canview_uart_stream_reset(&stream) == CANVIEW_OK);
    for (size_t trial = 0U; trial < 5000U; ++trial)
    {
        const size_t size = trial % sizeof(bytes);
        for (size_t i = 0U; i < size; ++i)
        {
            state = state * UINT32_C(1664525) + UINT32_C(1013904223);
            bytes[i] = (uint8_t)(state >> 24U);
            (void)canview_uart_stream_feed(&stream, bytes[i], &view);
        }
        (void)canview_wire_envelope_decode(CANVIEW_WIRE_ESPNOW, bytes, size, &view);
        (void)canview_wire_envelope_decode(CANVIEW_WIRE_UART, bytes, size, &view);
        (void)canview_wire_can_batch_decode(bytes, size, &batch);
        (void)canview_uart_stream_feed(&stream, 0U, &view);
    }
    return 0;
}

typedef struct
{
    uint32_t safe_calls;
    uint32_t idle_calls;
    canview_status_t result;
} fake_port_t;
static canview_status_t fake_safe(void *context)
{
    fake_port_t *fake = context;
    ++fake->safe_calls;
    return fake->result;
}
static void fake_idle(void *context)
{
    fake_port_t *fake = context;
    ++fake->idle_calls;
}
static int test_app(void)
{
    for (uint32_t role = 0U; role < 4U; ++role)
    {
        fake_port_t fake = {0};
        canview_platform_port_t port = {fake_safe, fake_idle, &fake};
        canview_app_t app = {0};
        CHECK(canview_app_start(&app, (canview_app_role_t)role, &port) == CANVIEW_OK);
        CHECK(app.state == CANVIEW_APP_SAFE_IDLE && fake.safe_calls == 1U);
        CHECK(canview_app_start(&app, (canview_app_role_t)role, &port) == CANVIEW_INVALID_ARGUMENT);
        CHECK(canview_app_step(&app) == CANVIEW_OK && fake.idle_calls == 1U);
    }
    fake_port_t fake = {0};
    fake.result = CANVIEW_NOT_IMPLEMENTED;
    canview_platform_port_t port = {fake_safe, fake_idle, &fake};
    canview_app_t app = {0};
    CHECK(canview_app_step(&app) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_app_step(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_app_start(NULL, CANVIEW_APP_CONTROLLER, &port) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_app_start(&app, CANVIEW_APP_CONTROLLER, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_app_start(&app, (canview_app_role_t)99, &port) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_app_start(&app, (canview_app_role_t)-1, &port) == CANVIEW_INVALID_ARGUMENT);
    port.enter_safe_state = NULL;
    CHECK(canview_app_start(&app, CANVIEW_APP_CONTROLLER, &port) == CANVIEW_INVALID_ARGUMENT);
    port.enter_safe_state = fake_safe;
    port.idle = NULL;
    CHECK(canview_app_start(&app, CANVIEW_APP_CONTROLLER, &port) == CANVIEW_INVALID_ARGUMENT);
    port.idle = fake_idle;
    CHECK(canview_app_start(&app, CANVIEW_APP_CONTROLLER, &port) == CANVIEW_NOT_IMPLEMENTED);
    CHECK(app.state == CANVIEW_APP_FAULT && fake.safe_calls == 1U);
    CHECK(canview_app_step(&app) == CANVIEW_NOT_IMPLEMENTED && fake.idle_calls == 1U);
    app.state = CANVIEW_APP_UNINITIALIZED;
    CHECK(canview_app_step(&app) == CANVIEW_INVALID_ARGUMENT);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        return 2;
    }
    if (strcmp(argv[1], "crc") == 0)
    {
        return test_crc();
    }
    if (strcmp(argv[1], "envelope") == 0)
    {
        return test_envelope();
    }
    if (strcmp(argv[1], "cobs") == 0)
    {
        return test_cobs();
    }
    if (strcmp(argv[1], "stream") == 0)
    {
        return test_stream();
    }
    if (strcmp(argv[1], "can") == 0)
    {
        return test_can();
    }
    if (strcmp(argv[1], "sequence") == 0)
    {
        return test_sequence();
    }
    if (strcmp(argv[1], "nulls") == 0)
    {
        return test_nulls();
    }
    if (strcmp(argv[1], "noise") == 0)
    {
        return test_noise();
    }
    if (strcmp(argv[1], "app") == 0)
    {
        return test_app();
    }
    return 2;
}
