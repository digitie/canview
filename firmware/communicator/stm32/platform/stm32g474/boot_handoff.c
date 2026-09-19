/* SPDX-License-Identifier: GPL-3.0-only */
/** @file boot_handoff.c @brief G474 primary vector 검사와 최소 Cortex-M 앱 진입. */
#include "canview_boot_handoff.h"
#include "canview_boot_runtime.h"
#include "canview_stm_flash_read.h"
#include "flash_layout.h"
#include <stdbool.h>
#include <stddef.h>
#if defined(CANVIEW_STM_REGISTER_TEST)
#if defined(__arm__) || defined(__thumb__)
#error Host_register_model_must_not_be_built_for_target
#endif
#include "register_model.h"
#define BOOT_BRANCH(stack, entry) canview_stm_handoff_test_branch(stack, entry)
#else
#include "stm32g474xx.h"
/* MSP 교체 이후 C prologue/epilogue/stack 접근이 없어야 한다. 실제 ELF에서 검사한다. */
__attribute__((naked, noreturn, noinline)) static void boot_branch(
    uint32_t stack __attribute__((unused)), uint32_t entry __attribute__((unused)))
{
    __asm__(
        "mvn lr, #0\n"
        "msr msp, r0\n"
        "isb\n"
        "cpsie i\n"
        "bx r1\n");
}
#define BOOT_BRANCH(stack, entry) boot_branch(stack, entry)
#endif

#define BOOT_VECTOR_WORD_BYTES (4U)
#define BOOT_VECTOR_PREFIX_BYTES (2U * BOOT_VECTOR_WORD_BYTES)
#define BOOT_THUMB_BIT (UINT32_C(1))
/* Thumb-2 최초 명령이32bit여도 authenticated payload 밖을 fetch하지 않는다. */
#define BOOT_ENTRY_FETCH_BYTES (4U)

static uint32_t handoff_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) |
        ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}

static bool handoff_ready(void)
{
    return canview_boot_runtime_ready() && MPU->CTRL == 0U &&
        (FPU->FPCCR & FPU_FPCCR_LSPACT_Msk) == 0U;
}

canview_status_t canview_boot_handoff(uint32_t payload_bytes)
{
    if (payload_bytes < CANVIEW_STM_PRIMARY_VECTOR_BYTES + BOOT_ENTRY_FETCH_BYTES ||
        payload_bytes > CANVIEW_STM_APP_PAYLOAD_MAX_BYTES)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (!handoff_ready()) { return CANVIEW_RESOURCE_BUSY; }
    uint8_t prefix[BOOT_VECTOR_PREFIX_BYTES];
    const canview_status_t status = canview_stm_flash_read(CANVIEW_STM_PRIMARY_VECTOR, prefix, sizeof(prefix));
    if (status != CANVIEW_OK) { return status; }
    const uint32_t stack = handoff_u32(prefix);
    const uint32_t entry = handoff_u32(prefix + BOOT_VECTOR_WORD_BYTES);
    const uint32_t pc = entry & ~BOOT_THUMB_BIT;
    if (stack != CANVIEW_STM_APP_STACK_TOP || (entry & BOOT_THUMB_BIT) == 0U ||
        pc < CANVIEW_STM_PRIMARY_VECTOR + CANVIEW_STM_PRIMARY_VECTOR_BYTES ||
        pc > CANVIEW_STM_PRIMARY_VECTOR + payload_bytes - BOOT_ENTRY_FETCH_BYTES)
    {
        return CANVIEW_AUTH_FAILED;
    }
    /* ECC read가 복원한 context와 전체 boot budget을 마지막으로 검사한다. */
    if (!handoff_ready()) { return CANVIEW_RESOURCE_BUSY; }
    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    for (size_t index = 0U; index < sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]); ++index)
    {
        NVIC->ICER[index] = UINT32_MAX;
        NVIC->ICPR[index] = UINT32_MAX;
    }
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    __DSB();
    SCB->VTOR = CANVIEW_STM_PRIMARY_VECTOR;
    __DSB();
    __ISB();
    /* CONTROL/BASEPRI/FAULTMASK는 runtime이0임을 검사했다. clock/IWDG는 유지한다. */
    BOOT_BRANCH(stack, entry);
}
