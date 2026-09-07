/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_esp_runtime.h"
#include "canview_board.h"
#include <setjmp.h>
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
void app_main(void);
static struct
{
    const char *scenario;
    uint64_t time;
    unsigned safe;
    unsigned watchdog;
    unsigned samples;
    unsigned feeds;
    unsigned waits;
    unsigned reports;
    canview_status_t last_status;
    canview_esp_core_state_t last_state;
    jmp_buf stopped;
} fake;
static bool selected(const char *name)
{
    return strcmp(fake.scenario, name) == 0;
}
static canview_status_t safe(void *context)
{
    CHECK(context == &fake);
    ++fake.safe;
    return selected("gpio") ? CANVIEW_NOT_IMPLEMENTED : CANVIEW_OK;
}
static canview_status_t watchdog(void *context)
{
    CHECK(context == &fake && fake.safe == 1U);
    ++fake.watchdog;
    return selected("watchdog") ? CANVIEW_NOT_IMPLEMENTED : CANVIEW_OK;
}
static canview_status_t now(void *context, uint64_t *time)
{
    CHECK(context == &fake && time != NULL);
    *time = fake.time;
    return CANVIEW_OK;
}
static canview_status_t sample(void *context, canview_esp_core_sample_t *value)
{
    CHECK(context == &fake && fake.watchdog == 1U);
    ++fake.samples;
    *value = (canview_esp_core_sample_t){CANVIEW_ESP_CORE_FLASH_BYTES,
                                         CANVIEW_ESP_CORE_PSRAM_BYTES,
                                         CANVIEW_ESP_CORE_HEAP_MIN,
                                         CANVIEW_ESP_CORE_BLOCK_MIN,
                                         CANVIEW_ESP_CORE_STACK_MIN,
                                         1U,
                                         false,
                                         false};
    return selected("memory") ? CANVIEW_NOT_IMPLEMENTED : CANVIEW_OK;
}
static canview_status_t feed(void *context, uint64_t earliest, uint64_t deadline, uint64_t *time)
{
    CHECK(context == &fake && fake.time >= earliest && fake.time <= deadline);
    CHECK(!selected("late"));
    ++fake.feeds;
    *time = fake.time;
    return CANVIEW_OK;
}
static void lock(void *context)
{
    CHECK(context == &fake);
}
static canview_status_t wait(void *context)
{
    CHECK(context == &fake);
    ++fake.waits;
    fake.time += selected("late") ? 250001U : 100000U;
    return selected("wait") || fake.waits == 3U ? CANVIEW_TIMEOUT : CANVIEW_OK;
}
static void report(void *context, const canview_esp_core_t *core, canview_status_t status)
{
    CHECK(context == &fake && core != NULL);
    ++fake.reports;
    fake.last_state = core->state;
    fake.last_status = status;
}
static void idle(void *context)
{
    CHECK(context == &fake);
    longjmp(fake.stopped, 1);
}
canview_platform_port_t canview_board_port(void)
{
    const canview_platform_port_t port = {safe, idle, &fake};
    return port;
}
canview_status_t canview_esp_board_runtime(canview_esp_runtime_t *runtime,
                                           canview_esp_runtime_port_t *port)
{
    CHECK(runtime != NULL && port != NULL);
    if (selected("open"))
    {
        return CANVIEW_NOT_IMPLEMENTED;
    }
    *port = (canview_esp_runtime_port_t){
        {safe, watchdog, now, sample, feed, &fake}, {lock, lock, &fake}, wait, report, &fake};
    if (selected("pool"))
    {
        port->pool.enter = NULL;
    }
    return CANVIEW_OK;
}
int main(int argc, char **argv)
{
    CHECK(argc == 2);
    fake.scenario = argv[1];
    CHECK(selected("open") || selected("gpio") || selected("watchdog") || selected("memory") ||
          selected("pool") || selected("wait") || selected("late") || selected("healthy"));
    fake.time = 1000U;
    if (setjmp(fake.stopped) == 0)
    {
        app_main();
        CHECK(false);
    }
    CHECK(fake.feeds == (selected("healthy") ? 2U : 0U));
    if (selected("open"))
    {
        CHECK(fake.safe == 0U && fake.watchdog == 0U && fake.reports == 0U);
    }
    else
    {
        CHECK(fake.safe == 1U && fake.reports == 2U && fake.last_status != CANVIEW_OK);
        CHECK(fake.watchdog == (selected("gpio") ? 0U : 1U));
        if (selected("gpio") || selected("watchdog") || selected("memory") || selected("late"))
        {
            CHECK(fake.last_state == CANVIEW_ESP_CORE_FAULT);
        }
        else
        {
            CHECK(fake.last_state == CANVIEW_ESP_CORE_SAFE_BENCH);
        }
    }
    (void)puts("PASS: actual app composition reaches no-feed terminal idle");
    return 0;
}
