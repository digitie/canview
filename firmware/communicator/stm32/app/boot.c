/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_core.h"

canview_status_t canview_stm_boot_start(canview_stm_boot_t *boot,
                                        const canview_stm_boot_port_t *port)
{
    if (boot == NULL || port == NULL || port->safe == NULL || port->watchdog_start == NULL ||
        port->clock_start == NULL || port->time_start == NULL || port->fault == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (boot->state != CANVIEW_STM_BOOT_UNINITIALIZED)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    boot->state = CANVIEW_STM_BOOT_STARTING;
    canview_stm_action_fn *const actions[] = {port->safe, port->watchdog_start, port->clock_start,
                                             port->time_start};
    for (size_t index = 0U; index < sizeof(actions) / sizeof(actions[0]); ++index)
    {
        boot->result = actions[index](port->context);
        if (boot->result != CANVIEW_OK)
        {
            boot->state = CANVIEW_STM_BOOT_FAULT;
            port->fault(port->context, CANVIEW_STM_FAULT_BOOT);
            return boot->result;
        }
        if (index == 1U)
        {
            boot->watchdog_started = true;
        }
    }
    boot->state = CANVIEW_STM_BOOT_READY;
    return CANVIEW_OK;
}
