/* SPDX-License-Identifier: GPL-3.0-only */
/** @file ota.h @brief Communicator staging의 읽기 전용 ESP image 검사 연결. */
#ifndef CANVIEW_COMM_OTA_H
#define CANVIEW_COMM_OTA_H
#include "manifest.h"

/** @brief 고정 bundle_stage에서 SDK 검증 뒤 공통 metadata를 대조한다.
 * @param identity 신뢰된 로컬 provisioning identity. HTTP/package 입력 금지.
 * @param expected 인증된 manifest의 COMM_ESP descriptor. offset은 bundle 상대값.
 * @return OK, INVALID_ARGUMENT(NULL), AUTH_FAILED(identity/native 오류),
 * INCOMPLETE(staging 없음), MALFORMED(partition/offset), RESOURCE_BUSY(SDK 메모리 부족),
 * NOT_IMPLEMENTED(서명 설정 미지원).
 * @details synchronous/read-only. caller는 모든 SDK OTA/mmap 호출을 단일 task에서
 * 직렬화하고 함수 종료까지 staging bytes·인자를 불변으로 보장해야 한다. ISR/재진입 금지.
 * board ID와 실제 staging 위치/크기를 generated BSP 계약에 결합한다. 외부 주소나
 * partition pointer를 받지 않는다. 반환 metadata pointer를 보존하지 않는다.
 * 이 함수는 mutex/writer/부팅 선택/영속 floor를 만들지 않으며 OK는 설치 승인이 아니다.
 * 정상 OTA task/Flash owner 연결 전에는 read-only build/test 경로로만 사용한다.
 */
canview_status_t canview_comm_ota_image_check(const canview_ota_identity_t *identity,
    const canview_ota_image_t *expected);
#endif
