/* SPDX-License-Identifier: GPL-3.0-only */
/* 실제 C와 register event 모형. timing/ECC/전원 물리 emulator는 아니다. */
#include "canview_stm_flash_command.h"
#include "flash_layout.h"
#include "register_model.h"
#include <stdbool.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

model_flash_t model_flash;
model_rcc_t model_rcc;
model_syscfg_t model_syscfg;
model_scb_t model_scb;
model_debug_t model_debug;
model_dwt_t model_dwt;
uint16_t model_flash_size_kib;
uint32_t model_ipsr, model_control;
static uint32_t flash_words[CANVIEW_STM_FLASH_BYTES / 4U];
static uint32_t mask, keys, writes, erases, polls, command, pending_address, pending_low, pending_high;
static uint32_t completion, tick_step;
static bool unlock_failure, stalled, ram_available, load_fault, fatal_expected;
static jmp_buf fatal_jump;
#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); return 1; } } while (0)
#define REQUIRE(test) do { if (!(test)) { \
    fprintf(stderr, "MODEL FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); abort(); } } while (0)
#define ORIGINAL_VECTOR (UINT32_C(0x08000000))
#define ORIGINAL_CACHE (FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_4WS)

static void critical_check(void)
{
    REQUIRE(mask == 1U && SCB->VTOR != ORIGINAL_VECTOR && SCB->VTOR % 512U == 0U);
    const uintptr_t *const table = (const uintptr_t *)SCB->VTOR;
    REQUIRE(table[0] == UINT32_C(0x20018000) && table[1] != 0U);
    for (size_t i = 2U; i < 128U; ++i) { REQUIRE(table[i] == table[1]); }
    REQUIRE((FLASH->ACR & (FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_ICRST | FLASH_ACR_DCRST)) == 0U);
}

uint32_t model_primask(void) { return mask; }
void model_disable_irq(void) { mask = 1U; }
void model_set_primask(uint32_t value) { mask = value; }
uintptr_t canview_stm_test_stack_top(void) { return UINT32_C(0x20018000); }
bool canview_stm_flash_test_ram_ready(void) { return ram_available; }

uint32_t canview_stm_flash_test_load(uint32_t address)
{
    critical_check();
    REQUIRE(address >= CANVIEW_STM_PRIMARY_ADDRESS && address < CANVIEW_STM_POLICY_A_ADDRESS);
    if (load_fault)
    {
        const uintptr_t *const table = (const uintptr_t *)SCB->VTOR;
        void (*const nmi)(void) = (void (*)(void))table[2];
        nmi();
        abort();
    }
    return flash_words[(address - CANVIEW_STM_FLASH_BASE) / 4U];
}

void canview_stm_flash_test_key(uint32_t value)
{
    critical_check();
    REQUIRE(value == (keys % 2U == 0U ? UINT32_C(0x45670123) : UINT32_C(0xcdef89ab)));
    ++keys;
    if (keys % 2U == 0U && !unlock_failure) { FLASH->CR &= ~FLASH_CR_LOCK; }
}

void canview_stm_flash_test_clear(uint32_t value)
{
    critical_check();
    REQUIRE((value & FLASH_SR_BSY) == 0U);
    FLASH->SR &= ~value; /* W1C semantic */
}

void canview_stm_flash_test_store(uint32_t address, uint32_t value)
{
    critical_check();
    REQUIRE(FLASH->CR == (FLASH_CR_OPTLOCK | FLASH_CR_PG));
    if (writes % 2U == 0U) { pending_address = address; pending_low = value; }
    else
    {
        REQUIRE(address == pending_address + 4U);
        pending_high = value;
        command = FLASH_CR_PG;
        FLASH->SR = FLASH_SR_BSY;
    }
    ++writes;
}

void canview_stm_flash_test_start(void)
{
    critical_check();
    REQUIRE((FLASH->CR & ~(uint32_t)(FLASH_CR_OPTLOCK | FLASH_CR_PER | FLASH_CR_PNB | FLASH_CR_BKER)) == 0U);
    REQUIRE((FLASH->CR & FLASH_CR_PER) != 0U);
    pending_address = CANVIEW_STM_FLASH_BASE +
        ((FLASH->CR & FLASH_CR_BKER) != 0U ? CANVIEW_STM_FLASH_BYTES / 2U : 0U) +
        ((FLASH->CR & FLASH_CR_PNB) >> FLASH_CR_PNB_Pos) * CANVIEW_STM_FLASH_PAGE_BYTES;
    REQUIRE(pending_address >= CANVIEW_STM_PRIMARY_ADDRESS &&
        pending_address < CANVIEW_STM_POLICY_A_ADDRESS);
    command = FLASH_CR_PER;
    ++erases;
    FLASH->SR = FLASH_SR_BSY;
}

void canview_stm_flash_test_poll(void)
{
    critical_check();
    ++polls;
    DWT->CYCCNT += tick_step;
    if (polls % 3U == 0U && !stalled)
    {
        if (completion == FLASH_SR_EOP)
        {
            uint32_t *const target = &flash_words[(pending_address - CANVIEW_STM_FLASH_BASE) / 4U];
            if (command == FLASH_CR_PER) { (void)memset(target, 0xff, CANVIEW_STM_FLASH_PAGE_BYTES); }
            else { target[0] = pending_low; target[1] = pending_high; }
        }
        FLASH->SR = completion;
    }
}

__attribute__((noreturn)) void canview_stm_flash_test_fatal(void)
{
    critical_check();
    REQUIRE(fatal_expected);
    longjmp(fatal_jump, 1);
}

static void fixture(void)
{
    (void)memset(&model_flash, 0, sizeof(model_flash));
    (void)memset(&model_rcc, 0, sizeof(model_rcc));
    (void)memset(&model_syscfg, 0, sizeof(model_syscfg));
    (void)memset(flash_words, 0xff, sizeof(flash_words));
    model_flash_size_kib = 512U;
    RCC->APB2ENR = RCC_APB2ENR_SYSCFGEN;
    FLASH->OPTR = FLASH_OPTR_DBANK | FLASH_OPTR_NRST_MODE | UINT32_C(0xaa);
    FLASH->WRP1AR = 31U << 16U;
    FLASH->WRP1BR = FLASH->WRP2AR = FLASH->WRP2BR = 127U;
    FLASH->CR = FLASH_CR_LOCK | FLASH_CR_OPTLOCK;
    FLASH->ACR = ORIGINAL_CACHE;
    SCB->VTOR = ORIGINAL_VECTOR;
    CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL = DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = UINT32_MAX - 1U; /* 정상 경로도 wrap을 지난다. */
    mask = keys = writes = erases = polls = command = model_ipsr = model_control = 0U;
    completion = FLASH_SR_EOP;
    tick_step = 1U;
    unlock_failure = stalled = load_fault = fatal_expected = false;
    ram_available = true;
}

static bool restored(uint32_t expected_mask)
{
    return mask == expected_mask && SCB->VTOR == ORIGINAL_VECTOR && FLASH->ACR == ORIGINAL_CACHE &&
        FLASH->CR == (FLASH_CR_LOCK | FLASH_CR_OPTLOCK);
}

int main(void)
{
    fixture();
    for (uint32_t address = CANVIEW_STM_PRIMARY_ADDRESS; address < CANVIEW_STM_POLICY_A_ADDRESS; address += 8U)
    {
        CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_PROGRAM, address, address, 7U) == CANVIEW_OK);
        CHECK(restored(0U));
        CHECK(flash_words[(address - CANVIEW_STM_FLASH_BASE) / 4U] == address);
        CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_PROGRAM, address, 1U, 2U) == CANVIEW_DUPLICATE);
    }
    for (uint32_t address = CANVIEW_STM_PRIMARY_ADDRESS; address < CANVIEW_STM_POLICY_A_ADDRESS; address += 2048U)
    {
        CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_ERASE, address, 0U, 0U) == CANVIEW_OK);
        CHECK(pending_address == address && restored(0U));
    }
    CHECK(erases == 193U);
    for (size_t i = 0U; i < sizeof(flash_words) / sizeof(flash_words[0]); ++i) { CHECK(flash_words[i] == UINT32_MAX); }
    fixture();
    CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_READ, CANVIEW_STM_PRIMARY_ADDRESS, 0U, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_flash_command((canview_stm_flash_operation_t)-1, 0U, 0U, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_flash_command((canview_stm_flash_operation_t)3, 0U, 0U, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_PROGRAM, CANVIEW_STM_PRIMARY_ADDRESS, UINT32_MAX, UINT32_MAX) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_ERASE, CANVIEW_STM_PRIMARY_ADDRESS, 1U, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_ERASE, CANVIEW_STM_PRIMARY_ADDRESS, 0U, 1U) == CANVIEW_INVALID_ARGUMENT);
    for (uint32_t offset = 1U; offset < 2048U; ++offset)
    {
        CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_ERASE, CANVIEW_STM_PRIMARY_ADDRESS + offset, 0U, 0U) == CANVIEW_INVALID_ARGUMENT);
        if (offset % 8U != 0U) { CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_PROGRAM, CANVIEW_STM_PRIMARY_ADDRESS + offset, 1U, 0U) == CANVIEW_INVALID_ARGUMENT); }
    }
    const uint32_t forbidden[] = {0U, 8U, UINT32_MAX - 7U, CANVIEW_STM_FLASH_BASE,
        CANVIEW_STM_PRIMARY_ADDRESS - 8U, CANVIEW_STM_POLICY_A_ADDRESS, CANVIEW_STM_CONFIG_A_ADDRESS};
    for (size_t i = 0U; i < sizeof(forbidden) / sizeof(forbidden[0]); ++i)
    {
        CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_PROGRAM, forbidden[i], 1U, 0U) == CANVIEW_AUTH_FAILED);
    }
    CHECK(keys == 0U && writes == 0U && erases == 0U && restored(0U));
    for (uint32_t fault = 0U; fault < 13U; ++fault)
    {
        fixture();
        switch (fault)
        {
            case 0U: model_ipsr = 2U; break;
            case 1U: model_control = 1U; break;
            case 2U: DWT->CTRL = 0U; break;
            case 3U: CoreDebug->DEMCR = 0U; break;
            case 4U: ram_available = false; break;
            case 5U: FLASH->OPTR &= ~FLASH_OPTR_DBANK; break;
            case 6U: FLASH->OPTR &= ~FLASH_OPTR_RDP; break;
            case 7U: FLASH->CR &= ~FLASH_CR_LOCK; break;
            case 8U: FLASH->CR &= ~FLASH_CR_OPTLOCK; break;
            case 9U: FLASH->SR = FLASH_SR_PROGERR; break;
            case 10U: FLASH->ACR |= FLASH_ACR_ICRST; break;
            case 11U: FLASH->CR |= FLASH_CR_PG; break;
            default: FLASH->SR = FLASH_SR_BSY; break;
        }
        const model_flash_t original = model_flash;
        CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_ERASE, CANVIEW_STM_PRIMARY_ADDRESS, 0U, 0U) != CANVIEW_OK);
        CHECK(memcmp(&original, &model_flash, sizeof(original)) == 0);
        CHECK(keys == 0U && mask == 0U && SCB->VTOR == ORIGINAL_VECTOR);
    }
    const uint32_t errors[] = {0U, FLASH_SR_OPERR, FLASH_SR_PROGERR, FLASH_SR_WRPERR,
        FLASH_SR_PGAERR, FLASH_SR_SIZERR, FLASH_SR_PGSERR, FLASH_SR_MISERR,
        FLASH_SR_FASTERR, FLASH_SR_RDERR, FLASH_SR_OPTVERR};
    for (size_t i = 0U; i < sizeof(errors) / sizeof(errors[0]); ++i)
    {
        fixture();
        completion = errors[i];
        mask = 1U;
        CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_ERASE, CANVIEW_STM_PRIMARY_ADDRESS, 0U, 0U) == CANVIEW_INCOMPLETE);
        CHECK(restored(1U) && FLASH->SR == 0U);
    }
    fixture();
    unlock_failure = true;
    CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_ERASE, CANVIEW_STM_PRIMARY_ADDRESS, 0U, 0U) == CANVIEW_INCOMPLETE);
    CHECK(restored(0U) && erases == 0U && keys == 2U);
    fixture();
    flash_words[(CANVIEW_STM_PRIMARY_ADDRESS - CANVIEW_STM_FLASH_BASE) / 4U + 1U] = 0U;
    CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_PROGRAM, CANVIEW_STM_PRIMARY_ADDRESS, 1U, 0U) == CANVIEW_DUPLICATE);
    CHECK(restored(0U) && keys == 0U);
    fixture();
    CHECK(canview_stm_flash_command(CANVIEW_STM_FLASH_PROGRAM, CANVIEW_STM_PRIMARY_ADDRESS, UINT32_MAX, 0U) == CANVIEW_OK);
    CHECK(restored(0U));
    for (uint32_t rdp = 0U; rdp <= UINT8_MAX; ++rdp)
    {
        fixture();
        FLASH->OPTR = (FLASH->OPTR & ~FLASH_OPTR_RDP) | rdp;
        const canview_status_t result = canview_stm_flash_command(CANVIEW_STM_FLASH_ERASE,
            CANVIEW_STM_SECONDARY_ADDRESS, 0U, 0U);
        CHECK((result == CANVIEW_OK) == (rdp == UINT32_C(0xaa)));
        CHECK(restored(0U) && FLASH->OPTR == (FLASH_OPTR_DBANK | FLASH_OPTR_NRST_MODE | rdp));
        CHECK(erases == (rdp == UINT32_C(0xaa) ? 1U : 0U));
    }
    /* NMI와 DWT deadline/정지 counter: 반환 대신 SRAM fatal 경로. */
    for (uint32_t fault = 0U; fault < 3U; ++fault)
    {
        fixture();
        fatal_expected = true;
        load_fault = fault == 0U;
        stalled = !load_fault;
        tick_step = fault == 1U ? UINT32_C(8500000) : 0U;
        if (setjmp(fatal_jump) == 0)
        {
            (void)canview_stm_flash_command(CANVIEW_STM_FLASH_PROGRAM, CANVIEW_STM_PRIMARY_ADDRESS, 1U, 0U);
            CHECK(false);
        }
        CHECK(mask == 1U && SCB->VTOR != ORIGINAL_VECTOR);
    }
    puts("PASS: SRAM command register model, 49408 words/193 pages, errors/timeout/NMI; physical NOT_RUN");
    return 0;
}
