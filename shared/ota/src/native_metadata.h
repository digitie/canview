/* SPDX-License-Identifier: GPL-3.0-only */
/** @file native_metadata.h @brief ESP/STM의 동일 CANView signed metadata 계약. */
#ifndef CANVIEW_OTA_NATIVE_METADATA_H
#define CANVIEW_OTA_NATIVE_METADATA_H
#include "manifest.h"

#define CANVIEW_OTA_NATIVE_METADATA_BYTES (168U)
#define CANVIEW_OTA_ESP_VERSION_BYTES (32U)

/** @brief native 서명으로 보호된 고정 metadata를 신뢰된 기대값과 대조한다.
 * @param data size만큼 읽을 수 있는 불변 metadata. NULL 불가.
 * @param size 정확히168byte. 구조체 cast 없이 little-endian으로 읽는다.
 * @param identity 신뢰된 BSP identity. 파일/웹에서 구성하지 않는다.
 * @param expected 서명 검증된 manifest image descriptor. NULL 불가.
 * @return OK, INVALID_ARGUMENT(NULL), MALFORMED(길이), AUTH_FAILED(불일치).
 * @details role/target/signature 조합과 board/layout/epoch/ABI/u64 sequence를 검사한다.
 * 무힙/무I/O/무상태이며 모든 입력은 호출 중 불변, pointer를 보존하지 않는다.
 * 성공은 metadata 일치일 뿐 native 서명·hash·floor·설치 승인이 아니다.
 */
canview_status_t canview_ota_native_metadata_check(const uint8_t *data, size_t size,
    const canview_ota_identity_t *identity, const canview_ota_image_t *expected);

/** @brief ESP SDK가 검증한 app version과 custom metadata를 대조한다.
 * @param data SDK가 반환한 signed custom metadata. NULL 불가.
 * @param size 정확히168byte.
 * @param version SDK app descriptor의32byte version 배열. NULL 불가.
 * @param identity 신뢰된 BSP identity.
 * @param expected 서명 검증된 ESP manifest descriptor.
 * @return metadata 오류 또는 AUTH_FAILED(ESP target/version 불일치), OK.
 * @details version은1..31 printable ASCII와 NUL/zero padding으로 정확히 대조한다.
 * 문자열로 release 순서를 비교하지 않는다. 순서는 signed u64 sequence다.
 * 앞선 SDK native 서명/전체 hash 성공은 caller가 보장해야 한다. raw 웹 metadata를
 * 검증 증거로 사용하지 않는다. 무상태/무I/O이며 입력 수명은 위 함수와 같다.
 */
canview_status_t canview_ota_esp_metadata_check(const uint8_t *data, size_t size,
    const char version[CANVIEW_OTA_ESP_VERSION_BYTES],
    const canview_ota_identity_t *identity, const canview_ota_image_t *expected);
#endif
