/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_platform_port.h
 * @brief app이 소비하는 안전 초기화와 대기 callback.
 */
#ifndef CANVIEW_PLATFORM_PORT_H
#define CANVIEW_PLATFORM_PORT_H
#include "canview_status.h"

/** @brief 안전 초기화 callback. 성공 전후로 차량 송신을 허용하지 않는다. */
typedef canview_status_t canview_safe_state_fn(void *context);
/** @brief 호출한 main/worker를 양보하거나 대기하는 callback. */
typedef void canview_idle_fn(void *context);

/**
 * BSP가 구현하고 app이 소비하는 단일-owner 포트.
 *
 * context는 app 종료까지 유효해야 한다. safe callback은 초기 출력 latch를
 * 설정한 후 pin mode를 전환하며 TX arm, NVS 삭제, radio 시작을 하지 않는다.
 * idle은 MCU별 대기만 수행한다. ISR 또는 다른 thread에서 함께 호출하지 않는다.
 */
typedef struct
{
    canview_safe_state_fn *enter_safe_state;
    canview_idle_fn *idle;
    void *context;
} canview_platform_port_t;
#endif
