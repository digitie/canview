/* SPDX-License-Identifier: GPL-3.0-only */
/** @file cbor_head.h
 * @brief OTA 내부 CBOR head primitive. 완전한 manifest/서명 검증기가 아니다.
 */
#ifndef CANVIEW_OTA_CBOR_HEAD_H
#define CANVIEW_OTA_CBOR_HEAD_H

#include <stddef.h>
#include <stdint.h>
#include "canview_status.h"

/** @brief RFC 8949 major type 중 OTA manifest가 사용할 형식. */
typedef enum
{
    CANVIEW_OTA_CBOR_UINT = 0,
    CANVIEW_OTA_CBOR_BYTES = 2,
    CANVIEW_OTA_CBOR_TEXT = 3,
    CANVIEW_OTA_CBOR_ARRAY = 4,
    CANVIEW_OTA_CBOR_MAP = 5
} canview_ota_cbor_type_t;

/** @brief 해석한 head. 문자열 본문이나 child item은 포함하지 않는다. */
typedef struct
{
    uint64_t argument; /**< 정수 값 또는 문자열/collection 길이. */
    size_t encoded_size; /**< head만의 길이, 성공 시 1~9 byte. */
    canview_ota_cbor_type_t type; /**< 지원하는 major type. */
} canview_ota_cbor_head_t;

/** @brief head 한 개를 최소 길이 definite CBOR로 해석한다.
 * @param data 호출 중만 빌리는 입력. NULL 불가, out과 겹치면 안 된다.
 * @param length 실제 읽을 수 있는 입력 길이. 0은 INCOMPLETE다.
 * @param out 성공 결과. 실패 시 세 field 모두 0이며 NULL은 INVALID_ARGUMENT.
 * @return OK, INVALID_ARGUMENT, INCOMPLETE, MALFORMED 또는 UNSUPPORTED_MESSAGE.
 * @details unsigned/bytes/text/array/map만 지원한다. reserve/indefinite length와
 * 비최소 인코딩은 거부한다. 음수/tag/float/simple은 지원하지 않는다. 본문 길이,
 * UTF-8, map 순서/중복, depth, schema, 서명은 상위 parser의 별도 책임이다.
 * 힙/전역 상태/입력 보존/callback/재귀 없음, 최대 9 byte 읽기. 독립 입력/output에
 * thread-safe하며 실패 후 동일 입력의 더 긴 prefix로 다시 호출할 수 있다.
 * OK는 Flash 쓰기·install·boot selector 변경 권한이 아니다.
 */
canview_status_t canview_ota_cbor_read_head(const uint8_t *data, size_t length,
                                         canview_ota_cbor_head_t *out);

#endif
