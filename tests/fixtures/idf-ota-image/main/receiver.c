/* SPDX-License-Identifier: GPL-3.0-only */
/** @file receiver.c @brief 같은 C 수신 흐름을 실제 SDK PSA 또는 host CNG로 검사한다. */
#include "receiver.h"
#include "body.h"
#if defined(CANVIEW_OTA_FIXTURE_CNG)
#include "cng_provider.h"
#else
#include "ota_crypto.h"
#endif

#define FIXTURE_PREFIX_CHUNK (31U)
#define FIXTURE_CASES (4U)
#define FIXTURE_SIGNATURE_TAMPER (1U)
#define FIXTURE_BODY_TAMPER (2U)
#define FIXTURE_IDENTITY_TAMPER (3U)
#define FIXTURE_PUBLIC_BYTES (65U)

/* ota-signed-golden/manifest-public.pem의 공개 X9.62 point. 제품 신뢰 root가 아니다. */
static const uint8_t fixture_public_key[FIXTURE_PUBLIC_BYTES] = {
    0x04U, 0x95U, 0x11U, 0xb2U, 0xd7U, 0x31U, 0x64U, 0xadU, 0xb3U, 0x82U, 0xa8U, 0x83U, 0x43U,
    0x74U, 0x5cU, 0x0bU, 0x70U, 0x28U, 0x8dU, 0x57U, 0xdfU, 0x69U, 0xa4U, 0x9bU, 0xc3U, 0x27U,
    0xb7U, 0x18U, 0xd5U, 0x1aU, 0xecU, 0x24U, 0x3aU, 0x04U, 0xe1U, 0x6eU, 0x9dU, 0xc7U, 0xbdU,
    0x42U, 0x3eU, 0x67U, 0x63U, 0x31U, 0x19U, 0x69U, 0xc9U, 0xefU, 0x19U, 0x60U, 0x1fU, 0xecU,
    0x55U, 0xabU, 0x5aU, 0xa8U, 0x03U, 0x1fU, 0x45U, 0x79U, 0xffU, 0x7eU, 0xfcU, 0xbaU, 0xf8U
};

bool canview_ota_fixture_receiver_test(const uint8_t *data, size_t size)
{
    /* SDK task stack에16KiB prefix/body를 올리지 않는다. 동시/재호출 금지. */
    static canview_ota_prefix_t prefix;
    static canview_ota_body_t body;
    if (data == NULL || size == 0U || size > CANVIEW_OTA_ENVELOPE_BUNDLE_MAX) { return false; }
#if defined(CANVIEW_OTA_FIXTURE_CNG)
    static canview_test_cng_hash_t crypto;
    const canview_ota_hash_t hash = canview_test_cng_hash_provider(&crypto);
    const canview_ota_manifest_verify_fn verify = canview_test_p256_verify;
    void *const verify_context = (void *)(fixture_public_key + 1U);
#else
    static canview_esp_ota_crypto_t crypto;
    if (canview_esp_ota_crypto_init(&crypto, fixture_public_key) != CANVIEW_OK)
    {
        if (canview_esp_ota_crypto_close(&crypto) != CANVIEW_OK) { return false; }
        return false;
    }
    const canview_ota_hash_t hash = canview_esp_ota_hash_provider(&crypto);
    const canview_ota_manifest_verify_fn verify = canview_esp_ota_manifest_verify;
    void *const verify_context = &crypto;
#endif
    bool passed = true;
    for (uint32_t scenario = 0U; scenario < FIXTURE_CASES && passed; ++scenario)
    {
        canview_ota_identity_t identity = {CANVIEW_OTA_ROLE_COMMUNICATOR, "synthetic-board", "synthetic-layout", 7U, 11U};
        const canview_ota_runtime_t runtime = {true, 1U, 1U, 1U, 1U, 2U, 2U, 1U, UINT64_MAX};
        const canview_ota_floor_t floor = {.ready = true, .identity = identity, .count = 2U,
            .records = {{.target = CANVIEW_OTA_TARGET_COMM_ESP}, {.target = CANVIEW_OTA_TARGET_COMM_STM}}};
        canview_status_t status = canview_ota_prefix_init(&prefix);
        bool body_tamper_rejected = false;
        size_t offset = 0U;
        while (status == CANVIEW_OK || status == CANVIEW_INCOMPLETE)
        {
            if (offset == size) { status = CANVIEW_INCOMPLETE; break; }
            const size_t count = size - offset < FIXTURE_PREFIX_CHUNK ? size - offset : FIXTURE_PREFIX_CHUNK;
            size_t consumed = 0U;
            status = canview_ota_prefix_feed(&prefix, (uint32_t)offset, data + offset, count, &consumed);
            offset += consumed;
            if (status != CANVIEW_INCOMPLETE) { break; }
        }
        if (status == CANVIEW_OK) { status = canview_ota_prefix_finish(&prefix); }
        if (status == CANVIEW_OK)
        {
            if (scenario == FIXTURE_SIGNATURE_TAMPER) { prefix.data[prefix.received - 1U] ^= 1U; }
            if (scenario == FIXTURE_IDENTITY_TAMPER) { identity.board_revision[0] = 'x'; }
            status = canview_ota_body_open(&body, prefix.data, prefix.received, &identity, &runtime,
                &floor, verify, verify_context, &hash);
        }
        while (status == CANVIEW_OK && offset < size)
        {
            size_t count = size - offset < CANVIEW_OTA_BODY_CHUNK_MAX ? size - offset : CANVIEW_OTA_BODY_CHUNK_MAX;
            if (scenario == FIXTURE_BODY_TAMPER && count == size - offset)
            {
                if (count > 1U) { --count; }
                else
                {
                    const uint8_t changed = data[offset] ^ 1U;
                    status = canview_ota_body_feed(&body, (uint32_t)offset, &changed, 1U);
                    body_tamper_rejected = status == CANVIEW_AUTH_FAILED;
                    ++offset;
                    break;
                }
            }
            status = canview_ota_body_feed(&body, (uint32_t)offset, data + offset, count);
            offset += count;
        }
        if (status == CANVIEW_OK) { status = canview_ota_body_finish(&body); }
        const canview_status_t expected = scenario == 0U ? CANVIEW_OK : CANVIEW_AUTH_FAILED;
        passed = status == expected &&
            (scenario != FIXTURE_BODY_TAMPER || (body_tamper_rejected && offset == size));
        /* cleanup 실패면 static 자원을 보존하며 SDK close로 수명을 뒤집지 않는다. */
        if (canview_ota_body_reset(&body) != CANVIEW_OK) { return false; }
    }
#if !defined(CANVIEW_OTA_FIXTURE_CNG)
    if (canview_esp_ota_crypto_close(&crypto) != CANVIEW_OK) { return false; }
#endif
    return passed;
}
