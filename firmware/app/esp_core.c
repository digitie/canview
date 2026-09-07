/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_esp_runtime.h"
#include "canview_board.h"

void app_main(void);
void app_main(void)
{
    static canview_esp_core_t core;
    static canview_esp_pool_t pool;
    static canview_esp_runtime_t runtime;
    canview_esp_runtime_port_t port = {0};
    const canview_platform_port_t board = canview_board_port();
    /* runtime_open 이전에도 외부 gate가 볼 수 있는 safe latch를 먼저 설정한다. */
    canview_status_t status = CANVIEW_INVALID_ARGUMENT;
    if (board.enter_safe_state == NULL || board.idle == NULL)
    {
        /* 필수 BSP callback이 없으면 초기화나 report 없이 terminal path로 간다. */
        status = CANVIEW_INVALID_ARGUMENT;
    }
    else
    {
        status = board.enter_safe_state(board.context);
    }
    if (status == CANVIEW_OK)
    {
        status = canview_esp_board_runtime(&runtime, &port);
    }
    if (status == CANVIEW_OK)
    {
        status = canview_esp_core_boot(&core, &port.core);
    }
    if (status == CANVIEW_OK)
    {
        status = canview_esp_pool_init(&pool, CANVIEW_ESP_POOL_SLOTS, &port.pool);
    }
    if (port.report != NULL)
    {
        port.report(port.context, &core, status);
    }
    while (status == CANVIEW_OK)
    {
        status = port.wait(port.context);
        if (status == CANVIEW_OK)
        {
            status = canview_esp_core_step(&core);
        }
    }
    /* 최초 실패 이후 feed/재초기화 없음. 등록 후 실패는 TWDT가 reset한다.
     * runtime_open 이전 GPIO 실패도 safe idle을 유지한다. 외부 TX gate는 별도 필수다. */
    if (port.report != NULL)
    {
        port.report(port.context, &core, status);
    }
    if (board.idle != NULL)
    {
        for (;;)
        {
            board.idle(board.context);
        }
    }
}
