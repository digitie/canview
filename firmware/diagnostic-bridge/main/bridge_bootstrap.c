/* SPDX-License-Identifier: GPL-3.0-only */
#include "bridge_bootstrap.h"

canview_status_t canview_bridge_bootstrap(canview_esp_core_t *core,
                                          canview_esp_pool_t *pool,
                                          canview_esp_runtime_t *runtime,
                                          canview_esp_runtime_port_t *port,
                                          const canview_platform_port_t *board)
{
    if (core == NULL || pool == NULL || runtime == NULL || port == NULL || board == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_status_t status = canview_esp_board_preflight(board);
    if (status == CANVIEW_OK && (board->enter_safe_state == NULL || board->idle == NULL))
    {
        status = CANVIEW_INVALID_ARGUMENT;
    }
    else if (status == CANVIEW_OK)
    {
        status = board->enter_safe_state(board->context);
    }
    if (status == CANVIEW_OK)
    {
        status = canview_esp_board_runtime(runtime, port);
    }
    if (status == CANVIEW_OK)
    {
        status = canview_esp_core_boot_deferred(core, &port->core);
    }
    if (status == CANVIEW_OK)
    {
        status = canview_esp_pool_init(pool, CANVIEW_ESP_POOL_SLOTS, &port->pool);
    }
    return status;
}
