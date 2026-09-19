/* SPDX-License-Identifier: GPL-3.0-only */
/** @file flash_read.c @brief G474 DBANK1 guarded read. NMI에서 복구/erase하지 않는다. */
#include "canview_stm_flash_read.h"
#include "canview_boot_flash.h"
#include "flash_layout.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#if defined(CANVIEW_STM_REGISTER_TEST)
#if defined(__arm__) || defined(__thumb__)
#error Host_register_model_must_not_be_built_for_target
#endif
#include "register_model.h"
#define READ_RAM __attribute__((noinline))
#define READ_WORD(address) canview_stm_flash_test_load(address)
#define ECC_CLEAR(flags) canview_stm_flash_test_ecc_clear(flags)
#define NMI_POLL() canview_stm_flash_test_poll()
#else
#include "stm32g474xx.h"
#define READ_RAM __attribute__((section(".canview_flash_read_ram"), noinline))
#define READ_WORD(address) (*(volatile const uint32_t *)(uintptr_t)(address))
#define ECC_CLEAR(flags) (FLASH->ECCR = (flags))
#define NMI_POLL() ((void)0)
#endif

#define READ_VECTOR_COUNT (128U)
#define READ_SRAM_BEGIN (UINT32_C(0x20000000))
#define READ_SRAM_END (UINT32_C(0x20018000))
#define READ_CACHE_ENABLE (FLASH_ACR_ICEN | FLASH_ACR_DCEN)
#define READ_CACHE_RESET (FLASH_ACR_ICRST | FLASH_ACR_DCRST)
#define READ_ECC_FLAGS (FLASH_ECCR_ECCC | FLASH_ECCR_ECCD)
#define READ_ECC_RESERVED (FLASH_ECCR_ECCC2 | FLASH_ECCR_ECCD2)
#define READ_OTHER_NMI (RCC_CIFR_CSSF | RCC_CIFR_LSECSSF)
#define READ_NMI_POLL_LIMIT (32U)
#define READ_RDP_DEVELOPMENT (UINT32_C(0xaa))

/* VTOR가 이 객체의 첫 field를 가리킨다. NMI 공유 flag만 volatile이며 전역 상태는 없다. */
typedef struct
{
    uintptr_t vectors[READ_VECTOR_COUNT];
    volatile uint32_t armed;
    volatile uint32_t failed;
} read_context_t;

static READ_RAM __attribute__((noreturn)) void read_fault_reset(void)
{
#if defined(CANVIEW_STM_REGISTER_TEST)
    canview_stm_flash_test_fatal();
#else
    __DSB();
    SCB->AIRCR = (UINT32_C(0x5fa) << SCB_AIRCR_VECTKEY_Pos) |
        (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) | SCB_AIRCR_SYSRESETREQ_Msk;
    __DSB();
    for (;;) { __NOP(); }
#endif
}

static READ_RAM void read_nmi(void)
{
    read_context_t *const context = (read_context_t *)(uintptr_t)SCB->VTOR;
    const uint32_t flags = FLASH->ECCR;
    if (context->armed != 1U || (flags & FLASH_ECCR_ECCD) == 0U ||
        (flags & (READ_ECC_RESERVED | FLASH_ECCR_ECCIE)) != 0U ||
        (RCC->CIFR & READ_OTHER_NMI) != 0U || (SYSCFG->CFGR2 & SYSCFG_CFGR2_SPF) != 0U)
    {
        read_fault_reset();
    }
    context->failed = 1U;
    ECC_CLEAR(flags & READ_ECC_FLAGS);
    __DSB();
    if ((FLASH->ECCR & READ_ECC_FLAGS) != 0U) { read_fault_reset(); }
}

static READ_RAM canview_status_t read_execute(uint32_t address, uint8_t *stage,
    uint32_t length, read_context_t *context)
{
    const uint32_t acr = FLASH->ACR;
    const uint32_t disabled = acr & ~(uint32_t)(READ_CACHE_ENABLE | READ_CACHE_RESET);
    FLASH->ACR = disabled;
    FLASH->ACR = disabled | READ_CACHE_RESET;
    FLASH->ACR = disabled;
    __DSB();
    __ISB();
    context->armed = 1U;
    uint32_t copied = 0U;
    while (copied < length)
    {
        const uint32_t current = address + copied;
        const uint32_t word = READ_WORD(current & ~UINT32_C(3));
        __DSB();
        __ISB();
        /* NMI를 thread에서 clear하지 않는다. 전달 지연은 유한하게 기다린다. */
        uint32_t polls = 0U;
        while ((FLASH->ECCR & FLASH_ECCR_ECCD) != 0U)
        {
            if (++polls >= READ_NMI_POLL_LIMIT) { read_fault_reset(); }
            NMI_POLL();
        }
        const uint32_t flags = FLASH->ECCR;
        if ((flags & READ_ECC_RESERVED) != 0U) { read_fault_reset(); }
        if ((flags & FLASH_ECCR_ECCC) != 0U)
        {
            context->failed = 1U;
            ECC_CLEAR(FLASH_ECCR_ECCC);
            __DSB();
            if ((FLASH->ECCR & READ_ECC_FLAGS) != 0U) { read_fault_reset(); }
        }
        if (context->failed != 0U) { break; }
        for (uint32_t byte = current & UINT32_C(3); byte < 4U && copied < length; ++byte)
        {
            stage[copied++] = (uint8_t)(word >> (byte * 8U));
        }
    }
    context->armed = 0U;
    FLASH->ACR = disabled | READ_CACHE_RESET;
    FLASH->ACR = disabled;
    FLASH->ACR = acr;
    __DSB();
    __ISB();
    return context->failed == 0U ? CANVIEW_OK : CANVIEW_INCOMPLETE;
}

static bool read_ram_ready(uintptr_t table, const void *destination, uint32_t length)
{
#if defined(CANVIEW_STM_REGISTER_TEST)
    (void)table;
    (void)destination;
    (void)length;
    return canview_stm_flash_test_ram_ready();
#else
    const uintptr_t execute = (uintptr_t)read_execute & ~(uintptr_t)1U;
    const uintptr_t nmi = (uintptr_t)read_nmi & ~(uintptr_t)1U;
    const uintptr_t fatal = (uintptr_t)read_fault_reset & ~(uintptr_t)1U;
    const uintptr_t output = (uintptr_t)destination;
    return execute >= READ_SRAM_BEGIN && execute < READ_SRAM_END &&
        nmi >= READ_SRAM_BEGIN && nmi < READ_SRAM_END &&
        fatal >= READ_SRAM_BEGIN && fatal < READ_SRAM_END &&
        table >= READ_SRAM_BEGIN && table <= READ_SRAM_END - sizeof(read_context_t) &&
        output >= READ_SRAM_BEGIN && output <= READ_SRAM_END - length;
#endif
}

canview_status_t canview_stm_flash_read(uint32_t address, void *destination, uint32_t length)
{
    if (destination == NULL || length == 0U || length > CANVIEW_STM_FLASH_READ_MAX)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (address < CANVIEW_STM_PRIMARY_ADDRESS || address > CANVIEW_STM_POLICY_A_ADDRESS - length)
    {
        return CANVIEW_AUTH_FAILED;
    }
    if (__get_IPSR() != 0U || __get_CONTROL() != 0U) { return CANVIEW_RESOURCE_BUSY; }
    read_context_t context __attribute__((aligned(512)));
    uint8_t stage[CANVIEW_STM_FLASH_READ_MAX];
    if (!read_ram_ready((uintptr_t)&context, destination, length)) { return CANVIEW_RESOURCE_BUSY; }
    for (size_t index = 0U; index < READ_VECTOR_COUNT; ++index)
    {
        context.vectors[index] = (uintptr_t)read_fault_reset;
    }
    context.vectors[0] = __get_MSP();
    context.vectors[2] = (uintptr_t)read_nmi;
    context.armed = 0U;
    context.failed = 0U;
    const uint32_t mask = __get_PRIMASK();
    __disable_irq();
    canview_status_t result = CANVIEW_RESOURCE_BUSY;
    if (canview_boot_flash_check() == 0 && FLASH->CR == (FLASH_CR_LOCK | FLASH_CR_OPTLOCK) &&
        (FLASH->OPTR & FLASH_OPTR_RDP) == READ_RDP_DEVELOPMENT &&
        (FLASH->SR & ~(uint32_t)FLASH_SR_EOP) == 0U &&
        (FLASH->ACR & READ_CACHE_RESET) == 0U &&
        (FLASH->ECCR & (READ_ECC_FLAGS | READ_ECC_RESERVED | FLASH_ECCR_ECCIE)) == 0U &&
        (RCC->CIFR & READ_OTHER_NMI) == 0U && (SYSCFG->CFGR2 & SYSCFG_CFGR2_SPF) == 0U)
    {
        const uintptr_t old_vector = SCB->VTOR;
        SCB->VTOR = (uintptr_t)&context;
        __DSB();
        __ISB();
        result = read_execute(address, stage, length, &context);
        SCB->VTOR = old_vector;
        __DSB();
        __ISB();
    }
    __set_PRIMASK(mask);
    if (result == CANVIEW_OK) { memcpy(destination, stage, length); }
    return result;
}
