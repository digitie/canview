/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_esp_runtime.h
 * @brief SDK 경계를 감춘 단일 bench service의 BSP/platform 연결.
 */
#ifndef CANVIEW_ESP_RUNTIME_H
#define CANVIEW_ESP_RUNTIME_H
#include "canview_esp_core.h"
#include "canview_esp_pool.h"

typedef struct
{
    canview_esp_core_stage_fn_t *safe_gpio;
    void *safe_context;
    uint8_t service_run_pin;
    uint8_t usb_service_pin;
} canview_esp_runtime_config_t;

/** Caller 소유 정적 저장소. zero-init 뒤 open 한 번, 필드 직접 접근 금지. */
typedef struct
{
    canview_esp_runtime_config_t config;
    void *owner;
    uint32_t wake_tick;
    bool initialized;
    bool watchdog_ready;
    bool wait_started;
} canview_esp_runtime_t;

typedef canview_status_t canview_esp_runtime_wait_fn_t(void *context);
typedef void canview_esp_runtime_report_fn_t(void *context, const canview_esp_core_t *core,
                                             canview_status_t service_status);
typedef struct
{
    canview_esp_core_port_t core;
    canview_esp_pool_port_t pool;
    canview_esp_runtime_wait_fn_t *wait;
    canview_esp_runtime_report_fn_t *report;
    void *context;
} canview_esp_runtime_port_t;

/**
 * @brief 현재 SDK main task를 단일 owner로 결합한다. GPIO/driver를 시작하지 않는다.
 * @param runtime zero-init 저장소. port와 모든 사용자의 수명보다 길어야 한다.
 * @param config BSP의 safe 함수와 입력 pin. 복사한다.
 * @param port 성공 시 callback 사본을 반환한다. runtime과 겹치지 않는 저장소다.
 * @return 인자/소유권 오류 또는 재초기화는 거부한다.
 */
canview_status_t canview_esp_runtime_open(canview_esp_runtime_t *runtime,
                                          const canview_esp_runtime_config_t *config,
                                          canview_esp_runtime_port_t *port);
/**
 * @brief 보드의 고정 safe/sense mapping으로 runtime을 연다.
 * @param runtime zero-init 정적 저장소.
 * @param port 성공 시 BSP/platform callback.
 * @return platform open의 결과. 성공도 차량 권한은 항상0이다.
 */
canview_status_t canview_esp_board_runtime(canview_esp_runtime_t *runtime,
                                           canview_esp_runtime_port_t *port);
#endif
