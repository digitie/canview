/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_STM_CORE_HW_H
#define CANVIEW_STM_CORE_HW_H
#include "canview_stm_board_core.h"
#define CANVIEW_STM_HSE_HZ (16000000UL)
#define CANVIEW_STM_PLL_M (4UL)
#define CANVIEW_STM_PLL_N (80UL)
#define CANVIEW_STM_PLL_R (2UL)
#define CANVIEW_STM_PLL_Q (4UL)
#define CANVIEW_STM_SYSCLK_HZ                                                                      \
    (CANVIEW_STM_HSE_HZ / CANVIEW_STM_PLL_M * CANVIEW_STM_PLL_N / CANVIEW_STM_PLL_R)
#define CANVIEW_STM_PCLK_HZ (CANVIEW_STM_SYSCLK_HZ / 2UL)
#define CANVIEW_STM_FDCAN_HZ                                                                       \
    (CANVIEW_STM_HSE_HZ / CANVIEW_STM_PLL_M * CANVIEW_STM_PLL_N / CANVIEW_STM_PLL_Q)
#define CANVIEW_STM_UART_BAUD (4000000UL)
#define CANVIEW_STM_UART_BRR (CANVIEW_STM_PCLK_HZ / CANVIEW_STM_UART_BAUD)
#if CANVIEW_STM_SYSCLK_HZ != 160000000UL || CANVIEW_STM_PCLK_HZ != 80000000UL ||                   \
    CANVIEW_STM_FDCAN_HZ != 80000000UL || CANVIEW_STM_UART_BRR != 20UL
#error Invalid_R1_clock_contract
#endif

/** @brief MCU singleton의 bounded 초기화 callback. context는 사용하지 않는다. */
canview_status_t canview_stm_watchdog_start(void *context);
/** @brief HSE crystal·range1 boost·PLL/분주·kernel clock 구성. 실패는 TIMEOUT. */
canview_status_t canview_stm_clock_start(void *context);
/** @brief TIM2 1MHz·SysTick 1ms. clock 성공 뒤만 허용. */
canview_status_t canview_stm_time_start(void *context);
/** @brief TIM2 u32 microseconds. single register read, 약71.6분 wrap. */
uint32_t canview_stm_now_us(void *context);
/** @brief scheduler 전용 IWDG feed. fault/clock loss 뒤에는 거부. */
canview_status_t canview_stm_watchdog_feed(void *context);
/** @brief ISR에서 호출 가능한 fail-stop latch. GPIO reset은 BSP 책임. */
void canview_stm_hw_latch_fault(void);
/** @brief PRIMASK 저장 후 mask. 메모리 barrier 포함, NMI/HardFault는 제외. */
uint32_t canview_stm_critical_enter(void *context);
/** @brief enter의 이전 mask를 그대로 복원. 무조건 enable 금지. */
void canview_stm_critical_leave(void *context, uint32_t saved_mask);
#endif
