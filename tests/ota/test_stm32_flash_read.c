/* SPDX-License-Identifier: GPL-3.0-only */
/* 동일 C + register event 모형. 실제 ECC/exception timing은 HIL 대상이다. */
#include "canview_stm_flash_read.h"
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
uint16_t model_flash_size_kib;
uint32_t model_ipsr, model_control;
static uint32_t mask, loads, clears, polls, fault_at, fault_flags, delivery_delay, exception_index;
static uint32_t concurrent_clock_fault, concurrent_parity_fault;
static uint32_t barriers, barrier_fault_at;
static uint32_t load_error_flags;
static bool ram_available, sticky, fatal_expected, nested;
static jmp_buf fatal_jump;
#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); return 1; } } while (0)
#define REQUIRE(test) do { if (!(test)) { \
    fprintf(stderr, "MODEL FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); abort(); } } while (0)
#define ORIGINAL_VECTOR (UINT32_C(0x08000000))
#define ORIGINAL_CACHE (FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_4WS)
#define ECC_FLAGS (FLASH_ECCR_ECCC | FLASH_ECCR_ECCD)

uint32_t model_primask(void) { return mask; }
void model_disable_irq(void) { mask = 1U; }
void model_set_primask(uint32_t value) { mask = value; }
uintptr_t canview_stm_test_stack_top(void) { return UINT32_C(0x20018000); }
bool canview_stm_flash_test_ram_ready(void) { return ram_available; }

static void critical_check(void)
{
    REQUIRE(mask == 1U && SCB->VTOR != ORIGINAL_VECTOR && SCB->VTOR % 512U == 0U);
    const uintptr_t *const table = (const uintptr_t *)SCB->VTOR;
    REQUIRE(table[0] == UINT32_C(0x20018000) && table[1] != table[2]);
    for (size_t index = 3U; index < 128U; ++index) { REQUIRE(table[index] == table[1]); }
    REQUIRE((FLASH->ACR & (FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_ICRST | FLASH_ACR_DCRST)) == 0U);
    REQUIRE(FLASH->CR == (FLASH_CR_LOCK | FLASH_CR_OPTLOCK));
    REQUIRE(FLASH->KEYR == 0U && (FLASH->SR & ~load_error_flags) == 0U);
}

static void exception(void)
{
    const uintptr_t *const table = (const uintptr_t *)SCB->VTOR;
    void (*const handler)(void) = (void (*)(void))table[exception_index];
    model_ipsr = exception_index;
    handler();
    model_ipsr = 0U;
}

void canview_stm_read_test_barrier(void)
{
    if (SCB->VTOR != ORIGINAL_VECTOR && ++barriers == barrier_fault_at)
    {
        FLASH->ECCR |= FLASH_ECCR_ECCD;
        exception();
    }
}

uint32_t canview_stm_flash_test_load(uint32_t address)
{
    critical_check();
    REQUIRE(address >= CANVIEW_STM_PRIMARY_ADDRESS && address < CANVIEW_STM_POLICY_A_ADDRESS);
    REQUIRE(address % 4U == 0U);
    if (nested)
    {
        uint32_t value = 0U;
        model_ipsr = 2U;
        REQUIRE(canview_stm_flash_read(address, &value, sizeof(value)) == CANVIEW_RESOURCE_BUSY);
        model_ipsr = 0U;
    }
    if (++loads == fault_at)
    {
        FLASH->SR |= load_error_flags;
        FLASH->ECCR |= fault_flags;
        RCC->CIFR |= concurrent_clock_fault;
        SYSCFG->CFGR2 |= concurrent_parity_fault;
        if ((fault_flags & FLASH_ECCR_ECCC) == 0U || (fault_flags & FLASH_ECCR_ECCD) != 0U)
        {
            if (delivery_delay == 0U) { exception(); }
        }
    }
    return address ^ UINT32_C(0xa5c39e70);
}

void canview_stm_flash_test_ecc_clear(uint32_t value)
{
    critical_check();
    REQUIRE(value != 0U && (value & ~ECC_FLAGS) == 0U);
    ++clears;
    if (!sticky) { FLASH->ECCR &= ~value; }
}

void canview_stm_flash_test_poll(void)
{
    critical_check();
    if (++polls == delivery_delay) { exception(); }
}

__attribute__((noreturn)) void canview_stm_flash_test_fatal(void)
{
    REQUIRE(fatal_expected);
    longjmp(fatal_jump, 1);
}

static void reset_model(void)
{
    memset(&model_flash, 0, sizeof(model_flash));
    memset(&model_rcc, 0, sizeof(model_rcc));
    memset(&model_syscfg, 0, sizeof(model_syscfg));
    FLASH->OPTR = FLASH_OPTR_DBANK | FLASH_OPTR_NRST_MODE | UINT32_C(0xaa);
    FLASH->WRP1AR = UINT32_C(31) << 16U;
    FLASH->WRP1BR = FLASH->WRP2AR = FLASH->WRP2BR = UINT32_C(0x7f);
    FLASH->CR = FLASH_CR_LOCK | FLASH_CR_OPTLOCK;
    FLASH->ACR = ORIGINAL_CACHE;
    RCC->APB2ENR = RCC_APB2ENR_SYSCFGEN;
    model_flash_size_kib = 512U;
    SCB->VTOR = ORIGINAL_VECTOR;
    model_ipsr = model_control = mask = loads = clears = polls = 0U;
    fault_at = UINT32_MAX;
    fault_flags = delivery_delay = 0U;
    concurrent_clock_fault = concurrent_parity_fault = 0U;
    barriers = 0U;
    barrier_fault_at = UINT32_MAX;
    load_error_flags = 0U;
    exception_index = 2U;
    ram_available = true;
    sticky = fatal_expected = nested = false;
}

static int returned_clean(void)
{
    CHECK(SCB->VTOR == ORIGINAL_VECTOR && FLASH->ACR == ORIGINAL_CACHE);
    CHECK(FLASH->CR == (FLASH_CR_LOCK | FLASH_CR_OPTLOCK) && FLASH->KEYR == 0U);
    CHECK(FLASH->SR == 0U);
    return 0;
}

static int expect_fatal_read(uint8_t *output)
{
    if (setjmp(fatal_jump) == 0)
    {
        (void)canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS, output, 4U);
        CHECK(false);
    }
    return 0;
}

int main(void)
{
    uint8_t output[CANVIEW_STM_FLASH_READ_MAX + 2U];
    for (uint32_t offset = 0U; offset < 4U; ++offset)
    {
        for (uint32_t length = 1U; length <= CANVIEW_STM_FLASH_READ_MAX; ++length)
        {
            reset_model();
            memset(output, 0x5a, sizeof(output));
            const uint32_t address = CANVIEW_STM_PRIMARY_ADDRESS + offset;
            CHECK(canview_stm_flash_read(address, output + 1U, length) == CANVIEW_OK);
            for (uint32_t index = 0U; index < length; ++index)
            {
                const uint32_t at = address + index;
                CHECK(output[index + 1U] == (uint8_t)(((at & ~UINT32_C(3)) ^ UINT32_C(0xa5c39e70)) >> ((at % 4U) * 8U)));
            }
            CHECK(output[0] == 0x5aU && output[length + 1U] == 0x5aU);
            CHECK(loads == (offset + length + 3U) / 4U && mask == 0U && returned_clean() == 0);
        }
    }
    for (uint32_t address = CANVIEW_STM_PRIMARY_ADDRESS; address < CANVIEW_STM_POLICY_A_ADDRESS; address += 4U)
    {
        reset_model();
        CHECK(canview_stm_flash_read(address, output, 4U) == CANVIEW_OK);
    }
    reset_model();
    CHECK(canview_stm_flash_read(CANVIEW_STM_POLICY_A_ADDRESS - 1U, output, 1U) == CANVIEW_OK);
    const uint32_t invalid_addresses[] = {0U, CANVIEW_STM_PRIMARY_ADDRESS - 1U,
        CANVIEW_STM_POLICY_A_ADDRESS, UINT32_MAX};
    for (size_t index = 0U; index < sizeof(invalid_addresses) / sizeof(invalid_addresses[0]); ++index)
    {
        reset_model();
        CHECK(canview_stm_flash_read(invalid_addresses[index], output, 1U) == CANVIEW_AUTH_FAILED);
        CHECK(loads == 0U && returned_clean() == 0);
    }
    reset_model();
    CHECK(canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS, NULL, 1U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS, output, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS, output, 257U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS, output, UINT32_MAX) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_flash_read(CANVIEW_STM_POLICY_A_ADDRESS - 1U, output, 2U) == CANVIEW_AUTH_FAILED);
    for (uint32_t index = 0U; index < 16U; ++index)
    {
        reset_model();
        switch (index)
        {
            case 0U: model_ipsr = 1U; break;
            case 1U: model_control = 1U; break;
            case 2U: ram_available = false; break;
            case 3U: RCC->APB2ENR = 0U; break;
            case 4U: FLASH->CR = 0U; break;
            case 5U: FLASH->SR = FLASH_SR_BSY; break;
            case 6U: FLASH->SR = FLASH_SR_OPERR; break;
            case 7U: FLASH->ACR |= FLASH_ACR_DCRST; break;
            case 8U: FLASH->ECCR = FLASH_ECCR_ECCC; break;
            case 9U: FLASH->ECCR = FLASH_ECCR_ECCD; break;
            case 10U: FLASH->ECCR = FLASH_ECCR_ECCC2; break;
            case 11U: FLASH->ECCR = FLASH_ECCR_ECCD2; break;
            case 12U: FLASH->ECCR = FLASH_ECCR_ECCIE; break;
            case 13U: RCC->CIFR = RCC_CIFR_CSSF; break;
            case 14U: RCC->CIFR = RCC_CIFR_LSECSSF; break;
            default: SYSCFG->CFGR2 = SYSCFG_CFGR2_SPF; break;
        }
        memset(output, 0x5a, sizeof(output));
        CHECK(canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS, output, 1U) == CANVIEW_RESOURCE_BUSY);
        CHECK(loads == 0U && output[0] == 0x5aU && mask == 0U && SCB->VTOR == ORIGINAL_VECTOR);
    }
    for (uint32_t rdp = 0U; rdp <= UINT8_MAX; ++rdp)
    {
        reset_model();
        FLASH->OPTR = (FLASH->OPTR & ~FLASH_OPTR_RDP) | rdp;
        CHECK(canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS, output, 1U) ==
            (rdp == UINT32_C(0xaa) ? CANVIEW_OK : CANVIEW_RESOURCE_BUSY));
    }
    for (uint32_t index = 1U; index <= 65U; ++index)
    {
        for (uint32_t kind = 0U; kind < 3U; ++kind)
        {
            reset_model();
            memset(output, 0x5a, sizeof(output));
            fault_at = index;
            fault_flags = kind == 0U ? FLASH_ECCR_ECCC : FLASH_ECCR_ECCD;
            if (kind == 2U) { fault_flags |= FLASH_ECCR_ECCC; }
            FLASH->ECCR = UINT32_C(0x00500008); /* 잘못된 ECC 주소/은행도 erase에 사용하지 않는다. */
            CHECK(canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS + 1U, output, 256U) == CANVIEW_INCOMPLETE);
            for (size_t byte = 0U; byte < sizeof(output); ++byte) { CHECK(output[byte] == 0x5aU); }
            CHECK(loads == index && clears == 1U && returned_clean() == 0);
            CHECK((FLASH->ECCR & ECC_FLAGS) == 0U);
        }
    }
    for (uint32_t delay = 1U; delay < 32U; ++delay)
    {
        reset_model();
        fault_at = 1U;
        fault_flags = FLASH_ECCR_ECCD;
        delivery_delay = delay;
        CHECK(canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS, output, 4U) == CANVIEW_INCOMPLETE);
        CHECK(polls == delay && returned_clean() == 0);
    }
    for (uint32_t index = 1U; index <= 65U; ++index)
    {
        reset_model();
        fault_at = index;
        load_error_flags = FLASH_SR_RDERR;
        delivery_delay = UINT32_MAX; /* SR 단독 오류에는 ECC NMI를 만들지 않는다. */
        memset(output, 0x5a, sizeof(output));
        CHECK(canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS + 1U, output, 256U) == CANVIEW_INCOMPLETE);
        CHECK(loads == index && FLASH->SR == FLASH_SR_RDERR && clears == 0U && mask == 0U);
        CHECK(SCB->VTOR == ORIGINAL_VECTOR && FLASH->ACR == ORIGINAL_CACHE);
        for (size_t byte = 0U; byte < sizeof(output); ++byte) { CHECK(output[byte] == 0x5aU); }
    }
    reset_model();
    fault_at = 1U;
    load_error_flags = FLASH_SR_RDERR;
    delivery_delay = UINT32_MAX;
    CHECK(canview_stm_flash_read(CANVIEW_STM_POLICY_A_ADDRESS - 1U, output, 1U) == CANVIEW_INCOMPLETE);
    CHECK(FLASH->SR == FLASH_SR_RDERR && loads == 1U);
    for (size_t byte = 0U; byte < sizeof(output); ++byte) { CHECK(output[byte] == 0x5aU); }
    for (uint32_t failure = 0U; failure < 13U; ++failure)
    {
        reset_model();
        fault_at = 1U;
        fault_flags = FLASH_ECCR_ECCD;
        fatal_expected = true;
        if (failure == 0U) { sticky = true; }
        if (failure == 1U) { delivery_delay = UINT32_MAX; }
        if (failure == 2U) { fault_flags = 0U; }
        if (failure == 3U) { exception_index = 3U; }
        if (failure == 4U) { fault_flags = FLASH_ECCR_ECCD2; }
        if (failure == 5U) { fault_flags |= FLASH_ECCR_ECCIE; }
        if (failure == 6U) { concurrent_clock_fault = RCC_CIFR_CSSF; }
        if (failure == 7U) { concurrent_clock_fault = RCC_CIFR_LSECSSF; }
        if (failure == 8U) { concurrent_parity_fault = SYSCFG_CFGR2_SPF; }
        if (failure == 9U) { fault_flags = FLASH_ECCR_ECCC; sticky = true; }
        if (failure == 10U) { fault_flags = FLASH_ECCR_ECCC | FLASH_ECCR_ECCC2; }
        if (failure == 11U) { fault_flags = FLASH_ECCR_ECCC | FLASH_ECCR_ECCD2; }
        if (failure == 12U)
        {
            fault_flags = 0U;
            load_error_flags = FLASH_SR_BSY;
            delivery_delay = UINT32_MAX;
        }
        memset(output, 0x5a, sizeof(output));
        CHECK(expect_fatal_read(output) == 0);
        CHECK(loads == 1U && mask == 1U);
        CHECK(FLASH->CR == (FLASH_CR_LOCK | FLASH_CR_OPTLOCK) && FLASH->KEYR == 0U);
        for (size_t byte = 0U; byte < sizeof(output); ++byte) { CHECK(output[byte] == 0x5aU); }
    }
    /* private VTOR 설치 직후와 read 종료 직후의 NMI는 현재 load로 오인하지 않는다. */
    for (uint32_t boundary = 1U; boundary <= 4U; boundary += 3U)
    {
        reset_model();
        barrier_fault_at = boundary;
        fatal_expected = true;
        memset(output, 0x5a, sizeof(output));
        CHECK(expect_fatal_read(output) == 0);
        CHECK(barriers == boundary && mask == 1U && clears == 0U);
        CHECK(loads == (boundary == 1U ? 0U : 1U));
        for (size_t byte = 0U; byte < sizeof(output); ++byte) { CHECK(output[byte] == 0x5aU); }
    }
    reset_model();
    mask = 1U;
    nested = true;
    CHECK(canview_stm_flash_read(CANVIEW_STM_PRIMARY_ADDRESS, output, 256U) == CANVIEW_OK);
    CHECK(mask == 1U && returned_clean() == 0);
    puts("PASS: bounded ECC read, all slot words, alignment/length, per-load ECC, delayed NMI, fail-stop and cleanup");
    return 0;
}
