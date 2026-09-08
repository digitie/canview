/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_bridge_web.h
 *  @brief Diagnostic Bridge의 고정 channel, read-only HTTP/WebSocket shell.
 */
#ifndef CANVIEW_BRIDGE_WEB_H
#define CANVIEW_BRIDGE_WEB_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define CANVIEW_BRIDGE_WEB_MAX_JSON_BYTES (8192U)
#define CANVIEW_BRIDGE_WEB_MAX_URI_BYTES (128U)
#define CANVIEW_BRIDGE_WEB_MAX_RESPONSE_BYTES (4096U)
#define CANVIEW_BRIDGE_WEB_MAX_ORIGIN_BYTES (64U)
#define CANVIEW_BRIDGE_WEB_MAX_AUTH_HEADER_BYTES (96U)
#define CANVIEW_BRIDGE_WEB_MAX_WS_FRAME_BYTES (512U)
#define CANVIEW_BRIDGE_WEB_AP_PASSWORD_BYTES (64U)
#define CANVIEW_BRIDGE_WEB_SERVICE_HOLD_MS (3000U)
#define CANVIEW_BRIDGE_WEB_SERVICE_WINDOW_MS (600000U)
#define CANVIEW_BRIDGE_WEB_CLIENT_IDLE_TIMEOUT_MS (300000U)
#define CANVIEW_BRIDGE_WEB_POLL_PERIOD_MS (100U)
#define CANVIEW_BRIDGE_WEB_WIFI_CHANNEL (6U)

typedef bool canview_bridge_web_button_pressed_fn(void *context);

typedef struct
{
    /** @brief NVS에서 읽은 32-byte SHA-256 PIN digest. 이 함수가 복사한다. */
    const uint8_t *pin_digest;
    /** @brief NVS에서 읽은 WPA2 password. 소스·로그·HTTP 응답에는 넣지 않는다. */
    const char *ap_password;
    canview_bridge_web_button_pressed_fn *button_pressed;
    void *button_context;
} canview_bridge_web_config_t;

/**
 * @brief singleton web service를 시작한다.
 *
 * APSTA의 STA는 외부 AP에 연결하지 않는다. 성공은 source-level service 시작일 뿐이며
 * 실제 AP association, phone browser, power/reset/HIL gate를 닫지 않는다.
 */
esp_err_t canview_bridge_web_start(const canview_bridge_web_config_t *config);

/** @brief HTTP/DNS/AP 자원을 idempotent하게 중지하고 인증 상태를 폐기한다. */
esp_err_t canview_bridge_web_stop(void);

/**
 * @brief app의 단일 owner loop에서 100 ms마다 service button을 갱신한다.
 *
 * 3초 연속 hold 뒤 service window를 열고, release 후에도 10분 동안 유지한다. 이 함수는
 * HTTP task와 mutex로 동기화되며 ISR에서 호출하면 안 된다. 유휴 client는
 * `CANVIEW_BRIDGE_WEB_CLIENT_IDLE_TIMEOUT_MS` 뒤에 세션을 폐기하고 연결을 닫는다.
 */
esp_err_t canview_bridge_web_poll(void);

/** @brief 현재 physical service window 상태를 반환한다. */
bool canview_bridge_web_service_window_open(void);

#endif
