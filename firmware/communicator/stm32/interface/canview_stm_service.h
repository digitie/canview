/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_service.h
 * @brief protected control-root와 service-reset policy skeleton.
 */
#ifndef CANVIEW_STM_SERVICE_H
#define CANVIEW_STM_SERVICE_H

#include "canview_build_mode.h"
#include "canview_stm_reset.h"
#include "canview_status.h"
#include <stdbool.h>
#include <stdint.h>

#define CANVIEW_STM_SERVICE_ROOT_MAGIC UINT32_C(0x43565254)
#define CANVIEW_STM_SERVICE_ROOT_VERSION UINT16_C(1)

/**
 * @brief Flash에서 읽은 root page의 검증 결과와 service 입력.
 *
 * T-102에서는 Flash read/write를 하지 않는다. T-107이 authenticated page를
 * 읽어 이 caller-owned record를 채우며, 알 수 없는 값은 false로 남긴다.
 */
typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t byte_size;
    uint32_t generation;
    bool authenticity_known;
    bool authenticity_valid;
    bool production_debug_lock_known;
    bool production_debug_locked;
    bool service_window_asserted;
} canview_stm_service_root_t;

typedef struct
{
    bool root_page_valid;
    bool authenticity_known;
    bool production_debug_lock_known;
    bool control_policy_satisfied;
    uint32_t control_capabilities;
    bool tx_permit;
    bool erase_on_service_reset;
} canview_stm_service_decision_t;

/** @brief 안전한 unknown 상태의 root record를 만든다. */
canview_status_t canview_stm_service_root_init(canview_stm_service_root_t *root);

/**
 * @brief root shape과 trust 상태를 평가한다.
 *
 * 실제 Flash erase나 control 권한 부여를 수행하지 않는다. 현재 build mode의
 * control/TX 결과는 항상 zero/false이며, service reset은 pending decision만
 * 반환한다.
 */
canview_status_t canview_stm_service_policy_evaluate(
    const canview_stm_service_root_t *root, canview_stm_reset_reason_t reset_reason,
    canview_stm_service_decision_t *decision);

#endif
