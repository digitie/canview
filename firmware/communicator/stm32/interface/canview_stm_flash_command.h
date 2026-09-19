/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_flash_command.h @brief Boot 전용 G474 단일 Flash 명령. */
#ifndef CANVIEW_STM_FLASH_COMMAND_H
#define CANVIEW_STM_FLASH_COMMAND_H
#include "canview_stm_flash_layout.h"

/** @brief primary/secondary의 한 page erase 또는 한 doubleword program.
 * @param operation PROGRAM 또는 ERASE. READ/다른 enum은 거절한다.
 * @param address 절대 주소. ERASE는 2KiB, PROGRAM은8B 정렬이어야 한다.
 * @param low PROGRAM의 낮은32bit. ERASE에서는0이어야 한다.
 * @param high PROGRAM의 높은32bit. ERASE에서는0이어야 한다.
 * @return 잘못된 입력 INVALID_ARGUMENT, 보호/profile AUTH_FAILED, 준비 미완료
 * RESOURCE_BUSY, 중복 DUPLICATE, Flash 오류 INCOMPLETE, 성공 OK. busy timeout/예외는 SRAM에서
 * reset 요청 후 반환하지 않는다. 진행 중인 Flash에서 안전하게 return할 수 없기 때문이다.
 *
 * Boot 단일 privileged thread/MSP owner 전용. ISR/RTOS/callback/reentry 금지.
 * 시작 전 DMA/주변장치와 다른 Flash 사용자를 정지하고 IWDG를 가동해야 한다.
 * 이 함수는 IWDG를 끄거나 feed하지 않는다. DWT cycle counter가 먼저 켜져 있어야 한다.
 * .canview_flash_ram의 code/literal을 SRAM1/2로 복사한 뒤 호출한다. 임시 SRAM vector와
 * PRIMASK는 함수가 소유하며 정상/오류 반환 때 복원한다. NMI/HardFault는 reset만 한다.
 * RDP0 개발 환경만 허용한다. option-byte 변경이나 생산 보호 승인 API가 아니다.
 * PROGRAM은 read-back상 erased인 word에만 허용하고 all-FF 입력은 거절한다.
 * 상위 backend가 all-FF를 skip하고 fresh erase/중복/ECC/유일한 정상본을 관리해야 한다.
 * 이 primitive만으로 torn-word 복구나 MCUboot adapter IO가 완성되는 것은 아니다.
 */
canview_status_t canview_stm_flash_command(canview_stm_flash_operation_t operation,
    uint32_t address, uint32_t low, uint32_t high);
#endif
