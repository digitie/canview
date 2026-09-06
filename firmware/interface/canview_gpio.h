/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_GPIO_H
#define CANVIEW_GPIO_H
#include "canview_status.h"
#include <stdbool.h>
#include <stdint.h>
/** @brief 출력 latch를 먼저 설정하고 mode를 바꾼다. GPIO 번호는 BSP 정본 사용. */
canview_status_t canview_gpio_output(uint8_t pin, bool high, bool open_drain);
/** @brief 외부 pull을 유지하는 입력. 내부 pull을 임의로 추가하지 않는다. */
canview_status_t canview_gpio_input(uint8_t pin);
/** @brief SDK task 문맥에서 10 ms 이상 양보. ISR 호출 금지. */
void canview_platform_idle(void *context);
#endif
