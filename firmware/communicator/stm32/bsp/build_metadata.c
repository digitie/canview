/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_build.h"
#include "board_pins.h"
#include "canview_protocol.h"
#include "canview_uart_protocol.h"

#ifndef CANVIEW_BOARD_HARDWARE_DIGEST
#error Generated_board_hardware_digest_is_required
#endif

#define CANVIEW_STM_FNV_OFFSET UINT32_C(2166136261)
#define CANVIEW_STM_FNV_PRIME UINT32_C(16777619)

const uint32_t canview_stm_capture_only_contract_anchor =
    CANVIEW_STM_CAPTURE_ONLY_CONTRACT_ANCHOR_VALUE;

static uint32_t digest_text(uint32_t digest, const char *text)
{
    while (*text != '\0')
    {
        digest ^= (uint32_t)(uint8_t)*text;
        digest *= CANVIEW_STM_FNV_PRIME;
        ++text;
    }
    return digest;
}

static uint32_t build_contract_digest(void)
{
    uint32_t digest = CANVIEW_STM_FNV_OFFSET;
    digest ^= canview_stm_capture_only_contract_anchor;
    digest = digest_text(digest, CANVIEW_PROTOCOL_SCHEMA_SHA256);
    digest = digest_text(digest, CANVIEW_UART_PROTOCOL_SCHEMA_SHA256);
    digest = digest_text(digest, CANVIEW_BOARD_HARDWARE_DIGEST);
    digest = digest_text(digest, CANVIEW_STM_BUILD_MODE_NAME);
    for (uint32_t shift = 0U; shift < 32U; shift += 8U)
    {
        digest ^= (CANVIEW_BOARD_PROFILE >> shift) & UINT32_C(0xff);
        digest *= CANVIEW_STM_FNV_PRIME;
    }
    /* A zero fingerprint is reserved for an uninitialized diagnostic record. */
    return digest | UINT32_C(1);
}

canview_status_t canview_stm_build_metadata_get(canview_stm_build_metadata_t *metadata)
{
    if (metadata == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_stm_build_metadata_t result = {
        CANVIEW_PROTOCOL_SCHEMA_SHA256,
        CANVIEW_UART_PROTOCOL_SCHEMA_SHA256,
        CANVIEW_BOARD_HARDWARE_DIGEST,
        CANVIEW_STM_BUILD_MODE_NAME,
        CANVIEW_BOARD_PROFILE,
        build_contract_digest(),
        CANVIEW_STM_CONTROL_CAPABILITIES,
        CANVIEW_STM_TX_PERMIT};
    *metadata = result;
    return CANVIEW_OK;
}
