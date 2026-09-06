/* SPDX-License-Identifier: GPL-3.0-only */
#include "core_hw.h"
#if defined(CANVIEW_STM_REGISTER_TEST)
#if defined(__arm__) || defined(__thumb__)
#error Host_register_model_must_not_be_built_for_target
#endif
#include "register_model.h"
#define REGISTER_POLL() canview_stm_test_poll()
#else
#include "stm32g474xx.h"
#define REGISTER_POLL() ((void)0)
#endif

#define REGISTER_POLL_LIMIT (UINT32_C(1000000))
#define BOOST_SETTLE_CYCLES (UINT32_C(160))
#define IWDG_ENABLE_KEY (UINT32_C(0xcccc))
#define IWDG_WRITE_KEY (UINT32_C(0x5555))
#define IWDG_FEED_KEY (UINT32_C(0xaaaa))
#define IWDG_DIV32 (UINT32_C(3))
#define IWDG_RELOAD (UINT32_C(374))
#define IWDG_WINDOW_DISABLED (UINT32_C(0xfff))
#define MICROSECOND_HZ (UINT32_C(1000000))
#define MILLISECOND_HZ (UINT32_C(1000))

/* 단일 MCU context. ISR 공유 member에만 volatile을 적용한다. */
typedef struct
{
    volatile uint32_t milliseconds;
    volatile bool fault;
    uint32_t reset_flags;
    uint32_t previous_health_ms;
    uint32_t previous_health_us;
    bool health_sampled;
    bool watchdog_ready;
    bool clock_ready;
    bool time_ready;
} canview_stm_hardware_t;
static canview_stm_hardware_t hardware;

#if defined(CANVIEW_STM_REGISTER_TEST)
void canview_stm_test_reset(void)
{
    const canview_stm_hardware_t cleared = {0};
    hardware = cleared;
}
#endif

void SysTick_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);

static bool wait_register(volatile const uint32_t *reg, uint32_t mask, uint32_t wanted)
{
    for (uint32_t attempt = 0U; attempt < REGISTER_POLL_LIMIT; ++attempt)
    {
        REGISTER_POLL();
        if ((*reg & mask) == wanted)
        {
            return true;
        }
    }
    return false;
}

canview_status_t canview_stm_watchdog_start(void *context)
{
    (void)context;
    if (hardware.watchdog_ready || hardware.fault)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    hardware.reset_flags = RCC->CSR;
    RCC->CSR |= RCC_CSR_LSION;
    if (!wait_register(&RCC->CSR, RCC_CSR_LSIRDY, RCC_CSR_LSIRDY))
    {
        return CANVIEW_TIMEOUT;
    }
    IWDG->KR = IWDG_ENABLE_KEY;
    IWDG->KR = IWDG_WRITE_KEY;
    IWDG->PR = IWDG_DIV32;
    IWDG->RLR = IWDG_RELOAD;
    IWDG->WINR = IWDG_WINDOW_DISABLED;
    if (!wait_register(&IWDG->SR, IWDG_SR_PVU | IWDG_SR_RVU | IWDG_SR_WVU, 0U))
    {
        return CANVIEW_TIMEOUT;
    }
    IWDG->KR = IWDG_FEED_KEY; /* 초기 설정 반영. 런타임 feed는 scheduler만 허용. */
    hardware.watchdog_ready = true;
    RCC->CSR |= RCC_CSR_RMVF;
    return CANVIEW_OK;
}

canview_status_t canview_stm_clock_start(void *context)
{
    (void)context;
    if (!hardware.watchdog_ready || hardware.clock_ready || hardware.fault ||
        (RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    if (!wait_register(&PWR->SR2, PWR_SR2_VOSF, 0U))
    {
        return CANVIEW_TIMEOUT;
    }
    /* RM0440 Rev9 §6: 중간 HCLK /2 → boost → latency → PLL → 1us → /1. */
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_HPRE_DIV2 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV2;
    PWR->CR5 &= ~PWR_CR5_R1MODE;
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_4WS;
    if (!wait_register(&FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_4WS))
    {
        return CANVIEW_TIMEOUT;
    }
    RCC->CR &= ~(RCC_CR_HSEBYP | RCC_CR_PLLON);
    if (!wait_register(&RCC->CR, RCC_CR_PLLRDY, 0U))
    {
        return CANVIEW_TIMEOUT;
    }
    RCC->CR |= RCC_CR_HSEON;
    if (!wait_register(&RCC->CR, RCC_CR_HSERDY, RCC_CR_HSERDY))
    {
        return CANVIEW_TIMEOUT;
    }
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSE | ((CANVIEW_STM_PLL_M - 1UL) << RCC_PLLCFGR_PLLM_Pos) |
                   (CANVIEW_STM_PLL_N << RCC_PLLCFGR_PLLN_Pos) |
                   ((CANVIEW_STM_PLL_Q / 2UL - 1UL) << RCC_PLLCFGR_PLLQ_Pos) |
                   ((CANVIEW_STM_PLL_R / 2UL - 1UL) << RCC_PLLCFGR_PLLR_Pos) | RCC_PLLCFGR_PLLQEN |
                   RCC_PLLCFGR_PLLREN;
    RCC->CR |= RCC_CR_PLLON;
    if (!wait_register(&RCC->CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY))
    {
        return CANVIEW_TIMEOUT;
    }
    RCC->CCIPR = (RCC->CCIPR & ~(RCC_CCIPR_USART2SEL | RCC_CCIPR_FDCANSEL)) | RCC_CCIPR_FDCANSEL_0;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    if (!wait_register(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL))
    {
        return CANVIEW_TIMEOUT;
    }
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    const uint32_t start = DWT->CYCCNT;
    bool settled = false;
    for (uint32_t attempt = 0U; attempt < REGISTER_POLL_LIMIT; ++attempt)
    {
        REGISTER_POLL();
        if ((uint32_t)(DWT->CYCCNT - start) >= BOOST_SETTLE_CYCLES)
        {
            settled = true;
            break;
        }
    }
    if (!settled)
    {
        return CANVIEW_TIMEOUT;
    }
    RCC->CFGR &= ~RCC_CFGR_HPRE;
    SystemCoreClock = CANVIEW_STM_SYSCLK_HZ;
    RCC->CR |= RCC_CR_CSSON;
    hardware.clock_ready = true;
    return CANVIEW_OK;
}

canview_status_t canview_stm_time_start(void *context)
{
    (void)context;
    if (!hardware.clock_ready || hardware.time_ready || hardware.fault)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    (void)RCC->APB1ENR1;
    TIM2->CR1 = 0U;
    TIM2->DIER = 0U;
    TIM2->PSC = CANVIEW_STM_SYSCLK_HZ / MICROSECOND_HZ - 1U;
    TIM2->ARR = UINT32_MAX;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;
    TIM2->CNT = 0U;
    TIM2->CR1 = TIM_CR1_CEN;
    hardware.milliseconds = 0U;
    if (SysTick_Config(CANVIEW_STM_SYSCLK_HZ / MILLISECOND_HZ) != 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    hardware.time_ready = true;
    return CANVIEW_OK;
}

void SysTick_Handler(void)
{
    ++hardware.milliseconds;
}

uint32_t canview_stm_board_now_ms(void)
{
    return hardware.milliseconds;
}

uint32_t canview_stm_now_us(void *context)
{
    (void)context;
    return TIM2->CNT;
}

canview_status_t canview_stm_board_health(void *context)
{
    (void)context;
    if (hardware.fault || !hardware.time_ready || !hardware.clock_ready ||
        (RCC->CR & (RCC_CR_HSERDY | RCC_CR_PLLRDY)) != (RCC_CR_HSERDY | RCC_CR_PLLRDY) ||
        (RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL || (TIM2->CR1 & TIM_CR1_CEN) == 0U)
    {
        return CANVIEW_TIMEOUT;
    }
    const uint32_t current_ms = hardware.milliseconds;
    const uint32_t current_us = TIM2->CNT;
    if (hardware.health_sampled && current_ms != hardware.previous_health_ms)
    {
        const uint32_t elapsed_ms = current_ms - hardware.previous_health_ms;
        const uint32_t elapsed_us = current_us - hardware.previous_health_us;
        if (elapsed_ms > CANVIEW_STM_HEALTH_WINDOW_MS || elapsed_us == 0U ||
            elapsed_us > (elapsed_ms + 1U) * MILLISECOND_HZ)
        {
            hardware.fault = true;
            return CANVIEW_TIMEOUT;
        }
    }
    if (!hardware.health_sampled || current_ms != hardware.previous_health_ms)
    {
        hardware.previous_health_ms = current_ms;
        hardware.previous_health_us = current_us;
        hardware.health_sampled = true;
    }
    return CANVIEW_OK;
}

canview_status_t canview_stm_watchdog_feed(void *context)
{
    if (!hardware.watchdog_ready || canview_stm_board_health(context) != CANVIEW_OK)
    {
        return CANVIEW_TIMEOUT;
    }
    IWDG->KR = IWDG_FEED_KEY;
    return CANVIEW_OK;
}

void canview_stm_hw_latch_fault(void)
{
    hardware.fault = true;
}

void NMI_Handler(void)
{
    hardware.fault = true;
    if ((RCC->CIFR & RCC_CIFR_CSSF) != 0U)
    {
        RCC->CICR = RCC_CICR_CSSC;
    }
    /* PHY request/ARM/WDI는 이 bench 전체에서 안전 latch로 고정된다. */
}

void HardFault_Handler(void)
{
    hardware.fault = true;
    for (;;)
    {
        __WFI(); /* IWDG refresh 없음. PHY는 reset/default standby. */
    }
}

uint32_t canview_stm_critical_enter(void *context)
{
    (void)context;
    const uint32_t saved = __get_PRIMASK();
    __disable_irq();
    __DMB();
    return saved;
}

void canview_stm_critical_leave(void *context, uint32_t saved_mask)
{
    (void)context;
    __DMB();
    __set_PRIMASK(saved_mask);
}

void canview_stm_board_diagnostic(canview_stm_diagnostic_t *diagnostic)
{
    if (diagnostic != NULL)
    {
        const canview_stm_diagnostic_t snapshot = {
            hardware.reset_flags,
            hardware.clock_ready && !hardware.fault ? CANVIEW_STM_SYSCLK_HZ : 0U,
            hardware.clock_ready && !hardware.fault ? CANVIEW_STM_PCLK_HZ : 0U,
            0U,
            false,
            false,
            false};
        *diagnostic = snapshot;
    }
}

void canview_stm_board_wait_reset(void)
{
    hardware.fault = true;
    if (!hardware.watchdog_ready)
    {
        NVIC_SystemReset();
    }
    for (;;)
    {
        __WFI();
    }
}
