/* SPDX-License-Identifier: GPL-3.0-only */
/** @file ota_crypto.c @brief ESP-IDF6.0.3 PSA 공개키 검증·순차 hash의 수명 관리. */
#include <string.h>
#include "ota_crypto.h"

#define CRYPTO_P256_BITS (256U)
#define CRYPTO_UNCOMPRESSED_POINT (4U)
#define CRYPTO_SIGNATURE_ALGORITHM (PSA_ALG_ECDSA(PSA_ALG_SHA_256))

static canview_status_t crypto_status(psa_status_t status)
{
    if (status == PSA_SUCCESS) { return CANVIEW_OK; }
    if (status == PSA_ERROR_INVALID_SIGNATURE) { return CANVIEW_AUTH_FAILED; }
    if (status == PSA_ERROR_INVALID_ARGUMENT) { return CANVIEW_INVALID_ARGUMENT; }
    if (status == PSA_ERROR_NOT_SUPPORTED) { return CANVIEW_NOT_IMPLEMENTED; }
    return CANVIEW_RESOURCE_BUSY;
}

static bool crypto_overlaps(const canview_esp_ota_crypto_t *context, const void *data, size_t size)
{
    const uintptr_t start = (uintptr_t)context;
    const uintptr_t input = (uintptr_t)data;
    if (size > UINTPTR_MAX - input || sizeof(*context) > UINTPTR_MAX - start) { return true; }
    return size != 0U && input < start + sizeof(*context) && start < input + size;
}

static canview_status_t crypto_enter(canview_esp_ota_crypto_t *context)
{
    if (context == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    if (context->busy || context->closing) { return CANVIEW_RESOURCE_BUSY; }
    if (!context->ready) { return CANVIEW_INVALID_ARGUMENT; }
    context->busy = true;
    return CANVIEW_OK;
}

canview_status_t canview_esp_ota_crypto_init(canview_esp_ota_crypto_t *context,
    const uint8_t public_key[CANVIEW_ESP_OTA_PUBLIC_KEY_BYTES])
{
    if (context == NULL || public_key == NULL ||
        crypto_overlaps(context, public_key, CANVIEW_ESP_OTA_PUBLIC_KEY_BYTES))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (context->busy || context->ready || context->closing || context->hash_dirty ||
        !mbedtls_svc_key_id_is_null(context->key)) { return CANVIEW_RESOURCE_BUSY; }
    if (public_key[0] != CRYPTO_UNCOMPRESSED_POINT) { return CANVIEW_INVALID_ARGUMENT; }
    context->busy = true;
    context->hash = psa_hash_operation_init();
    psa_status_t status = psa_crypto_init();
    if (status == PSA_SUCCESS)
    {
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
        psa_set_key_bits(&attributes, CRYPTO_P256_BITS);
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
        psa_set_key_algorithm(&attributes, CRYPTO_SIGNATURE_ALGORITHM);
        psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
        status = psa_import_key(&attributes, public_key, CANVIEW_ESP_OTA_PUBLIC_KEY_BYTES, &context->key);
        psa_reset_key_attributes(&attributes);
    }
    context->ready = status == PSA_SUCCESS;
    context->busy = false;
    return crypto_status(status);
}

canview_status_t canview_esp_ota_crypto_close(canview_esp_ota_crypto_t *context)
{
    if (context == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    if (context->busy) { return CANVIEW_RESOURCE_BUSY; }
    context->busy = true;
    context->closing = true;
    psa_status_t status = PSA_SUCCESS;
    if (context->hash_dirty) { status = psa_hash_abort(&context->hash); }
    if (status == PSA_SUCCESS)
    {
        context->hash_dirty = false;
        context->hash_active = false;
        if (!mbedtls_svc_key_id_is_null(context->key)) { status = psa_destroy_key(context->key); }
    }
    context->busy = false;
    if (status == PSA_SUCCESS) { (void)memset(context, 0, sizeof(*context)); }
    return crypto_status(status);
}

canview_status_t canview_esp_ota_manifest_verify(void *opaque, const uint8_t *message,
    size_t size, const uint8_t signature[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES])
{
    canview_esp_ota_crypto_t *context = opaque;
    if (context == NULL || message == NULL || signature == NULL || size == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (size > CANVIEW_OTA_ENVELOPE_MANIFEST_MAX) { return CANVIEW_OVERSIZE; }
    if (crypto_overlaps(context, message, size) ||
        crypto_overlaps(context, signature, CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t entered = crypto_enter(context);
    if (entered != CANVIEW_OK) { return entered; }
    const psa_status_t status = psa_verify_message(context->key, CRYPTO_SIGNATURE_ALGORITHM,
        message, size, signature, CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES);
    context->busy = false;
    return crypto_status(status);
}

static canview_status_t crypto_hash_start(void *opaque)
{
    canview_esp_ota_crypto_t *context = opaque;
    const canview_status_t entered = crypto_enter(context);
    if (entered != CANVIEW_OK) { return entered; }
    if (context->hash_dirty)
    {
        context->busy = false;
        return CANVIEW_RESOURCE_BUSY;
    }
    context->hash_dirty = true;
    const psa_status_t status = psa_hash_setup(&context->hash, PSA_ALG_SHA_256);
    context->hash_active = status == PSA_SUCCESS;
    context->busy = false;
    return crypto_status(status);
}

static canview_status_t crypto_hash_update(void *opaque, const uint8_t *data, size_t size)
{
    canview_esp_ota_crypto_t *context = opaque;
    if (context == NULL || (data == NULL && size != 0U)) { return CANVIEW_INVALID_ARGUMENT; }
    if (size > CANVIEW_OTA_BODY_CHUNK_MAX) { return CANVIEW_OVERSIZE; }
    if (crypto_overlaps(context, data, size)) { return CANVIEW_INVALID_ARGUMENT; }
    const canview_status_t entered = crypto_enter(context);
    if (entered != CANVIEW_OK) { return entered; }
    if (!context->hash_active)
    {
        context->busy = false;
        return CANVIEW_INVALID_ARGUMENT;
    }
    const psa_status_t status = size == 0U ? PSA_SUCCESS : psa_hash_update(&context->hash, data, size);
    if (status != PSA_SUCCESS) { context->hash_active = false; }
    context->busy = false;
    return crypto_status(status);
}

static canview_status_t crypto_hash_finish(void *opaque, uint8_t digest[CANVIEW_OTA_DIGEST_BYTES])
{
    canview_esp_ota_crypto_t *context = opaque;
    if (context == NULL || digest == NULL || crypto_overlaps(context, digest, CANVIEW_OTA_DIGEST_BYTES))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t entered = crypto_enter(context);
    if (entered != CANVIEW_OK) { return entered; }
    (void)memset(digest, 0, CANVIEW_OTA_DIGEST_BYTES);
    if (!context->hash_active)
    {
        context->busy = false;
        return CANVIEW_INVALID_ARGUMENT;
    }
    size_t length = 0U;
    const psa_status_t status = psa_hash_finish(&context->hash, digest, CANVIEW_OTA_DIGEST_BYTES, &length);
    context->hash_active = false;
    context->busy = false;
    if (status != PSA_SUCCESS || length != CANVIEW_OTA_DIGEST_BYTES)
    {
        (void)memset(digest, 0, CANVIEW_OTA_DIGEST_BYTES);
        return status == PSA_SUCCESS ? CANVIEW_AUTH_FAILED : crypto_status(status);
    }
    return CANVIEW_OK;
}

static canview_status_t crypto_hash_reset(void *opaque)
{
    canview_esp_ota_crypto_t *context = opaque;
    const canview_status_t entered = crypto_enter(context);
    if (entered != CANVIEW_OK) { return entered; }
    psa_status_t status = PSA_SUCCESS;
    if (context->hash_dirty) { status = psa_hash_abort(&context->hash); }
    context->hash_active = false;
    if (status == PSA_SUCCESS) { context->hash_dirty = false; }
    context->busy = false;
    return crypto_status(status);
}

canview_ota_hash_t canview_esp_ota_hash_provider(canview_esp_ota_crypto_t *context)
{
    return (canview_ota_hash_t){context, crypto_hash_start, crypto_hash_update, crypto_hash_finish, crypto_hash_reset};
}
