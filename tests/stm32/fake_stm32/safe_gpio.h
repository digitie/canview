/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_STM_SAFE_GPIO_H
#define CANVIEW_STM_SAFE_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#include "canview_status.h"

canview_status_t canview_stm_output(uint8_t port, uint8_t pin, bool high);

#endif
