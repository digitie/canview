/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_board_core.h
 * @brief 단일 STM32 BSP의 bench core 포트. 모든 TX capability는 항상 0이다.
 */
#ifndef CANVIEW_STM_BOARD_CORE_H
#define CANVIEW_STM_BOARD_CORE_H
#include "canview_stm_core.h"
#include "canview_stm_diagnostic.h"
#include "canview_stm_queue.h"

/** @brief 정적 MCU 포트. main에서 한 번 사용하며 외부 PHY는 standby 유지. */
canview_stm_boot_port_t canview_stm_board_boot_port(void);
/** @brief 단일 core scheduler의 clock/feed/fault 포트. */
canview_stm_scheduler_port_t canview_stm_board_scheduler_port(void);
/** @brief 이전 PRIMASK를 복원하는 queue용 bounded critical port. */
canview_stm_critical_t canview_stm_board_critical(void);
/** @brief SysTick u32 monotonic ms. RTC와 무관, 49.7일 wrap. */
uint32_t canview_stm_board_now_ms(void);
/** @brief 실제 clock/timer register 상태 확인. 미준비/clock loss는 실패. */
canview_status_t canview_stm_board_health(void *context);
/** @brief reset/build/stack bench 진단 snapshot.
 * UART transport는 후속 T-104가 고정 record를 사용하며 pointer를 wire로 보내지 않는다.
 * @param diagnostic caller 출력. NULL이면 아무 동작도 하지 않는다.
 */
void canview_stm_board_diagnostic(canview_stm_diagnostic_t *diagnostic);
/** @brief T-104 UART가 운반할 수 있는 fixed diagnostic record를 만든다. */
canview_status_t canview_stm_board_diagnostic_encode(uint8_t *buffer, size_t capacity,
                                                     size_t *written);
/** @brief fault에서 안전 출력 유지·refresh 없이 reset 대기. 반환하지 않는다.
 * IWDG 자체 초기화 실패이면 system reset을 요청한다. main 전용, ISR 호출 금지.
 */
void canview_stm_board_wait_reset(void);
#endif
