/* SPDX-License-Identifier: GPL-3.0-only */
/** @file cbor_probe.c @brief Host 전용 length-prefixed CBOR differential runner. */
#include <stdio.h>
#include "cbor_document.h"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main(void)
{
    uint8_t data[CANVIEW_OTA_CBOR_MAX_BYTES + 1U];
    uint8_t header[4];
#ifdef _WIN32
    if (_setmode(_fileno(stdin), _O_BINARY) < 0)
    {
        return 1;
    }
#endif
    for (;;)
    {
        uint32_t length;
        const size_t count = fread(header, 1U, sizeof(header), stdin);
        if (count == 0U && feof(stdin) != 0 && ferror(stdin) == 0)
        {
            return fflush(stdout) == 0 ? 0 : 1;
        }
        if (count != sizeof(header))
        {
            return 1;
        }
        length = (uint32_t)header[0] | ((uint32_t)header[1] << 8U) |
                 ((uint32_t)header[2] << 16U) | ((uint32_t)header[3] << 24U);
        if (length > sizeof(data) || fread(data, 1U, length, stdin) != length)
        {
            return 1;
        }
        if (printf("%u\n", (unsigned int)canview_ota_cbor_validate(data, length)) < 0)
        {
            return 1;
        }
    }
}
