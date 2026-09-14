/* SPDX-License-Identifier: GPL-3.0-only */
/** @file native_stm_probe.c @brief 실제 CNG와 비암호 모형을 구분한 MCUboot oracle. */
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif
#include "native_stm.h"
#if defined(CANVIEW_TEST_CNG)
#include "cng_provider.h"
#endif

typedef struct
{
    const uint8_t *data;
    uint32_t size;
    uint32_t signed_size;
    uint32_t calls;
    uint32_t fault;
    canview_status_t signature_status;
    uint8_t public_key[64];
    uint8_t full_hash[32];
    uint8_t signed_hash[32];
    uint8_t signature[64];
} probe_t;

static uint32_t probe_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) | ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U);
}

static canview_status_t probe_hash(void *context, const uint8_t *data, size_t size, uint8_t digest[32])
{
    probe_t *probe = context;
    ++probe->calls;
    if (data != probe->data || probe->calls > 2U || size != (probe->calls == 1U ? probe->size : probe->signed_size))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (probe->calls == probe->fault) { return CANVIEW_RESOURCE_BUSY; }
#if defined(CANVIEW_TEST_CNG)
    return canview_test_native_sha256(probe->public_key, data, size, digest);
#else
    /* 사전에 Python이 계산한 결과를 반환하는 경계/흐름 모형. 암호 검증이 아니다. */
    (void)memcpy(digest, probe->calls == 1U ? probe->full_hash : probe->signed_hash, 32U);
    return CANVIEW_OK;
#endif
}

static canview_status_t probe_verify(void *context, const uint8_t digest[32], const uint8_t signature[64])
{
    probe_t *probe = context;
    ++probe->calls;
    if (probe->calls != 3U || memcmp(digest, probe->signed_hash, 32U) != 0 ||
        memcmp(signature, probe->signature, 64U) != 0)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (probe->calls == probe->fault) { return CANVIEW_RESOURCE_BUSY; }
#if defined(CANVIEW_TEST_CNG)
    return canview_test_p256_digest_verify(probe->public_key, digest, signature);
#else
    return probe->signature_status;
#endif
}

static bool probe_read(void *out, size_t size)
{
    return fread(out, 1U, size, stdin) == size;
}

int main(void)
{
    uint8_t header[48];
    uint8_t data[CANVIEW_OTA_STM_IMAGE_MAX + 1U];
    canview_ota_identity_t identity = {0};
    canview_ota_image_t expected = {0};
    probe_t probe = {0};
    canview_ota_native_crypto_t crypto = {.context = &probe, .sha256 = probe_hash, .verify = probe_verify};
    if (canview_ota_stm_image_check(NULL, 0U, &identity, &expected, &crypto) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_stm_image_check(data, 0U, NULL, &expected, &crypto) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_stm_image_check(data, 0U, &identity, NULL, &crypto) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_stm_image_check(data, 0U, &identity, &expected, NULL) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_stm_image_check(data, SIZE_MAX, &identity, &expected, &crypto) != CANVIEW_OVERSIZE)
    {
        return 1;
    }
    for (uint32_t index = 0U; index < 3U; ++index)
    {
        canview_ota_native_crypto_t invalid = crypto;
        if (index == 0U) { invalid.context = NULL; }
        if (index == 1U) { invalid.sha256 = NULL; }
        if (index == 2U) { invalid.verify = NULL; }
        if (canview_ota_stm_image_check(data, 0U, &identity, &expected, &invalid) != CANVIEW_INVALID_ARGUMENT)
        {
            return 1;
        }
    }
#if defined(_WIN32)
    if (_setmode(_fileno(stdin), _O_BINARY) < 0) { return 1; }
#endif
    for (;;)
    {
        const size_t count = fread(header, 1U, sizeof(header), stdin);
        if (count == 0U && feof(stdin) != 0 && ferror(stdin) == 0)
        {
            return fflush(stdout) == 0 ? 0 : 1;
        }
        if (count != sizeof(header)) { return 1; }
        probe.size = probe_u32(header);
        probe.fault = probe_u32(header + 4U);
        identity.role = (canview_ota_role_t)probe_u32(header + 8U);
        expected.target = (canview_ota_target_t)probe_u32(header + 12U);
        expected.signature = (canview_ota_image_signature_t)probe_u32(header + 16U);
        expected.abi = probe_u32(header + 20U);
        identity.security_epoch = probe_u32(header + 24U);
        expected.length = probe_u32(header + 28U);
        expected.release_sequence = (uint64_t)probe_u32(header + 32U) | ((uint64_t)probe_u32(header + 36U) << 32U);
        probe.signed_size = probe_u32(header + 40U);
        probe.signature_status = (canview_status_t)probe_u32(header + 44U);
        probe.calls = 0U;
        probe.data = data;
        if (probe.size > sizeof(data) || probe.fault > 3U ||
            !probe_read(probe.public_key, sizeof(probe.public_key)) ||
            !probe_read(crypto.key_digest, sizeof(crypto.key_digest)) ||
            !probe_read(expected.sha256, sizeof(expected.sha256)) ||
            !probe_read(probe.full_hash, sizeof(probe.full_hash)) ||
            !probe_read(probe.signed_hash, sizeof(probe.signed_hash)) ||
            !probe_read(probe.signature, sizeof(probe.signature)) ||
            !probe_read(identity.board_revision, sizeof(identity.board_revision)) ||
            !probe_read(identity.layout_id, sizeof(identity.layout_id)) ||
            !probe_read(expected.version, sizeof(expected.version)) || !probe_read(data, probe.size))
        {
            return 1;
        }
        const canview_status_t status = canview_ota_stm_image_check(data, probe.size, &identity, &expected, &crypto);
        if (printf("%u %u\n", (unsigned int)status, (unsigned int)probe.calls) < 0) { return 1; }
    }
}
