/* SPDX-License-Identifier: GPL-3.0-only */
/** @file bridge_button.h
 *  @brief Diagnostic Bridge의 active-low service-window 입력.
 */
#ifndef CANVIEW_BRIDGE_BUTTON_H
#define CANVIEW_BRIDGE_BUTTON_H

#include <stdbool.h>

/** @brief GPIO4의 외부 pull-up 버튼을 SDK task 문맥에서 읽는다. */
bool canview_bridge_button_pressed(void);

#endif
