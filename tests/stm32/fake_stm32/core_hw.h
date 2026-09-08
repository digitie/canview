/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_STM_CORE_HW_H
#define CANVIEW_STM_CORE_HW_H

#include <stdint.h>

#include "canview_status.h"

#define CANVIEW_STM_SYSCLK_HZ (160000000U)

uint32_t canview_stm_now_us(void *context);
uint32_t canview_stm_critical_enter(void *context);
void canview_stm_critical_leave(void *context, uint32_t saved_mask);

#endif
