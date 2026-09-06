/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_status.h
 * @brief 공용 반환 상태. 통신 ACK의 wire status 값으로 직접 cast하지 않는다.
 */
#ifndef CANVIEW_STATUS_H
#define CANVIEW_STATUS_H

/** @brief 성공/입력 손상/미구현을 분리한 local 상태. */
typedef enum
{
    CANVIEW_OK = 0,
    CANVIEW_INVALID_ARGUMENT,
    CANVIEW_BUFFER_TOO_SMALL,
    CANVIEW_MALFORMED,
    CANVIEW_UNSUPPORTED_VERSION,
    CANVIEW_UNSUPPORTED_MESSAGE,
    CANVIEW_CRC_MISMATCH,
    CANVIEW_OVERSIZE,
    CANVIEW_INCOMPLETE,
    CANVIEW_DUPLICATE,
    CANVIEW_STALE,
    CANVIEW_NOT_IMPLEMENTED,
    CANVIEW_AUTH_FAILED,
    CANVIEW_RESOURCE_BUSY,
    CANVIEW_TIMEOUT
} canview_status_t;

#endif
