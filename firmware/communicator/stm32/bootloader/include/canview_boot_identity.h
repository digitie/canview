/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_boot_identity.h @brief 부트 전용 신뢰된 identity 공급 계약. */
#ifndef CANVIEW_BOOT_IDENTITY_H
#define CANVIEW_BOOT_IDENTITY_H
#include "native_metadata.h"

/** @brief BSP/provisioning에서 고정 identity와 지원 STM ABI를 읽는다.
 * @param identity 성공 시 고정 role/board/layout/제조 epoch. NULL 불가.
 * @param abi 성공 시 지원하는 STM image ABI. NULL 불가.
 * @return OK 또는 미확인/손상/IO 오류. 미확인 값을 기본값으로 허용하지 않는다.
 * @details boot 단일 owner가 동기 호출하며 pointer를 보존하지 않는다. 호출 중
 * 입력 image/HTTP/manifest로 기대값을 구성하거나 수정하면 안 된다. 실제 BSP
 * 구현 없이 제품 loader를 link할 수 없다. host 구현은 시험 파일에만 둔다.
 * key_id는 서명키 선택을 대체하지 않으며 MCUboot 공개키 검증은 별도로 필수다.
 */
canview_status_t canview_boot_identity_read(canview_ota_identity_t *identity, uint32_t *abi);
#endif
