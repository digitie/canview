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
    /* GPIO 또는 SDK 호출 전, runtime object가 기대한 BSP profile인지 fail-closed로 확인한다. */
    canview_status_t status = canview_esp_board_preflight(&board);
    if (status == CANVIEW_OK && (board.enter_safe_state == NULL || board.idle == NULL))
    {
        /* 필수 BSP callback이 없으면 초기화나 report 없이 terminal path로 간다. */
        status = CANVIEW_INVALID_ARGUMENT;
    }
    else if (status == CANVIEW_OK)
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
