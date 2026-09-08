/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_build_mode.h
 * @brief STM32 firmware build mode의 단방향 안전 계약.
 */
#ifndef CANVIEW_BUILD_MODE_H
#define CANVIEW_BUILD_MODE_H

#include <stdbool.h>
#include <stdint.h>

/* CMake가 모든 STM32 C translation unit에 주입하는 immutable composition token이다. */
#ifndef CANVIEW_STM_CAPTURE_ONLY_CONTRACT
#error CANVIEW_STM_CAPTURE_ONLY_contract_is_required
#endif
#if CANVIEW_STM_CAPTURE_ONLY_CONTRACT != 1
#error CANVIEW_STM_CAPTURE_ONLY_contract_must_be_one
#endif

/* 이 image는 capture-only bench foundation이다. 다른 mode를 compile flag로
 * 주입하는 우회 경로를 제공하지 않는다. */
#if defined(CANVIEW_STM_BUILD_MODE) || defined(CANVIEW_STM_BUILD_MODE_CAPTURE_ONLY) || \
    defined(CANVIEW_STM_CONTROL_CAPABILITIES) || defined(CANVIEW_STM_TX_PERMIT)
#error CANVIEW_STM_BUILD_MODE_must_not_be_overridden
#endif
#if defined(CANVIEW_STM_ENABLE_BENCH_TX) || defined(CANVIEW_STM_ENABLE_VEHICLE_TX)
#error STM32_TX_mode_is_not_supported_by_this_image
#endif

#define CANVIEW_STM_BUILD_MODE_CAPTURE_ONLY (0U)
#define CANVIEW_STM_BUILD_MODE (CANVIEW_STM_BUILD_MODE_CAPTURE_ONLY)
#define CANVIEW_STM_BUILD_MODE_NAME "CAPTURE_ONLY"
#define CANVIEW_STM_CONTROL_CAPABILITIES (UINT32_C(0))
#define CANVIEW_STM_TX_PERMIT (false)

typedef enum
{
    CANVIEW_STM_MODE_CAPTURE_ONLY = CANVIEW_STM_BUILD_MODE_CAPTURE_ONLY
} canview_stm_build_mode_t;

#endif
