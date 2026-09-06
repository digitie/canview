/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_board.h"
#include "core_hw.h"

static canview_status_t core_safe(void *context)
{
    (void)context;
    const canview_platform_port_t port = canview_board_port();
    return port.enter_safe_state(port.context);
}

static void core_fault(void *context, canview_stm_fault_t reason)
{
    (void)reason;
    canview_stm_hw_latch_fault();
    if (core_safe(context) != CANVIEW_OK)
    {
        /* 외부 pull/gate가 최종 경계. 실패를 성공/refresh로 전환하지 않는다. */
        canview_stm_hw_latch_fault();
    }
}

canview_stm_boot_port_t canview_stm_board_boot_port(void)
{
    const canview_stm_boot_port_t port = {core_safe,
                                          canview_stm_watchdog_start,
                                          canview_stm_clock_start,
                                          canview_stm_time_start,
                                          core_fault,
                                          NULL};
    return port;
}

canview_stm_scheduler_port_t canview_stm_board_scheduler_port(void)
{
    const canview_stm_scheduler_port_t port = {canview_stm_now_us, canview_stm_watchdog_feed,
                                               core_fault, NULL};
    return port;
}

canview_stm_critical_t canview_stm_board_critical(void)
{
    const canview_stm_critical_t port = {canview_stm_critical_enter, canview_stm_critical_leave,
                                         NULL};
    return port;
}
