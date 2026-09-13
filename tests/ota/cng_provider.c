/* SPDX-License-Identifier: GPL-3.0-only */
/** @file cng_provider.c @brief 테스트에서만 쓰는 Windows CNG. 장치 펌웨어에 링크하지 않는다. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <string.h>
#include "cng_provider.h"

#define TEST_PUBLIC_BYTES (64U)
#define TEST_COORDINATE_BYTES (32U)

canview_status_t canview_test_p256_verify(
    void *context, const uint8_t *message, size_t message_size,
    const uint8_t signature[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES])
{
    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    BCRYPT_ECCKEY_BLOB header = {BCRYPT_ECDSA_PUBLIC_P256_MAGIC, TEST_COORDINATE_BYTES};
    uint8_t blob[sizeof(header) + TEST_PUBLIC_BYTES];
    uint8_t digest[CANVIEW_OTA_DIGEST_BYTES];
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

static canview_status_t cng_start(void *context)
{
    canview_test_cng_hash_t *hash = context;
    if (hash == NULL || hash->handle != NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    return BCryptCreateHash(BCRYPT_SHA256_ALG_HANDLE, &hash->handle, NULL, 0U, NULL, 0U, 0U) == 0 ?
        CANVIEW_OK : CANVIEW_RESOURCE_BUSY;
}

static canview_status_t cng_update(void *context, const uint8_t *data, size_t size)
{
    const canview_test_cng_hash_t *hash = context;
    if (hash == NULL || hash->handle == NULL || data == NULL || size > CANVIEW_OTA_BODY_CHUNK_MAX)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    return BCryptHashData(hash->handle, (PUCHAR)data, (ULONG)size, 0U) == 0 ? CANVIEW_OK : CANVIEW_AUTH_FAILED;
}

static canview_status_t cng_finish(void *context, uint8_t digest[CANVIEW_OTA_DIGEST_BYTES])
{
    const canview_test_cng_hash_t *hash = context;
    if (hash == NULL || hash->handle == NULL || digest == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    return BCryptFinishHash(hash->handle, digest, CANVIEW_OTA_DIGEST_BYTES, 0U) == 0 ? CANVIEW_OK : CANVIEW_AUTH_FAILED;
}

static canview_status_t cng_reset(void *context)
{
    canview_test_cng_hash_t *hash = context;
    if (hash == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (hash->handle != NULL)
    {
        if (BCryptDestroyHash(hash->handle) != 0)
        {
            return CANVIEW_RESOURCE_BUSY;
        }
        hash->handle = NULL;
    }
    return CANVIEW_OK;
}

canview_ota_hash_t canview_test_cng_hash_provider(canview_test_cng_hash_t *context)
{
    return (canview_ota_hash_t){context, cng_start, cng_update, cng_finish, cng_reset};
}
