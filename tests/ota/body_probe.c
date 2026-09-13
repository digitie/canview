/* SPDX-License-Identifier: GPL-3.0-only */
/** @file body_probe.c @brief 순차 수신/자원 실패 oracle. CNG 빌드만 실제 암호를 사용한다. */
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif
#include "body.h"
#if defined(CANVIEW_TEST_CNG)
#include "cng_provider.h"
#endif

#define PROBE_HEADER_BYTES (28U)
#define PROBE_RUNTIME_BYTES (40U)
#define PROBE_PUBLIC_BYTES (64U)
#define PROBE_WIRE_MAX (5U * 1024U * 1024U)
#define PROBE_FAULT_START (1U)
#define PROBE_FAULT_UPDATE (2U)
#define PROBE_FAULT_FINISH (3U)
#define PROBE_FAULT_RESET (4U)

#if !defined(CANVIEW_TEST_CNG)
/* 수명/실패 주입 시험용 sum/length 모형. SHA-256으로 보고하면 안 된다. */
typedef struct { uint32_t sum; uint32_t size; bool live; } mock_hash_t;
static canview_status_t mock_start(void *context)
{
    mock_hash_t *hash = context;
    *hash = (mock_hash_t){0U, 0U, true};
    return CANVIEW_OK;
}
static canview_status_t mock_update(void *context, const uint8_t *data, size_t size)
{
    mock_hash_t *hash = context;
    if (!hash->live || data == NULL || size > CANVIEW_OTA_BODY_CHUNK_MAX)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    for (size_t index = 0U; index < size; ++index)
    {
        hash->sum += data[index];
    }
    hash->size += (uint32_t)size;
    return CANVIEW_OK;
}
static canview_status_t mock_finish(void *context, uint8_t digest[CANVIEW_OTA_DIGEST_BYTES])
{
    const mock_hash_t *hash = context;
    (void)memset(digest, 0, CANVIEW_OTA_DIGEST_BYTES);
    for (uint32_t index = 0U; index < 4U; ++index)
    {
        digest[index] = (uint8_t)(hash->sum >> (index * 8U));
        digest[index + 4U] = (uint8_t)(hash->size >> (index * 8U));
    }
    return hash->live ? CANVIEW_OK : CANVIEW_INVALID_ARGUMENT;
}
static canview_status_t mock_reset(void *context)
{
    *(mock_hash_t *)context = (mock_hash_t){0};
    return CANVIEW_OK;
}
#endif

typedef struct
{
    canview_ota_body_t *body;
    canview_ota_hash_t inner;
    uint8_t public_key[PROBE_PUBLIC_BYTES];
    uint32_t fault;
    uint32_t fault_calls;
    uint32_t start_calls;
    bool fired;
} probe_context_t;

static bool probe_reentry(const probe_context_t *probe)
{
    return canview_ota_body_open(probe->body, NULL, 0U, NULL, NULL, NULL, NULL, NULL, NULL) == CANVIEW_RESOURCE_BUSY &&
           canview_ota_body_feed(probe->body, 0U, NULL, 0U) == CANVIEW_RESOURCE_BUSY &&
           canview_ota_body_finish(probe->body) == CANVIEW_RESOURCE_BUSY &&
           canview_ota_body_reset(probe->body) == CANVIEW_RESOURCE_BUSY;
}

static canview_status_t probe_inject(probe_context_t *probe, uint32_t fault, canview_status_t status)
{
    if (!probe_reentry(probe))
    {
        return CANVIEW_MALFORMED;
    }
    if (!probe->fired && (probe->fault & 0xFFU) == fault)
    {
        ++probe->fault_calls;
        if (probe->fault_calls == (probe->fault >> 8U) + 1U)
        {
            probe->fired = true;
            return CANVIEW_RESOURCE_BUSY;
        }
    }
    return status;
}

static canview_status_t probe_start(void *context)
{
    probe_context_t *probe = context;
    ++probe->start_calls;
    const canview_status_t status = probe->inner.start(probe->inner.context);
    return probe_inject(probe, PROBE_FAULT_START, status);
}
static canview_status_t probe_update(void *context, const uint8_t *data, size_t size)
{
    probe_context_t *probe = context;
    const canview_status_t status = probe->inner.update(probe->inner.context, data, size);
    return probe_inject(probe, PROBE_FAULT_UPDATE, status);
}
static canview_status_t probe_finish(void *context, uint8_t digest[CANVIEW_OTA_DIGEST_BYTES])
{
    probe_context_t *probe = context;
    const canview_status_t status = probe->inner.finish(probe->inner.context, digest);
    return probe_inject(probe, PROBE_FAULT_FINISH, status);
}
static canview_status_t probe_reset(void *context)
{
    probe_context_t *probe = context;
    const canview_status_t status = probe_inject(probe, PROBE_FAULT_RESET, CANVIEW_OK);
    return status == CANVIEW_OK ? probe->inner.reset(probe->inner.context) : status;
}
static canview_status_t probe_verify(void *context, const uint8_t *data, size_t size,
    const uint8_t signature[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES])
{
    const probe_context_t *probe = context;
    if (!probe_reentry(probe))
    {
        return CANVIEW_MALFORMED;
    }
#if defined(CANVIEW_TEST_CNG)
    return canview_test_p256_verify((void *)probe->public_key, data, size, signature);
#else
    return data != NULL && size != 0U && signature != NULL ? CANVIEW_OK : CANVIEW_INVALID_ARGUMENT;
#endif
}

static uint32_t probe_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) |
        ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}

static bool probe_arguments(void)
{
    canview_ota_body_t body = {0};
    canview_ota_manifest_t manifest;
    probe_context_t context = {.body = &body};
    const canview_ota_hash_t null_context = {NULL, probe_start, probe_update, probe_finish, probe_reset};
    const canview_ota_hash_t valid_hash = {&context, probe_start, probe_update, probe_finish, probe_reset};
    (void)memset(&manifest, 0xA5, sizeof(manifest));
    if (canview_ota_manifest_preflight(NULL, 0U, NULL, NULL, NULL, NULL, NULL) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_manifest_preflight(NULL, 0U, NULL, NULL, NULL, NULL, &manifest) != CANVIEW_INVALID_ARGUMENT)
    {
        return false;
    }
    for (size_t index = 0U; index < sizeof(manifest); ++index)
    {
        if (((const uint8_t *)&manifest)[index] != 0U) { return false; }
    }
    if (canview_ota_body_open(&body, NULL, 0U, NULL, NULL, NULL, NULL, NULL, NULL) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_body_reset(&body) != CANVIEW_OK ||
        canview_ota_body_open(&body, NULL, 0U, NULL, NULL, NULL, NULL, NULL, &valid_hash) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_body_reset(&body) != CANVIEW_OK ||
        canview_ota_body_open(&body, NULL, 0U, NULL, NULL, NULL, NULL, NULL, &null_context) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_body_reset(&body) != CANVIEW_OK ||
        canview_ota_body_finish(&body) != CANVIEW_INCOMPLETE ||
        canview_ota_body_reset(&body) != CANVIEW_OK ||
        canview_ota_body_feed(&body, 0U, NULL, 0U) != CANVIEW_STALE ||
        canview_ota_body_reset(&body) != CANVIEW_OK)
    {
        return false;
    }
    for (uint32_t index = 0U; index < 4U; ++index)
    {
        canview_ota_hash_t hash = {&context, probe_start, probe_update, probe_finish, probe_reset};
        if (index == 0U) { hash.start = NULL; }
        if (index == 1U) { hash.update = NULL; }
        if (index == 2U) { hash.finish = NULL; }
        if (index == 3U) { hash.reset = NULL; }
        if (canview_ota_body_open(&body, NULL, 0U, NULL, NULL, NULL, NULL, NULL, &hash) != CANVIEW_INVALID_ARGUMENT ||
            canview_ota_body_reset(&body) != CANVIEW_OK)
        {
            return false;
        }
    }
    return true;
}

int main(void)
{
    uint8_t header[PROBE_HEADER_BYTES];
    uint8_t local_bytes[PROBE_RUNTIME_BYTES];
    uint8_t prefix[CANVIEW_OTA_ENVELOPE_PREFIX_MAX + 1U];
    uint8_t chunk[CANVIEW_OTA_BODY_CHUNK_MAX + 1U];
    if (canview_ota_body_open(NULL, NULL, 0U, NULL, NULL, NULL, NULL, NULL, NULL) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_body_feed(NULL, 0U, NULL, 0U) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_body_finish(NULL) != CANVIEW_INVALID_ARGUMENT ||
        canview_ota_body_reset(NULL) != CANVIEW_INVALID_ARGUMENT || !probe_arguments())
    {
        return 1;
    }
#if defined(_WIN32)
    if (_setmode(_fileno(stdin), _O_BINARY) < 0)
    {
        return 1;
    }
#endif
    for (;;)
    {
        canview_ota_body_t body = {0};
        probe_context_t probe = {.body = &body};
        canview_ota_identity_t identity = {CANVIEW_OTA_ROLE_COMMUNICATOR, "synthetic-board", "synthetic-layout", 7U, 11U};
        canview_ota_runtime_t runtime = {0};
        canview_ota_floor_t floor = {0};
        canview_ota_hash_t hash = {&probe, probe_start, probe_update, probe_finish, probe_reset};
#if defined(CANVIEW_TEST_CNG)
        canview_test_cng_hash_t native = {0};
        probe.inner = canview_test_cng_hash_provider(&native);
#else
        mock_hash_t native = {0};
        probe.inner = (canview_ota_hash_t){&native, mock_start, mock_update, mock_finish, mock_reset};
#endif
        const size_t header_size = fread(header, 1U, sizeof(header), stdin);
        if (header_size == 0U && feof(stdin) != 0 && ferror(stdin) == 0)
        {
            return fflush(stdout) == 0 ? 0 : 1;
        }
        if (header_size != sizeof(header))
        {
            return 1;
        }
        const uint32_t role = probe_u32(header);
        const uint32_t prefix_size = probe_u32(header + 4U);
        const uint32_t body_size = probe_u32(header + 8U);
        const uint32_t chunk_size = probe_u32(header + 12U);
        const uint32_t scenario = probe_u32(header + 16U);
        const uint32_t policy_case = probe_u32(header + 24U);
        probe.fault = probe_u32(header + 20U);
        if (role < 1U || role > 3U || prefix_size > sizeof(prefix) || body_size > PROBE_WIRE_MAX ||
            chunk_size == 0U || chunk_size > sizeof(chunk) || scenario > 7U || policy_case > 5U ||
            (probe.fault & 0xFFU) > PROBE_FAULT_RESET || (probe.fault >> 8U) > 1U ||
            fread(local_bytes, 1U, sizeof(local_bytes), stdin) != sizeof(local_bytes) ||
            probe_u32(local_bytes) > 1U ||
            fread(probe.public_key, 1U, PROBE_PUBLIC_BYTES, stdin) != PROBE_PUBLIC_BYTES ||
            fread(prefix, 1U, prefix_size, stdin) != prefix_size)
        {
            return 1;
        }
        identity.role = (canview_ota_role_t)role;
        floor.ready = policy_case != 1U;
        floor.identity = identity;
        floor.count = role == 1U ? 2U : 1U;
        floor.records[0].target = role == 1U ? CANVIEW_OTA_TARGET_COMM_ESP :
            (role == 2U ? CANVIEW_OTA_TARGET_CONTROLLER : CANVIEW_OTA_TARGET_BRIDGE);
        floor.records[1].target = CANVIEW_OTA_TARGET_COMM_STM;
        /* 기존 body 모형의 신뢰된 합성 floor0. 실제 journal/provider가 아니다. */
        if (policy_case == 2U)
        {
            floor.records[0].minimum_sequence = UINT64_MAX;
            floor.records[1].minimum_sequence = UINT64_MAX;
        }
        if (policy_case == 3U) { floor.identity.board_revision[0] = 'x'; }
        if (policy_case == 4U) { floor.count = 0U; }
        runtime.available = probe_u32(local_bytes) != 0U;
        runtime.esp_abi = probe_u32(local_bytes + 4U);
        runtime.stm_abi = probe_u32(local_bytes + 8U);
        runtime.esp_bootloader = probe_u32(local_bytes + 12U);
        runtime.stm_bootloader = probe_u32(local_bytes + 16U);
        runtime.esp_recovery = probe_u32(local_bytes + 20U);
        runtime.stm_recovery = probe_u32(local_bytes + 24U);
        runtime.config_schema = probe_u32(local_bytes + 28U);
        runtime.hardware_capabilities = (uint64_t)probe_u32(local_bytes + 32U) |
            ((uint64_t)probe_u32(local_bytes + 36U) << 32U);
        const canview_ota_floor_t *policy = policy_case == 5U ? NULL : &floor;
        canview_status_t status = canview_ota_body_open(&body, prefix, prefix_size, &identity, &runtime, policy, probe_verify, &probe, &hash);
        if (status != CANVIEW_OK && probe.fault == 0U && probe.start_calls != 0U)
        {
            return 1;
        }
        if (status != CANVIEW_OK)
        {
            if (body.floor_result.images[0] != CANVIEW_OTA_FLOOR_UNCHECKED ||
                body.floor_result.images[1] != CANVIEW_OTA_FLOOR_UNCHECKED) { return 1; }
            for (size_t index = 0U; index < sizeof(body.manifest); ++index)
            {
                if (((const uint8_t *)&body.manifest)[index] != 0U) { return 1; }
            }
        }
        if (status == CANVIEW_OK && scenario == 3U)
        {
            status = canview_ota_body_reset(&body);
            if (status == CANVIEW_OK)
            {
                status = canview_ota_body_open(&body, prefix, prefix_size, &identity, &runtime, policy, probe_verify, &probe, &hash);
            }
        }
        /* borrowed prefix/identity/함수표를 덮어써도 body는 자체 descriptor/함수표를 소유한다. */
        (void)memset(prefix, 0xA5, sizeof(prefix));
        (void)memset(&identity, 0, sizeof(identity));
        (void)memset(&runtime, 0, sizeof(runtime));
        (void)memset(&floor, 0, sizeof(floor));
        hash = (canview_ota_hash_t){0};
        if (status == CANVIEW_OK)
        {
            if (canview_ota_body_open(&body, NULL, 0U, NULL, NULL, NULL, NULL, NULL, NULL) != CANVIEW_RESOURCE_BUSY)
            {
                return 1;
            }
            status = canview_ota_body_feed(&body, prefix_size, NULL, 0U);
        }
        if (status == CANVIEW_OK && scenario == 5U)
        {
            status = canview_ota_body_feed(&body, prefix_size, NULL, 1U);
        }
        if (status == CANVIEW_OK && scenario == 6U)
        {
            status = canview_ota_body_feed(&body, prefix_size, chunk, SIZE_MAX);
        }
        uint32_t consumed = 0U;
        uint32_t chunk_index = 0U;
        while (consumed < body_size)
        {
            const uint32_t count = body_size - consumed < chunk_size ? body_size - consumed : chunk_size;
            if (fread(chunk, 1U, count, stdin) != count)
            {
                return 1;
            }
            if (status == CANVIEW_OK && !(scenario == 4U && chunk_index != 0U))
            {
                uint32_t offset = prefix_size + consumed;
                if (chunk_index == 1U && (scenario == 1U || scenario == 2U))
                {
                    offset = scenario == 1U ? offset - 1U : offset + 1U;
                }
                status = canview_ota_body_feed(&body, offset, chunk, count);
                if (status == CANVIEW_OK && scenario == 7U && chunk_index == 0U)
                {
                    status = canview_ota_body_reset(&body);
                }
            }
            (void)memset(chunk, 0xA5, sizeof(chunk));
            consumed += count;
            ++chunk_index;
        }
        const canview_status_t finished = canview_ota_body_finish(&body);
        if (status == CANVIEW_OK)
        {
            status = finished;
        }
        if (status != finished || canview_ota_body_finish(&body) != finished || (status != CANVIEW_OK &&
            (body.manifest.image_count != 0U || body.next_offset != 0U || body.state != CANVIEW_OTA_BODY_FAILED ||
             body.floor_result.images[0] != CANVIEW_OTA_FLOOR_UNCHECKED ||
             body.floor_result.images[1] != CANVIEW_OTA_FLOOR_UNCHECKED)))
        {
            return 1;
        }
        if (status != CANVIEW_OK && (canview_ota_body_feed(&body, 0U, NULL, 0U) != status ||
            canview_ota_body_open(&body, NULL, 0U, NULL, NULL, NULL, NULL, NULL, NULL) != CANVIEW_RESOURCE_BUSY))
        {
            return 1;
        }
        if (printf("%u %u %u %u %u", (unsigned int)status, (unsigned int)body.state,
            (unsigned int)body.error, (unsigned int)body.cleanup_error, body.hash_live ? 1U : 0U) < 0)
        {
            return 1;
        }
        canview_status_t reset = canview_ota_body_reset(&body);
        if (reset != CANVIEW_OK)
        {
            reset = canview_ota_body_reset(&body);
        }
        if (reset != CANVIEW_OK || body.state != CANVIEW_OTA_BODY_EMPTY || body.hash_live || body.busy ||
            canview_ota_body_reset(&body) != CANVIEW_OK)
        {
            return 1;
        }
#if defined(CANVIEW_TEST_CNG)
        if (native.handle != NULL)
#else
        if (native.live)
#endif
        {
            return 1;
        }
        if (printf(" %u\n", (unsigned int)reset) < 0)
        {
            return 1;
        }
    }
}
