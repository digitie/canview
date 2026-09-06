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

static canview_status_t safe_gpio(void *context)
{
    canview_esp_runtime_t *runtime = context;
    if (!owned(runtime))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    return runtime->config.safe_gpio(runtime->config.safe_context);
}

static canview_status_t watchdog_start(void *context)
{
    canview_esp_runtime_t *runtime = context;
    if (!owned(runtime) || runtime->watchdog_ready)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const esp_err_t status = esp_task_wdt_status(NULL);
    const esp_task_wdt_config_t config = {.timeout_ms = CANVIEW_ESP_CORE_WATCHDOG_MS,
                                          .idle_core_mask =
                                              (UINT32_C(1) << configNUMBER_OF_CORES) - 1U,
                                          .trigger_panic = true};
    esp_err_t result;
    if (status == ESP_ERR_INVALID_STATE)
    {
        result = esp_task_wdt_init(&config);
    }
    else if (status == ESP_ERR_NOT_FOUND)
    {
        result = esp_task_wdt_reconfigure(&config);
    }
    else
    {
        /* 이미 등록된 task의 subscription을 가로채지 않는다. */
        return CANVIEW_RESOURCE_BUSY;
    }
    if (result != ESP_OK || esp_task_wdt_add(NULL) != ESP_OK)
    {
        return CANVIEW_NOT_IMPLEMENTED;
    }
    runtime->watchdog_ready = true;
    return CANVIEW_OK;
}

static canview_status_t now_us(void *context, uint64_t *now)
{
    if (!owned(context) || now == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const int64_t value = esp_timer_get_time();
    if (value < 0)
    {
        return CANVIEW_TIMEOUT;
    }
    *now = (uint64_t)value;
    return CANVIEW_OK;
}

static canview_status_t sample(void *context, canview_esp_core_sample_t *output)
{
    const canview_esp_runtime_t *runtime = context;
    if (!owned(runtime) || output == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_esp_core_sample_t value = {0};
    if (!esp_psram_is_initialized() || esp_flash_get_size(NULL, &value.flash_bytes) != ESP_OK)
    {
        return CANVIEW_NOT_IMPLEMENTED;
    }
    const size_t psram = esp_psram_get_size();
    const size_t heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const size_t block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#if SIZE_MAX > UINT32_MAX
    if (psram > UINT32_MAX || heap > UINT32_MAX || block > UINT32_MAX)
    {
        return CANVIEW_OVERSIZE;
    }
#endif
    value.psram_bytes = (uint32_t)psram;
    value.heap_free_bytes = (uint32_t)heap;
    value.largest_block_bytes = (uint32_t)block;
    /* ESP-IDF6 task.h는 byte 단위다. upstream FreeRTOS의 word 변환을 적용하지 않는다. */
    value.stack_free_bytes = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    value.reset_reason = (uint32_t)esp_reset_reason();
    value.service_run_sense = gpio_get_level((gpio_num_t)runtime->config.service_run_pin) != 0;
    value.usb_service_sense = gpio_get_level((gpio_num_t)runtime->config.usb_service_pin) != 0;
    *output = value;
    return CANVIEW_OK;
}

static canview_status_t feed(void *context, uint64_t not_before, uint64_t deadline,
                             uint64_t *fed_at)
{
    const canview_esp_runtime_t *runtime = context;
    if (!owned(runtime) || !runtime->watchdog_ready || fed_at == NULL || not_before > deadline)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_status_t status = CANVIEW_TIMEOUT;
    /* Timer 검사와 SDK reset 사이의 task 선점을 막는다. SDK 내부 lock은 별개다.
     * ISR/NMI·cache 정지와 SDK lock 대기의 실제 상한은 T-200 HIL로 검증해야 한다. */
    portENTER_CRITICAL(&feed_mux);
    const int64_t value = esp_timer_get_time();
    if (value >= 0 && (uint64_t)value >= not_before && (uint64_t)value <= deadline)
    {
        if (esp_task_wdt_reset() == ESP_OK)
        {
            *fed_at = (uint64_t)value;
            status = CANVIEW_OK;
        }
        else
        {
            status = CANVIEW_NOT_IMPLEMENTED;
        }
    }
    portEXIT_CRITICAL(&feed_mux);
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
    if (!owned(runtime))
    {
        return CANVIEW_INVALID_ARGUMENT;
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
    return delayed == pdTRUE ? CANVIEW_OK : CANVIEW_TIMEOUT;
}

static void report(void *context, const canview_esp_core_t *core, canview_status_t service_status)
{
    if (!owned(context) || core == NULL)
    {
        return;
    }
    const esp_app_desc_t *description = esp_app_get_description();
    ESP_LOGI("core",
             "bench-only build=%s idf=%s cap=0 tx=0 state=%u fault=%u status=%u reset=%" PRIu32,
             description->version, description->idf_ver, (unsigned)core->state,
             (unsigned)core->fault, (unsigned)service_status, core->sample.reset_reason);
    ESP_LOGI("core",
             "flash=%" PRIu32 " psram=%" PRIu32 " internal=%" PRIu32 " block=%" PRIu32
             " stack-free=%" PRIu32 " run-sense=%u usb-sense=%u",
             core->sample.flash_bytes, core->sample.psram_bytes, core->sample.heap_free_bytes,
             core->sample.largest_block_bytes, core->sample.stack_free_bytes,
             (unsigned)core->sample.service_run_sense, (unsigned)core->sample.usb_service_sense);
}

canview_status_t canview_esp_runtime_open(canview_esp_runtime_t *runtime,
                                          const canview_esp_runtime_config_t *config,
                                          canview_esp_runtime_port_t *port)
{
    if (runtime == NULL || config == NULL || port == NULL || config->safe_gpio == NULL ||
        !GPIO_IS_VALID_GPIO(config->service_run_pin) ||
        !GPIO_IS_VALID_GPIO(config->usb_service_pin) ||
        config->service_run_pin == config->usb_service_pin)
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
    *port = (canview_esp_runtime_port_t){{safe_gpio, watchdog_start, now_us, sample, feed, runtime},
                                         {pool_enter, pool_leave, NULL},
                                         wait_period,
                                         report,
                                         runtime};
    return CANVIEW_OK;
}
