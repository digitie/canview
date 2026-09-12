/* SPDX-License-Identifier: GPL-3.0-only */
/** @file envelope_crypto_probe.c
 * @brief Windows host 전용 실제 CNG ECDSA-P256/SHA-256 oracle. 장치용 provider가 아니다.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include "envelope.h"

#define TEST_PUBLIC_BYTES (64U)
#define TEST_COORDINATE_BYTES (32U)
#define TEST_DIGEST_BYTES (32U)

static canview_status_t probe_verify(
    void *context, const uint8_t *message, size_t message_size,
    const uint8_t signature[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES])
{
    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    BCRYPT_ECCKEY_BLOB header = {BCRYPT_ECDSA_PUBLIC_P256_MAGIC, TEST_COORDINATE_BYTES};
    uint8_t blob[sizeof(header) + TEST_PUBLIC_BYTES];
    uint8_t digest[TEST_DIGEST_BYTES];
    canview_status_t result = CANVIEW_AUTH_FAILED;

    if (context == NULL || message == NULL || signature == NULL ||
        message_size > CANVIEW_OTA_ENVELOPE_MANIFEST_MAX)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    (void)memcpy(blob, &header, sizeof(header));
    (void)memcpy(blob + sizeof(header), context, TEST_PUBLIC_BYTES);
    if (BCryptHash(BCRYPT_SHA256_ALG_HANDLE, NULL, 0U, (PUCHAR)message,
                   (ULONG)message_size, digest, sizeof(digest)) != 0)
    {
        return CANVIEW_AUTH_FAILED;
    }
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0U) != 0)
    {
        return CANVIEW_AUTH_FAILED;
    }
    if (BCryptImportKeyPair(algorithm, NULL, BCRYPT_ECCPUBLIC_BLOB, &key,
                            blob, sizeof(blob), 0U) == 0)
    {
        if (BCryptVerifySignature(key, NULL, digest, sizeof(digest), (PUCHAR)signature,
                                  CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES, 0U) == 0)
        {
            result = CANVIEW_OK;
        }
        if (BCryptDestroyKey(key) != 0)
        {
            result = CANVIEW_AUTH_FAILED;
        }
    }
    if (BCryptCloseAlgorithmProvider(algorithm, 0U) != 0)
    {
        result = CANVIEW_AUTH_FAILED;
    }
    return result;
}

int main(void)
{
    uint8_t data[CANVIEW_OTA_ENVELOPE_PREFIX_MAX + 1U];
    uint8_t public_key[TEST_PUBLIC_BYTES];
    uint8_t size_bytes[4];
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
        (void)memset(&view, 0xA5, sizeof(view));
        result = canview_ota_envelope_check(data, size, probe_verify, public_key, &view);
        if (result != CANVIEW_OK && (view.manifest_offset != 0U || view.manifest_size != 0U ||
            view.images_offset != 0U || view.declared_total_size != 0U || view.declared_image_count != 0U))
        {
            return 1;
        }
        if (printf("%u\n", (unsigned int)result) < 0)
        {
            return 1;
        }
    }
}
