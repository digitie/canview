/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_app.h"
#include "canview_board.h"

#if defined(CANVIEW_ENTRY_ESP_IDF)
void app_main(void);
void app_main(void)
#else
int main(void)
#endif
{
    canview_app_t app = {0};
    const canview_platform_port_t port = canview_board_port();
    const canview_status_t status = canview_app_start(&app, CANVIEW_TARGET_ROLE, &port);
    if (status != CANVIEW_OK)
    {
        /* Fault is latched. No restart, storage erase or peripheral enable. */
        for (;;)
        {
            port.idle(port.context);
        }
    }
    for (;;)
    {
        if (canview_app_step(&app) != CANVIEW_OK)
        {
            for (;;)
            {
                port.idle(port.context);
            }
        }
    }
}
