/* SPDX-License-Identifier: GPL-3.0-only */
/** @file envelope_crypto_probe.c
 * @brief Windows host 전용 실제 CNG ECDSA-P256/SHA-256 oracle. 장치용 provider가 아니다.
 */
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include "envelope.h"
#include "manifest.h"
#include "cng_provider.h"

#define TEST_PUBLIC_BYTES (64U)
int main(int argc, char **argv)
{
    uint8_t data[CANVIEW_OTA_ENVELOPE_PREFIX_MAX + 1U];
    uint8_t public_key[TEST_PUBLIC_BYTES];
    uint8_t size_bytes[4];
    canview_ota_identity_t identity = {CANVIEW_OTA_ROLE_COMMUNICATOR, "synthetic-board", "synthetic-layout", 7U, 11U};
    if (argc == 2 && strcmp(argv[1], "2") == 0)
    {
        identity.role = CANVIEW_OTA_ROLE_CONTROLLER;
    }
    else if (argc == 2 && strcmp(argv[1], "3") == 0)
    {
        identity.role = CANVIEW_OTA_ROLE_BRIDGE;
    }
    else if (argc != 1 && (argc != 2 || strcmp(argv[1], "1") != 0))
    {
        return 1;
    }
    if (_setmode(_fileno(stdin), _O_BINARY) < 0)
    {
        return 1;
    }
    for (;;)
    {
        canview_ota_envelope_t view;
        canview_status_t result;
        uint32_t size;
        const size_t count = fread(size_bytes, 1U, sizeof(size_bytes), stdin);
        if (count == 0U && feof(stdin) != 0 && ferror(stdin) == 0)
        {
            return fflush(stdout) == 0 ? 0 : 1;
        }
        if (count != sizeof(size_bytes))
        {
            return 1;
        }
        size = (uint32_t)size_bytes[0] | ((uint32_t)size_bytes[1] << 8U) |
               ((uint32_t)size_bytes[2] << 16U) | ((uint32_t)size_bytes[3] << 24U);
        if (size > sizeof(data) || fread(public_key, 1U, sizeof(public_key), stdin) != sizeof(public_key) ||
            fread(data, 1U, size, stdin) != size)
        {
            return 1;
        }
        if (argc == 1)
        {
            (void)memset(&view, 0xA5, sizeof(view));
            result = canview_ota_envelope_check(data, size, canview_test_p256_verify, public_key, &view);
            if (result != CANVIEW_OK && (view.manifest_offset != 0U || view.manifest_size != 0U ||
                view.images_offset != 0U || view.declared_total_size != 0U || view.declared_image_count != 0U))
            {
                return 1;
            }
        }
        else
        {
            canview_ota_manifest_t manifest;
            (void)memset(&manifest, 0xA5, sizeof(manifest));
            result = canview_ota_manifest_check(data, size, &identity, canview_test_p256_verify, public_key, &manifest);
            if (result != CANVIEW_OK)
            {
                const uint8_t *bytes = (const uint8_t *)&manifest;
                for (size_t index = 0U; index < sizeof(manifest); ++index)
                {
                    if (bytes[index] != 0U)
                    {
                        return 1;
                    }
                }
            }
        }
        if (printf("%u\n", (unsigned int)result) < 0)
        {
            return 1;
        }
    }
}
