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

#define CANVIEW_STM_CAPTURE_ONLY_CONTRACT_ANCHOR_VALUE UINT32_C(0x43415030)
#define CANVIEW_STM_BUILD_ID_DIGEST_SIZE (16U)

/** @brief target link가 CAPTURE_ONLY BSP provider를 실제로 포함했음을 나타내는 sentinel. */
extern const uint32_t canview_stm_capture_only_contract_anchor;

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

/** @brief BSP metadata provider가 조립한 schema/profile와 CAPTURE_ONLY metadata를 복사한다. */
canview_status_t canview_stm_build_metadata_get(canview_stm_build_metadata_t *metadata);

/**
 * @brief UART HELLO에 넣을 최종 링크 build ID의 앞 16 byte를 복사한다.
 * @param digest caller-owned output buffer, exactly 16 bytes.
 * @return `CANVIEW_OK` 또는 null output.
 *
 * GNU linker SHA-1 build ID를 식별 용도로만 사용한다. 서명·인증이나 최종 BIN의
 * SHA-256을 대체하지 않는다. boot/device identity와는 독립적이다.
 */
canview_status_t canview_stm_build_id_digest(
    uint8_t digest[CANVIEW_STM_BUILD_ID_DIGEST_SIZE]);

#endif
