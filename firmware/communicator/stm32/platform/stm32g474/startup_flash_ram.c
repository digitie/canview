/* SPDX-License-Identifier: GPL-3.0-only */
/** @file startup_flash_ram.c @brief SDK data copy 뒤 SRAM 명령 실행을 동기화한다. */
#if !defined(__arm__) && !defined(__thumb__)
#error STM32_flash_startup_is_target_only
#endif
#include "stm32g474xx.h"

/* SDK Reset_Handler: SystemInit -> data/code copy -> bss -> preinit -> main.
 * boot 전용 linker와 함께 사용한다. 새 ISR/heap/복사 루프를 만들지 않는다.
 * 초기화 전 SRAM 코드 호출, DMA/IRQ 시작, 다른 entry로 main 직접 호출은 금지한다. */
static void flash_ram_sync(void)
{
    __DSB();
    __ISB();
}

static void (*const flash_ram_preinit)(void)
    __attribute__((section(".preinit_array"), used)) = flash_ram_sync;
