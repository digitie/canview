/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_esp_runtime.h"
#include <inttypes.h>
#include <stddef.h>
#if defined(CANVIEW_ESP_SDK_TEST)
#include "sdk_fixture.h"
#else
#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

/* SDK adapter의 두 독립 lock. pool lock 아래서 watchdog/SDK callback을 호출하지 않는다. */
static portMUX_TYPE pool_mux = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE feed_mux = portMUX_INITIALIZER_UNLOCKED;

static bool owned(const canview_esp_runtime_t *runtime)
{
    return runtime != NULL && runtime->initialized && runtime->owner == xTaskGetCurrentTaskHandle();
}

static canview_status_t callback_enter(canview_esp_runtime_t *runtime)
{
    if (runtime == NULL || xPortInIsrContext() != pdFALSE || !owned(runtime) ||
        runtime->callback_active)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    runtime->callback_active = true;
    return CANVIEW_OK;
}

static void callback_leave(canview_esp_runtime_t *runtime)
{
    runtime->callback_active = false;
}

static canview_status_t pool_context_valid(void *context)
{
    const canview_esp_runtime_t *runtime = context;
    if (runtime == NULL || !runtime->initialized || xPortInIsrContext() != pdFALSE)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    return CANVIEW_OK;
}

static canview_status_t safe_gpio(void *context)
{
    canview_esp_runtime_t *runtime = context;
    const canview_status_t entry = callback_enter(runtime);
    if (entry != CANVIEW_OK)
    {
        return entry;
    }
    const canview_status_t status = runtime->config.safe_gpio(runtime->config.safe_context);
    callback_leave(runtime);
    return status;
}

static canview_status_t watchdog_start(void *context)
{
    canview_esp_runtime_t *runtime = context;
    const canview_status_t entry = callback_enter(runtime);
    if (entry != CANVIEW_OK)
    {
        return entry;
    }
    canview_status_t result = CANVIEW_OK;
    if (runtime->watchdog_ready)
    {
        result = CANVIEW_INVALID_ARGUMENT;
    }
    else
    {
        const esp_err_t status = esp_task_wdt_status(NULL);
        if (status != ESP_ERR_NOT_FOUND)
        {
            /* IDF startup가 global TWDT를 초기화한다. 다른 subscription이나 설정을 가로채지 않는다. */
            result = status == ESP_OK ? CANVIEW_RESOURCE_BUSY : CANVIEW_NOT_IMPLEMENTED;
        }
        else if (esp_task_wdt_add(NULL) != ESP_OK)
        {
            result = CANVIEW_NOT_IMPLEMENTED;
        }
        else
        {
            runtime->watchdog_ready = true;
        }
    }
    callback_leave(runtime);
    return result;
}

static canview_status_t now_us(void *context, uint64_t *now)
{
    canview_esp_runtime_t *runtime = context;
    if (now == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t entry = callback_enter(runtime);
    if (entry != CANVIEW_OK)
    {
        return entry;
    }
    const int64_t value = esp_timer_get_time();
    if (value < 0)
    {
        callback_leave(runtime);
        return CANVIEW_TIMEOUT;
    }
    *now = (uint64_t)value;
    callback_leave(runtime);
    return CANVIEW_OK;
}

static canview_status_t sample(void *context, canview_esp_core_sample_t *output)
{
    const canview_esp_runtime_t *runtime = context;
    if (output == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t entry = callback_enter((canview_esp_runtime_t *)runtime);
    if (entry != CANVIEW_OK)
    {
        return entry;
    }
    canview_esp_core_sample_t value = {0};
    if (!esp_psram_is_initialized() || esp_flash_get_size(NULL, &value.flash_bytes) != ESP_OK)
    {
        callback_leave((canview_esp_runtime_t *)runtime);
        return CANVIEW_NOT_IMPLEMENTED;
    }
    const size_t psram = esp_psram_get_size();
    const size_t heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const size_t block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#if SIZE_MAX > UINT32_MAX
    if (psram > UINT32_MAX || heap > UINT32_MAX || block > UINT32_MAX)
    {
        callback_leave((canview_esp_runtime_t *)runtime);
        return CANVIEW_OVERSIZE;
    }
#endif
    value.psram_bytes = (uint32_t)psram;
    value.heap_free_bytes = (uint32_t)heap;
    value.largest_block_bytes = (uint32_t)block;
    /* ESP-IDF6 task.h는 byte 단위다. upstream FreeRTOS의 word 변환을 적용하지 않는다. */
    value.stack_free_bytes = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    value.reset_reason = (uint32_t)esp_reset_reason();
    for (uint8_t index = 0U; index < runtime->config.input_count; ++index)
    {
        const uint8_t bit = (uint8_t)(1U << index);
        value.input_valid_mask |= bit;
        if (gpio_get_level((gpio_num_t)runtime->config.input_pins[index]) != 0)
        {
            value.input_level_mask |= bit;
        }
    }
    *output = value;
    callback_leave((canview_esp_runtime_t *)runtime);
    return CANVIEW_OK;
}

static canview_status_t feed(void *context, uint64_t not_before, uint64_t deadline,
                             uint64_t *fed_at)
{
    canview_esp_runtime_t *runtime = context;
    if (fed_at == NULL || not_before > deadline)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t entry = callback_enter(runtime);
    if (entry != CANVIEW_OK)
    {
        return entry;
    }
    if (!runtime->watchdog_ready)
    {
        callback_leave(runtime);
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_status_t status = CANVIEW_TIMEOUT;
    /* Timer 검사와 SDK reset 사이의 task 선점을 막는다. SDK 내부 lock은 별개다.
     * ISR/NMI·cache 정지와 SDK lock 대기의 실제 상한은 T-200/T-400 HIL로 검증해야 한다. */
    portENTER_CRITICAL(&feed_mux);
    const int64_t before = esp_timer_get_time();
    if (before >= 0 && (uint64_t)before >= not_before && (uint64_t)before <= deadline)
    {
        if (esp_task_wdt_reset() == ESP_OK)
        {
            const int64_t after = esp_timer_get_time();
            if (after >= 0 && (uint64_t)after >= not_before && (uint64_t)after <= deadline &&
                after >= before)
            {
                *fed_at = (uint64_t)after;
                status = CANVIEW_OK;
            }
        }
        else
        {
            status = CANVIEW_NOT_IMPLEMENTED;
        }
    }
    portEXIT_CRITICAL(&feed_mux);
    callback_leave(runtime);
    return status;
}

static void pool_enter(void *context)
{
    (void)context;
    portENTER_CRITICAL(&pool_mux);
}

static void pool_leave(void *context)
{
    (void)context;
    portEXIT_CRITICAL(&pool_mux);
}

static canview_status_t wait_period(void *context)
{
    canview_esp_runtime_t *runtime = context;
    const canview_status_t entry = callback_enter(runtime);
    if (entry != CANVIEW_OK)
    {
        return entry;
    }
    TickType_t tick = (TickType_t)runtime->wake_tick;
    if (!runtime->wait_started)
    {
        tick = xTaskGetTickCount();
        runtime->wait_started = true;
    }
    /* 늦게 호출하면 즉시 반환한다. health의 절대 deadline이 catch-up feed를 차단한다. */
    const BaseType_t delayed = xTaskDelayUntil(&tick, pdMS_TO_TICKS(CANVIEW_ESP_CORE_PERIOD_MS));
    runtime->wake_tick = (uint32_t)tick;
    callback_leave(runtime);
    return delayed == pdTRUE ? CANVIEW_OK : CANVIEW_TIMEOUT;
}

static void report(void *context, const canview_esp_core_t *core, canview_status_t service_status)
{
    canview_esp_runtime_t *runtime = context;
    if (core == NULL || callback_enter(runtime) != CANVIEW_OK)
    {
        return;
    }
    const canview_esp_core_sample_t sample = core->sample_valid ? core->sample
                                                                 : (canview_esp_core_sample_t){0};
    const esp_app_desc_t *description = esp_app_get_description();
    ESP_LOGI("core",
             "bench-only project=%s build=%s idf=%s cap=0 tx=0 state=%u fault=%u status=%u "
             "sample-valid=%u reset=%" PRIu32,
             description->project_name, description->version, description->idf_ver,
             (unsigned)core->state, (unsigned)core->fault, (unsigned)service_status,
             core->sample_valid ? 1U : 0U, sample.reset_reason);
    ESP_LOGI("core",
             "flash=%" PRIu32 " psram=%" PRIu32 " internal=%" PRIu32 " block=%" PRIu32
             " stack-free=%" PRIu32 " input-valid=%u input-level=%u",
             sample.flash_bytes, sample.psram_bytes, sample.heap_free_bytes,
             sample.largest_block_bytes, sample.stack_free_bytes,
             (unsigned)sample.input_valid_mask, (unsigned)sample.input_level_mask);
    callback_leave(runtime);
}

canview_status_t canview_esp_runtime_open(canview_esp_runtime_t *runtime,
                                          const canview_esp_runtime_config_t *config,
                                          canview_esp_runtime_port_t *port)
{
    if (runtime == NULL || config == NULL || port == NULL || config->safe_gpio == NULL ||
        config->memory.flash_bytes == 0U || config->memory.psram_bytes == 0U ||
        config->input_count == 0U || config->input_count > CANVIEW_ESP_RUNTIME_INPUTS ||
        config->board_profile == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (xPortInIsrContext() != pdFALSE)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    for (uint8_t index = 0U; index < CANVIEW_ESP_RUNTIME_INPUTS; ++index)
    {
        if (index < config->input_count ? !GPIO_IS_VALID_GPIO(config->input_pins[index])
                                        : config->input_pins[index] != 0U)
        {
            return CANVIEW_INVALID_ARGUMENT;
        }
    }
    if (config->input_count == CANVIEW_ESP_RUNTIME_INPUTS &&
        config->input_pins[0] == config->input_pins[1])
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (runtime->initialized)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    void *owner = xTaskGetCurrentTaskHandle();
    if (owner == NULL || pdMS_TO_TICKS(CANVIEW_ESP_CORE_PERIOD_MS) == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    runtime->config = *config;
    runtime->owner = owner;
    runtime->wake_tick = (uint32_t)xTaskGetTickCount();
    runtime->initialized = true;
    *port = (canview_esp_runtime_port_t){{safe_gpio, watchdog_start, now_us, sample, feed, runtime,
                                           config->memory},
                                         {pool_enter, pool_leave, pool_context_valid, runtime},
                                         wait_period,
                                         report,
                                         runtime};
    return CANVIEW_OK;
}
