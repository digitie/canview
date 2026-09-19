/* SPDX-License-Identifier: GPL-3.0-only */
/** @file stage_probe.c @brief 실제 parser와 동기 storage 모형의 순서/실패 연결 시험. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif
#include "stage.h"
#if defined(CANVIEW_TEST_CNG)
#include "cng_provider.h"
#endif

#define CHECK(value) do { if (!(value)) { (void)fprintf(stderr, "%s:%d\n", __FILE__, __LINE__); exit(EXIT_FAILURE); } } while (0)
#define INPUT_MAX (262144U)
#define PUBLIC_BYTES (64U)

typedef enum
{
    NORMAL, SIGNATURE_FAIL, HASH_START_FAIL, BEGIN_FAIL, WRITE_FAIL, NATIVE_FAIL,
    CLOSE_FAIL, HASH_RESET_FAIL, BAD_IDENTITY, BAD_RUNTIME, BAD_FLOOR,
    TRUNCATED, DUPLICATE_OFFSET, HOLE, BAD_BODY, OVERLAP, OVERSIZE_CHUNK,
    NULL_CHUNK, RESET_PARTIAL, SCENARIO_COUNT
} scenario_t;

typedef struct
{
    canview_ota_stage_t *stage;
    canview_ota_hash_t inner;
    const uint8_t *input;
    size_t size;
    size_t written;
    size_t sum;
    uint32_t hash_bytes;
    uint8_t public_key[PUBLIC_BYTES];
    uint32_t begins;
    uint32_t writes;
    uint32_t verifies;
    uint32_t closes;
    uint32_t resets;
    scenario_t scenario;
} fixture_t;

static void reentry(fixture_t *fixture)
{
    CHECK(canview_ota_stage_open(fixture->stage, NULL, 0U, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == CANVIEW_RESOURCE_BUSY);
    CHECK(canview_ota_stage_feed(fixture->stage, 0U, NULL, 0U) == CANVIEW_RESOURCE_BUSY);
    CHECK(canview_ota_stage_finish(fixture->stage) == CANVIEW_RESOURCE_BUSY);
    CHECK(canview_ota_stage_reset(fixture->stage) == CANVIEW_RESOURCE_BUSY);
}

static canview_status_t signature(void *context, const uint8_t *data, size_t size,
    const uint8_t value[CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES])
{
    fixture_t *fixture = context;
    reentry(fixture);
    if (fixture->scenario == SIGNATURE_FAIL) { return CANVIEW_AUTH_FAILED; }
#if defined(CANVIEW_TEST_CNG)
    return canview_test_p256_verify(fixture->public_key, data, size, value);
#else
    return data != NULL && size != 0U && value != NULL ? CANVIEW_OK : CANVIEW_INVALID_ARGUMENT;
#endif
}

static canview_status_t hash_start(void *context)
{
    fixture_t *fixture = context;
    reentry(fixture);
    if (fixture->scenario == HASH_START_FAIL) { return CANVIEW_RESOURCE_BUSY; }
#if defined(CANVIEW_TEST_CNG)
    return fixture->inner.start(fixture->inner.context);
#else
    fixture->sum = 0U;
    fixture->hash_bytes = 0U;
    return CANVIEW_OK;
#endif
}

static canview_status_t hash_update(void *context, const uint8_t *data, size_t size)
{
    fixture_t *fixture = context;
    reentry(fixture);
#if defined(CANVIEW_TEST_CNG)
    return fixture->inner.update(fixture->inner.context, data, size);
#else
    for (size_t index = 0U; index < size; ++index) { fixture->sum += data[index]; }
    fixture->hash_bytes += (uint32_t)size;
    return CANVIEW_OK;
#endif
}

static canview_status_t hash_finish(void *context, uint8_t digest[CANVIEW_OTA_DIGEST_BYTES])
{
    fixture_t *fixture = context;
    reentry(fixture);
#if defined(CANVIEW_TEST_CNG)
    return fixture->inner.finish(fixture->inner.context, digest);
#else
    (void)memset(digest, 0, CANVIEW_OTA_DIGEST_BYTES);
    for (uint32_t index = 0U; index < 4U; ++index)
    {
        digest[index] = (uint8_t)(fixture->sum >> (index * 8U));
        digest[index + 4U] = (uint8_t)(fixture->hash_bytes >> (index * 8U));
    }
    return CANVIEW_OK;
#endif
}

static canview_status_t hash_reset(void *context)
{
    fixture_t *fixture = context;
    reentry(fixture);
    ++fixture->resets;
    if (fixture->scenario == HASH_RESET_FAIL) { return CANVIEW_RESOURCE_BUSY; }
#if defined(CANVIEW_TEST_CNG)
    return fixture->inner.reset(fixture->inner.context);
#else
    return CANVIEW_OK;
#endif
}

static canview_status_t storage_begin(void *context, const canview_ota_manifest_t *manifest,
    const uint8_t *prefix, size_t size)
{
    fixture_t *fixture = context;
    reentry(fixture);
    ++fixture->begins;
    CHECK(fixture->stage->body.state == CANVIEW_OTA_BODY_RECEIVING);
    CHECK(manifest->total_size == fixture->size && size <= fixture->size);
    CHECK(memcmp(prefix, fixture->input, size) == 0);
    for (uint32_t index = 0U; index < manifest->image_count; ++index)
    {
        const canview_ota_target_t target = manifest->images[index].target;
        CHECK((manifest->role == CANVIEW_OTA_ROLE_COMMUNICATOR &&
            (target == CANVIEW_OTA_TARGET_COMM_ESP || target == CANVIEW_OTA_TARGET_COMM_STM)) ||
            (manifest->role == CANVIEW_OTA_ROLE_CONTROLLER && target == CANVIEW_OTA_TARGET_CONTROLLER) ||
            (manifest->role == CANVIEW_OTA_ROLE_BRIDGE && target == CANVIEW_OTA_TARGET_BRIDGE));
    }
    fixture->written = size;
    return fixture->scenario == BEGIN_FAIL ? CANVIEW_AUTH_FAILED : CANVIEW_OK;
}

static canview_status_t storage_write(void *context, uint32_t offset, const uint8_t *data, size_t size)
{
    fixture_t *fixture = context;
    reentry(fixture);
    ++fixture->writes;
    CHECK(fixture->begins == 1U && offset == fixture->written && size <= fixture->size - fixture->written);
    CHECK(memcmp(data, fixture->input + offset, size) == 0);
    if (fixture->scenario == WRITE_FAIL) { return CANVIEW_RESOURCE_BUSY; }
    fixture->written += size;
    return CANVIEW_OK;
}

static canview_status_t storage_verify(void *context, const canview_ota_manifest_t *manifest)
{
    fixture_t *fixture = context;
    reentry(fixture);
    ++fixture->verifies;
    CHECK(fixture->written == fixture->size && fixture->stage->body.state == CANVIEW_OTA_BODY_HASHES_MATCHED);
    CHECK(manifest->total_size == fixture->size);
    /* native SDK 동작은 별도 시험이다. 이 모형은 호출 시점과 실패 전파만 검사한다. */
    return fixture->scenario == NATIVE_FAIL ? CANVIEW_AUTH_FAILED : CANVIEW_OK;
}

static canview_status_t storage_close(void *context)
{
    fixture_t *fixture = context;
    reentry(fixture);
    ++fixture->closes;
    return fixture->scenario == CLOSE_FAIL ? CANVIEW_RESOURCE_BUSY : CANVIEW_OK;
}

static void run(const uint8_t *data, size_t size, size_t prefix_size, uint32_t role,
    const uint8_t *public_key, scenario_t scenario, size_t chunk, bool reject_prefix)
{
    canview_ota_stage_t stage = {0};
    fixture_t fixture = {.stage = &stage, .input = data, .size = size, .scenario = scenario};
    (void)memcpy(fixture.public_key, public_key, PUBLIC_BYTES);
#if defined(CANVIEW_TEST_CNG)
    canview_test_cng_hash_t native = {0};
    fixture.inner = canview_test_cng_hash_provider(&native);
#endif
    canview_ota_identity_t identity = {(canview_ota_role_t)role, "synthetic-board", "synthetic-layout", 7U, 11U};
    canview_ota_runtime_t runtime = {true, 1U, 1U, 1U, 1U, 2U, 2U, 1U, UINT64_MAX};
    canview_ota_floor_t floor = {.ready = true, .identity = identity, .count = role == 1U ? 2U : 1U};
    floor.records[0].target = role == 1U ? CANVIEW_OTA_TARGET_COMM_ESP :
        (role == 2U ? CANVIEW_OTA_TARGET_CONTROLLER : CANVIEW_OTA_TARGET_BRIDGE);
    floor.records[1].target = CANVIEW_OTA_TARGET_COMM_STM;
    if (scenario == BAD_IDENTITY) { identity.board_revision[0] = 'x'; }
    if (scenario == BAD_RUNTIME) { runtime.available = false; }
    if (scenario == BAD_FLOOR) { floor.ready = false; }
    const canview_ota_hash_t hash = {&fixture, hash_start, hash_update, hash_finish, hash_reset};
    const canview_ota_storage_t storage = {&fixture, storage_begin, storage_write, storage_verify, storage_close};
    canview_status_t result = canview_ota_stage_open(&stage, data, prefix_size, &identity,
        &runtime, &floor, signature, &fixture, &hash, &storage);
    if (reject_prefix || scenario == SIGNATURE_FAIL || scenario == HASH_START_FAIL ||
        scenario == BAD_IDENTITY || scenario == BAD_RUNTIME || scenario == BAD_FLOOR)
    {
        CHECK(result != CANVIEW_OK && fixture.begins == 0U && fixture.writes == 0U && fixture.verifies == 0U);
    }
    else
    {
        CHECK(fixture.begins == 1U);
        CHECK(canview_ota_stage_open(&stage, data, prefix_size, &identity,
            &runtime, &floor, signature, &fixture, &hash, &storage) == CANVIEW_RESOURCE_BUSY);
        if (scenario == RESET_PARTIAL)
        {
            CHECK(canview_ota_stage_reset(&stage) == CANVIEW_OK);
            CHECK(fixture.closes == 1U && fixture.writes == 0U && fixture.verifies == 0U);
            return;
        }
        size_t offset = prefix_size;
        if (result == CANVIEW_OK)
        {
            CHECK(canview_ota_stage_feed(&stage, (uint32_t)offset, NULL, 0U) == CANVIEW_OK);
            CHECK(fixture.writes == 0U);
        }
        const size_t end = scenario == TRUNCATED ? size - 1U : size;
        while (result == CANVIEW_OK && offset < end)
        {
            size_t count = end - offset < chunk ? end - offset : chunk;
            if (scenario == OVERLAP || scenario == OVERSIZE_CHUNK || scenario == NULL_CHUNK)
            {
                const uint8_t *invalid = scenario == OVERLAP ? (const uint8_t *)&stage :
                    (scenario == NULL_CHUNK ? NULL : data + offset);
                const size_t invalid_size = scenario == OVERSIZE_CHUNK ? CANVIEW_OTA_BODY_CHUNK_MAX + 1U : 1U;
                result = canview_ota_stage_feed(&stage, (uint32_t)offset, invalid, invalid_size);
                CHECK(result != CANVIEW_OK && fixture.writes == 0U);
                break;
            }
            if (scenario == DUPLICATE_OFFSET || scenario == HOLE)
            {
                result = canview_ota_stage_feed(&stage, (uint32_t)offset + (scenario == HOLE ? 1U : UINT32_MAX), data + offset, count);
                CHECK(result != CANVIEW_OK && fixture.writes == 0U);
                break;
            }
            if (scenario == BAD_BODY && offset + count == size)
            {
                if (count > 1U) { --count; }
                else
                {
                    const uint8_t changed = data[offset] ^ 1U;
                    result = canview_ota_stage_feed(&stage, (uint32_t)offset, &changed, 1U);
                    CHECK(result == CANVIEW_AUTH_FAILED);
                    break;
                }
            }
            result = canview_ota_stage_feed(&stage, (uint32_t)offset, data + offset, count);
            offset += count;
        }
        if (result == CANVIEW_OK) { result = canview_ota_stage_finish(&stage); }
        const bool valid = scenario == NORMAL || scenario == CLOSE_FAIL;
        CHECK((result == CANVIEW_OK) == valid);
        CHECK(fixture.verifies == ((valid || scenario == NATIVE_FAIL) ? 1U : 0U));
        CHECK((stage.state == CANVIEW_OTA_STAGE_NATIVE_MATCHED) == valid);
    }
    const uint32_t old_writes = fixture.writes;
    const uint32_t old_verifies = fixture.verifies;
    CHECK(canview_ota_stage_finish(&stage) != CANVIEW_OK);
    CHECK(fixture.verifies == old_verifies);
    CHECK(canview_ota_stage_feed(&stage, 0U, data, 1U) != CANVIEW_OK);
    CHECK(fixture.writes == old_writes);
    const canview_status_t original = stage.error;
    if (scenario == NATIVE_FAIL) { fixture.scenario = CLOSE_FAIL; }
    canview_status_t cleanup = canview_ota_stage_reset(&stage);
    if (scenario == CLOSE_FAIL || scenario == HASH_RESET_FAIL || scenario == NATIVE_FAIL)
    {
        CHECK(cleanup == CANVIEW_RESOURCE_BUSY && stage.error == original);
        fixture.scenario = NORMAL;
        cleanup = canview_ota_stage_reset(&stage);
    }
    CHECK(cleanup == CANVIEW_OK && stage.state == CANVIEW_OTA_STAGE_EMPTY);
    CHECK(fixture.closes == (fixture.begins == 0U ? 0U :
        ((scenario == CLOSE_FAIL || scenario == NATIVE_FAIL) ? 2U : 1U)));
    CHECK(canview_ota_stage_reset(&stage) == CANVIEW_OK);
}

static uint32_t u32(const uint8_t *value)
{
    return (uint32_t)value[0] | ((uint32_t)value[1] << 8U) |
        ((uint32_t)value[2] << 16U) | ((uint32_t)value[3] << 24U);
}

static void arguments(void)
{
    canview_ota_stage_t stage = {0};
    CHECK(canview_ota_stage_open(NULL, NULL, 0U, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_stage_feed(NULL, 0U, NULL, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_stage_finish(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_stage_reset(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_stage_finish(&stage) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_stage_feed(&stage, 0U, NULL, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_stage_reset(&stage) == CANVIEW_OK);
}

static canview_status_t argument_hash_start(void *context)
{
    /* context 방어 변이에서도 잘못된 객체를 역참조하지 않는다. 검출은 아래 CHECK다. */
    return context == NULL ? CANVIEW_INVALID_ARGUMENT : CANVIEW_OK;
}

static void argument_guards(const uint8_t *data, size_t size, size_t prefix_size,
    uint32_t role, const uint8_t *public_key)
{
    canview_ota_stage_t stage = {0};
    fixture_t fixture = {.stage = &stage, .input = data, .size = size};
    (void)memcpy(fixture.public_key, public_key, PUBLIC_BYTES);
#if defined(CANVIEW_TEST_CNG)
    canview_test_cng_hash_t native = {0};
    fixture.inner = canview_test_cng_hash_provider(&native);
#endif
    const canview_ota_identity_t identity = {(canview_ota_role_t)role, "synthetic-board", "synthetic-layout", 7U, 11U};
    const canview_ota_runtime_t runtime = {true, 1U, 1U, 1U, 1U, 2U, 2U, 1U, UINT64_MAX};
    const canview_ota_floor_t floor = {.ready = true, .identity = identity, .count = role == 1U ? 2U : 1U,
        .records = {{.target = role == 1U ? CANVIEW_OTA_TARGET_COMM_ESP :
            (role == 2U ? CANVIEW_OTA_TARGET_CONTROLLER : CANVIEW_OTA_TARGET_BRIDGE)},
            {.target = CANVIEW_OTA_TARGET_COMM_STM}}};
    canview_ota_hash_t hash = {&fixture, argument_hash_start, hash_update, hash_finish, hash_reset};
    const canview_ota_storage_t storage = {&fixture, storage_begin, storage_write, storage_verify, storage_close};
    /* 같은 인자의 양성 대조. 이후 case는 한 인자/field만 바꾼다. */
    CHECK(canview_ota_stage_open(&stage, data, prefix_size, &identity, &runtime, &floor,
        signature, &fixture, &hash, &storage) == CANVIEW_OK);
    CHECK(fixture.begins == 1U);
    CHECK(canview_ota_stage_reset(&stage) == CANVIEW_OK && fixture.closes == 1U);
    for (uint32_t item = 0U; item < 7U; ++item)
    {
        canview_ota_storage_t invalid = storage;
        if (item == 0U) { invalid.context = NULL; }
        if (item == 1U) { invalid.begin = NULL; }
        if (item == 2U) { invalid.write = NULL; }
        if (item == 3U) { invalid.verify = NULL; }
        if (item == 4U) { invalid.close = NULL; }
        if (item == 5U) { invalid.context = &stage; }
        CHECK(canview_ota_stage_open(&stage, data, prefix_size, &identity, &runtime, &floor, signature,
            &fixture, &hash, item == 6U ? NULL : &invalid) == CANVIEW_INVALID_ARGUMENT);
        CHECK(stage.body.state == CANVIEW_OTA_BODY_EMPTY);
        CHECK(canview_ota_stage_reset(&stage) == CANVIEW_OK);
    }
    /* 정렬된 stage 내부에 실제 유효 값을 복사한다. 제어/함수표 영역은 건드리지 않는다. */
    CHECK(prefix_size <= sizeof(stage.body.manifest));
    CHECK(sizeof(identity) <= sizeof(stage.body.manifest) && sizeof(runtime) <= sizeof(stage.body.manifest));
    CHECK(sizeof(floor) <= sizeof(stage.body.manifest) && sizeof(hash) <= sizeof(stage.body.manifest));
    CHECK(sizeof(storage) <= sizeof(stage.body.manifest));
    for (uint32_t item = 0U; item < 7U; ++item)
    {
        void *const inside = &stage;
        if (item == 0U) { (void)memcpy(inside, data, prefix_size); }
        if (item == 1U) { (void)memcpy(inside, &identity, sizeof(identity)); }
        if (item == 2U) { (void)memcpy(inside, &runtime, sizeof(runtime)); }
        if (item == 3U) { (void)memcpy(inside, &floor, sizeof(floor)); }
        if (item == 5U) { (void)memcpy(inside, &hash, sizeof(hash)); }
        if (item == 6U) { (void)memcpy(inside, &storage, sizeof(storage)); }
        CHECK(canview_ota_stage_open(&stage, item == 0U ? inside : data, prefix_size,
            item == 1U ? inside : &identity, item == 2U ? inside : &runtime, item == 3U ? inside : &floor,
            signature, item == 4U ? (void *)&stage : (void *)&fixture,
            item == 5U ? inside : &hash, item == 6U ? inside : &storage) == CANVIEW_INVALID_ARGUMENT);
        /* 같은 오류라도 하위 parser가 인자를 지운 뒤 거절한 경우는 실패다. */
        CHECK(stage.body.state == CANVIEW_OTA_BODY_EMPTY);
        CHECK(canview_ota_stage_reset(&stage) == CANVIEW_OK);
    }
    hash.context = &stage;
    CHECK(canview_ota_stage_open(&stage, data, prefix_size, &identity, &runtime, &floor, signature,
        &fixture, &hash, &storage) == CANVIEW_INVALID_ARGUMENT);
    CHECK(stage.body.state == CANVIEW_OTA_BODY_EMPTY);
    CHECK(canview_ota_stage_reset(&stage) == CANVIEW_OK);
    hash.context = &fixture;
    CHECK(canview_ota_stage_open(&stage, data, SIZE_MAX, &identity, &runtime,
        &floor, signature, &fixture, &hash, &storage) == CANVIEW_INVALID_ARGUMENT);
    CHECK(stage.body.state == CANVIEW_OTA_BODY_EMPTY);
    CHECK(canview_ota_stage_reset(&stage) == CANVIEW_OK);
    CHECK(fixture.begins == 1U && fixture.writes == 0U && fixture.verifies == 0U && fixture.closes == 1U);
}

int main(void)
{
    static uint8_t input[INPUT_MAX];
    uint8_t header[16U];
    uint8_t public_key[PUBLIC_BYTES];
    arguments();
#if defined(_WIN32)
    CHECK(_setmode(_fileno(stdin), _O_BINARY) >= 0);
#endif
    CHECK(fread(header, 1U, sizeof(header), stdin) == sizeof(header));
    const uint32_t role = u32(header);
    const uint32_t prefix = u32(header + 4U);
    const uint32_t size = u32(header + 8U);
    const bool reject = u32(header + 12U) != 0U;
    CHECK(role >= 1U && role <= 3U && prefix <= size && size <= sizeof(input));
    CHECK(fread(public_key, 1U, sizeof(public_key), stdin) == sizeof(public_key));
    CHECK(fread(input, 1U, size, stdin) == size && fgetc(stdin) == EOF && ferror(stdin) == 0);
    if (reject) { run(input, size, prefix, role, public_key, NORMAL, CANVIEW_OTA_BODY_CHUNK_MAX, true); }
    else
    {
        argument_guards(input, size, prefix, role, public_key);
        for (uint32_t scenario = 0U; scenario < (uint32_t)SCENARIO_COUNT; ++scenario)
        {
            run(input, size, prefix, role, public_key, (scenario_t)scenario, CANVIEW_OTA_BODY_CHUNK_MAX, false);
        }
        run(input, size, prefix, role, public_key, NORMAL, 1U, false);
        run(input, size, prefix, role, public_key, NORMAL, 31U, false);
    }
    (void)puts("PASS: parser/storage call-order model; Flash/native device/install NOT_RUN");
    return 0;
}
