/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_service.h"
#include <stddef.h>

canview_status_t canview_stm_service_root_init(canview_stm_service_root_t *root)
{
    if (root == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_stm_service_root_t result = {
        CANVIEW_STM_SERVICE_ROOT_MAGIC,
        CANVIEW_STM_SERVICE_ROOT_VERSION,
        (uint16_t)sizeof(canview_stm_service_root_t),
        0U,
        false,
        false,
        false,
        false,
        false};
    *root = result;
    return CANVIEW_OK;
}

static bool reset_reason_valid(canview_stm_reset_reason_t reset_reason)
{
    const int32_t value = (int32_t)reset_reason;
    return value >= (int32_t)CANVIEW_STM_RESET_REASON_UNKNOWN &&
           value <= (int32_t)CANVIEW_STM_RESET_REASON_MAX;
}

canview_status_t canview_stm_service_policy_evaluate(
    const canview_stm_service_root_t *root, canview_stm_reset_reason_t reset_reason,
    canview_stm_service_decision_t *decision)
{
    if (decision == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *decision = (const canview_stm_service_decision_t){0};
    if (root == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (!reset_reason_valid(reset_reason))
    {
        return CANVIEW_MALFORMED;
    }
    if (root->magic != CANVIEW_STM_SERVICE_ROOT_MAGIC ||
        root->version != CANVIEW_STM_SERVICE_ROOT_VERSION ||
        root->byte_size != (uint16_t)sizeof(canview_stm_service_root_t))
    {
        return CANVIEW_MALFORMED;
    }

    const bool trust_ok = root->authenticity_known && root->authenticity_valid &&
                          root->production_debug_lock_known && root->production_debug_locked;
    const canview_stm_service_decision_t result = {
        true,
        root->authenticity_known,
        root->production_debug_lock_known,
        trust_ok,
        CANVIEW_STM_CONTROL_CAPABILITIES,
        CANVIEW_STM_TX_PERMIT && trust_ok,
        root->service_window_asserted && reset_reason != CANVIEW_STM_RESET_REASON_UNKNOWN &&
            reset_reason != CANVIEW_STM_RESET_REASON_AMBIGUOUS};
    *decision = result;
    return CANVIEW_OK;
}
