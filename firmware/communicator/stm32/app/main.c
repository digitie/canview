/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_board_core.h"
#include "canview_board.h"

/* 이것은 실 CAN/UART worker가 아니다. T-103/T-104의 health vote를 대신하지 않는다. */
static canview_stm_boot_t boot;
static canview_stm_scheduler_t scheduler;

int main(void)
{
    const canview_stm_boot_port_t boot_port = canview_stm_board_boot_port();
    if (canview_stm_boot_start(&boot, &boot_port) != CANVIEW_OK)
    {
        canview_stm_board_wait_reset();
    }
    const canview_stm_scheduler_port_t schedule_port = canview_stm_board_scheduler_port();
    const canview_stm_worker_t workers[] = {{canview_stm_board_health, NULL, 1U, 10U, 100U, true}};
    if (canview_stm_scheduler_init(&scheduler, workers, sizeof(workers) / sizeof(workers[0]),
                                   &schedule_port, canview_stm_board_now_ms()) != CANVIEW_OK)
    {
        boot_port.fault(boot_port.context, CANVIEW_STM_FAULT_BOOT);
        canview_stm_board_wait_reset();
    }
    const canview_platform_port_t board = canview_board_port();
    for (;;)
    {
        if (canview_stm_scheduler_step(&scheduler, canview_stm_board_now_ms()) != CANVIEW_OK)
        {
            canview_stm_board_wait_reset();
        }
        board.idle(board.context);
    }
}
