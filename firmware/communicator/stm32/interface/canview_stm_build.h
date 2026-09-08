/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_build.h
 * @brief protocol/board/build provenance metadata 계약.
 */
#ifndef CANVIEW_STM_BUILD_H
#define CANVIEW_STM_BUILD_H

#include "canview_build_mode.h"
#include "canview_status.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 정적 문자열을 가리키는 build contract snapshot.
 *
 * pointer는 process-local diagnostic API 전용이며 wire에 struct를 memcpy하지
 * 않는다. build_contract_digest는 cryptographic authenticity가 아닌 고정
 * provenance fingerprint다.
 */
typedef struct
{
    const char *protocol_schema_sha256;
    const char *uart_protocol_schema_sha256;
    const char *hardware_digest;
    const char *build_mode;
    uint32_t board_profile;
    uint32_t build_contract_digest;
    uint32_t control_capabilities;
    bool tx_permit;
} canview_stm_build_metadata_t;

/** @brief generated schema/profile와 CAPTURE_ONLY metadata를 caller에 복사한다. */
canview_status_t canview_stm_build_metadata_get(canview_stm_build_metadata_t *metadata);

#endif
