/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_esp_runtime.h"
#include "canview_board.h"
#include "sdk_fixture.h"
#include <stdarg.h>
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
    void *owner;
    int64_t time;
    int64_t critical_delay;
    size_t psram;
    size_t heap;
    size_t block;
    uint32_t flash;
    uint32_t stack;
    uint32_t tick;
    esp_err_t status;
    esp_err_t init;
    esp_err_t reconfigure;
    esp_err_t add;
    esp_err_t reset;
    esp_err_t flash_status;
    unsigned init_calls;
    unsigned reconfigure_calls;
    unsigned add_calls;
    unsigned reset_calls;
    unsigned safe_calls;
    unsigned depth;
    unsigned enters;
    unsigned leaves;
    unsigned logs;
    bool memory_ready;
    bool delayed;
    bool run_sense;
    bool usb_sense;
    canview_status_t safe_status;
} fake_t;
static fake_t fake;
static int owner_token;
static int other_owner;
static void reset_fake(void)
{
    memset(&fake, 0, sizeof(fake));
    fake.owner = &owner_token;
    fake.time = 1000;
    fake.tick = 100U;
    fake.psram = CANVIEW_ESP_CORE_PSRAM_BYTES;
    fake.flash = CANVIEW_ESP_CORE_FLASH_BYTES;
    fake.heap = CANVIEW_ESP_CORE_HEAP_MIN;
    fake.block = CANVIEW_ESP_CORE_BLOCK_MIN;
    fake.stack = CANVIEW_ESP_CORE_STACK_MIN;
    fake.status = ESP_ERR_NOT_FOUND;
    fake.memory_ready = true;
    fake.delayed = true;
}
bool mock_valid_pin(uint8_t pin)
{
    return pin < 22U || (pin >= 26U && pin <= 48U);
}
void mock_enter(portMUX_TYPE *mux)
{
    CHECK(mux != NULL && *mux == 0U);
    ++*mux;
    ++fake.depth;
    ++fake.enters;
    fake.time += fake.critical_delay;
}
void mock_leave(portMUX_TYPE *mux)
{
    CHECK(mux != NULL && *mux == 1U && fake.depth == 1U);
    --*mux;
    --fake.depth;
    ++fake.leaves;
}
void mock_log(const char *tag, const char *format, ...)
{
    CHECK(strcmp(tag, "core") == 0);
    char output[512];
    va_list args;
    va_start(args, format);
    const int length = vsnprintf(output, sizeof(output), format, args);
    va_end(args);
    CHECK(length > 0 && (size_t)length < sizeof(output));
    if ((fake.logs % 2U) == 0U)
    {
        CHECK(strstr(output, "bench-only build=fixture idf=6.0.3 cap=0 tx=0") != NULL);
    }
    ++fake.logs;
}
void *xTaskGetCurrentTaskHandle(void)
{
    return fake.owner;
}
TickType_t xTaskGetTickCount(void)
{
    return fake.tick;
}
BaseType_t xTaskDelayUntil(TickType_t *tick, TickType_t period)
{
    CHECK(period == 10U && *tick == fake.tick);
    *tick += period;
    fake.tick = *tick;
    return fake.delayed ? pdTRUE : 0;
}
UBaseType_t uxTaskGetStackHighWaterMark(void *task)
{
    CHECK(task == NULL);
    return fake.stack;
}
esp_err_t esp_task_wdt_status(void *task)
{
    CHECK(task == NULL);
    return fake.status;
}
static void check_wdt(const esp_task_wdt_config_t *config)
{
    CHECK(config->timeout_ms == 2000U && config->idle_core_mask == 3U && config->trigger_panic);
}
esp_err_t esp_task_wdt_init(const esp_task_wdt_config_t *config)
{
    check_wdt(config);
    ++fake.init_calls;
    return fake.init;
}
esp_err_t esp_task_wdt_reconfigure(const esp_task_wdt_config_t *config)
{
    check_wdt(config);
    ++fake.reconfigure_calls;
    return fake.reconfigure;
}
esp_err_t esp_task_wdt_add(void *task)
{
    CHECK(task == NULL);
    ++fake.add_calls;
    return fake.add;
}
esp_err_t esp_task_wdt_reset(void)
{
    CHECK(fake.depth == 1U);
    ++fake.reset_calls;
    return fake.reset;
}
int64_t esp_timer_get_time(void)
{
    return fake.time;
}
bool esp_psram_is_initialized(void)
{
    return fake.memory_ready;
}
size_t esp_psram_get_size(void)
{
    return fake.psram;
}
esp_err_t esp_flash_get_size(void *chip, uint32_t *size)
{
    CHECK(chip == NULL && size != NULL);
    *size = fake.flash;
    return fake.flash_status;
}
size_t heap_caps_get_free_size(uint32_t caps)
{
    CHECK(caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    return fake.heap;
}
size_t heap_caps_get_largest_free_block(uint32_t caps)
{
    CHECK(caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    return fake.block;
}
int esp_reset_reason(void)
{
    return 7;
}
int gpio_get_level(gpio_num_t pin)
{
    CHECK(pin == 48 || pin == 38);
    return pin == 48 ? fake.run_sense : fake.usb_sense;
}
const esp_app_desc_t *esp_app_get_description(void)
{
    static const esp_app_desc_t description = {"fixture", "6.0.3"};
    return &description;
}
static canview_status_t safe(void *context)
{
    CHECK(context == &fake);
    ++fake.safe_calls;
    return fake.safe_status;
}
static void idle(void *context)
{
    CHECK(context == &fake);
}
canview_platform_port_t canview_board_port(void)
{
    const canview_platform_port_t port = {safe, idle, &fake};
    return port;
}
static canview_esp_runtime_port_t open_runtime(canview_esp_runtime_t *runtime)
{
    canview_esp_runtime_port_t port = {0};
    CHECK(canview_esp_board_runtime(runtime, &port) == CANVIEW_OK);
    CHECK(runtime->config.service_run_pin == 48U && runtime->config.usb_service_pin == 38U);
    return port;
}
static void argument_tests(void)
{
    reset_fake();
    canview_esp_runtime_t runtime = {0};
    canview_esp_runtime_config_t config = {safe, &fake, 48U, 38U};
    canview_esp_runtime_port_t port = {0};
    CHECK(canview_esp_runtime_open(NULL, &config, &port) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_runtime_open(&runtime, NULL, &port) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_esp_runtime_open(&runtime, &config, NULL) == CANVIEW_INVALID_ARGUMENT);
    config.safe_gpio = NULL;
    CHECK(canview_esp_runtime_open(&runtime, &config, &port) == CANVIEW_INVALID_ARGUMENT);
    config.safe_gpio = safe;
    config.service_run_pin = 23U;
    CHECK(canview_esp_runtime_open(&runtime, &config, &port) == CANVIEW_INVALID_ARGUMENT);
    config.service_run_pin = 48U;
    config.usb_service_pin = 49U;
    CHECK(canview_esp_runtime_open(&runtime, &config, &port) == CANVIEW_INVALID_ARGUMENT);
    config.usb_service_pin = 48U;
    CHECK(canview_esp_runtime_open(&runtime, &config, &port) == CANVIEW_INVALID_ARGUMENT);
    config.usb_service_pin = 38U;
    fake.owner = NULL;
    CHECK(canview_esp_runtime_open(&runtime, &config, &port) == CANVIEW_INVALID_ARGUMENT);
    fake.owner = &owner_token;
    port = open_runtime(&runtime);
    CHECK(canview_esp_board_runtime(&runtime, &port) == CANVIEW_RESOURCE_BUSY);
    uint64_t time = 99U;
    canview_esp_core_sample_t sample_value = {0};
    CHECK(port.core.now_us(&runtime, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(port.core.sample(&runtime, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(port.core.feed(&runtime, 0U, 1U, &time) == CANVIEW_INVALID_ARGUMENT);
    for (unsigned fault = 0U; fault < 3U; ++fault)
    {
        void *context = fault == 0U ? NULL : &runtime;
        if (fault == 1U)
        {
            fake.owner = &other_owner;
        }
        if (fault == 2U)
        {
            fake.owner = &owner_token;
            runtime.initialized = false;
        }
        CHECK(port.core.safe_gpio(context) == CANVIEW_INVALID_ARGUMENT);
        CHECK(port.core.watchdog_start(context) == CANVIEW_INVALID_ARGUMENT);
        CHECK(port.core.now_us(context, &time) == CANVIEW_INVALID_ARGUMENT);
        CHECK(port.core.sample(context, &sample_value) == CANVIEW_INVALID_ARGUMENT);
        CHECK(port.core.feed(context, 0U, 1U, &time) == CANVIEW_INVALID_ARGUMENT);
        CHECK(port.wait(context) == CANVIEW_INVALID_ARGUMENT);
        port.report(context, NULL, CANVIEW_INVALID_ARGUMENT);
    }
    CHECK(fake.safe_calls == 0U && fake.add_calls == 0U && fake.reset_calls == 0U);
}
static void watchdog_tests(void)
{
    for (unsigned fault = 0U; fault < 7U; ++fault)
    {
        reset_fake();
        canview_esp_runtime_t runtime = {0};
        const canview_esp_runtime_port_t port = open_runtime(&runtime);
        if (fault == 0U || fault == 1U)
        {
            fake.status = ESP_ERR_INVALID_STATE;
        }
        if (fault == 1U)
        {
            fake.init = ESP_FAIL;
        }
        if (fault == 2U)
        {
            fake.reconfigure = ESP_FAIL;
        }
        if (fault == 3U)
        {
            fake.add = ESP_FAIL;
        }
        if (fault == 4U)
        {
            fake.status = ESP_OK;
        }
        if (fault == 5U)
        {
            fake.status = ESP_FAIL;
        }
        const canview_status_t status = port.core.watchdog_start(&runtime);
        CHECK((status == CANVIEW_OK) == (fault == 0U || fault == 6U));
        CHECK(runtime.watchdog_ready == (status == CANVIEW_OK));
        CHECK(fake.add_calls == ((fault == 0U || fault == 3U || fault == 6U) ? 1U : 0U));
        if (status == CANVIEW_OK)
        {
            CHECK(port.core.watchdog_start(&runtime) == CANVIEW_INVALID_ARGUMENT);
        }
        CHECK(fake.init_calls == (fault < 2U ? 1U : 0U));
        CHECK(fake.reconfigure_calls == ((fault == 2U || fault == 3U || fault == 6U) ? 1U : 0U));
    }
}
static void memory_time_tests(void)
{
    reset_fake();
    canview_esp_runtime_t runtime = {0};
    const canview_esp_runtime_port_t port = open_runtime(&runtime);
    canview_esp_core_sample_t value = {0};
    CHECK(port.core.safe_gpio(&runtime) == CANVIEW_OK && fake.safe_calls == 1U);
    fake.safe_status = CANVIEW_NOT_IMPLEMENTED;
    CHECK(port.core.safe_gpio(&runtime) == CANVIEW_NOT_IMPLEMENTED);
    for (unsigned mask = 0U; mask < 4U; ++mask)
    {
        fake.run_sense = (mask & 1U) != 0U;
        fake.usb_sense = (mask & 2U) != 0U;
        CHECK(port.core.sample(&runtime, &value) == CANVIEW_OK);
        CHECK(value.service_run_sense == fake.run_sense &&
              value.usb_service_sense == fake.usb_sense);
        CHECK(value.flash_bytes == 16777216U && value.psram_bytes == 7864320U);
        CHECK(value.heap_free_bytes == 81920U && value.largest_block_bytes == 32768U);
        CHECK(value.stack_free_bytes == 1024U && value.reset_reason == 7U);
    }
    const canview_esp_core_sample_t saved = value;
    fake.memory_ready = false;
    CHECK(port.core.sample(&runtime, &value) == CANVIEW_NOT_IMPLEMENTED);
    fake.memory_ready = true;
    fake.flash_status = ESP_FAIL;
    CHECK(port.core.sample(&runtime, &value) == CANVIEW_NOT_IMPLEMENTED);
    fake.flash_status = ESP_OK;
#if SIZE_MAX > UINT32_MAX
    for (unsigned index = 0U; index < 3U; ++index)
    {
        size_t *item = index == 0U ? &fake.psram : (index == 1U ? &fake.heap : &fake.block);
        const size_t old = *item;
        *item = (size_t)UINT32_MAX + 1U;
        CHECK(port.core.sample(&runtime, &value) == CANVIEW_OVERSIZE);
        *item = old;
    }
#endif
    CHECK(memcmp(&saved, &value, sizeof(value)) == 0);
    uint64_t time = 99U;
    fake.time = -1;
    CHECK(port.core.now_us(&runtime, &time) == CANVIEW_TIMEOUT && time == 99U);
    fake.time = 0;
    CHECK(port.core.now_us(&runtime, &time) == CANVIEW_OK && time == 0U);
    fake.time = INT64_MAX;
    CHECK(port.core.now_us(&runtime, &time) == CANVIEW_OK && time == (uint64_t)INT64_MAX);
    CHECK(port.wait(&runtime) == CANVIEW_OK);
    fake.delayed = false;
    CHECK(port.wait(&runtime) == CANVIEW_TIMEOUT);
    CHECK(runtime.wake_tick == 120U);
    runtime.wake_tick = UINT32_MAX - 5U;
    fake.tick = runtime.wake_tick;
    fake.delayed = true;
    CHECK(port.wait(&runtime) == CANVIEW_OK && runtime.wake_tick == 4U);
    canview_esp_core_t core = {0};
    core.sample = saved;
    port.report(&runtime, &core, CANVIEW_OK);
    port.report(&runtime, NULL, CANVIEW_OK);
    CHECK(fake.logs == 2U);
    port.pool.enter(port.pool.context);
    port.pool.leave(port.pool.context);
    CHECK(fake.depth == 0U && fake.enters == fake.leaves);
}
static void feed_tests(void)
{
    for (unsigned fault = 0U; fault < 8U; ++fault)
    {
        reset_fake();
        canview_esp_runtime_t runtime = {0};
        const canview_esp_runtime_port_t port = open_runtime(&runtime);
        CHECK(port.core.watchdog_start(&runtime) == CANVIEW_OK);
        uint64_t time = 99U;
        CHECK(port.core.feed(&runtime, 0U, 2000U, NULL) == CANVIEW_INVALID_ARGUMENT);
        CHECK(port.core.feed(&runtime, 2U, 1U, &time) == CANVIEW_INVALID_ARGUMENT);
        fake.time = 1000;
        if (fault == 1U)
        {
            fake.time = -1;
        }
        if (fault == 2U)
        {
            fake.time = 999;
        }
        if (fault == 3U)
        {
            fake.time = 2001;
        }
        if (fault == 4U)
        {
            fake.critical_delay = 1001;
        }
        if (fault == 5U)
        {
            fake.reset = ESP_FAIL;
        }
        if (fault == 6U)
        {
            fake.time = 2000;
        }
        if (fault == 7U)
        {
            fake.time = 1500;
        }
        const canview_status_t status = port.core.feed(&runtime, 1000U, 2000U, &time);
        const bool valid = fault == 0U || fault == 6U || fault == 7U;
        CHECK((status == CANVIEW_OK) == valid);
        CHECK(fake.reset_calls == ((valid || fault == 5U) ? 1U : 0U));
        CHECK(valid ? time == (uint64_t)fake.time : time == 99U);
        CHECK(fake.depth == 0U && fake.enters == fake.leaves);
    }
    reset_fake();
    canview_esp_runtime_t runtime = {0};
    const canview_esp_runtime_port_t port = open_runtime(&runtime);
    canview_esp_core_t core = {0};
    CHECK(canview_esp_core_boot(&core, &port.core) == CANVIEW_OK);
    fake.time += 100000;
    CHECK(port.wait(&runtime) == CANVIEW_OK);
    CHECK(canview_esp_core_step(&core) == CANVIEW_OK && fake.reset_calls == 1U);
    fake.time += 250001;
    CHECK(canview_esp_core_step(&core) == CANVIEW_TIMEOUT && fake.reset_calls == 1U);
    CHECK(core.state == CANVIEW_ESP_CORE_FAULT);
}
int main(void)
{
    argument_tests();
    watchdog_tests();
    memory_time_tests();
    feed_tests();
    (void)puts("PASS: actual IDF adapter with SDK fixture; not physical WDT/PSRAM/HIL");
    return 0;
}
