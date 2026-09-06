/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_app.h
 * @brief 안전 idle 상태만 제공하는 공용 app 뼈대.
 */
#ifndef CANVIEW_APP_H
#define CANVIEW_APP_H
#include <stdint.h>
#include "canview_platform_port.h"

/** 빌드 시 고정하는 장치 역할. 무선 peer가 주장하는 권한과 무관하다. */
typedef enum
{
    CANVIEW_APP_CONTROLLER = 0,
    CANVIEW_APP_COMMUNICATOR_ESP32,
    CANVIEW_APP_COMMUNICATOR_STM32,
    CANVIEW_APP_DIAGNOSTIC_BRIDGE
} canview_app_role_t;

/** 부팅 뼈대 상태. ONLINE 또는 차량 TX 허용 상태는 존재하지 않는다. */
typedef enum
{
    CANVIEW_APP_UNINITIALIZED = 0,
    CANVIEW_APP_SAFE_IDLE,
    CANVIEW_APP_FAULT
} canview_app_state_t;

/** Caller 소유 app context. zero-init 후 start, 단일 main owner만 접근한다. */
typedef struct
{
    canview_app_role_t role;
    canview_app_state_t state;
    canview_platform_port_t port;
} canview_app_t;

/**
 * @brief 포트를 복사하고 안전 초기화만 수행한다.
 * @param app zero-init된 context; 성공 이후 반복 start는 거부한다.
 * @param role 빌드에 고정한 네 역할 중 하나.
 * @param port 필수 callback 두 개와 수명 보장된 context.
 * @return 성공은 SAFE_IDLE일 뿐 통신/차량 준비 완료가 아니다.
 *
 * 초기화 실패 시 FAULT를 유지하며 자동 재시도하지 않는다. heap, ISR, RTOS 의존 없음.
 */
canview_status_t canview_app_start(canview_app_t *app, canview_app_role_t role,
                                   const canview_platform_port_t *port);

/**
 * @brief 안전 idle 한 단계. SAFE_IDLE 또는 초기화 실패 FAULT에서 대기한다.
 * @param app start로 포트를 지정한 단일-owner context.
 * @return 포트가 없으면 INVALID_ARGUMENT. Fault에서는 초기 실패 상태를
 * 지우지 않고 NOT_IMPLEMENTED를 반환한다. 기능 스케줄러는 후속 task다.
 */
canview_status_t canview_app_step(canview_app_t *app);
#endif
