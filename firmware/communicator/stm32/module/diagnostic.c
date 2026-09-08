/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_diagnostic.h"
#include "canview_build_mode.h"

static void put_u16_le(uint8_t *buffer, size_t offset, uint16_t value)
{
    buffer[offset] = (uint8_t)value;
    buffer[offset + 1U] = (uint8_t)(value >> 8U);
}

static void put_u32_le(uint8_t *buffer, size_t offset, uint32_t value)
{
    buffer[offset] = (uint8_t)value;
    buffer[offset + 1U] = (uint8_t)(value >> 8U);
    buffer[offset + 2U] = (uint8_t)(value >> 16U);
    buffer[offset + 3U] = (uint8_t)(value >> 24U);
}

static bool reset_reason_valid(canview_stm_reset_reason_t reset_reason)
{
    const int32_t value = (int32_t)reset_reason;
    return value >= (int32_t)CANVIEW_STM_RESET_REASON_UNKNOWN &&
           value <= (int32_t)CANVIEW_STM_RESET_REASON_MAX;
}

static uint16_t status_bits(const canview_stm_diagnostic_t *diagnostic)
{
    uint16_t status = 0U;
    /* CAPTURE_ONLY validation above makes the TX bit permanently clear. */
    if (diagnostic->authenticity_known)
    {
        status = (uint16_t)(status | CANVIEW_STM_DIAGNOSTIC_STATUS_AUTHENTICITY_KNOWN);
    }
    if (diagnostic->production_debug_lock_known)
    {
        status = (uint16_t)(status | CANVIEW_STM_DIAGNOSTIC_STATUS_DEBUG_LOCK_KNOWN);
    }
    if (diagnostic->build_metadata_valid)
    {
        status = (uint16_t)(status | CANVIEW_STM_DIAGNOSTIC_STATUS_BUILD_VALID);
    }
    if (diagnostic->stack_watermark_valid)
    {
        status = (uint16_t)(status | CANVIEW_STM_DIAGNOSTIC_STATUS_STACK_VALID);
    }
    if (diagnostic->service_reset_erase_pending)
    {
        status = (uint16_t)(status | CANVIEW_STM_DIAGNOSTIC_STATUS_ERASE_PENDING);
    }
    return status;
}

canview_status_t canview_stm_diagnostic_encode(const canview_stm_diagnostic_t *diagnostic,
                                               uint8_t *buffer, size_t capacity,
                                               size_t *written)
{
    if (diagnostic == NULL || buffer == NULL || written == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *written = 0U;
    if (!reset_reason_valid(diagnostic->reset_reason))
    {
        return CANVIEW_MALFORMED;
    }
    if (diagnostic->control_capabilities != CANVIEW_STM_CONTROL_CAPABILITIES ||
        diagnostic->tx_permit != CANVIEW_STM_TX_PERMIT)
    {
        return CANVIEW_MALFORMED;
    }
    if (diagnostic->stack_free_bytes > (size_t)UINT32_MAX ||
        diagnostic->stack_min_free_bytes > (size_t)UINT32_MAX)
    {
        return CANVIEW_MALFORMED;
    }
    if (capacity < CANVIEW_STM_DIAGNOSTIC_ENCODED_BYTES)
    {
        return CANVIEW_BUFFER_TOO_SMALL;
    }

    buffer[4U] = CANVIEW_STM_DIAGNOSTIC_RECORD_VERSION;
    buffer[5U] = (uint8_t)diagnostic->reset_reason;
    put_u32_le(buffer, 0U, CANVIEW_STM_DIAGNOSTIC_RECORD_MAGIC);
    put_u16_le(buffer, 6U, status_bits(diagnostic));
    put_u32_le(buffer, 8U, diagnostic->reset_flags);
    put_u32_le(buffer, 12U, diagnostic->sysclk_hz);
    put_u32_le(buffer, 16U, diagnostic->peripheral_hz);
    put_u32_le(buffer, 20U, diagnostic->board_profile);
    put_u32_le(buffer, 24U, diagnostic->build_contract_digest);
    put_u32_le(buffer, 28U, (uint32_t)diagnostic->stack_free_bytes);
    put_u32_le(buffer, 32U, (uint32_t)diagnostic->stack_min_free_bytes);
    put_u32_le(buffer, 36U, diagnostic->control_capabilities);
    *written = CANVIEW_STM_DIAGNOSTIC_ENCODED_BYTES;
    return CANVIEW_OK;
}
