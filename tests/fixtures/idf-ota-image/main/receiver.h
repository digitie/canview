/* SPDX-License-Identifier: GPL-3.0-only */
/** @file receiver.h @brief 공개 합성 golden의 SDK/CNG 수신 시험. 설치 기능이 아니다. */
#ifndef CANVIEW_OTA_FIXTURE_RECEIVER_H
#define CANVIEW_OTA_FIXTURE_RECEIVER_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief 합성 identity/floor로 정상·서명 변조·본문 변조·identity 오류를 검사한다.
 * @details 단일 task가 한 번만 호출한다. 입력은 호출 내내 불변이며 보존하지 않는다.
 * 내부 static storage는 cleanup 실패 때도 유지된다. Flash/부팅 선택/native 검증 없음.
 * true는 시험 기대값 일치이며 제품 identity/provisioning/설치 권한이 아니다.
 */
bool canview_ota_fixture_receiver_test(const uint8_t *data, size_t size);
#endif
