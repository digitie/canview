/* SPDX-License-Identifier: GPL-3.0-only */
/** @file startup_ram.c @brief ES0430 Rev9 §2.2.7: SRAM parity를 보존하는 초기 접근. */
#include <stdint.h>
#if !defined(__arm__) && !defined(__thumb__)
#error STM32_startup_wrapper_is_target_only
#endif

/* 첫 write가 소실되어도 되는 전용 word. linker가 SRAM1 시작에4B를 예약한다.
 * 동작 상태가 아니며 C 코드/ISR에서 접근하지 않는다. */
static uint32_t startup_sram_dummy __attribute__((section(".startup_sram_dummy"), used));

/** @brief SDK Reset_Handler의 첫 SystemInit 호출을 linker --wrap으로 받는다.
 * SRAM1 첫 cut은 parity 활성 가능성이 있어 전용 dummy word를 두 번 초기화한다.
 * 첫 write 손실은 허용하되 DSB 뒤 두 번째 write가 data/parity를 확정한다.
 * 나머지 세 cut은 parity가 없으므로 read로 준비한다. C prologue 없이
 * 원래 SDK SystemInit으로 tail branch하므로 LR와 SDK 초기화 순서를 유지한다.
 * CCM parity 초기화는 별도이며 현재 ELF에서 CCM 사용량0을 검증한다.
 */
void __wrap_SystemInit(void);

__attribute__((naked)) void __wrap_SystemInit(void)
{
    __asm__(
        "ldr r2, =0x20000000\n"
        "movs r3, #0\n"
        "str r3, [r2]\n"
        "dsb\n"
        "str r3, [r2]\n"
        "dsb\n"
        "add.w r2, r2, #0x8000\n"
        "ldr r3, [r2]\n"
        "add.w r2, r2, #0x8000\n"
        "ldr r3, [r2]\n"
        "add.w r2, r2, #0x4000\n"
        "ldr r3, [r2]\n"
        "dsb\n"
        "b __real_SystemInit\n");
}
