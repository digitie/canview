/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_wire.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static void print_hex(const uint8_t *bytes, size_t size)
{
    for (size_t index = 0U; index < size; ++index)
    {
        (void)printf("%02x", (unsigned int)bytes[index]);
    }
    (void)putchar('\n');
}

static int print_can_vectors(void)
{
    /* Independent Python struct reference covers every packed field and flag. */
    for (uint8_t count = 0U; count <= 12U; ++count)
    {
        for (uint8_t variant = 0U; variant < 16U; ++variant)
        {
            canview_wire_can_batch_t batch = {0}, decoded;
            uint8_t bytes[204];
            size_t written = 0U;
            batch.count = count;
            batch.base_time_us = UINT64_C(0x123456789abc0000) + (uint64_t)variant * 0x123U;
            batch.dropped_since_last = (uint8_t)(variant * 13U);
            for (size_t i = 0U; i < count; ++i)
            {
                canview_wire_can_record_t *record = &batch.records[i];
                record->delta_us = (uint16_t)(0x102U + i * 0x101U);
                record->bus_id = (uint8_t)((i + variant) % 3U);
                record->flags = (uint8_t)((i + variant) % 16U);
                record->dlc = (uint8_t)((i + variant) % 9U);
                record->can_id = (record->flags & 1U) != 0U
                                     ? UINT32_C(0x1234567) + (uint32_t)i * 257U
                                     : 0x321U + (uint32_t)i * 7U;
                if ((record->flags & 2U) == 0U)
                {
                    for (size_t j = 0U; j < record->dlc; ++j)
                    {
                        record->data[j] = (uint8_t)(17U + i * 29U + j * 37U + variant);
                    }
                }
            }
            if (canview_wire_can_batch_encode(&batch, bytes, sizeof(bytes), &written) !=
                    CANVIEW_OK ||
                canview_wire_can_batch_decode(bytes, written, &decoded) != CANVIEW_OK ||
                decoded.count != batch.count || decoded.base_time_us != batch.base_time_us ||
                decoded.dropped_since_last != batch.dropped_since_last)
            {
                return 1;
            }
            for (size_t i = 0U; i < count; ++i)
            {
                const canview_wire_can_record_t *a = &batch.records[i], *b = &decoded.records[i];
                if (a->delta_us != b->delta_us || a->bus_id != b->bus_id || a->flags != b->flags ||
                    a->dlc != b->dlc || a->can_id != b->can_id || memcmp(a->data, b->data, 8U) != 0)
                {
                    return 1;
                }
            }
            print_hex(bytes, written);
        }
    }
    return 0;
}

int main(void)
{
    uint8_t payload[992];
    uint8_t frame[1024];
    uint8_t serial[1030];
    for (size_t index = 0U; index < sizeof(payload); ++index)
    {
        payload[index] = (uint8_t)(index * 37U);
    }
    for (unsigned int transport = 0U; transport < 2U; ++transport)
    {
        const size_t limit = transport == 0U ? 208U : 992U;
        canview_wire_header_t header = {0};
        header.message_type = 0x20U;
        header.flags = 1U;
        header.sequence = UINT32_C(0x89abcdef);
        header.correlation_id = UINT32_C(0x12345678);
        header.sender_time = transport == 0U ? UINT64_C(0x11223344) : UINT64_C(0x1122334455667788);
        header.session_id = transport == 0U ? UINT32_C(0xaabbccdd) : 0U;
        header.priority = transport == 0U ? 4U : 0U;
        for (size_t size = 0U; size <= limit; ++size)
        {
            size_t written = 0U;
            if (canview_wire_envelope_encode((canview_wire_transport_t)transport, &header, payload,
                                             size, frame, sizeof(frame), &written) != CANVIEW_OK)
            {
                return 1;
            }
            print_hex(frame, written);
            if (transport == 1U)
            {
                if (canview_uart_packet_encode(&header, payload, size, frame, sizeof(frame), serial,
                                               sizeof(serial), &written) != CANVIEW_OK)
                {
                    return 1;
                }
                print_hex(serial, written);
            }
        }
    }
    return print_can_vectors() != 0 || ferror(stdout) != 0 ? 1 : 0;
}
