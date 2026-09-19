/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_flash_read.h @brief Boot 전용 bounded Flash/ECC read. */
#ifndef CANVIEW_STM_FLASH_READ_H
#define CANVIEW_STM_FLASH_READ_H
#include "canview_status.h"
#include <stdint.h>

#define CANVIEW_STM_FLASH_READ_MAX (256U)

/** @brief primary/secondary에서1..256B를 읽는다. unaligned byte 범위를 허용한다.
 * @param address 절대 Flash 주소. 슬롯 밖/overflow는 거절한다.
 * @param destination 호출자가 소유한 length B의 SRAM1/2 buffer. NULL 불가.
 * @param length 1..CANVIEW_STM_FLASH_READ_MAX.
 * @return OK일 때만 destination을 갱신한다. 잘못된 입력 INVALID_ARGUMENT,
 * 영역 밖 AUTH_FAILED, 환경 미준비/기존 오류 RESOURCE_BUSY, 읽기 중 ECC INCOMPLETE.
 * @details 단일 boot privileged MSP owner 전용. ISR/RTOS/reentry 금지. DMA와 다른
 * Flash 사용자를 멈추고 .canview_flash_read_ram을 SRAM1/2로 복사한 뒤 호출한다.
 * 함수가 임시 vector/PRIMASK/cache를 소유하고 반환 전에 복원한다. RDP0 개발
 * profile만 허용한다. 생산 보호 조합은 별도 수용 대상이다. NMI는 guarded load의
 * ECCD만 실패로 바꾼다. 다른 NMI/HardFault/flag 해제 실패는 reset/fail-stop.
 * ECCR 주소를 erase 선택에 쓰지 않는다. erase/서명/floor/복구 정책을 수행하지 않으며
 * 실제 ECC/NMI latency와 reset/power-cut 수용은 별도 HIL gate다.
 */
canview_status_t canview_stm_flash_read(uint32_t address, void *destination, uint32_t length);
#endif
