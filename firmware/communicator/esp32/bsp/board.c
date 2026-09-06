/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_board.h"
#include "canview_gpio.h"
#include "board_pins.h"
#include <stddef.h>

static canview_status_t enter_safe_state(void *context)
{
    (void)context;
    /* RUN_OK low first: never arm vehicle TX or feed the external watchdog. */
    const struct
    {
        uint8_t pin;
        bool high;
        bool open_drain;
    } outputs[] = {{CANVIEW_BOARD_ESP_RUN_OK_GPIO, false, false},
                   {CANVIEW_BOARD_STM_BOOT0_REQ_GPIO, false, false},
                   {CANVIEW_BOARD_GPS_PWR_REQ_GPIO, false, false},
                   {CANVIEW_BOARD_STM_RESET_CMD_N_GPIO, true, false},
                   {CANVIEW_BOARD_STM_RECOVERY_N_GPIO, true, true}};
    for (size_t index = 0; index < sizeof(outputs) / sizeof(outputs[0]); ++index)
    {
        const canview_status_t status =
            canview_gpio_output(outputs[index].pin, outputs[index].high, outputs[index].open_drain);
        if (status != CANVIEW_OK)
        {
            return status;
        }
    }
    return canview_gpio_input(CANVIEW_BOARD_RECOVERY_BUTTON_N_GPIO);
}

canview_platform_port_t canview_board_port(void)
{
    const canview_platform_port_t port = {enter_safe_state, canview_platform_idle, NULL};
    return port;
}
