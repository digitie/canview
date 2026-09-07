/* SPDX-License-Identifier: GPL-3.0-only */
#include "bridge_button.h"
#include "board_pins.h"
#include "driver/gpio.h"

bool canview_bridge_button_pressed(void)
{
    const gpio_num_t pin = (gpio_num_t)CANVIEW_BOARD_PAIR_BUTTON_N_GPIO;
    if (!GPIO_IS_VALID_GPIO(pin))
    {
        return false;
    }
    return gpio_get_level(pin) == 0;
}
