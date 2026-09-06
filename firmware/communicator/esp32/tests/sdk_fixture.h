/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_ESP_SDK_FIXTURE_H
#define CANVIEW_ESP_SDK_FIXTURE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
typedef int gpio_num_t;
typedef uint32_t TickType_t;
typedef uint32_t UBaseType_t;
typedef int BaseType_t;
typedef unsigned portMUX_TYPE;
typedef struct
{
    uint32_t timeout_ms;
    uint32_t idle_core_mask;
    bool trigger_panic;
} esp_task_wdt_config_t;
typedef struct
{
    char version[32];
    char idf_ver[32];
} esp_app_desc_t;
#define ESP_OK (0)
#define ESP_ERR_INVALID_STATE (1)
#define ESP_ERR_NOT_FOUND (2)
#define ESP_FAIL (3)
#define configNUMBER_OF_CORES (2U)
#define MALLOC_CAP_INTERNAL (1U)
#define MALLOC_CAP_8BIT (2U)
#define portMUX_INITIALIZER_UNLOCKED (0U)
#define pdTRUE (1)
#define pdMS_TO_TICKS(ms) ((ms) / 10U)
#define GPIO_IS_VALID_GPIO(pin) (mock_valid_pin(pin))
#define portENTER_CRITICAL(mux) mock_enter(mux)
#define portEXIT_CRITICAL(mux) mock_leave(mux)
#define ESP_LOGI(...) mock_log(__VA_ARGS__)
bool mock_valid_pin(uint8_t pin);
void mock_enter(portMUX_TYPE *mux);
void mock_leave(portMUX_TYPE *mux);
void mock_log(const char *tag, const char *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;
void *xTaskGetCurrentTaskHandle(void);
TickType_t xTaskGetTickCount(void);
BaseType_t xTaskDelayUntil(TickType_t *tick, TickType_t period);
UBaseType_t uxTaskGetStackHighWaterMark(void *task);
esp_err_t esp_task_wdt_status(void *task);
esp_err_t esp_task_wdt_init(const esp_task_wdt_config_t *config);
esp_err_t esp_task_wdt_reconfigure(const esp_task_wdt_config_t *config);
esp_err_t esp_task_wdt_add(void *task);
esp_err_t esp_task_wdt_reset(void);
int64_t esp_timer_get_time(void);
bool esp_psram_is_initialized(void);
size_t esp_psram_get_size(void);
esp_err_t esp_flash_get_size(void *chip, uint32_t *size);
size_t heap_caps_get_free_size(uint32_t caps);
size_t heap_caps_get_largest_free_block(uint32_t caps);
int esp_reset_reason(void);
int gpio_get_level(gpio_num_t pin);
const esp_app_desc_t *esp_app_get_description(void);
#endif
