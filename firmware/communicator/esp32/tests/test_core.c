/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_esp_core.h"
#include "canview_esp_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(value)                                                                               \
    do                                                                                             \
    {                                                                                              \
        if (!(value))                                                                              \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #value);                      \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

typedef struct
{
    canview_esp_core_t *core;
    canview_esp_core_port_t port;
    canview_esp_core_sample_t sample;
    uint64_t now;
    uint32_t sample_delay;
    uint32_t feed_delay;
    uint32_t calls;
    uint32_t fail_at;
    uint32_t reenter_at;
    uint32_t feeds;
    bool reenter_boot;
    bool backward_sample;
    bool backward_feed;
    bool forged_feed_time;
    char trace[64];
} fixture_t;

static canview_status_t visit(fixture_t *fixture, char stage)
{
    CHECK(fixture->calls + 1U < sizeof(fixture->trace));
    fixture->trace[fixture->calls++] = stage;
    fixture->trace[fixture->calls] = '\0';
    if (fixture->calls == fixture->reenter_at)
    {
        const canview_status_t status = fixture->reenter_boot
                                            ? canview_esp_core_boot(fixture->core, &fixture->port)
                                            : canview_esp_core_step(fixture->core);
        CHECK(status == CANVIEW_RESOURCE_BUSY);
    }
    return fixture->calls == fixture->fail_at ? CANVIEW_NOT_IMPLEMENTED : CANVIEW_OK;
}
static canview_status_t safe_gpio(void *context)
{
    return visit(context, 'S');
}
static canview_status_t watchdog_start(void *context)
{
    return visit(context, 'W');
}
static canview_status_t now_us(void *context, uint64_t *now)
{
    fixture_t *fixture = context;
    const canview_status_t status = visit(fixture, 'T');
    *now = fixture->now;
    return status;
}
static canview_status_t sample(void *context, canview_esp_core_sample_t *value)
{
    fixture_t *fixture = context;
    const canview_status_t status = visit(fixture, 'M');
    *value = fixture->sample;
    fixture->now += fixture->sample_delay;
    if (fixture->backward_sample)
    {
        --fixture->now;
    }
    return status;
}
static canview_status_t feed(void *context, uint64_t earliest, uint64_t deadline, uint64_t *fed_at)
{
    fixture_t *fixture = context;
    const canview_status_t status = visit(fixture, 'F');
    if (status != CANVIEW_OK)
    {
        return status;
    }
    fixture->now += fixture->feed_delay;
    if (fixture->backward_feed)
    {
        fixture->now = earliest - 1U;
    }
    if (fixture->now < earliest || fixture->now > deadline)
    {
        return CANVIEW_TIMEOUT;
    }
    ++fixture->feeds;
    *fed_at = fixture->forged_feed_time ? deadline + 1U : fixture->now;
    return CANVIEW_OK;
}
static void initialize(fixture_t *fixture, canview_esp_core_t *core)
{
    memset(fixture, 0, sizeof(*fixture));
    memset(core, 0, sizeof(*core));
    fixture->core = core;
    const canview_esp_core_port_t port = {safe_gpio, watchdog_start, now_us, sample, feed, fixture};
    fixture->port = port;
    const canview_esp_core_sample_t healthy = {CANVIEW_ESP_CORE_FLASH_BYTES,
                                               CANVIEW_ESP_CORE_PSRAM_BYTES,
                                               131072U,
                                               65536U,
                                               4096U,
                                               1U,
                                               false,
                                               false};
    fixture->sample = healthy;
}
static void boot_healthy(fixture_t *fixture, canview_esp_core_t *core)
{
    initialize(fixture, core);
    CHECK(canview_esp_core_boot(core, &fixture->port) == CANVIEW_OK);
    CHECK(strcmp(fixture->trace, "SWTMT") == 0);
    CHECK(core->state == CANVIEW_ESP_CORE_SAFE_BENCH && core->watchdog_ready);
    CHECK(core->feeds == 0U && fixture->feeds == 0U);
    fixture->calls = 0U;
}
static void make_bad_memory(canview_esp_core_sample_t *value, uint32_t scenario)
{
    switch (scenario)
    {
    case 0:
        value->flash_bytes = 4194304U;
        break;
    case 1:
        value->psram_bytes = 8388608U;
        break;
    case 2:
        value->psram_bytes = 0U;
        break;
    case 3:
        value->heap_free_bytes = CANVIEW_ESP_CORE_HEAP_MIN - 1U;
        break;
    case 4:
        value->heap_free_bytes = CANVIEW_ESP_CORE_INTERNAL_MAX + 1U;
        break;
    case 5:
        value->largest_block_bytes = CANVIEW_ESP_CORE_BLOCK_MIN - 1U;
        break;
    case 6:
        value->largest_block_bytes = value->heap_free_bytes + 1U;
        break;
    case 7:
        value->stack_free_bytes = CANVIEW_ESP_CORE_STACK_MIN - 1U;
        break;
    case 8:
        value->stack_free_bytes = CANVIEW_ESP_CORE_STACK_BYTES + 1U;
        break;
    default:
        CHECK(false);
        break;
    }
}
static void boot_tests(void)
{
    fixture_t fixture;
    canview_esp_core_t core;
    initialize(&fixture, &core);
    CHECK(canview_esp_core_boot(NULL, &fixture.port) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_core_boot(&core, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_core_step(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_core_step(&core) == CANVIEW_INVALID_ARGUMENT);
    canview_esp_core_port_t ports[5] = {fixture.port, fixture.port, fixture.port, fixture.port,
                                        fixture.port};
    ports[0].safe_gpio = NULL;
    ports[1].watchdog_start = NULL;
    ports[2].now_us = NULL;
    ports[3].sample = NULL;
    ports[4].feed = NULL;
    for (uint32_t index = 0U; index < 5U; ++index)
    {
        CHECK(canview_esp_core_boot(&core, &ports[index]) == CANVIEW_INVALID_ARGUMENT);
        CHECK(fixture.calls == 0U && core.state == CANVIEW_ESP_CORE_UNINITIALIZED);
    }
    for (uint32_t stage = 1U; stage <= 5U; ++stage)
    {
        initialize(&fixture, &core);
        fixture.fail_at = stage;
        CHECK(canview_esp_core_boot(&core, &fixture.port) == CANVIEW_NOT_IMPLEMENTED);
        CHECK(core.state == CANVIEW_ESP_CORE_FAULT && !core.busy);
        CHECK(fixture.calls == stage && fixture.feeds == 0U);
        const canview_esp_core_fault_t fault = core.fault;
        CHECK(canview_esp_core_boot(&core, &fixture.port) == CANVIEW_RESOURCE_BUSY);
        CHECK(canview_esp_core_step(&core) == CANVIEW_TIMEOUT && core.fault == fault);
        for (uint32_t entry = 0U; entry < 2U; ++entry)
        {
            initialize(&fixture, &core);
            fixture.reenter_at = stage;
            fixture.reenter_boot = entry != 0U;
            CHECK(canview_esp_core_boot(&core, &fixture.port) != CANVIEW_OK);
            CHECK(core.fault == CANVIEW_ESP_FAULT_REENTRY && fixture.feeds == 0U);
        }
    }
    for (uint32_t scenario = 0U; scenario < 9U; ++scenario)
    {
        initialize(&fixture, &core);
        make_bad_memory(&fixture.sample, scenario);
        CHECK(canview_esp_core_boot(&core, &fixture.port) == CANVIEW_TIMEOUT);
        CHECK(core.fault == CANVIEW_ESP_FAULT_MEMORY && fixture.feeds == 0U);
    }
    initialize(&fixture, &core);
    fixture.now = UINT64_MAX - CANVIEW_ESP_CORE_DEADLINE_US + 1U;
    CHECK(canview_esp_core_boot(&core, &fixture.port) == CANVIEW_TIMEOUT);
    CHECK(core.fault == CANVIEW_ESP_FAULT_CLOCK);
    initialize(&fixture, &core);
    fixture.now = 1U;
    fixture.backward_sample = true;
    CHECK(canview_esp_core_boot(&core, &fixture.port) == CANVIEW_TIMEOUT);
    CHECK(core.fault == CANVIEW_ESP_FAULT_CLOCK);
    initialize(&fixture, &core);
    fixture.sample_delay = (uint32_t)CANVIEW_ESP_CORE_BUDGET_US + 1U;
    CHECK(canview_esp_core_boot(&core, &fixture.port) == CANVIEW_TIMEOUT);
    CHECK(core.fault == CANVIEW_ESP_FAULT_BUDGET);
    boot_healthy(&fixture, &core);
    CHECK(canview_esp_core_boot(&core, &fixture.port) == CANVIEW_RESOURCE_BUSY);
}
static void health_tests(void)
{
    fixture_t fixture;
    canview_esp_core_t core;
    for (uint32_t sense = 0U; sense < 4U; ++sense)
    {
        boot_healthy(&fixture, &core);
        fixture.sample.service_run_sense = (sense & 1U) != 0U;
        fixture.sample.usb_service_sense = (sense & 2U) != 0U;
        fixture.sample.heap_free_bytes = CANVIEW_ESP_CORE_HEAP_MIN;
        fixture.sample.largest_block_bytes = CANVIEW_ESP_CORE_BLOCK_MIN;
        fixture.sample.stack_free_bytes = CANVIEW_ESP_CORE_STACK_MIN;
        for (uint32_t tick = 0U; tick < 20U; ++tick)
        {
            fixture.calls = 0U;
            fixture.now += 100000U;
            CHECK(canview_esp_core_step(&core) == CANVIEW_OK);
            CHECK(strcmp(fixture.trace, "TMTF") == 0);
            CHECK(core.state == CANVIEW_ESP_CORE_SAFE_BENCH);
            CHECK(core.feeds == tick + 1U && core.checks == core.feeds);
            CHECK(core.last_progress_us == fixture.now &&
                  core.sample.heap_free_bytes == CANVIEW_ESP_CORE_HEAP_MIN);
        }
        core.feeds = UINT32_MAX;
        core.checks = UINT32_MAX;
        fixture.calls = 0U;
        fixture.now += 100000U;
        CHECK(canview_esp_core_step(&core) == CANVIEW_OK);
        CHECK(core.feeds == UINT32_MAX && core.checks == UINT32_MAX);
    }
    const uint32_t delay[] = {0U, 1000U, 20000U};
    for (uint32_t index = 0U; index < 3U; ++index)
    {
        boot_healthy(&fixture, &core);
        fixture.sample_delay = delay[index];
        fixture.now = CANVIEW_ESP_CORE_DEADLINE_US - delay[index];
        CHECK(canview_esp_core_step(&core) == CANVIEW_OK);
        CHECK(core.last_progress_us == CANVIEW_ESP_CORE_DEADLINE_US && fixture.feeds == 1U);
    }
}
static void fault_tests(void)
{
    fixture_t fixture;
    canview_esp_core_t core;
    for (uint32_t stage = 1U; stage <= 4U; ++stage)
    {
        boot_healthy(&fixture, &core);
        fixture.now = 100000U;
        fixture.fail_at = stage;
        CHECK(canview_esp_core_step(&core) == CANVIEW_NOT_IMPLEMENTED);
        CHECK(core.state == CANVIEW_ESP_CORE_FAULT && core.last_progress_us == 0U &&
              fixture.feeds == 0U);
        for (uint32_t entry = 0U; entry < 2U; ++entry)
        {
            boot_healthy(&fixture, &core);
            fixture.now = 100000U;
            fixture.reenter_at = stage;
            fixture.reenter_boot = entry != 0U;
            CHECK(canview_esp_core_step(&core) != CANVIEW_OK);
            CHECK(core.fault == CANVIEW_ESP_FAULT_REENTRY && core.last_progress_us == 0U);
        }
    }
    for (uint32_t scenario = 0U; scenario < 9U; ++scenario)
    {
        boot_healthy(&fixture, &core);
        fixture.now = 100000U;
        make_bad_memory(&fixture.sample, scenario);
        CHECK(canview_esp_core_step(&core) == CANVIEW_TIMEOUT);
        CHECK(core.fault == CANVIEW_ESP_FAULT_MEMORY && fixture.feeds == 0U);
    }
    for (uint32_t scenario = 0U; scenario < 9U; ++scenario)
    {
        boot_healthy(&fixture, &core);
        fixture.now = 100000U;
        canview_esp_core_fault_t expected = CANVIEW_ESP_FAULT_CLOCK;
        switch (scenario)
        {
        case 0:
            fixture.now = 0U;
            break; /* 정지한 timer */
        case 1:
            fixture.now = UINT64_MAX;
            break;
        case 2:
            fixture.now = CANVIEW_ESP_CORE_DEADLINE_US + 1U;
            expected = CANVIEW_ESP_FAULT_DEADLINE;
            break;
        case 3:
            fixture.now = CANVIEW_ESP_CORE_DEADLINE_US;
            fixture.sample_delay = 1U;
            expected = CANVIEW_ESP_FAULT_DEADLINE;
            break;
        case 4:
            fixture.sample_delay = (uint32_t)CANVIEW_ESP_CORE_BUDGET_US + 1U;
            expected = CANVIEW_ESP_FAULT_BUDGET;
            break;
        case 5:
            fixture.backward_sample = true;
            break;
        case 6:
            fixture.feed_delay = (uint32_t)CANVIEW_ESP_CORE_BUDGET_US + 1U;
            expected = CANVIEW_ESP_FAULT_WATCHDOG;
            break;
        case 7:
            fixture.backward_feed = true;
            expected = CANVIEW_ESP_FAULT_WATCHDOG;
            break;
        case 8:
            fixture.forged_feed_time = true;
            break;
        default:
            CHECK(false);
            break;
        }
        CHECK(canview_esp_core_step(&core) == CANVIEW_TIMEOUT);
        CHECK(core.fault == expected && core.last_progress_us == 0U);
        CHECK(fixture.feeds == (scenario == 8U ? 1U : 0U));
        CHECK(canview_esp_core_step(&core) == CANVIEW_TIMEOUT);
    }
    boot_healthy(&fixture, &core);
    fixture.now = 100000U;
    CHECK(canview_esp_core_step(&core) == CANVIEW_OK);
    fixture.now -= 1U;
    CHECK(canview_esp_core_step(&core) == CANVIEW_TIMEOUT);
    CHECK(core.fault == CANVIEW_ESP_FAULT_CLOCK);
}

typedef struct
{
    uint32_t depth;
    uint32_t enters;
    uint32_t leaves;
} lock_t;
static void enter(void *context)
{
    lock_t *lock = context;
    ++lock->depth;
    ++lock->enters;
}
static void leave(void *context)
{
    lock_t *lock = context;
    CHECK(lock->depth != 0U);
    --lock->depth;
    ++lock->leaves;
}
static void pool_tests(void)
{
    canview_esp_pool_t pool = {0};
    canview_esp_pool_t other = {0};
    lock_t lock = {2U, 0U, 0U};
    const canview_esp_pool_port_t port = {enter, leave, &lock};
    uint8_t data[256];
    uint8_t output[256];
    memset(data, 0x5a, sizeof(data));
    memset(output, 0xa5, sizeof(output));
    canview_esp_pool_token_t token = {0};
    size_t length = 7U;
    canview_esp_pool_stats_t stats = {0};
    CHECK(canview_esp_pool_init(NULL, 1U, &port) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_init(&pool, 1U, NULL) == CANVIEW_INVALID_ARGUMENT);
    canview_esp_pool_port_t invalid = port;
    invalid.enter = NULL;
    CHECK(canview_esp_pool_init(&pool, 1U, &invalid) == CANVIEW_INVALID_ARGUMENT);
    invalid = port;
    invalid.leave = NULL;
    CHECK(canview_esp_pool_init(&pool, 1U, &invalid) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_init(&pool, 0U, &port) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_init(&pool, 17U, &port) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_release(&pool, token) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_acquire(NULL, data, 1U, &token) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_copy(&pool, token, output, sizeof(output), &length) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_stats(&pool, &stats) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_init(&pool, 2U, &port) == CANVIEW_OK);
    CHECK(canview_esp_pool_init(&pool, 2U, &port) == CANVIEW_RESOURCE_BUSY);
    CHECK(canview_esp_pool_init(&other, 1U, &port) == CANVIEW_OK);
    CHECK(canview_esp_pool_acquire(&pool, NULL, 1U, &token) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_acquire(&pool, data, 1U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_acquire(&pool, data, 0U, &token) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_acquire(&pool, data, 257U, &token) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_acquire(&pool, &pool, 1U, &token) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_acquire(&pool, data, 1U, (canview_esp_pool_token_t *)&pool) ==
          CANVIEW_INVALID_ARGUMENT);
    union
    {
        canview_esp_pool_token_t token;
        uint8_t bytes[256];
    } alias = {0};
    CHECK(canview_esp_pool_acquire(&pool, alias.bytes, 256U, &alias.token) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_acquire(&pool, data, sizeof(data), &token) == CANVIEW_OK);
    memset(data, 0xff, sizeof(data));
    CHECK(canview_esp_pool_copy(&pool, token, output, sizeof(output), &length) == CANVIEW_OK);
    CHECK(length == 256U && output[0] == 0x5aU && output[255] == 0x5aU);
    CHECK(canview_esp_pool_copy(&other, token, output, sizeof(output), &length) == CANVIEW_STALE);
    CHECK(canview_esp_pool_copy(&pool, token, NULL, 256U, &length) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_copy(&pool, token, output, 256U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_copy(&pool, token, &pool, 1U, &length) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_copy(&pool, token, output, 256U, (size_t *)&pool) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_copy(&pool, token, &length, sizeof(length), &length) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_copy(&pool, token, output, SIZE_MAX, &length) ==
          CANVIEW_INVALID_ARGUMENT);
    length = 99U;
    output[0] = 0x11U;
    CHECK(canview_esp_pool_copy(&pool, token, output, 255U, &length) == CANVIEW_BUFFER_TOO_SMALL);
    CHECK(length == 99U && output[0] == 0x11U);
    CHECK(canview_esp_pool_release(&pool, token) == CANVIEW_OK);
    for (uint32_t index = 0U; index < 256U; ++index)
    {
        CHECK(pool.slots[token.slot].data[index] == 0U);
    }
    CHECK(canview_esp_pool_release(&pool, token) == CANVIEW_STALE);
    CHECK(canview_esp_pool_copy(&pool, token, output, 256U, &length) == CANVIEW_STALE);
    const canview_esp_pool_token_t stale = token;
    CHECK(canview_esp_pool_acquire(&pool, data, 1U, &token) == CANVIEW_OK);
    CHECK(token.generation != stale.generation);
    CHECK(canview_esp_pool_release(&pool, stale) == CANVIEW_STALE);
    CHECK(canview_esp_pool_release(&pool, token) == CANVIEW_OK);
    token.generation = 0U;
    CHECK(canview_esp_pool_release(&pool, token) == CANVIEW_STALE);
    token.slot = 16U;
    CHECK(canview_esp_pool_release(&pool, token) == CANVIEW_STALE);
    CHECK(canview_esp_pool_stats(&pool, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_stats(&pool, &pool.stats) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_pool_stats(&pool, &stats) == CANVIEW_OK);
    CHECK(stats.used == 0U && stats.high_water == 1U && stats.stale != 0U);
    for (uint16_t capacity = 1U; capacity <= 16U; ++capacity)
    {
        memset(&pool, 0, sizeof(pool)); /* host fixture: 새 pool lifetime */
        CHECK(canview_esp_pool_init(&pool, capacity, &port) == CANVIEW_OK);
        canview_esp_pool_token_t tokens[16] = {{0}};
        for (uint16_t index = 0U; index < capacity; ++index)
        {
            CHECK(canview_esp_pool_acquire(&pool, data, (size_t)index + 1U, &tokens[index]) ==
                  CANVIEW_OK);
        }
        const canview_esp_pool_token_t before = token;
        CHECK(canview_esp_pool_acquire(&pool, data, 1U, &token) == CANVIEW_RESOURCE_BUSY);
        CHECK(token.owner == before.owner && token.slot == before.slot &&
              token.generation == before.generation);
        CHECK(canview_esp_pool_stats(&pool, &stats) == CANVIEW_OK);
        CHECK(stats.exhausted == 1U && stats.used == capacity && stats.high_water == capacity);
        for (uint16_t index = 0U; index < capacity; ++index)
        {
            CHECK(canview_esp_pool_release(&pool, tokens[index]) == CANVIEW_OK);
        }
    }
    memset(&pool, 0, sizeof(pool));
    CHECK(canview_esp_pool_init(&pool, 1U, &port) == CANVIEW_OK);
    pool.slots[0].generation = UINT32_MAX - 1U; /* wrap fault injection */
    CHECK(canview_esp_pool_acquire(&pool, data, 1U, &token) == CANVIEW_OK);
    CHECK(token.generation == UINT32_MAX);
    CHECK(canview_esp_pool_release(&pool, token) == CANVIEW_OK);
    CHECK(canview_esp_pool_acquire(&pool, data, 1U, &token) == CANVIEW_RESOURCE_BUSY);
    pool.stats.exhausted = UINT32_MAX;
    pool.stats.stale = UINT32_MAX;
    CHECK(canview_esp_pool_acquire(&pool, data, 1U, &token) == CANVIEW_RESOURCE_BUSY);
    CHECK(canview_esp_pool_release(&pool, token) == CANVIEW_STALE);
    CHECK(canview_esp_pool_stats(&pool, &stats) == CANVIEW_OK);
    CHECK(stats.retired == 1U && stats.exhausted == UINT32_MAX && stats.stale == UINT32_MAX);
    CHECK(lock.depth == 2U && lock.enters == lock.leaves);
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    CHECK(strcmp(argv[1], "boot") == 0 || strcmp(argv[1], "health") == 0 ||
          strcmp(argv[1], "faults") == 0 || strcmp(argv[1], "pool") == 0 ||
          strcmp(argv[1], "all") == 0);
    if (strcmp(argv[1], "boot") == 0 || strcmp(argv[1], "all") == 0)
    {
        boot_tests();
    }
    if (strcmp(argv[1], "health") == 0 || strcmp(argv[1], "all") == 0)
    {
        health_tests();
    }
    if (strcmp(argv[1], "faults") == 0 || strcmp(argv[1], "all") == 0)
    {
        fault_tests();
    }
    if (strcmp(argv[1], "pool") == 0 || strcmp(argv[1], "all") == 0)
    {
        pool_tests();
    }
    (void)puts("PASS: ESP32 portable core/pool fixture, not board/HIL");
    return 0;
}
