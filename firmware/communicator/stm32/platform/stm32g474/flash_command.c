/* SPDX-License-Identifier: GPL-3.0-only */
/** @file flash_command.c @brief Boot 전용 단일 명령. 제품 Flash backend는 별도 연결한다. */
#include "canview_stm_flash_command.h"
#include "canview_boot_flash.h"
#include "flash_layout.h"
#include <stdbool.h>
#include <stddef.h>
#if defined(CANVIEW_STM_REGISTER_TEST)
#if defined(__arm__) || defined(__thumb__)
#error Host_register_model_must_not_be_built_for_target
#endif
#include "register_model.h"
#define RAM_CODE __attribute__((noinline))
#define FLASH_LOAD(address) canview_stm_flash_test_load(address)
#define FLASH_STORE(address, value) canview_stm_flash_test_store(address, value)
#define FLASH_KEY(value) canview_stm_flash_test_key(value)
#define FLASH_CLEAR(value) canview_stm_flash_test_clear(value)
#define FLASH_START() canview_stm_flash_test_start()
#define FLASH_POLL() canview_stm_flash_test_poll()
#else
#include "stm32g474xx.h"
#define RAM_CODE __attribute__((section(".canview_flash_ram"), noinline))
#define FLASH_LOAD(address) (*(volatile const uint32_t *)(uintptr_t)(address))
#define FLASH_STORE(address, value) (*(volatile uint32_t *)(uintptr_t)(address) = (value))
#define FLASH_KEY(value) (FLASH->KEYR = (value))
#define FLASH_CLEAR(value) (FLASH->SR = (value))
#define FLASH_START() (FLASH->CR |= FLASH_CR_STRT)
#define FLASH_POLL() ((void)0)
#endif

#define FLASH_ERRORS (FLASH_SR_OPERR | FLASH_SR_PROGERR | FLASH_SR_WRPERR | \
    FLASH_SR_PGAERR | FLASH_SR_SIZERR | FLASH_SR_PGSERR | FLASH_SR_MISERR | \
    FLASH_SR_FASTERR | FLASH_SR_RDERR | FLASH_SR_OPTVERR)
#define FLASH_CACHE_ENABLE (FLASH_ACR_ICEN | FLASH_ACR_DCEN)
#define FLASH_CACHE_RESET (FLASH_ACR_ICRST | FLASH_ACR_DCRST)
#define FLASH_UNLOCK_FIRST (UINT32_C(0x45670123))
#define FLASH_UNLOCK_SECOND (UINT32_C(0xcdef89ab))
#define FLASH_RDP_DEVELOPMENT (UINT32_C(0xaa))
#define FLASH_BANK_BYTES (CANVIEW_STM_FLASH_BYTES / 2U)
#define FLASH_POLL_LIMIT (UINT32_C(17000000))
#define FLASH_CYCLE_LIMIT (UINT32_C(8500000))
#define FLASH_VECTOR_COUNT (128U)
#define FLASH_SRAM_BEGIN (UINT32_C(0x20000000))
#define FLASH_SRAM_END (UINT32_C(0x20018000))

/* Flash busy 상태에서는 Flash 코드로 돌아가거나 기존 vector를 읽지 않는다.
 * 이 함수 자체와 literal도 .canview_flash_ram에 있어야 한다. reset 실패 시
 * watchdog을 feed하지 않는 SRAM fail-stop이다. 예외에서 Flash/ECC 복구는 하지 않는다. */
static RAM_CODE __attribute__((noreturn)) void flash_fault_reset(void)
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

static RAM_CODE canview_status_t flash_execute(canview_stm_flash_operation_t operation,
    uint32_t address, uint32_t low, uint32_t high)
{
    const uint32_t acr = FLASH->ACR;
    const uint32_t disabled = acr & ~(uint32_t)(FLASH_CACHE_ENABLE | FLASH_CACHE_RESET);
    FLASH->ACR = disabled;
    FLASH->ACR = disabled | FLASH_CACHE_RESET;
    FLASH->ACR = disabled;
    __DSB();
    __ISB();
    canview_status_t result = CANVIEW_INCOMPLETE;
    if (operation == CANVIEW_STM_FLASH_PROGRAM &&
        (FLASH_LOAD(address) != UINT32_MAX || FLASH_LOAD(address + 4U) != UINT32_MAX))
    {
        result = CANVIEW_DUPLICATE;
    }
    else
    {
        FLASH_CLEAR(FLASH_SR_EOP);
        FLASH_KEY(FLASH_UNLOCK_FIRST);
        FLASH_KEY(FLASH_UNLOCK_SECOND);
        if ((FLASH->CR & FLASH_CR_LOCK) == 0U)
        {
            const uint32_t start = DWT->CYCCNT;
            if (operation == CANVIEW_STM_FLASH_PROGRAM)
            {
                FLASH->CR |= FLASH_CR_PG;
                FLASH_STORE(address, low);
                __ISB(); /* CubeG4 FLASH_Program_DoubleWord와 같은 순서 */
                FLASH_STORE(address + 4U, high);
            }
            else
            {
                const uint32_t offset = address - CANVIEW_STM_FLASH_BASE;
                const uint32_t page = (offset % FLASH_BANK_BYTES) / CANVIEW_STM_FLASH_PAGE_BYTES;
                FLASH->CR |= FLASH_CR_PER | (page << FLASH_CR_PNB_Pos) |
                    (offset >= FLASH_BANK_BYTES ? FLASH_CR_BKER : 0U);
                FLASH_START();
            }
            __DSB();
            uint32_t polls = 0U;
            while ((FLASH->SR & FLASH_SR_BSY) != 0U)
            {
                if (++polls >= FLASH_POLL_LIMIT || (uint32_t)(DWT->CYCCNT - start) >= FLASH_CYCLE_LIMIT)
                {
                    flash_fault_reset();
                }
                FLASH_POLL();
            }
            const uint32_t status = FLASH->SR;
            FLASH->CR &= ~(uint32_t)(FLASH_CR_PG | FLASH_CR_PER | FLASH_CR_PNB | FLASH_CR_BKER);
            FLASH_CLEAR(status & (FLASH_ERRORS | FLASH_SR_EOP));
            if ((status & (FLASH_ERRORS | FLASH_SR_EOP)) == FLASH_SR_EOP)
            {
                result = CANVIEW_OK;
            }
        }
        FLASH->CR |= FLASH_CR_LOCK;
    }
    /* 실패한 명령도 cache를 폐기한다. read-back은 다음 계층의 ECC-safe IO 책임이다. */
    FLASH->ACR = disabled | FLASH_CACHE_RESET;
    FLASH->ACR = disabled;
    FLASH->ACR = acr;
    __DSB();
    __ISB();
    return result;
}

static bool ram_ready(const uintptr_t *vectors)
{
#if defined(CANVIEW_STM_REGISTER_TEST)
    (void)vectors;
    return canview_stm_flash_test_ram_ready();
#else
    const uintptr_t code = (uintptr_t)flash_execute & ~(uintptr_t)1U;
    const uintptr_t fault = (uintptr_t)flash_fault_reset & ~(uintptr_t)1U;
    const uintptr_t table = (uintptr_t)vectors;
    return code >= FLASH_SRAM_BEGIN && code < FLASH_SRAM_END &&
        fault >= FLASH_SRAM_BEGIN && fault < FLASH_SRAM_END &&
        table >= FLASH_SRAM_BEGIN && table <= FLASH_SRAM_END - FLASH_VECTOR_COUNT * sizeof(*vectors);
#endif
}

canview_status_t canview_stm_flash_command(canview_stm_flash_operation_t operation,
    uint32_t address, uint32_t low, uint32_t high)
{
    const bool program = operation == CANVIEW_STM_FLASH_PROGRAM;
    const bool erase = operation == CANVIEW_STM_FLASH_ERASE;
    const uint32_t unit = program ? CANVIEW_STM_FLASH_WRITE_BYTES : CANVIEW_STM_FLASH_PAGE_BYTES;
    if ((!program && !erase) || (address % unit) != 0U ||
        (program && low == UINT32_MAX && high == UINT32_MAX) ||
        (erase && (low != 0U || high != 0U)))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (address < CANVIEW_STM_PRIMARY_ADDRESS || address > CANVIEW_STM_POLICY_A_ADDRESS - unit)
    {
        return CANVIEW_AUTH_FAILED;
    }
    if (__get_IPSR() != 0U || __get_CONTROL() != 0U ||
        (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U ||
        (CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0U)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    uintptr_t vectors[FLASH_VECTOR_COUNT] __attribute__((aligned(512)));
    if (!ram_ready(vectors)) { return CANVIEW_RESOURCE_BUSY; }
    for (size_t index = 0U; index < FLASH_VECTOR_COUNT; ++index)
    {
        vectors[index] = (uintptr_t)flash_fault_reset;
    }
    vectors[0] = __get_MSP();
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    canview_status_t result = CANVIEW_AUTH_FAILED;
    if (canview_boot_flash_check() == 0 && (FLASH->OPTR & FLASH_OPTR_RDP) == FLASH_RDP_DEVELOPMENT &&
        FLASH->CR == (FLASH_CR_LOCK | FLASH_CR_OPTLOCK) && (FLASH->SR & FLASH_ERRORS) == 0U &&
        (FLASH->ACR & FLASH_CACHE_RESET) == 0U)
    {
        const uintptr_t old_vector = SCB->VTOR;
        SCB->VTOR = (uintptr_t)vectors;
        __DSB();
        __ISB();
        result = flash_execute(operation, address, low, high);
        SCB->VTOR = old_vector;
        __DSB();
        __ISB();
    }
    __set_PRIMASK(primask);
    return result;
}
