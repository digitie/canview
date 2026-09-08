/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_diagnostic.h
 * @brief reset/build/stack diagnostic snapshot과 고정 길이 record.
 */
#ifndef CANVIEW_STM_DIAGNOSTIC_H
#define CANVIEW_STM_DIAGNOSTIC_H

#include "canview_stm_reset.h"
#include "canview_status.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint32_t reset_flags;
    canview_stm_reset_reason_t reset_reason;
    uint32_t sysclk_hz;
    uint32_t peripheral_hz;
    uint32_t control_capabilities;
    bool tx_permit;
    bool authenticity_known;
    bool production_debug_lock_known;
    bool build_metadata_valid;
    uint32_t build_contract_digest;
    uint32_t board_profile;
    size_t stack_free_bytes;
    size_t stack_min_free_bytes;
    bool stack_watermark_valid;
    bool service_reset_erase_pending;
    const char *protocol_schema_sha256;
    const char *uart_protocol_schema_sha256;
    const char *hardware_digest;
    const char *build_mode;
} canview_stm_diagnostic_t;

#define CANVIEW_STM_DIAGNOSTIC_RECORD_MAGIC UINT32_C(0x31445643)
#define CANVIEW_STM_DIAGNOSTIC_RECORD_VERSION UINT8_C(1)
#define CANVIEW_STM_DIAGNOSTIC_ENCODED_BYTES (40U)

#define CANVIEW_STM_DIAGNOSTIC_STATUS_TX_PERMIT UINT16_C(0x0001)
#define CANVIEW_STM_DIAGNOSTIC_STATUS_AUTHENTICITY_KNOWN UINT16_C(0x0002)
#define CANVIEW_STM_DIAGNOSTIC_STATUS_DEBUG_LOCK_KNOWN UINT16_C(0x0004)
#define CANVIEW_STM_DIAGNOSTIC_STATUS_BUILD_VALID UINT16_C(0x0008)
#define CANVIEW_STM_DIAGNOSTIC_STATUS_STACK_VALID UINT16_C(0x0010)
#define CANVIEW_STM_DIAGNOSTIC_STATUS_ERASE_PENDING UINT16_C(0x0020)

/**
 * @brief pointer를 포함하지 않는 little-endian diagnostic record를 만든다.
 *
 * T-104 UART가 이 record를 운반할 수 있지만, 이것은 UART wire ABI가 아니다.
 * capacity 부족/invalid enum/size overflow에서는 output record를 변경하지 않는다.
 */
canview_status_t canview_stm_diagnostic_encode(const canview_stm_diagnostic_t *diagnostic,
                                               uint8_t *buffer, size_t capacity,
                                               size_t *written);

#endif
