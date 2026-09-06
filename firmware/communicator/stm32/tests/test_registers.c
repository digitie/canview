/* SPDX-License-Identifier: GPL-3.0-only */
#include "core_hw.h"
#include "register_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#define CHECK(condition)                                                                           \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition);                  \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
model_rcc_t model_rcc;
model_pwr_t model_pwr;
model_flash_t model_flash;
model_iwdg_t model_iwdg;
model_timer_t model_timer;
model_debug_t model_debug;
model_dwt_t model_dwt;
uint32_t SystemCoreClock;
static uint32_t fault_stage;
static uint32_t polls;
static uint32_t irq_mask;
static uint32_t reset_requests;
static uint32_t wait_calls;
static uint32_t systick_reload;
static bool nmi_before_feed;
static bool reset_returns;
static jmp_buf stopped;
void SysTick_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);

void canview_stm_test_before_feed(void)
{
    if (nmi_before_feed)
    {
        RCC->CIFR = RCC_CIFR_CSSF;
        NMI_Handler();
    }
}

uint32_t model_primask(void)
{
    return irq_mask;
}
void model_disable_irq(void)
{
    irq_mask = 1U;
}
void model_set_primask(uint32_t mask)
{
    irq_mask = mask;
}
void model_wait(void)
{
    ++wait_calls;
    longjmp(stopped, 1);
}
void model_reset(void)
{
    ++reset_requests;
    if (reset_returns)
    {
        return;
    }
    longjmp(stopped, 1);
}
uint32_t model_systick_config(uint32_t ticks)
{
    systick_reload = ticks;
    return fault_stage == 9U ? 1U : 0U;
}

void canview_stm_test_poll(void)
{
    ++polls;
    CHECK(polls <= UINT32_C(2000100));
    if ((RCC->CSR & RCC_CSR_LSION) != 0U && fault_stage != 1U)
    {
        RCC->CSR |= RCC_CSR_LSIRDY;
    }
    if (fault_stage == 2U)
    {
        IWDG->SR = IWDG_SR_PVU | IWDG_SR_RVU | IWDG_SR_WVU;
    }
    if (fault_stage == 3U)
    {
        PWR->SR2 = PWR_SR2_VOSF;
    }
    if (fault_stage == 4U)
    {
        FLASH->ACR = 0U;
    }
    if ((RCC->CR & RCC_CR_HSEON) != 0U && fault_stage != 5U)
    {
        RCC->CR |= RCC_CR_HSERDY;
    }
    if ((RCC->CR & RCC_CR_PLLON) != 0U)
    {
        CHECK((FLASH->ACR & FLASH_ACR_LATENCY) == FLASH_ACR_LATENCY_4WS);
        CHECK((PWR->CR5 & PWR_CR5_R1MODE) == 0U);
        CHECK((RCC->CFGR & RCC_CFGR_HPRE) == RCC_CFGR_HPRE_DIV2);
        if (fault_stage != 6U)
        {
            RCC->CR |= RCC_CR_PLLRDY;
        }
    }
    else if (fault_stage == 10U)
    {
        RCC->CR |= RCC_CR_PLLRDY;
    }
    else
    {
        RCC->CR &= ~RCC_CR_PLLRDY;
    }
    if ((RCC->CFGR & RCC_CFGR_SW) == RCC_CFGR_SW_PLL && fault_stage != 7U)
    {
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SWS) | RCC_CFGR_SWS_PLL;
    }
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0U && fault_stage != 8U)
    {
        DWT->CYCCNT += 80U;
    }
}

static void initialize(uint32_t stage)
{
    canview_stm_test_reset();
    memset(&model_rcc, 0, sizeof(model_rcc));
    memset(&model_pwr, 0, sizeof(model_pwr));
    memset(&model_iwdg, 0, sizeof(model_iwdg));
    memset(&model_flash, 0, sizeof(model_flash));
    memset(&model_timer, 0, sizeof(model_timer));
    memset(&model_debug, 0, sizeof(model_debug));
    memset(&model_dwt, 0, sizeof(model_dwt));
    RCC->CFGR = RCC_CFGR_SWS_HSI;
    RCC->CSR = UINT32_C(0x20000000);
    PWR->CR5 = PWR_CR5_R1MODE;
    fault_stage = stage;
    polls = 0U;
    reset_requests = 0U;
    wait_calls = 0U;
    systick_reload = 0U;
    nmi_before_feed = false;
    reset_returns = false;
}

static void check_no_control(void)
{
    canview_stm_diagnostic_t diagnostic;
    memset(&diagnostic, 0xff, sizeof(diagnostic));
    canview_stm_board_diagnostic(&diagnostic);
    CHECK(diagnostic.control_capabilities == 0U && !diagnostic.tx_permit);
    CHECK(!diagnostic.authenticity_known && !diagnostic.production_debug_lock_known);
    canview_stm_board_diagnostic(NULL);
}

static void timeout_tests(void)
{
    for (uint32_t stage = 1U; stage <= 10U; ++stage)
    {
        initialize(stage);
        check_no_control();
        if (stage <= 2U)
        {
            CHECK(canview_stm_watchdog_start(NULL) == CANVIEW_TIMEOUT);
        }
        else
        {
            CHECK(canview_stm_watchdog_start(NULL) == CANVIEW_OK);
            if (stage != 9U)
            {
                CHECK(canview_stm_clock_start(NULL) == CANVIEW_TIMEOUT);
            }
            else
            {
                CHECK(canview_stm_clock_start(NULL) == CANVIEW_OK);
                CHECK(canview_stm_time_start(NULL) == CANVIEW_INVALID_ARGUMENT);
            }
        }
        CHECK(canview_stm_watchdog_feed(NULL) == CANVIEW_TIMEOUT);
        CHECK(canview_stm_board_health(NULL) == CANVIEW_TIMEOUT);
        check_no_control();
        if (stage != 9U)
        {
            CHECK(polls >= UINT32_C(1000000));
        }
    }
}

static void healthy_boot(void)
{
    initialize(0U);
    CHECK(canview_stm_clock_start(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_time_start(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_watchdog_start(NULL) == CANVIEW_OK);
    CHECK(canview_stm_watchdog_start(NULL) == CANVIEW_RESOURCE_BUSY);
    CHECK(IWDG->PR == 3U && IWDG->RLR == 374U && IWDG->WINR == 4095U);
    CHECK(canview_stm_clock_start(NULL) == CANVIEW_OK);
    CHECK(canview_stm_clock_start(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(SystemCoreClock == 160000000U);
    CHECK((RCC->CFGR & RCC_CFGR_HPRE) == 0U);
    CHECK((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV2);
    CHECK((RCC->CFGR & RCC_CFGR_PPRE2) == RCC_CFGR_PPRE2_DIV2);
    CHECK(RCC->PLLCFGR == (UINT32_C(3) | (UINT32_C(3) << 4U) | (UINT32_C(80) << 8U) |
                           (UINT32_C(1) << 21U) | (UINT32_C(1) << 20U) | (UINT32_C(1) << 24U)));
    CHECK((RCC->CCIPR & RCC_CCIPR_USART2SEL) == 0U);
    CHECK((RCC->CCIPR & RCC_CCIPR_FDCANSEL) == RCC_CCIPR_FDCANSEL_0);
    CHECK(canview_stm_time_start(NULL) == CANVIEW_OK);
    CHECK(canview_stm_time_start(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(TIM2->PSC == 159U && TIM2->ARR == UINT32_MAX && TIM2->DIER == 0U);
    CHECK(systick_reload == 160000U);
    CHECK(canview_stm_board_now_ms() == 0U);
    SysTick_Handler();
    SysTick_Handler();
    CHECK(canview_stm_board_now_ms() == 2U);
    TIM2->CNT = UINT32_MAX;
    CHECK(canview_stm_now_us(NULL) == UINT32_MAX);
    CHECK(canview_stm_board_health(NULL) == CANVIEW_OK);
    CHECK(canview_stm_watchdog_feed(NULL) == CANVIEW_OK && IWDG->KR == 0xaaaaU);
    check_no_control();
    canview_stm_diagnostic_t diagnostic;
    canview_stm_board_diagnostic(&diagnostic);
    CHECK(diagnostic.reset_flags == UINT32_C(0x20000000));
    CHECK(diagnostic.sysclk_hz == 160000000U && diagnostic.peripheral_hz == 80000000U);
}

static void fault_tests(void)
{
    healthy_boot();
    IWDG->KR = 0U;
    nmi_before_feed = true;
    if (setjmp(stopped) == 0)
    {
        (void)canview_stm_watchdog_feed(NULL);
        CHECK(false); /* NMI에서 중단된 feed로 복귀하면 실패. */
    }
    CHECK(reset_requests == 1U && IWDG->KR == 0U && RCC->CICR == RCC_CICR_CSSC);
    healthy_boot();
    reset_returns = true;
    if (setjmp(stopped) == 0)
    {
        NMI_Handler();
        CHECK(false);
    }
    CHECK(reset_requests == 1U && wait_calls == 1U);
    healthy_boot();
    SysTick_Handler();
    CHECK(canview_stm_board_health(NULL) == CANVIEW_TIMEOUT); /* TIM2 freeze, EN은 여전히1 */
    TIM2->CNT += 1000U;
    CHECK(canview_stm_watchdog_feed(NULL) == CANVIEW_TIMEOUT); /* latched */
    healthy_boot();
    SysTick_Handler();
    TIM2->CNT += 1000U; /* u32 wrap */
    CHECK(canview_stm_board_health(NULL) == CANVIEW_OK);
    SysTick_Handler();
    TIM2->CNT -= 1U; /* backward */
    CHECK(canview_stm_board_health(NULL) == CANVIEW_TIMEOUT);
    healthy_boot();
    for (uint32_t tick = 0U; tick < 21U; ++tick)
    {
        SysTick_Handler();
    }
    TIM2->CNT += 21000U;
    CHECK(canview_stm_board_health(NULL) == CANVIEW_TIMEOUT);
    for (uint32_t stage = 0U; stage < 5U; ++stage)
    {
        healthy_boot();
        if (stage == 0U)
        {
            RCC->CR &= ~RCC_CR_HSERDY;
        }
        if (stage == 1U)
        {
            RCC->CR &= ~RCC_CR_PLLRDY;
        }
        if (stage == 2U)
        {
            RCC->CFGR &= ~RCC_CFGR_SWS;
        }
        if (stage == 3U)
        {
            TIM2->CR1 = 0U;
        }
        if (stage == 4U)
        {
            canview_stm_hw_latch_fault();
        }
        CHECK(canview_stm_board_health(NULL) == CANVIEW_TIMEOUT);
        CHECK(canview_stm_watchdog_feed(NULL) == CANVIEW_TIMEOUT);
        check_no_control();
    }
    /* longjmp 경계와 반복 변수의 수명을 분리한다. */
    healthy_boot();
    RCC->CIFR = RCC_CIFR_CSSF;
    if (setjmp(stopped) == 0)
    {
        NMI_Handler();
        CHECK(false);
    }
    CHECK(reset_requests == 1U && RCC->CICR == RCC_CICR_CSSC);
    CHECK(canview_stm_board_health(NULL) == CANVIEW_TIMEOUT);
    CHECK(canview_stm_watchdog_feed(NULL) == CANVIEW_TIMEOUT);
    check_no_control();
    healthy_boot();
    if (setjmp(stopped) == 0)
    {
        NMI_Handler();
        CHECK(false);
    }
    CHECK(reset_requests == 1U);
    CHECK(canview_stm_watchdog_feed(NULL) == CANVIEW_TIMEOUT);
    healthy_boot();
    if (setjmp(stopped) == 0)
    {
        HardFault_Handler();
        CHECK(false);
    }
    CHECK(wait_calls == 1U && canview_stm_watchdog_feed(NULL) == CANVIEW_TIMEOUT);
    healthy_boot();
    if (setjmp(stopped) == 0)
    {
        canview_stm_board_wait_reset();
        CHECK(false);
    }
    CHECK(wait_calls == 1U && reset_requests == 0U);
    initialize(0U);
    if (setjmp(stopped) == 0)
    {
        canview_stm_board_wait_reset();
        CHECK(false);
    }
    CHECK(reset_requests == 1U && wait_calls == 0U);
    CHECK(canview_stm_watchdog_start(NULL) == CANVIEW_RESOURCE_BUSY);
    initialize(0U);
    CHECK(canview_stm_watchdog_start(NULL) == CANVIEW_OK);
    RCC->CFGR = RCC_CFGR_SWS_PLL;
    CHECK(canview_stm_clock_start(NULL) == CANVIEW_INVALID_ARGUMENT);
    for (uint32_t mask = 0U; mask <= 1U; ++mask)
    {
        irq_mask = mask;
        const uint32_t saved = canview_stm_critical_enter(NULL);
        CHECK(saved == mask && irq_mask == 1U);
        const uint32_t inner = canview_stm_critical_enter(NULL);
        canview_stm_critical_leave(NULL, inner);
        CHECK(irq_mask == 1U);
        canview_stm_critical_leave(NULL, saved);
        CHECK(irq_mask == mask);
    }
}

int main(void)
{
    timeout_tests();
    healthy_boot();
    fault_tests();
    (void)puts("PASS: host named-register model, not physical STM32/HIL");
    return 0;
}
