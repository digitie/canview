/* SPDX-License-Identifier: GPL-3.0-only */
/** @file startup_ram.c @brief ES0430 Rev9 §2.2.7: 첫 stack/data 쓰기 전 SRAM read. */
#if !defined(__arm__) && !defined(__thumb__)
#error STM32_startup_wrapper_is_target_only
#endif

/** @brief SDK Reset_Handler의 첫 SystemInit 호출을 linker --wrap으로 받는다.
 * C prologue의 stack 쓰기를 피하는 naked wrapper다. SRAM1/2의 네 cut만 읽고
 * 원래 SDK SystemInit으로 tail branch하므로 LR와 SDK 초기화 순서를 유지한다.
 * CCM parity 초기화는 별도이며 현재 ELF에서 CCM 사용량0을 검증한다.
 */
void __wrap_SystemInit(void);

__attribute__((naked)) void __wrap_SystemInit(void)
{
    __asm__(
        "ldr r2, =0x20000000\n"
        "ldr r3, [r2]\n"
        "add.w r2, r2, #0x8000\n"
        "ldr r3, [r2]\n"
        "add.w r2, r2, #0x8000\n"
        "ldr r3, [r2]\n"
        "add.w r2, r2, #0x4000\n"
        "ldr r3, [r2]\n"
        "dsb\n"
        "b __real_SystemInit\n");
}
