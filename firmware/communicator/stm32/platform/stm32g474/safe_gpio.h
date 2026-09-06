/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_STM_SAFE_GPIO_H
#define CANVIEW_STM_SAFE_GPIO_H
#include "canview_status.h"
#include <stdbool.h>
#include <stdint.h>
/** @brief A/B 포트의 안전 출력만 지원. latch -> push-pull mode 순서. */
canview_status_t canview_stm_output(uint8_t port, uint8_t pin, bool high);
/** @brief CAN/UART/IRQ를 시작하지 않고 interrupt 대기. */
void canview_stm_idle(void *context);
#endif
