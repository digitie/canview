/* SPDX-License-Identifier: GPL-3.0-only */
/** @file bridge_bootstrap.h
 *  @brief Diagnostic Bridge의 host-testable core/bootstrap 조합.
 */
#ifndef CANVIEW_BRIDGE_BOOTSTRAP_H
#define CANVIEW_BRIDGE_BOOTSTRAP_H

#include "canview_board.h"
#include "canview_esp_pool.h"
#include "canview_esp_runtime.h"

/**
 * @brief board preflight, safe state, runtime, deferred core와 fixed pool을 연결한다.
 *
 * 네트워크·NVS는 이 함수에 포함하지 않는다. 성공 뒤 caller는 필요한 외부 초기화를
 * 완료하고 canview_esp_core_arm_watchdog()를 호출한 뒤 service loop를 시작한다.
 */
canview_status_t canview_bridge_bootstrap(canview_esp_core_t *core,
                                          canview_esp_pool_t *pool,
                                          canview_esp_runtime_t *runtime,
                                          canview_esp_runtime_port_t *port,
                                          const canview_platform_port_t *board);

#endif
