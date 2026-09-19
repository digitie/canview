/* SPDX-License-Identifier: GPL-3.0-only */
/** @file test_esp_ota_crypto.c @brief 실제 adapter의 PSA 수명/실패 모형. 암호 검증이 아니다. */
#include <stdio.h>
#include <string.h>
#include "ota_crypto.h"

#define CHECK(c) do { if (!(c)) { (void)fprintf(stderr, "OTA PSA line %d\n", __LINE__); return 1; } } while (0)
#define MOCK_KEY (17U)
enum { MOCK_INIT = 1, MOCK_IMPORT, MOCK_VERIFY, MOCK_SETUP, MOCK_UPDATE, MOCK_FINISH, MOCK_ABORT, MOCK_DESTROY, MOCK_END };
static uint32_t fault;
static psa_status_t failure;
static uint32_t calls[MOCK_END];
static bool correct = true;
static bool short_digest;
static canview_esp_ota_crypto_t *reentry;
static uint8_t root[CANVIEW_ESP_OTA_PUBLIC_KEY_BYTES] = {4U};
static uint8_t message[CANVIEW_OTA_ENVELOPE_MANIFEST_MAX];
static uint8_t signature[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES];

static psa_status_t mock_call(uint32_t operation)
{
    ++calls[operation];
    if (reentry != NULL)
    {
        uint8_t digest[CANVIEW_OTA_DIGEST_BYTES];
        const canview_ota_hash_t hash = canview_esp_ota_hash_provider(reentry);
        correct = correct && canview_esp_ota_crypto_close(reentry) == CANVIEW_RESOURCE_BUSY &&
            canview_esp_ota_crypto_init(reentry, root) == CANVIEW_RESOURCE_BUSY &&
            canview_esp_ota_manifest_verify(reentry, message, 1U, signature) == CANVIEW_RESOURCE_BUSY &&
            hash.start(reentry) == CANVIEW_RESOURCE_BUSY && hash.update(reentry, message, 1U) == CANVIEW_RESOURCE_BUSY &&
            hash.finish(reentry, digest) == CANVIEW_RESOURCE_BUSY && hash.reset(reentry) == CANVIEW_RESOURCE_BUSY;
    }
    return fault == operation ? failure : PSA_SUCCESS;
}

psa_status_t psa_crypto_init(void) { return mock_call(MOCK_INIT); }
psa_status_t psa_import_key(const psa_key_attributes_t *a, const uint8_t *data, size_t size, mbedtls_svc_key_id_t *key)
{
    correct = correct && a->type == PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1) && a->bits == 256U &&
        a->usage == PSA_KEY_USAGE_VERIFY_MESSAGE && a->algorithm == PSA_ALG_ECDSA(PSA_ALG_SHA_256) &&
        a->lifetime == PSA_KEY_LIFETIME_VOLATILE && size == sizeof(root) && memcmp(data, root, sizeof(root)) == 0;
    const psa_status_t result = mock_call(MOCK_IMPORT);
    *key = result == PSA_SUCCESS ? MOCK_KEY : 0U;
    return result;
}
psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key)
{
    correct = correct && key == MOCK_KEY;
    return mock_call(MOCK_DESTROY);
}
psa_status_t psa_verify_message(mbedtls_svc_key_id_t key, psa_algorithm_t algorithm,
    const uint8_t *data, size_t size, const uint8_t *sig, size_t signature_size)
{
    correct = correct && key == MOCK_KEY && algorithm == PSA_ALG_ECDSA(PSA_ALG_SHA_256) &&
        data == message && size > 0U && size <= sizeof(message) && sig == signature && signature_size == sizeof(signature);
    return mock_call(MOCK_VERIFY);
}
psa_status_t psa_hash_setup(psa_hash_operation_t *operation, psa_algorithm_t algorithm)
{
    correct = correct && operation->active == 0U && algorithm == PSA_ALG_SHA_256;
    operation->active = 1U; /* setup 실패도 partial resource를 남기는 모형 */
    return mock_call(MOCK_SETUP);
}
psa_status_t psa_hash_update(psa_hash_operation_t *operation, const uint8_t *data, size_t size)
{
    correct = correct && operation->active == 1U && data == message && size > 0U && size <= sizeof(message);
    return mock_call(MOCK_UPDATE);
}
psa_status_t psa_hash_finish(psa_hash_operation_t *operation, uint8_t *digest, size_t capacity, size_t *size)
{
    correct = correct && operation->active == 1U && capacity == CANVIEW_OTA_DIGEST_BYTES;
    (void)memset(digest, 0xa5, capacity); /* 실패 시 partial output 제거도 검사 */
    *size = short_digest ? 31U : CANVIEW_OTA_DIGEST_BYTES;
    return mock_call(MOCK_FINISH);
}
psa_status_t psa_hash_abort(psa_hash_operation_t *operation)
{
    const psa_status_t result = mock_call(MOCK_ABORT);
    if (result == PSA_SUCCESS) { operation->active = 0U; }
    return result;
}

int main(void)
{
    canview_esp_ota_crypto_t context = {0};
    canview_ota_hash_t hash = canview_esp_ota_hash_provider(&context);
    uint8_t digest[CANVIEW_OTA_DIGEST_BYTES];
    const uint8_t zeros[CANVIEW_OTA_DIGEST_BYTES] = {0};
    CHECK(canview_esp_ota_crypto_init(NULL, root) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_crypto_init(&context, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_crypto_init(&context, (const uint8_t *)&context) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_crypto_init(&context, (const uint8_t *)(UINTPTR_MAX - 3U)) == CANVIEW_INVALID_ARGUMENT);
    root[0] = 3U;
    CHECK(canview_esp_ota_crypto_init(&context, root) == CANVIEW_INVALID_ARGUMENT);
    root[0] = 4U;
    CHECK(canview_esp_ota_crypto_close(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_crypto_close(&context) == CANVIEW_OK);
    CHECK(hash.start(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.start(&context) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.reset(&context) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.update(NULL, message, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.finish(NULL, digest) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_manifest_verify(NULL, message, 1U, signature) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_manifest_verify(&context, message, 1U, signature) == CANVIEW_INVALID_ARGUMENT);
    CHECK(calls[MOCK_INIT] == 0U);

    reentry = &context;
    CHECK(canview_esp_ota_crypto_init(&context, root) == CANVIEW_OK);
    CHECK(canview_esp_ota_crypto_init(&context, root) == CANVIEW_RESOURCE_BUSY);
    CHECK(canview_esp_ota_manifest_verify(&context, NULL, 1U, signature) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_manifest_verify(&context, message, 0U, signature) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_manifest_verify(&context, message, 1U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_manifest_verify(&context, message, sizeof(message) + 1U, signature) == CANVIEW_OVERSIZE);
    CHECK(canview_esp_ota_manifest_verify(&context, (const uint8_t *)&context, 1U, signature) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_manifest_verify(&context, message, 1U, (const uint8_t *)&context) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_manifest_verify(&context, (const uint8_t *)(UINTPTR_MAX - 3U), 8U, signature) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_ota_manifest_verify(&context, message, 1U, signature) == CANVIEW_OK);
    CHECK(canview_esp_ota_manifest_verify(&context, message, sizeof(message), signature) == CANVIEW_OK);
    CHECK(calls[MOCK_VERIFY] == 2U);
    CHECK(hash.update(&context, message, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.finish(&context, digest) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.start(&context) == CANVIEW_OK);
    CHECK(hash.start(&context) == CANVIEW_RESOURCE_BUSY);
    CHECK(hash.update(&context, NULL, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.update(&context, message, sizeof(message) + 1U) == CANVIEW_OVERSIZE);
    CHECK(hash.update(&context, (const uint8_t *)&context, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.update(&context, (const uint8_t *)(UINTPTR_MAX - 3U), 8U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.update(&context, NULL, 0U) == CANVIEW_OK);
    CHECK(hash.update(&context, message, 1U) == CANVIEW_OK);
    CHECK(hash.update(&context, message, sizeof(message)) == CANVIEW_OK);
    CHECK(hash.finish(&context, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.finish(&context, (uint8_t *)&context) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.finish(&context, (uint8_t *)(UINTPTR_MAX - 3U)) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.finish(&context, digest) == CANVIEW_OK && digest[0] == 0xa5U);
    CHECK(hash.finish(&context, digest) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.update(&context, message, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(hash.start(&context) == CANVIEW_RESOURCE_BUSY);
    CHECK(hash.reset(&context) == CANVIEW_OK);
    CHECK(hash.reset(&context) == CANVIEW_OK);
    CHECK(canview_esp_ota_crypto_close(&context) == CANVIEW_OK);
    CHECK(canview_esp_ota_crypto_close(&context) == CANVIEW_OK);
    CHECK(correct && calls[MOCK_DESTROY] == 1U && calls[MOCK_ABORT] == 1U);

    const psa_status_t errors[] = {PSA_ERROR_INSUFFICIENT_MEMORY, PSA_ERROR_INVALID_SIGNATURE,
        PSA_ERROR_NOT_SUPPORTED, PSA_ERROR_INVALID_ARGUMENT, PSA_ERROR_HARDWARE_FAILURE};
    const canview_status_t expected[] = {CANVIEW_RESOURCE_BUSY, CANVIEW_AUTH_FAILED,
        CANVIEW_NOT_IMPLEMENTED, CANVIEW_INVALID_ARGUMENT, CANVIEW_RESOURCE_BUSY};
    for (size_t error = 0U; error < sizeof(errors) / sizeof(errors[0]); ++error)
    {
        for (uint32_t step = MOCK_INIT; step < MOCK_END; ++step)
        {
            failure = errors[error];
            fault = step <= MOCK_IMPORT ? step : 0U;
            const canview_status_t initialized = canview_esp_ota_crypto_init(&context, root);
            if (step <= MOCK_IMPORT) { CHECK(initialized == expected[error]); }
            else
            {
                CHECK(initialized == CANVIEW_OK);
                fault = step;
                if (step == MOCK_VERIFY)
                {
                    CHECK(canview_esp_ota_manifest_verify(&context, message, 1U, signature) == expected[error]);
                }
                else if (step == MOCK_DESTROY)
                {
                    CHECK(canview_esp_ota_crypto_close(&context) == expected[error]);
                    CHECK(context.key == MOCK_KEY && context.closing);
                    CHECK(hash.start(&context) == CANVIEW_RESOURCE_BUSY);
                    CHECK(canview_esp_ota_manifest_verify(&context, message, 1U, signature) == CANVIEW_RESOURCE_BUSY);
                }
                else
                {
                    const canview_status_t started = hash.start(&context);
                    if (step == MOCK_SETUP) { CHECK(started == expected[error] && context.hash_dirty); }
                    else
                    {
                        CHECK(started == CANVIEW_OK);
                        if (step == MOCK_UPDATE) { CHECK(hash.update(&context, message, 1U) == expected[error]); }
                        if (step == MOCK_FINISH)
                        {
                            CHECK(hash.finish(&context, digest) == expected[error]);
                            CHECK(memcmp(digest, zeros, sizeof(digest)) == 0);
                        }
                        if (step == MOCK_ABORT)
                        {
                            CHECK(hash.reset(&context) == expected[error] && context.hash_dirty);
                            CHECK(canview_esp_ota_crypto_close(&context) == expected[error]);
                            CHECK(context.key == MOCK_KEY && context.closing);
                        }
                    }
                    CHECK(hash.start(&context) == CANVIEW_RESOURCE_BUSY);
                }
            }
            fault = 0U;
            CHECK(canview_esp_ota_crypto_close(&context) == CANVIEW_OK);
            CHECK(!context.ready && !context.hash_dirty && !context.closing && context.key == 0U && correct);
        }
    }
    CHECK(canview_esp_ota_crypto_init(&context, root) == CANVIEW_OK);
    CHECK(hash.start(&context) == CANVIEW_OK);
    short_digest = true;
    CHECK(hash.finish(&context, digest) == CANVIEW_AUTH_FAILED);
    CHECK(memcmp(digest, zeros, sizeof(digest)) == 0);
    CHECK(canview_esp_ota_crypto_close(&context) == CANVIEW_OK && correct);
    (void)puts("PASS: PSA adapter lifecycle/fault/reentry model; actual SDK cryptography NOT_RUN");
    return 0;
}
