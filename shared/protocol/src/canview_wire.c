/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_wire.h"
#include <string.h>

#define WIRE_BITS_PER_BYTE (8U)
#define WIRE_CRC_OFFSET (28U)
#define WIRE_CRC_SIZE (4U)
#define WIRE_HEADER_SIZE (32U)
#define WIRE_COBS_BLOCK (254U)
#define WIRE_SEQUENCE_HALF UINT32_C(0x80000000)
#define WIRE_SEQUENCE_WINDOW (64U)
#define WIRE_CAN_IDE (1U)
#define WIRE_CAN_RTR (2U)
#define WIRE_CAN_FLAGS_MASK (15U)

static uint64_t read_le(const uint8_t *bytes, size_t width)
{
    uint64_t value = 0U;
    for (size_t i = 0U; i < width; ++i)
    {
        value |= (uint64_t)bytes[i] << (i * WIRE_BITS_PER_BYTE);
    }
    return value;
}

static void write_le(uint8_t *bytes, uint64_t value, size_t width)
{
    for (size_t i = 0U; i < width; ++i)
    {
        bytes[i] = (uint8_t)(value >> (i * WIRE_BITS_PER_BYTE));
    }
}

static uint32_t crc_step(uint32_t crc, uint8_t byte)
{
    crc ^= byte;
    for (uint32_t bit = 0U; bit < WIRE_BITS_PER_BYTE; ++bit)
    {
        crc = (crc >> 1U) ^ ((crc & 1U) != 0U ? CANVIEW_WIRE_CRC_POLYNOMIAL_REFLECTED : 0U);
    }
    return crc;
}

static uint32_t envelope_crc(const uint8_t *bytes, size_t size)
{
    uint32_t crc = CANVIEW_WIRE_CRC_INITIAL;
    for (size_t i = 0U; i < size; ++i)
    {
        const uint8_t byte =
            (i >= WIRE_CRC_OFFSET && i < WIRE_CRC_OFFSET + WIRE_CRC_SIZE) ? 0U : bytes[i];
        crc = crc_step(crc, byte);
    }
    return crc ^ CANVIEW_WIRE_CRC_XOR_OUT;
}

canview_status_t canview_wire_crc32(const uint8_t *bytes, size_t size, uint32_t *crc)
{
    if (crc == NULL || (bytes == NULL && size != 0U))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint32_t value = CANVIEW_WIRE_CRC_INITIAL;
    for (size_t i = 0U; i < size; ++i)
    {
        value = crc_step(value, bytes[i]);
    }
    *crc = value ^ CANVIEW_WIRE_CRC_XOR_OUT;
    return CANVIEW_OK;
}

static bool valid_transport(canview_wire_transport_t transport)
{
    return transport == CANVIEW_WIRE_ESPNOW || transport == CANVIEW_WIRE_UART;
}

canview_status_t canview_wire_envelope_encode(canview_wire_transport_t transport,
                                              const canview_wire_header_t *header,
                                              const uint8_t *payload, size_t payload_size,
                                              uint8_t *out, size_t capacity, size_t *written)
{
    if (written == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *written = 0U;
    if (!valid_transport(transport) || header == NULL || out == NULL ||
        (payload == NULL && payload_size != 0U))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const bool radio = transport == CANVIEW_WIRE_ESPNOW;
    const size_t maximum = radio ? CANVIEW_WIRE_ESPNOW_MAX_PAYLOAD : CANVIEW_WIRE_UART_MAX_PAYLOAD;
    const uint8_t mask = radio ? CANVIEW_WIRE_ESPNOW_FLAGS_MASK : CANVIEW_WIRE_UART_FLAGS_MASK;
    if (payload_size > maximum)
    {
        return CANVIEW_OVERSIZE;
    }
    if ((header->flags & (uint8_t)~mask) != 0U ||
        (radio && (header->priority > CANVIEW_WIRE_ESPNOW_MAX_PRIORITY ||
                   header->sender_time > UINT32_MAX)) ||
        (!radio && (header->priority != 0U || header->session_id != 0U)))
    {
        return CANVIEW_MALFORMED;
    }
    const size_t size = WIRE_HEADER_SIZE + payload_size;
    if (capacity < size)
    {
        return CANVIEW_BUFFER_TOO_SMALL;
    }
    memset(out, 0, WIRE_HEADER_SIZE);
    if (radio)
    {
        write_le(out + CANVIEW_WIRE_ESPNOW_MAGIC_OFFSET, CANVIEW_WIRE_ESPNOW_MAGIC, 2U);
        out[CANVIEW_WIRE_ESPNOW_MAJOR_OFFSET] = CANVIEW_WIRE_ESPNOW_MAJOR;
        out[CANVIEW_WIRE_ESPNOW_MINOR_OFFSET] = CANVIEW_WIRE_ESPNOW_MINOR;
        out[CANVIEW_WIRE_ESPNOW_HEADER_LEN_OFFSET] = WIRE_HEADER_SIZE;
        out[CANVIEW_WIRE_ESPNOW_MESSAGE_TYPE_OFFSET] = header->message_type;
        out[CANVIEW_WIRE_ESPNOW_FLAGS_OFFSET] = header->flags;
        out[CANVIEW_WIRE_ESPNOW_PRIORITY_OFFSET] = header->priority;
        write_le(out + CANVIEW_WIRE_ESPNOW_SESSION_ID_OFFSET, header->session_id, 4U);
        write_le(out + CANVIEW_WIRE_ESPNOW_SEQUENCE_OFFSET, header->sequence, 4U);
        write_le(out + CANVIEW_WIRE_ESPNOW_SENDER_TIME_MS_OFFSET, header->sender_time, 4U);
        write_le(out + CANVIEW_WIRE_ESPNOW_CORRELATION_ID_OFFSET, header->correlation_id, 4U);
        write_le(out + CANVIEW_WIRE_ESPNOW_PAYLOAD_LEN_OFFSET, payload_size, 2U);
    }
    else
    {
        write_le(out + CANVIEW_WIRE_UART_MAGIC_OFFSET, CANVIEW_WIRE_UART_MAGIC, 2U);
        out[CANVIEW_WIRE_UART_MAJOR_OFFSET] = CANVIEW_WIRE_UART_MAJOR;
        out[CANVIEW_WIRE_UART_MINOR_OFFSET] = CANVIEW_WIRE_UART_MINOR;
        out[CANVIEW_WIRE_UART_MESSAGE_TYPE_OFFSET] = header->message_type;
        out[CANVIEW_WIRE_UART_FLAGS_OFFSET] = header->flags;
        write_le(out + CANVIEW_WIRE_UART_HEADER_LEN_OFFSET, WIRE_HEADER_SIZE, 2U);
        write_le(out + CANVIEW_WIRE_UART_PAYLOAD_LEN_OFFSET, payload_size, 2U);
        write_le(out + CANVIEW_WIRE_UART_SEQUENCE_OFFSET, header->sequence, 4U);
        write_le(out + CANVIEW_WIRE_UART_CORRELATION_ID_OFFSET, header->correlation_id, 4U);
        write_le(out + CANVIEW_WIRE_UART_SENDER_TIME_US_OFFSET, header->sender_time, 8U);
    }
    if (payload_size != 0U)
    {
        memcpy(out + WIRE_HEADER_SIZE, payload, payload_size);
    }
    write_le(out + WIRE_CRC_OFFSET, envelope_crc(out, size), WIRE_CRC_SIZE);
    *written = size;
    return CANVIEW_OK;
}

canview_status_t canview_wire_envelope_decode(canview_wire_transport_t transport,
                                              const uint8_t *bytes, size_t size,
                                              canview_wire_view_t *view)
{
    if (view == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(view, 0, sizeof(*view));
    if (!valid_transport(transport) || bytes == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const bool radio = transport == CANVIEW_WIRE_ESPNOW;
    const size_t maximum = radio ? CANVIEW_WIRE_ESPNOW_MAX_FRAME : CANVIEW_WIRE_UART_MAX_FRAME;
    if (size > maximum)
    {
        return CANVIEW_OVERSIZE;
    }
    if (size < WIRE_HEADER_SIZE)
    {
        return CANVIEW_MALFORMED;
    }
    const uint16_t magic = radio ? CANVIEW_WIRE_ESPNOW_MAGIC : CANVIEW_WIRE_UART_MAGIC;
    const uint8_t major = radio ? CANVIEW_WIRE_ESPNOW_MAJOR : CANVIEW_WIRE_UART_MAJOR;
    const uint8_t minor = radio ? CANVIEW_WIRE_ESPNOW_MINOR : CANVIEW_WIRE_UART_MINOR;
    if (read_le(bytes, 2U) != magic)
    {
        return CANVIEW_MALFORMED;
    }
    if (bytes[2] != major || bytes[3] != minor)
    {
        return CANVIEW_UNSUPPORTED_VERSION;
    }
    const size_t payload_size =
        (size_t)read_le(bytes + (radio ? CANVIEW_WIRE_ESPNOW_PAYLOAD_LEN_OFFSET
                                       : CANVIEW_WIRE_UART_PAYLOAD_LEN_OFFSET),
                        2U);
    const size_t header_size =
        (size_t)read_le(bytes + (radio ? CANVIEW_WIRE_ESPNOW_HEADER_LEN_OFFSET
                                       : CANVIEW_WIRE_UART_HEADER_LEN_OFFSET),
                        radio ? 1U : 2U);
    const size_t reserved_offset =
        radio ? CANVIEW_WIRE_ESPNOW_RESERVED_OFFSET : CANVIEW_WIRE_UART_RESERVED_OFFSET;
    const uint8_t flags =
        bytes[radio ? CANVIEW_WIRE_ESPNOW_FLAGS_OFFSET : CANVIEW_WIRE_UART_FLAGS_OFFSET];
    const uint8_t mask = radio ? CANVIEW_WIRE_ESPNOW_FLAGS_MASK : CANVIEW_WIRE_UART_FLAGS_MASK;
    if (header_size != WIRE_HEADER_SIZE || payload_size != size - WIRE_HEADER_SIZE ||
        read_le(bytes + reserved_offset, 2U) != 0U || (flags & (uint8_t)~mask) != 0U ||
        (radio && bytes[CANVIEW_WIRE_ESPNOW_PRIORITY_OFFSET] > CANVIEW_WIRE_ESPNOW_MAX_PRIORITY))
    {
        return CANVIEW_MALFORMED;
    }
    if (read_le(bytes + WIRE_CRC_OFFSET, WIRE_CRC_SIZE) != envelope_crc(bytes, size))
    {
        return CANVIEW_CRC_MISMATCH;
    }
    view->header.flags = flags;
    view->header.message_type = bytes[radio ? CANVIEW_WIRE_ESPNOW_MESSAGE_TYPE_OFFSET
                                            : CANVIEW_WIRE_UART_MESSAGE_TYPE_OFFSET];
    view->header.sequence = (uint32_t)read_le(
        bytes + (radio ? CANVIEW_WIRE_ESPNOW_SEQUENCE_OFFSET : CANVIEW_WIRE_UART_SEQUENCE_OFFSET),
        4U);
    view->header.correlation_id =
        (uint32_t)read_le(bytes + (radio ? CANVIEW_WIRE_ESPNOW_CORRELATION_ID_OFFSET
                                         : CANVIEW_WIRE_UART_CORRELATION_ID_OFFSET),
                          4U);
    if (radio)
    {
        view->header.priority = bytes[CANVIEW_WIRE_ESPNOW_PRIORITY_OFFSET];
        view->header.session_id =
            (uint32_t)read_le(bytes + CANVIEW_WIRE_ESPNOW_SESSION_ID_OFFSET, 4U);
        view->header.sender_time = read_le(bytes + CANVIEW_WIRE_ESPNOW_SENDER_TIME_MS_OFFSET, 4U);
    }
    else
    {
        view->header.sender_time = read_le(bytes + CANVIEW_WIRE_UART_SENDER_TIME_US_OFFSET, 8U);
    }
    view->payload = bytes + WIRE_HEADER_SIZE;
    view->payload_size = payload_size;
    return CANVIEW_OK;
}

canview_status_t canview_wire_cobs_encode(const uint8_t *bytes, size_t size, uint8_t *out,
                                          size_t capacity, size_t *written)
{
    if (written == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *written = 0U;
    if (out == NULL || (bytes == NULL && size != 0U))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (size > CANVIEW_WIRE_UART_MAX_FRAME)
    {
        return CANVIEW_OVERSIZE;
    }
    if (capacity < size + size / WIRE_COBS_BLOCK + 1U)
    {
        return CANVIEW_BUFFER_TOO_SMALL;
    }
    size_t code_at = 0U;
    size_t used = 1U;
    uint8_t code = 1U;
    for (size_t i = 0U; i < size; ++i)
    {
        if (bytes[i] == 0U)
        {
            out[code_at] = code;
            code_at = used++;
            code = 1U;
        }
        else
        {
            out[used++] = bytes[i];
            ++code;
            if (code == UINT8_MAX)
            {
                out[code_at] = code;
                code_at = used++;
                code = 1U;
            }
        }
    }
    out[code_at] = code;
    *written = used;
    return CANVIEW_OK;
}

canview_status_t canview_wire_cobs_decode(const uint8_t *bytes, size_t size, uint8_t *out,
                                          size_t capacity, size_t *written)
{
    if (written == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *written = 0U;
    if (bytes == NULL || out == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (size == 0U)
    {
        return CANVIEW_MALFORMED;
    }
    if (size > CANVIEW_WIRE_UART_MAX_ENCODED)
    {
        return CANVIEW_OVERSIZE;
    }
    size_t read_at = 0U;
    size_t used = 0U;
    while (read_at < size)
    {
        const uint8_t code = bytes[read_at++];
        if (code == 0U || (size_t)(code - 1U) > size - read_at)
        {
            return CANVIEW_MALFORMED;
        }
        for (uint32_t i = 1U; i < code; ++i)
        {
            if (bytes[read_at] == 0U)
            {
                return CANVIEW_MALFORMED;
            }
            if (used >= capacity || used >= CANVIEW_WIRE_UART_MAX_FRAME)
            {
                return CANVIEW_BUFFER_TOO_SMALL;
            }
            out[used++] = bytes[read_at++];
        }
        if (code != UINT8_MAX && read_at < size)
        {
            if (used >= capacity || used >= CANVIEW_WIRE_UART_MAX_FRAME)
            {
                return CANVIEW_BUFFER_TOO_SMALL;
            }
            out[used++] = 0U;
        }
    }
    *written = used;
    return CANVIEW_OK;
}

canview_status_t canview_uart_stream_reset(canview_uart_stream_t *stream)
{
    if (stream == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(stream, 0, sizeof(*stream));
    return CANVIEW_OK;
}

canview_status_t canview_uart_stream_feed(canview_uart_stream_t *stream, uint8_t byte,
                                          canview_wire_view_t *view)
{
    if (view == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(view, 0, sizeof(*view));
    if (stream == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (stream->discarding)
    {
        if (byte == 0U)
        {
            stream->discarding = false;
            stream->used = 0U;
        }
        return CANVIEW_INCOMPLETE;
    }
    if (byte != 0U)
    {
        if (stream->used >= sizeof(stream->encoded))
        {
            stream->used = 0U;
            stream->discarding = true;
            return CANVIEW_OVERSIZE;
        }
        stream->encoded[stream->used++] = byte;
        return CANVIEW_INCOMPLETE;
    }
    const size_t size = stream->used;
    stream->used = 0U;
    if (size == 0U)
    {
        return CANVIEW_INCOMPLETE;
    }
    size_t decoded_size = 0U;
    const canview_status_t result = canview_wire_cobs_decode(
        stream->encoded, size, stream->decoded, sizeof(stream->decoded), &decoded_size);
    if (result != CANVIEW_OK)
    {
        return result;
    }
    return canview_wire_envelope_decode(CANVIEW_WIRE_UART, stream->decoded, decoded_size, view);
}

canview_status_t canview_uart_packet_encode(const canview_wire_header_t *header,
                                            const uint8_t *payload, size_t payload_size,
                                            uint8_t *scratch, size_t scratch_size, uint8_t *out,
                                            size_t capacity, size_t *written)
{
    if (written == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *written = 0U;
    if (out == NULL || scratch == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (capacity == 0U)
    {
        return CANVIEW_BUFFER_TOO_SMALL;
    }
    size_t size = 0U;
    canview_status_t result = canview_wire_envelope_encode(
        CANVIEW_WIRE_UART, header, payload, payload_size, scratch, scratch_size, &size);
    if (result != CANVIEW_OK)
    {
        return result;
    }
    result = canview_wire_cobs_encode(scratch, size, out, capacity - 1U, &size);
    if (result != CANVIEW_OK)
    {
        return result;
    }
    out[size] = 0U;
    *written = size + 1U;
    return CANVIEW_OK;
}

static bool record_valid(const canview_wire_can_record_t *record)
{
    if (record->bus_id >= CANVIEW_WIRE_CAN_BUS_COUNT || record->dlc > CANVIEW_WIRE_CAN_MAX_DLC ||
        record->flags > WIRE_CAN_FLAGS_MASK ||
        record->can_id > ((record->flags & WIRE_CAN_IDE) != 0U ? CANVIEW_WIRE_CAN_EXTENDED_ID_MAX
                                                               : CANVIEW_WIRE_CAN_STANDARD_ID_MAX))
    {
        return false;
    }
    const size_t first_unused = (record->flags & WIRE_CAN_RTR) != 0U ? 0U : record->dlc;
    for (size_t i = first_unused; i < sizeof(record->data); ++i)
    {
        if (record->data[i] != 0U)
        {
            return false;
        }
    }
    return true;
}

canview_status_t canview_wire_can_batch_decode(const uint8_t *bytes, size_t size,
                                               canview_wire_can_batch_t *batch)
{
    if (batch == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(batch, 0, sizeof(*batch));
    if (bytes == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (size < CANVIEW_WIRE_CAN_PREFIX_SIZE || bytes[8] > CANVIEW_WIRE_CAN_MAX_RECORDS ||
        size != CANVIEW_WIRE_CAN_PREFIX_SIZE + (size_t)bytes[8] * CANVIEW_WIRE_CAN_RECORD_SIZE ||
        read_le(bytes + 10U, 2U) != 0U)
    {
        return CANVIEW_MALFORMED;
    }
    canview_wire_can_batch_t candidate = {0};
    candidate.base_time_us = read_le(bytes, 8U);
    candidate.count = bytes[8];
    candidate.dropped_since_last = bytes[9];
    for (size_t i = 0U; i < candidate.count; ++i)
    {
        const uint8_t *source =
            bytes + CANVIEW_WIRE_CAN_PREFIX_SIZE + i * CANVIEW_WIRE_CAN_RECORD_SIZE;
        canview_wire_can_record_t *record = &candidate.records[i];
        record->delta_us = (uint16_t)read_le(source, 2U);
        record->bus_id = source[2];
        record->flags = source[3] >> 4U;
        record->dlc = source[3] & WIRE_CAN_FLAGS_MASK;
        record->can_id = (uint32_t)read_le(source + 4U, 4U);
        memcpy(record->data, source + 8U, sizeof(record->data));
        if (!record_valid(record) || candidate.base_time_us > UINT64_MAX - record->delta_us)
        {
            return CANVIEW_MALFORMED;
        }
    }
    *batch = candidate;
    return CANVIEW_OK;
}

canview_status_t canview_wire_can_batch_encode(const canview_wire_can_batch_t *batch, uint8_t *out,
                                               size_t capacity, size_t *written)
{
    if (written == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *written = 0U;
    if (batch == NULL || out == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (batch->count > CANVIEW_WIRE_CAN_MAX_RECORDS)
    {
        return CANVIEW_MALFORMED;
    }
    const size_t size =
        CANVIEW_WIRE_CAN_PREFIX_SIZE + (size_t)batch->count * CANVIEW_WIRE_CAN_RECORD_SIZE;
    if (capacity < size)
    {
        return CANVIEW_BUFFER_TOO_SMALL;
    }
    for (size_t i = 0U; i < batch->count; ++i)
    {
        if (!record_valid(&batch->records[i]) ||
            batch->base_time_us > UINT64_MAX - batch->records[i].delta_us)
        {
            return CANVIEW_MALFORMED;
        }
    }
    memset(out, 0, size);
    write_le(out, batch->base_time_us, 8U);
    out[8] = batch->count;
    out[9] = batch->dropped_since_last;
    for (size_t i = 0U; i < batch->count; ++i)
    {
        const canview_wire_can_record_t *record = &batch->records[i];
        uint8_t *destination =
            out + CANVIEW_WIRE_CAN_PREFIX_SIZE + i * CANVIEW_WIRE_CAN_RECORD_SIZE;
        write_le(destination, record->delta_us, 2U);
        destination[2] = record->bus_id;
        destination[3] = (uint8_t)((record->flags << 4U) | record->dlc);
        write_le(destination + 4U, record->can_id, 4U);
        memcpy(destination + 8U, record->data, sizeof(record->data));
    }
    *written = size;
    return CANVIEW_OK;
}

canview_status_t canview_sequence_window_reset(canview_sequence_window_t *window)
{
    if (window == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(window, 0, sizeof(*window));
    return CANVIEW_OK;
}

canview_status_t canview_sequence_window_accept(canview_sequence_window_t *window,
                                                uint32_t sequence)
{
    if (window == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (!window->initialized)
    {
        window->newest = sequence;
        window->seen = 1U;
        window->initialized = true;
        return CANVIEW_OK;
    }
    const uint32_t forward = sequence - window->newest;
    if (forward == 0U)
    {
        return CANVIEW_DUPLICATE;
    }
    if (forward < WIRE_SEQUENCE_HALF)
    {
        window->seen =
            forward >= WIRE_SEQUENCE_WINDOW ? UINT64_C(1) : (window->seen << forward) | UINT64_C(1);
        window->newest = sequence;
        return CANVIEW_OK;
    }
    const uint32_t age = window->newest - sequence;
    if (age >= WIRE_SEQUENCE_WINDOW)
    {
        return CANVIEW_STALE;
    }
    const uint64_t bit = UINT64_C(1) << age;
    if ((window->seen & bit) != 0U)
    {
        return CANVIEW_DUPLICATE;
    }
    window->seen |= bit;
    return CANVIEW_OK;
}
