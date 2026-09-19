/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_boot_runtime.h @brief STM boot 단일 실행의 시간과 IWDG 계약. */
#ifndef CANVIEW_BOOT_RUNTIME_H
#define CANVIEW_BOOT_RUNTIME_H
#include "canview_status.h"
#include <stdbool.h>

/** @brief reset 진입의 HSI16/DWT와 IWDG를 준비한다. 재호출은 거절한다.
 * @return OK, 잘못된 실행 context/clock RESOURCE_BUSY, 준비 실패 TIMEOUT.
 * @details SDK startup/copy 뒤 privileged MSP main에서 한 번만 호출한다.
 * 먼저 BSP safe output을 설정해야 하며 CAN/UART/DMA/IRQ를 시작하면 안 된다.
 * PLL/HSE/option-byte를 변경하지 않는다. IWDG는 prescaler256/reload4095,
 * window4095인 reset 상태만 허용하며 실제 LSI 주파수/timeout은 HIL에서 측정해야 한다.
 * 준비 실패 후 같은 boot에서 재시도하지 않는다. 시작된 IWDG는 끄지 않는다.
 */
canview_status_t canview_boot_runtime_start(void);

/** @brief 같은 boot의 시간/clock/초기화 상태가 아직 유효한지 검사한다.
 * @return 시작 전, ISR/다른 context, clock/DWT/IWDG 설정 변경, counter 정지,30초 만료 시 false.
 * @details watchdog을 갱신하지 않는다. 실패는 해당 boot 동안 latch된다.
 * boot_go 성공 후에도 정책/서명/앱 vector 검사와 이 검사를 거친 뒤에만 handoff한다.
 * 이 함수 자체는 부팅 승인이나 이미지 검증이 아니다. 단일 main owner 전용이다.
 * 30초는 HSI16 nominal의 초기 boot budget이며 실측 수용값이 아니다.
 * debug halt/clock 고장에 독립적인 시간 증거는 아니며 hardware IWDG 실측이 필요하다.
 */
bool canview_boot_runtime_ready(void);

#endif
