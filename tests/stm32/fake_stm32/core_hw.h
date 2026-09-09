/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_STM_CORE_HW_H
#define CANVIEW_STM_CORE_HW_H

#include <stdint.h>

#include "canview_status.h"

#define CANVIEW_STM_SYSCLK_HZ (160000000U)
#define CANVIEW_STM_UART_BRR (20U)

uint32_t canview_stm_now_us(void *context);
uint64_t canview_stm_now_us64(void *context);
uint64_t canview_stm_now_ms64(void *context);
uint32_t canview_stm_critical_enter(void *context);
void canview_stm_critical_leave(void *context, uint32_t saved_mask);

#endif
