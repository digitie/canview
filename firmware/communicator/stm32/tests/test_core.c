/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_core.h"
#include "canview_stm_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                                           \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);                  \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

typedef struct
{
    uint32_t us;
    uint32_t duration;
    uint32_t now;
    uint32_t calls;
    uint32_t feeds;
    uint32_t faults;
    uint32_t mask;
    uint32_t locks;
    uint32_t unlocks;
    canview_stm_fault_t reason;
    canview_status_t status;
    bool feed_fails;
    bool feed_reenter;
    bool reenter;
    canview_stm_scheduler_t *scheduler;
    uint32_t boot_stage;
    uint32_t fail_stage;
    uint32_t time_calls;
    uint32_t time_reenter_call;
    uint32_t time_jump_call;
    uint32_t time_jump_us;
} fixture_t;

static void fail(void *context, canview_stm_fault_t reason)
{
    fixture_t *const fixture = context;
    ++fixture->faults;
    fixture->reason = reason;
}

static uint32_t now_us(void *context)
{
    fixture_t *const fixture = context;
    ++fixture->time_calls;
    if (fixture->time_calls == fixture->time_reenter_call)
    {
        CHECK(canview_stm_scheduler_step(fixture->scheduler, fixture->now) == CANVIEW_TIMEOUT);
    }
    if (fixture->time_calls == fixture->time_jump_call)
    {
        fixture->us += fixture->time_jump_us;
    }
    return fixture->us;
}

static canview_status_t feed(void *context)
{
    fixture_t *const fixture = context;
    ++fixture->feeds;
    if (fixture->feed_reenter)
    {
        CHECK(canview_stm_scheduler_step(fixture->scheduler, fixture->now) == CANVIEW_TIMEOUT);
    }
    return fixture->feed_fails ? CANVIEW_TIMEOUT : CANVIEW_OK;
}

static canview_status_t worker(void *context)
{
    fixture_t *const fixture = context;
    ++fixture->calls;
    fixture->us += fixture->duration;
    if (fixture->reenter)
    {
        CHECK(canview_stm_scheduler_step(fixture->scheduler, fixture->now) == CANVIEW_TIMEOUT);
    }
    return fixture->status;
}

static canview_status_t boot_action(fixture_t *fixture, uint32_t stage)
{
    CHECK(fixture->boot_stage == stage);
    ++fixture->boot_stage;
    return stage == fixture->fail_stage ? CANVIEW_TIMEOUT : CANVIEW_OK;
}
static canview_status_t boot_safe(void *context)
{
    return boot_action(context, 0U);
}
static canview_status_t boot_watchdog(void *context)
{
    return boot_action(context, 1U);
}
static canview_status_t boot_clock(void *context)
{
    return boot_action(context, 2U);
}
static canview_status_t boot_time(void *context)
{
    return boot_action(context, 3U);
}

static canview_stm_scheduler_port_t port(fixture_t *fixture)
{
    const canview_stm_scheduler_port_t result = {now_us, feed, fail, fixture};
    return result;
}

static void initialize(canview_stm_scheduler_t *scheduler, fixture_t *fixture, uint32_t start)
{
    const canview_stm_worker_t workers[] = {{worker, fixture, 1U, 20U, 1000U, true}};
    const canview_stm_scheduler_port_t callbacks = port(fixture);
    fixture->scheduler = scheduler;
    CHECK(canview_stm_scheduler_init(scheduler, workers, 1U, &callbacks, start) == CANVIEW_OK);
}

static void boot_tests(void)
{
    for (uint32_t stage = 0U; stage <= 4U; ++stage)
    {
        fixture_t fixture = {0};
        fixture.fail_stage = stage;
        canview_stm_boot_t boot = {0};
        canview_stm_boot_port_t callbacks = {boot_safe, boot_watchdog, boot_clock,
                                             boot_time, fail,          &fixture};
        CHECK(canview_stm_boot_start(NULL, &callbacks) == CANVIEW_INVALID_ARGUMENT);
        CHECK(canview_stm_boot_start(&boot, NULL) == CANVIEW_INVALID_ARGUMENT);
        canview_stm_boot_port_t bad = callbacks;
        bad.safe = NULL;
        CHECK(canview_stm_boot_start(&boot, &bad) == CANVIEW_INVALID_ARGUMENT);
        bad = callbacks;
        bad.watchdog_start = NULL;
        CHECK(canview_stm_boot_start(&boot, &bad) == CANVIEW_INVALID_ARGUMENT);
        bad = callbacks;
        bad.clock_start = NULL;
        CHECK(canview_stm_boot_start(&boot, &bad) == CANVIEW_INVALID_ARGUMENT);
        bad = callbacks;
        bad.time_start = NULL;
        CHECK(canview_stm_boot_start(&boot, &bad) == CANVIEW_INVALID_ARGUMENT);
        bad = callbacks;
        bad.fault = NULL;
        CHECK(canview_stm_boot_start(&boot, &bad) == CANVIEW_INVALID_ARGUMENT);
        CHECK(fixture.boot_stage == 0U);
        CHECK(canview_stm_boot_start(&boot, &callbacks) ==
              (stage < 4U ? CANVIEW_TIMEOUT : CANVIEW_OK));
        CHECK(boot.state == (stage < 4U ? CANVIEW_STM_BOOT_FAULT : CANVIEW_STM_BOOT_READY));
        CHECK(boot.watchdog_started == (stage > 1U));
        CHECK(fixture.boot_stage == (stage < 4U ? stage + 1U : 4U));
        CHECK(fixture.faults == (stage < 4U ? 1U : 0U));
        CHECK(canview_stm_boot_start(&boot, &callbacks) == CANVIEW_RESOURCE_BUSY);
    }
}

static void scheduler_validation(void)
{
    fixture_t fixture = {0};
    canview_stm_scheduler_t scheduler = {0};
    canview_stm_scheduler_port_t callbacks = port(&fixture);
    const canview_stm_worker_t valid = {worker, &fixture, 1U, 20U, 1000U, true};
    CHECK(canview_stm_scheduler_step(NULL, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_scheduler_step(&scheduler, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_scheduler_init(NULL, &valid, 1U, &callbacks, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_scheduler_init(&scheduler, NULL, 1U, &callbacks, 0U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_scheduler_init(&scheduler, &valid, 1U, NULL, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_scheduler_init(&scheduler, &valid, 0U, &callbacks, 0U) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_scheduler_init(&scheduler, &valid, 9U, &callbacks, 0U) ==
          CANVIEW_INVALID_ARGUMENT);
    for (uint32_t scenario = 0U; scenario < 11U; ++scenario)
    {
        canview_stm_worker_t bad = valid;
        callbacks = port(&fixture);
        switch (scenario)
        {
        case 0U:
            bad.run = NULL;
            break;
        case 1U:
            bad.period_ms = 0U;
            break;
        case 2U:
            bad.period_ms = 1001U;
            break;
        case 3U:
            bad.deadline_ms = 0U;
            break;
        case 4U:
            bad.deadline_ms = 1001U;
            break;
        case 5U:
            bad.deadline_ms = 21U;
            break;
        case 6U:
            bad.budget_us = 0U;
            break;
        case 7U:
            bad.budget_us = 1001U;
            break;
        case 8U:
            callbacks.now_us = NULL;
            break;
        case 9U:
            callbacks.feed = NULL;
            break;
        default:
            callbacks.fault = NULL;
            break;
        }
        const canview_stm_scheduler_t before = scheduler;
        CHECK(canview_stm_scheduler_init(&scheduler, &bad, 1U, &callbacks, 0U) ==
              CANVIEW_INVALID_ARGUMENT);
        CHECK(memcmp(&before, &scheduler, sizeof(before)) == 0);
    }
    callbacks = port(&fixture);
    canview_stm_worker_t optional = valid;
    optional.required = false;
    CHECK(canview_stm_scheduler_init(&scheduler, &optional, 1U, &callbacks, 0U) ==
          CANVIEW_INVALID_ARGUMENT);
    scheduler.workers[0] = valid;
    scheduler.port = callbacks;
    CHECK(canview_stm_scheduler_init(&scheduler, scheduler.workers, 1U, &scheduler.port, 0U) ==
          CANVIEW_OK);
    CHECK(canview_stm_scheduler_init(&scheduler, &valid, 1U, &callbacks, 0U) ==
          CANVIEW_RESOURCE_BUSY);
}

static void scheduler_healthy(void)
{
    const uint32_t starts[] = {0U, UINT32_MAX - 5U};
    for (size_t index = 0U; index < sizeof(starts) / sizeof(starts[0]); ++index)
    {
        fixture_t fixture = {0};
        canview_stm_scheduler_t scheduler = {0};
        initialize(&scheduler, &fixture, starts[index]);
        fixture.us = UINT32_MAX - 500U;
        fixture.duration = 10U;
        for (uint32_t tick = 0U; tick <= 100U; ++tick)
        {
            CHECK(canview_stm_scheduler_step(&scheduler, starts[index] + tick) == CANVIEW_OK);
        }
        CHECK(fixture.calls == 101U && fixture.feeds == 10U && fixture.faults == 0U);
        scheduler.feeds = UINT32_MAX;
        CHECK(canview_stm_scheduler_step(&scheduler, starts[index] + 110U) == CANVIEW_OK);
        CHECK(scheduler.feeds == UINT32_MAX);
    }
    fixture_t fixture = {0};
    fixture_t idle = {0};
    idle.status = CANVIEW_RESOURCE_BUSY;
    canview_stm_scheduler_t scheduler = {0};
    canview_stm_worker_t workers[CANVIEW_STM_WORKERS_MAX];
    for (size_t index = 0U; index < CANVIEW_STM_WORKERS_MAX; ++index)
    {
        const canview_stm_worker_t item = {worker, &fixture, 10U, 20U, 1000U, true};
        workers[index] = item;
    }
    workers[7].context = &idle;
    workers[7].required = false;
    workers[7].period_ms = 1000U;
    workers[7].deadline_ms = 1000U;
    const canview_stm_scheduler_port_t callbacks = port(&fixture);
    CHECK(canview_stm_scheduler_init(&scheduler, workers, CANVIEW_STM_WORKERS_MAX, &callbacks,
                                     0U) == CANVIEW_OK);
    for (uint32_t tick = 0U; tick <= 20U; ++tick)
    {
        CHECK(canview_stm_scheduler_step(&scheduler, tick) == CANVIEW_OK);
    }
    CHECK(fixture.calls == 21U && idle.calls == 1U && fixture.feeds == 2U);
    CHECK(canview_stm_scheduler_step(&scheduler, 20U) == CANVIEW_OK);
    CHECK(fixture.calls == 21U && fixture.feeds == 2U);
}

static void scheduler_faults(void)
{
    for (uint32_t scenario = 0U; scenario < 14U; ++scenario)
    {
        fixture_t fixture = {0};
        canview_stm_scheduler_t scheduler = {0};
        initialize(&scheduler, &fixture, 100U);
        fixture.now = 100U;
        canview_stm_fault_t expected = CANVIEW_STM_FAULT_WORKER;
        if (scenario == 0U)
        {
            fixture.status = CANVIEW_CRC_MISMATCH;
        }
        if (scenario == 1U)
        {
            fixture.duration = 1001U;
            expected = CANVIEW_STM_FAULT_BUDGET;
        }
        if (scenario == 2U)
        {
            fixture.reenter = true;
            expected = CANVIEW_STM_FAULT_REENTRY;
        }
        if (scenario == 3U)
        {
            fixture.now = 99U;
            expected = CANVIEW_STM_FAULT_GAP;
        }
        if (scenario == 4U)
        {
            fixture.now = 121U;
            expected = CANVIEW_STM_FAULT_GAP;
        }
        if (scenario == 5U)
        {
            fixture.feed_fails = true;
            fixture.now = 110U;
            expected = CANVIEW_STM_FAULT_PORT;
        }
        if (scenario == 6U)
        {
            fixture.feed_reenter = true;
            fixture.now = 110U;
            expected = CANVIEW_STM_FAULT_REENTRY;
        }
        if (scenario == 7U)
        {
            fixture.status = CANVIEW_RESOURCE_BUSY;
            expected = CANVIEW_STM_FAULT_DEADLINE;
        }
        if (scenario == 7U)
        {
            for (uint32_t tick = 100U; tick <= 120U; ++tick)
            {
                CHECK(canview_stm_scheduler_step(&scheduler, tick) == CANVIEW_OK);
            }
            CHECK(fixture.feeds == 0U);
            fixture.now = 121U;
            fixture.status = CANVIEW_OK; /* expired worker를 late success로 살리지 않는다. */
        }
        if (scenario >= 8U && scenario <= 11U)
        {
            fixture.time_reenter_call = scenario - 7U;
            expected = CANVIEW_STM_FAULT_REENTRY;
        }
        if (scenario == 12U)
        {
            /* 각 worker는 budget 이내여도 owner 전체 경과가 20ms를 넘으면 거부. */
            fixture.time_jump_call = 4U;
            fixture.time_jump_us = 20001U;
            expected = CANVIEW_STM_FAULT_BUDGET;
        }
        if (scenario == 13U)
        {
            CHECK(canview_stm_scheduler_step(&scheduler, 100U) == CANVIEW_OK);
            fixture.now = 120U;
            fixture.status = CANVIEW_RESOURCE_BUSY;
            fixture.duration = 1U;
            expected = CANVIEW_STM_FAULT_DEADLINE;
        }
        CHECK(canview_stm_scheduler_step(&scheduler, fixture.now) == CANVIEW_TIMEOUT);
        CHECK(scheduler.fault == expected && fixture.faults == 1U && fixture.reason == expected);
        CHECK(!scheduler.running && scheduler.votes == 0U);
        const uint32_t feeds = fixture.feeds;
        fixture.status = CANVIEW_OK;
        CHECK(canview_stm_scheduler_step(&scheduler, fixture.now + 1U) == CANVIEW_TIMEOUT);
        CHECK(fixture.faults == 1U && fixture.feeds == feeds);
    }
}

static uint32_t enter(void *context)
{
    fixture_t *const fixture = context;
    ++fixture->locks;
    const uint32_t saved = fixture->mask;
    fixture->mask = 1U;
    return saved;
}
static void leave(void *context, uint32_t saved)
{
    fixture_t *const fixture = context;
    CHECK(fixture->mask == 1U);
    fixture->mask = saved;
    ++fixture->unlocks;
}

static void queue_tests(void)
{
    for (size_t capacity = 1U; capacity <= CANVIEW_STM_QUEUE_CAPACITY_MAX; capacity *= 2U)
    {
        fixture_t fixture = {0};
        fixture.mask = 1U;
        canview_stm_queue_t queue = {0};
        uint8_t storage[CANVIEW_STM_QUEUE_CAPACITY_MAX * CANVIEW_STM_QUEUE_RECORD_MAX];
        uint8_t input[CANVIEW_STM_QUEUE_RECORD_MAX];
        uint8_t output[CANVIEW_STM_QUEUE_RECORD_MAX];
        const canview_stm_critical_t critical = {enter, leave, &fixture};
        CHECK(canview_stm_queue_init(&queue, storage, sizeof(storage), capacity, sizeof(input),
                                     &critical) == CANVIEW_OK);
        CHECK(canview_stm_queue_init(&queue, storage, sizeof(storage), capacity, sizeof(input),
                                     &critical) == CANVIEW_RESOURCE_BUSY);
        CHECK(canview_stm_queue_push(&queue, storage, sizeof(input)) == CANVIEW_INVALID_ARGUMENT);
        CHECK(canview_stm_queue_pop(&queue, &queue, sizeof(output)) == CANVIEW_INVALID_ARGUMENT);
        CHECK(canview_stm_queue_pop(&queue, output, sizeof(output) - 1U) ==
              CANVIEW_INVALID_ARGUMENT);
        for (size_t cycle = 0U; cycle < 3U; ++cycle)
        {
            for (size_t index = 0U; index < capacity; ++index)
            {
                memset(input, (int)(index & 0xffU), sizeof(input));
                CHECK(canview_stm_queue_push(&queue, input, sizeof(input)) == CANVIEW_OK);
            }
            memset(input, 0xcc, sizeof(input));
            CHECK(canview_stm_queue_push(&queue, input, sizeof(input)) == CANVIEW_RESOURCE_BUSY);
            for (size_t index = 0U; index < capacity; ++index)
            {
                CHECK(canview_stm_queue_pop(&queue, output, sizeof(output)) == CANVIEW_OK);
                for (size_t byte = 0U; byte < sizeof(output); ++byte)
                {
                    CHECK(output[byte] == (uint8_t)(index & 0xffU));
                }
            }
            memset(output, 0xab, sizeof(output));
            CHECK(canview_stm_queue_pop(&queue, output, sizeof(output)) == CANVIEW_INCOMPLETE);
            CHECK(output[0] == 0xabU && output[sizeof(output) - 1U] == 0xabU);
        }
        CHECK(queue.high_water == capacity && queue.dropped == 3U && queue.count == 0U);
        CHECK(fixture.locks == fixture.unlocks && fixture.mask == 1U);
        fixture.mask = 0U;
        CHECK(canview_stm_queue_push(&queue, input, sizeof(input)) == CANVIEW_OK);
        CHECK(fixture.mask == 0U);
    }
    fixture_t fixture = {0};
    canview_stm_queue_t queue = {0};
    uint8_t storage[8] = {0};
    uint8_t input = 7U;
    canview_stm_critical_t critical = {enter, leave, &fixture};
    CHECK(canview_stm_queue_push(NULL, &input, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_pop(&queue, &input, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_init(NULL, storage, sizeof(storage), 1U, 1U, &critical) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_init(&queue, NULL, sizeof(storage), 1U, 1U, &critical) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_init(&queue, storage, sizeof(storage), 0U, 1U, &critical) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_init(&queue, storage, sizeof(storage), 257U, 1U, &critical) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_init(&queue, storage, sizeof(storage), 1U, 0U, &critical) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_init(&queue, storage, sizeof(storage), 1U, 65U, &critical) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_init(&queue, storage, 0U, 1U, 1U, &critical) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_init(&queue, (uint8_t *)&queue, sizeof(queue), 1U, 1U, &critical) ==
          CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_init(&queue, storage, sizeof(storage), 1U, 1U, NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    critical.enter = NULL;
    CHECK(canview_stm_queue_init(&queue, storage, sizeof(storage), 1U, 1U, &critical) ==
          CANVIEW_INVALID_ARGUMENT);
    critical.enter = enter;
    critical.leave = NULL;
    CHECK(canview_stm_queue_init(&queue, storage, sizeof(storage), 1U, 1U, &critical) ==
          CANVIEW_INVALID_ARGUMENT);
    critical.leave = leave;
    CHECK(canview_stm_queue_init(&queue, storage, sizeof(storage), 1U, 1U, &critical) ==
          CANVIEW_OK);
    CHECK(canview_stm_queue_push(&queue, NULL, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_queue_push(&queue, &input, 1U) == CANVIEW_OK);
    queue.dropped = UINT32_MAX;
    CHECK(canview_stm_queue_push(&queue, &input, 1U) == CANVIEW_RESOURCE_BUSY);
    CHECK(queue.dropped == UINT32_MAX);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        return 2;
    }
    const bool all = strcmp(argv[1], "all") == 0;
    if (all || strcmp(argv[1], "boot") == 0)
    {
        boot_tests();
    }
    else if (strcmp(argv[1], "scheduler-validation") == 0)
    {
        scheduler_validation();
    }
    else if (strcmp(argv[1], "scheduler-healthy") == 0)
    {
        scheduler_healthy();
    }
    else if (strcmp(argv[1], "scheduler-faults") == 0)
    {
        scheduler_faults();
    }
    else if (strcmp(argv[1], "queue") == 0)
    {
        queue_tests();
    }
    else
    {
        return 2;
    }
    if (all)
    {
        scheduler_validation();
        scheduler_healthy();
        scheduler_faults();
        queue_tests();
    }
    (void)puts("PASS: STM32 core host fixture");
    return 0;
}
