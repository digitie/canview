/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_board.h"
#include "board_pins.h"
#include "safe_gpio.h"
#include <stddef.h>

#define SAFE_OUTPUT(name, level) {CANVIEW_BOARD_##name##_PORT, CANVIEW_BOARD_##name##_PIN, level}

static canview_status_t enter_safe_state(void *context)
{
    (void)context;
    const struct
    {
        uint8_t port;
        uint8_t pin;
        bool high;
    } outputs[] = {SAFE_OUTPUT(STB1_REQ, true),    SAFE_OUTPUT(STB2_REQ, true),
                   SAFE_OUTPUT(FT_EN_REQ, false),  SAFE_OUTPUT(STM_ARM_EDGE, false),
                   SAFE_OUTPUT(WD_PULSE, false),   SAFE_OUTPUT(CAN1_TX_REQ, true),
                   SAFE_OUTPUT(CAN2_TX_REQ, true), SAFE_OUTPUT(CAN3_TX_REQ, true)};
    for (size_t index = 0; index < sizeof(outputs) / sizeof(outputs[0]); ++index)
    {
        const canview_status_t status =
            canview_stm_output(outputs[index].port, outputs[index].pin, outputs[index].high);
        if (status != CANVIEW_OK)
        {
            return status;
        }
    }
    return CANVIEW_OK;
}

canview_platform_port_t canview_board_port(void)
{
    const canview_platform_port_t port = {enter_safe_state, canview_stm_idle, NULL};
    return port;
}
