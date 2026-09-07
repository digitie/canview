/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_board.h"
#include "canview_esp_runtime.h"
#include "canview_gpio.h"
#include <stdio.h>

static unsigned runtime_open_calls;

canview_status_t canview_esp_runtime_open(canview_esp_runtime_t *runtime,
                                           const canview_esp_runtime_config_t *config,
                                           canview_esp_runtime_port_t *port)
{
    (void)runtime;
    (void)config;
    (void)port;
    ++runtime_open_calls;
    return CANVIEW_OK;
}

canview_status_t canview_gpio_output(uint8_t pin, bool high, bool open_drain)
{
    (void)pin;
    (void)high;
    (void)open_drain;
    return CANVIEW_OK;
}

canview_status_t canview_gpio_input(uint8_t pin)
{
    (void)pin;
    return CANVIEW_OK;
}

void canview_platform_idle(void *context)
{
    (void)context;
}

int main(void)
{
    canview_esp_runtime_t runtime = {0};
    canview_esp_runtime_port_t port = {0};
    if (canview_esp_board_runtime(&runtime, &port) != CANVIEW_INVALID_ARGUMENT ||
        runtime_open_calls != 0U || runtime.initialized || port.context != NULL)
    {
        return 1;
    }
    (void)puts("PASS: wrong ESP BSP profile is rejected before runtime open");
    return 0;
}
