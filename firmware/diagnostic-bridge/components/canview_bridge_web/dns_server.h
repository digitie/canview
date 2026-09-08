/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_BRIDGE_DNS_SERVER_H
#define CANVIEW_BRIDGE_DNS_SERVER_H

#include "esp_err.h"

esp_err_t canview_bridge_dns_start(void);

/** @brief captive DNS task/socket을 중지한다. 여러 번 호출해도 안전하다. */
esp_err_t canview_bridge_dns_stop(void);

/** @brief DNS task의 bounded heartbeat와 task-WDT user 상태를 확인한다. */
esp_err_t canview_bridge_dns_health(void);

#endif
