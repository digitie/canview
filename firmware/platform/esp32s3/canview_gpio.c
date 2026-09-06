/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_gpio.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

canview_status_t canview_gpio_output(uint8_t pin, bool high, bool open_drain)
{
    if (!GPIO_IS_VALID_OUTPUT_GPIO(pin))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    /* external default-safe pulls remain necessary before this code executes. */
    if (gpio_set_level((gpio_num_t)pin, high ? 1U : 0U) != ESP_OK)
    {
        return CANVIEW_NOT_IMPLEMENTED;
    }
    const gpio_config_t config = {.pin_bit_mask = UINT64_C(1) << pin,
                                  .mode = open_drain ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT,
                                  .pull_up_en = GPIO_PULLUP_DISABLE,
                                  .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                  .intr_type = GPIO_INTR_DISABLE};
    return gpio_config(&config) == ESP_OK ? CANVIEW_OK : CANVIEW_NOT_IMPLEMENTED;
}

canview_status_t canview_gpio_input(uint8_t pin)
{
    if (!GPIO_IS_VALID_GPIO(pin))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const gpio_config_t config = {.pin_bit_mask = UINT64_C(1) << pin,
                                  .mode = GPIO_MODE_INPUT,
                                  .pull_up_en = GPIO_PULLUP_DISABLE,
                                  .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                  .intr_type = GPIO_INTR_DISABLE};
    return gpio_config(&config) == ESP_OK ? CANVIEW_OK : CANVIEW_NOT_IMPLEMENTED;
}

void canview_platform_idle(void *context)
{
    (void)context;
    vTaskDelay(pdMS_TO_TICKS(10U) + 1U);
}
