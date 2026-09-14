/* SPDX-License-Identifier: GPL-3.0-only */
/** @file cbor_document.h
 * @brief OTA manifest용 deterministic CBOR 문서의 구조 검증.
 */
#ifndef CANVIEW_OTA_CBOR_DOCUMENT_H
#define CANVIEW_OTA_CBOR_DOCUMENT_H

#include <stddef.h>
#include <stdint.h>
#include "canview_status.h"

#define CANVIEW_OTA_CBOR_MAX_BYTES (16384U)
#define CANVIEW_OTA_CBOR_MAX_DEPTH (8U)
#define CANVIEW_OTA_CBOR_MAX_ITEMS (2048U)

/** @brief root map 한 개를 가진 OTA CBOR profile을 검사한다.
 * @param data 호출 중만 빌리는 불변 입력. NULL 불가, 저장하지 않는다.
 * @param length 실제 입력 길이. 0은 INCOMPLETE, 16 KiB 초과는 OVERSIZE.
 * @return OK, INVALID_ARGUMENT, INCOMPLETE, MALFORMED, DUPLICATE,
 * OVERSIZE 또는 UNSUPPORTED_MESSAGE.
 * @details UINT/BYTES/TEXT/ARRAY/MAP만 허용한다. 모든 map key는 uint64이며
 * 엄격한 오름차순이다. 같은 key는 DUPLICATE, 역순은 MALFORMED다. 정수와 길이는
 * 최소 인코딩이고 definite length만 허용한다. TEXT는 Unicode scalar UTF-8이다.
 * container depth는 root map=1, 최대8이며 key/value/container 각각을 세어 최대
 * 2048 item이다. 뒤의 다른 item/trailing byte도 거부한다. 힙/재귀/callback/전역
 * 가변 상태가 없고 실행량은 입력 byte+item 수에 선형이다. 독립 호출은 thread-safe.
 * 이는 구조 검증만이며 key 의미/required field/서명/role/image/policy를 검증하지
 * 않는다. OK가 erase/write/설치 권한 또는 PREPARED 상태를 뜻하지 않는다.
 */
canview_status_t canview_ota_cbor_validate(const uint8_t *data, size_t length);

#endif
