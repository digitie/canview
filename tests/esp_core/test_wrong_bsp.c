/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_board.h"
#include "canview_esp_runtime.h"
#include "canview_gpio.h"
#include <setjmp.h>
#include <stdio.h>

static unsigned runtime_open_calls;
static unsigned gpio_calls;
static unsigned idle_calls;
static jmp_buf stopped;

void app_main(void);

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
    ++gpio_calls;
    return CANVIEW_OK;
}

canview_status_t canview_gpio_input(uint8_t pin)
{
    (void)pin;
    ++gpio_calls;
    return CANVIEW_OK;
}

void canview_platform_idle(void *context)
{
    (void)context;
    ++idle_calls;
    longjmp(stopped, 1);
}

int main(void)
{
    if (setjmp(stopped) == 0)
    {
        app_main();
        return 1;
    }
    if (gpio_calls != 0U || runtime_open_calls != 0U || idle_calls != 1U)
    {
        return 1;
    }
    (void)puts("PASS: wrong ESP BSP profile is rejected before GPIO or runtime open");
    return 0;
}
