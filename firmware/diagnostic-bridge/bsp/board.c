/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_board.h"
#include "canview_gpio.h"
#include "board_pins.h"
#include <stddef.h>

static canview_status_t enter_safe_state(void *context)
{
    (void)context;
    const canview_status_t output =
        canview_gpio_output(CANVIEW_BOARD_STATUS_LED_GPIO, false, false);
    const canview_status_t input = canview_gpio_input(CANVIEW_BOARD_PAIR_BUTTON_N_GPIO);
    if (output != CANVIEW_OK)
    {
        return output;
    }
    return input;
}

canview_platform_port_t canview_board_port(void)
{
    const canview_platform_port_t port = {enter_safe_state, canview_platform_idle, NULL,
                                          CANVIEW_BOARD_PROFILE};
    return port;
}
