/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_boot_handoff.h @brief 승인된 primary의 STM32 앱 진입 계약. */
#ifndef CANVIEW_BOOT_HANDOFF_H
#define CANVIEW_BOOT_HANDOFF_H
#include "canview_status.h"
#include <stdint.h>

/** @brief 고정 primary vector를 ECC 확인 후 앱으로 진입한다. 성공하면 반환하지 않는다.
 * @param payload_bytes MCUboot가 서명 검증한 primary header의 payload 길이.
 * @return 잘못된 길이 INVALID_ARGUMENT, 잘못된 vector AUTH_FAILED,
 * runtime/context 미준비 RESOURCE_BUSY, 읽기 실패는 Flash read 오류. OK 반환은 없다.
 * @details 단일 boot privileged MSP main 전용. boot_go의 FIH 성공과 T-205 정책
 * 승인 이후에만 호출한다. 이 함수는 서명/floor/activation 검증을 대신하지 않는다.
 * 호출자는 같은 image의 authenticated 길이를 전달하고 이후 Flash 쓰기·DMA·ISR·
 * callback 접근을 금지한다. 임의 vector 주소나 secondary 진입을 받지 않는다.
 * reset PC는 vector 뒤이며 최초32bit Thumb-2 fetch까지 payload 안이어야 한다.
 * BSP safe output, reset HSI16, 실행 중 IWDG와 SRAM Flash read가 준비되어야 한다.
 * 앱 core_hw와 함께 link하지 않는다. MPU 활성 또는 lazy FPU 상태는 거절한다.
 * 실패 시 handoff register를 바꾸지 않는다. 기존 read/runtime의 오류 latch는 유지한다.
 * 성공 경로는 SysTick/NVIC pending을 정리하고 VTOR/MSP를 바꾼다. IRQ mask는0으로
 * 복원하되 NVIC enable은 모두0이며 앱이 다시 설정한다. IWDG는 중단/갱신하지 않는다.
 * 실제 NMI/reset timing과 앱 진입은 별도 HIL gate이며 source/host 검증은 대체 증거가 아니다.
 */
canview_status_t canview_boot_handoff(uint32_t payload_bytes);

#endif
