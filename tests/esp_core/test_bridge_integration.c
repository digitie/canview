/* SPDX-License-Identifier: GPL-3.0-only */
/* 실제 Diagnostic Bridge app/BSP/runtime composition을 SDK fixture로 실행한다. */
#include "board_fixture.h"
#include "board_pins.h"
#include "canview_esp_runtime.h"
#include "canview_gpio.h"
#include "sdk_fixture.h"
#include <setjmp.h>
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
    bool output;
    uint8_t pin;
    bool high;
    bool open_drain;
} gpio_call_t;

typedef struct
{
    void *owner;
    int64_t time;
    uint32_t tick;
    size_t psram;
    size_t heap;
    size_t block;
    unsigned waits;
    unsigned reports;
    unsigned logs;
    unsigned wdt_adds;
    unsigned wdt_resets;
    unsigned idle_calls;
    unsigned critical_depth;
    unsigned critical_enters;
    unsigned critical_leaves;
    unsigned gpio_calls;
    gpio_call_t gpio[4];
    char log_text[4][512];
    jmp_buf stopped;
} fake_t;

static fake_t fake;
static int owner_token;

void app_main(void);

bool mock_valid_pin(uint8_t pin)
{
    return pin < 22U || (pin >= 26U && pin <= 48U);
}

void mock_enter(portMUX_TYPE *mux)
{
    CHECK(mux != NULL && *mux == 0U);
    *mux = 1U;
    ++fake.critical_depth;
    ++fake.critical_enters;
}

void mock_leave(portMUX_TYPE *mux)
{
    CHECK(mux != NULL && *mux == 1U && fake.critical_depth == 1U);
    *mux = 0U;
    --fake.critical_depth;
    ++fake.critical_leaves;
}

void mock_log(const char *tag, const char *format, ...)
{
    CHECK(strcmp(tag, "core") == 0 && fake.logs < 4U);
    va_list args;
    va_start(args, format);
    const int length = vsnprintf(fake.log_text[fake.logs], sizeof(fake.log_text[0]), format, args);
    va_end(args);
    CHECK(length > 0 && (size_t)length < sizeof(fake.log_text[0]));
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
    CHECK(tick != NULL && period == 10U && *tick == fake.tick);
    ++fake.waits;
    *tick += period;
    fake.tick = *tick;
    if (fake.waits <= 2U)
    {
        fake.time += 100000U;
        return pdTRUE;
    }
    return 0;
}

UBaseType_t uxTaskGetStackHighWaterMark(void *task)
{
    CHECK(task == NULL);
    return CANVIEW_ESP_CORE_STACK_MIN;
}

esp_err_t esp_task_wdt_status(void *task)
{
    CHECK(task == NULL);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t esp_task_wdt_add(void *task)
{
    CHECK(task == NULL);
    ++fake.wdt_adds;
    return ESP_OK;
}

esp_err_t esp_task_wdt_reset(void)
{
    CHECK(fake.critical_depth == 1U);
    ++fake.wdt_resets;
    return ESP_OK;
}

int64_t esp_timer_get_time(void)
{
    return fake.time;
}

bool esp_psram_is_initialized(void)
{
    return true;
}

size_t esp_psram_get_size(void)
{
    return fake.psram;
}

esp_err_t esp_flash_get_size(void *chip, uint32_t *size)
{
    CHECK(chip == NULL && size != NULL);
    *size = CANVIEW_BOARD_FLASH_BYTES;
    return ESP_OK;
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
    CHECK(pin == (gpio_num_t)TEST_INPUT_PIN0);
    return 1;
}

const esp_app_desc_t *esp_app_get_description(void)
{
    static const esp_app_desc_t description = {"fixture", "6.0.3", TEST_PROJECT_NAME};
    return &description;
}

canview_status_t canview_gpio_output(uint8_t pin, bool high, bool open_drain)
{
    CHECK(fake.critical_depth == 0U && fake.gpio_calls < 4U);
    fake.gpio[fake.gpio_calls] = (gpio_call_t){true, pin, high, open_drain};
    ++fake.gpio_calls;
    return CANVIEW_OK;
}

canview_status_t canview_gpio_input(uint8_t pin)
{
    CHECK(fake.critical_depth == 0U && fake.gpio_calls < 4U);
    fake.gpio[fake.gpio_calls] = (gpio_call_t){false, pin, false, false};
    ++fake.gpio_calls;
    return CANVIEW_OK;
}

void canview_platform_idle(void *context)
{
    CHECK(context == NULL);
    ++fake.idle_calls;
    longjmp(fake.stopped, 1);
}

int main(void)
{
    (void)memset(&fake, 0, sizeof(fake));
    fake.owner = &owner_token;
    fake.time = 1000;
    fake.tick = 100U;
    fake.psram = TEST_PSRAM_BYTES;
    fake.heap = CANVIEW_ESP_CORE_HEAP_MIN;
    fake.block = CANVIEW_ESP_CORE_BLOCK_MIN;

    canview_esp_runtime_t runtime = {0};
    canview_esp_runtime_port_t port = {0};
    CHECK(canview_esp_board_runtime(&runtime, &port) == CANVIEW_OK);
    CHECK(runtime.config.input_count == TEST_INPUT_COUNT &&
          runtime.config.input_pins[0] == TEST_INPUT_PIN0 && runtime.config.input_pins[1] == 0U);
    CHECK(runtime.config.memory.flash_bytes == TEST_FLASH_BYTES &&
          runtime.config.memory.psram_bytes == TEST_PSRAM_BYTES);
    CHECK(port.core.safe_gpio != NULL && port.core.watchdog_start != NULL && port.wait != NULL &&
          port.report != NULL);

    if (setjmp(fake.stopped) == 0)
    {
        app_main();
        CHECK(false);
    }
    CHECK(fake.idle_calls == 1U && fake.wdt_adds == 1U && fake.wdt_resets == 2U);
    CHECK(fake.waits == 3U && fake.logs == 4U);
    CHECK(fake.critical_enters == 2U && fake.critical_leaves == 2U && fake.critical_depth == 0U);
    CHECK(fake.gpio_calls == 4U);
    for (unsigned index = 0U; index < fake.gpio_calls; index += 2U)
    {
        CHECK(fake.gpio[index].output && fake.gpio[index].pin == 5U && !fake.gpio[index].high &&
              !fake.gpio[index].open_drain);
        CHECK(!fake.gpio[index + 1U].output && fake.gpio[index + 1U].pin == TEST_INPUT_PIN0);
    }
    CHECK(strstr(fake.log_text[0], "bench-only project=" TEST_PROJECT_NAME
                " build=fixture idf=6.0.3 cap=0 tx=0") != NULL);
    CHECK(strstr(fake.log_text[0], "sample-valid=1") != NULL);
    CHECK(strstr(fake.log_text[1], "flash=8388608 psram=2097152 internal=81920 block=32768 "
                "stack-free=1024 input-valid=1 input-level=1") != NULL);
    CHECK(strstr(fake.log_text[2], "sample-valid=1") != NULL);
    CHECK(strstr(fake.log_text[3], "input-valid=1 input-level=1") != NULL);
    (void)puts("PASS: actual Diagnostic Bridge app+BSP+SDK adapter composition");
    return 0;
}
