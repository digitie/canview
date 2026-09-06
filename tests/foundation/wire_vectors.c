/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_wire.h"
#include <inttypes.h>
#include <stdio.h>

static void print_hex(const uint8_t *bytes, size_t size)
{
    for (size_t index = 0U; index < size; ++index)
    {
        (void)printf("%02x", (unsigned int)bytes[index]);
    }
    (void)putchar('\n');
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
    return ferror(stdout) != 0 ? 1 : 0;
}
