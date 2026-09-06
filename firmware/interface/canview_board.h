/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_BOARD_H
#define CANVIEW_BOARD_H
#include "canview_platform_port.h"
/** @brief BSP가 제공하는 정적 수명의 포트. 반환 값은 app이 복사한다. */
canview_platform_port_t canview_board_port(void);
#endif
