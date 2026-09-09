/* SPDX-License-Identifier: GPL-3.0-only */
#include "core_hw.h"
#include "canview_stm_build.h"
#include "canview_stm_service.h"
#include "canview_stm_stack.h"
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
#define STACK_GUARD_BYTES (64U)
#define TIM2_PRESCALER (CANVIEW_STM_SYSCLK_HZ / MICROSECOND_HZ - 1U)
#define HEALTH_SAMPLE_INTERVAL_MS (4U)
#define HEALTH_MIN_TIMER_US_PER_MS (500U)

#if defined(CANVIEW_STM_REGISTER_TEST)
#define REGISTER_TEST_STACK_BYTES (4096U)
static uint8_t register_test_stack[REGISTER_TEST_STACK_BYTES];
static uintptr_t register_test_stack_pointer;

uintptr_t canview_stm_test_stack_top(void)
{
    return register_test_stack_pointer;
}

uintptr_t canview_stm_test_stack_low(void)
{
    return (uintptr_t)register_test_stack;
}

void canview_stm_test_set_stack_pointer(uintptr_t stack_pointer)
{
    register_test_stack_pointer = stack_pointer;
}

void canview_stm_test_corrupt_stack(void)
{
    for (size_t index = 0U; index < sizeof(register_test_stack); ++index)
    {
        register_test_stack[index] = 0U;
    }
}
#else
extern uint8_t __stack_limit;
extern uint8_t _estack;
#endif

/* 단일 MCU context. ISR 공유 member에만 volatile을 적용한다. */
typedef struct
{
    volatile uint32_t milliseconds;
    uint32_t timer2_last_count;
    uint64_t timer2_epoch;
    bool timer2_epoch_valid;
    volatile bool fault;
    uint32_t reset_flags;
    canview_stm_reset_reason_t reset_reason;
    uint32_t previous_health_ms;
    uint32_t previous_health_us;
    uint32_t health_window_ms;
    uint32_t health_window_us;
    bool health_sampled;
    bool watchdog_ready;
    bool clock_ready;
    bool time_ready;
    canview_stm_stack_watermark_t stack_watermark;
} canview_stm_hardware_t;
static canview_stm_hardware_t hardware;

#if defined(CANVIEW_STM_REGISTER_TEST)
void canview_stm_test_reset(void)
{
    const canview_stm_hardware_t cleared = {0};
    hardware = cleared;
    register_test_stack_pointer = (uintptr_t)(register_test_stack + sizeof(register_test_stack));
}
#endif

void SysTick_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);

static canview_stm_reset_reason_t reset_reason_from_flags(uint32_t flags)
{
    uint32_t categories = 0U;
    if ((flags & (RCC_CSR_IWDGRSTF | RCC_CSR_WWDGRSTF)) != 0U)
    {
        categories |= UINT32_C(1);
    }
    if ((flags & RCC_CSR_BORRSTF) != 0U)
    {
        categories |= UINT32_C(2);
    }
    if ((flags & RCC_CSR_SFTRSTF) != 0U)
    {
        categories |= UINT32_C(4);
    }
    if ((flags & RCC_CSR_PINRSTF) != 0U)
    {
        categories |= UINT32_C(8);
    }
    if ((flags & RCC_CSR_LPWRRSTF) != 0U)
    {
        categories |= UINT32_C(16);
    }
    if ((flags & RCC_CSR_OBLRSTF) != 0U)
    {
        categories |= UINT32_C(32);
    }
    if (categories == 0U)
    {
        return CANVIEW_STM_RESET_REASON_UNKNOWN;
    }
    if ((categories & (categories - 1U)) != 0U)
    {
        return CANVIEW_STM_RESET_REASON_AMBIGUOUS;
    }
    if ((categories & UINT32_C(1)) != 0U)
    {
        return CANVIEW_STM_RESET_REASON_WATCHDOG;
    }
    if ((categories & UINT32_C(2)) != 0U)
    {
        return CANVIEW_STM_RESET_REASON_BROWNOUT;
    }
    if ((categories & UINT32_C(4)) != 0U)
    {
        return CANVIEW_STM_RESET_REASON_SOFTWARE;
    }
    if ((categories & UINT32_C(8)) != 0U)
    {
        return CANVIEW_STM_RESET_REASON_PIN;
    }
    if ((categories & UINT32_C(16)) != 0U)
    {
        return CANVIEW_STM_RESET_REASON_LOW_POWER;
    }
    return CANVIEW_STM_RESET_REASON_OPTION_BYTE;
}

static canview_status_t arm_stack_watermark(void)
{
#if defined(CANVIEW_STM_REGISTER_TEST)
    const uintptr_t stack_low = (uintptr_t)register_test_stack;
    const uintptr_t stack_top = (uintptr_t)(register_test_stack + sizeof(register_test_stack));
#else
    const uintptr_t stack_low = (uintptr_t)&__stack_limit;
    const uintptr_t stack_top = (uintptr_t)&_estack;
#endif
    const uintptr_t current_sp = (uintptr_t)__get_MSP();
    if (stack_low >= stack_top || current_sp <= stack_low || current_sp > stack_top ||
        current_sp - stack_low <= (uintptr_t)STACK_GUARD_BYTES)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const uintptr_t region_size = current_sp - stack_low - (uintptr_t)STACK_GUARD_BYTES;
    if (region_size > (uintptr_t)SIZE_MAX)
    {
        return CANVIEW_OVERSIZE;
    }
    return canview_stm_stack_watermark_arm(
        &hardware.stack_watermark, (volatile uint8_t *)stack_low, (size_t)region_size);
}

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
    hardware.reset_reason = reset_reason_from_flags(hardware.reset_flags);
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
    TIM2->PSC = TIM2_PRESCALER;
    TIM2->ARR = UINT32_MAX;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;
    TIM2->CNT = 0U;
    TIM2->CR1 = TIM_CR1_CEN;
    hardware.milliseconds = 0U;
    hardware.timer2_last_count = 0U;
    hardware.timer2_epoch = 0U;
    hardware.timer2_epoch_valid = false;
    if (SysTick_Config(CANVIEW_STM_SYSCLK_HZ / MILLISECOND_HZ) != 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t stack_status = arm_stack_watermark();
    if (stack_status != CANVIEW_OK)
    {
        hardware.fault = true;
        return stack_status;
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

uint64_t canview_stm_now_us64(void *context)
{
    (void)context;
    const uint32_t saved_mask = canview_stm_critical_enter(NULL);
    const uint32_t current = TIM2->CNT;
    if (!hardware.timer2_epoch_valid)
    {
        hardware.timer2_last_count = current;
        hardware.timer2_epoch_valid = true;
    }
    else if (current < hardware.timer2_last_count &&
             hardware.timer2_epoch <= UINT64_MAX - UINT64_C(0x100000000))
    {
        hardware.timer2_epoch += UINT64_C(0x100000000);
        hardware.timer2_last_count = current;
    }
    else if (current >= hardware.timer2_last_count)
    {
        hardware.timer2_last_count = current;
    }
    uint64_t timestamp = hardware.timer2_epoch;
    if (timestamp > UINT64_MAX - (uint64_t)current)
    {
        timestamp = UINT64_MAX;
    }
    else
    {
        timestamp += (uint64_t)current;
    }
    canview_stm_critical_leave(NULL, saved_mask);
    return timestamp;
}

#if defined(CANVIEW_STM_REGISTER_TEST)
void canview_stm_test_set_timer2_extension(uint32_t last_count, uint64_t epoch, bool valid)
{
    hardware.timer2_last_count = last_count;
    hardware.timer2_epoch = epoch;
    hardware.timer2_epoch_valid = valid;
}
#endif

canview_status_t canview_stm_board_health(void *context)
{
    (void)context;
    if (hardware.fault || !hardware.time_ready || !hardware.clock_ready ||
        (RCC->CR & (RCC_CR_HSERDY | RCC_CR_PLLRDY)) != (RCC_CR_HSERDY | RCC_CR_PLLRDY) ||
        (RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL || (TIM2->CR1 & TIM_CR1_CEN) == 0U ||
        TIM2->PSC != TIM2_PRESCALER || TIM2->ARR != UINT32_MAX || TIM2->DIER != 0U)
    {
        hardware.fault = true;
        return CANVIEW_TIMEOUT;
    }
    canview_stm_stack_watermark_snapshot_t stack_snapshot;
    if (canview_stm_stack_watermark_sample(&hardware.stack_watermark, &stack_snapshot) !=
            CANVIEW_OK ||
        !stack_snapshot.valid ||
        stack_snapshot.minimum_free_bytes < CANVIEW_STM_STACK_MIN_FREE_BYTES)
    {
        hardware.fault = true;
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
        hardware.previous_health_ms = current_ms;
        hardware.previous_health_us = current_us;
    }
    if (!hardware.health_sampled)
    {
        hardware.previous_health_ms = current_ms;
        hardware.previous_health_us = current_us;
        hardware.health_window_ms = current_ms;
        hardware.health_window_us = current_us;
        hardware.health_sampled = true;
    }
    else if ((uint32_t)(current_ms - hardware.health_window_ms) >= HEALTH_SAMPLE_INTERVAL_MS)
    {
        const uint32_t elapsed_ms = current_ms - hardware.health_window_ms;
        const uint32_t elapsed_us = current_us - hardware.health_window_us;
        if (elapsed_us < elapsed_ms * HEALTH_MIN_TIMER_US_PER_MS)
        {
            hardware.fault = true;
            return CANVIEW_TIMEOUT;
        }
        hardware.health_window_ms = current_ms;
        hardware.health_window_us = current_us;
    }
    return CANVIEW_OK;
}

canview_status_t canview_stm_watchdog_feed(void *context)
{
    if (!hardware.watchdog_ready || canview_stm_board_health(context) != CANVIEW_OK)
    {
        return CANVIEW_TIMEOUT;
    }
#if defined(CANVIEW_STM_REGISTER_TEST)
    canview_stm_test_before_feed();
#endif
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
    /* NMI는 중단된 feed 명령으로 복귀하지 않는다. 부팅 전 fault도 즉시 reset. */
    NVIC_SystemReset();
    for (;;)
    {
        __WFI();
    }
}

void HardFault_Handler(void)
{
    hardware.fault = true;
    NVIC_SystemReset();
    for (;;)
    {
        __WFI(); /* reset이 반환하는 비정상 구현에서도 IWDG refresh는 금지한다. */
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
        canview_stm_build_metadata_t metadata = {0};
        const canview_status_t metadata_status = canview_stm_build_metadata_get(&metadata);
        canview_stm_stack_watermark_snapshot_t stack_snapshot = {0U, 0U, false};
        const canview_status_t stack_status = canview_stm_stack_watermark_sample(
            &hardware.stack_watermark, &stack_snapshot);
        canview_stm_service_decision_t service_decision = {0};
        const canview_stm_service_root_t unloaded_root = {0};
        const canview_status_t service_status = canview_stm_service_policy_evaluate(
            &unloaded_root, hardware.reset_reason, &service_decision);
        const bool service_policy_safe = service_status != CANVIEW_OK;
        const bool capture_only_contract_valid =
            canview_stm_capture_only_contract_anchor ==
            CANVIEW_STM_CAPTURE_ONLY_CONTRACT_ANCHOR_VALUE;
        const canview_stm_diagnostic_t snapshot = {
            .reset_flags = hardware.reset_flags,
            .reset_reason = hardware.reset_reason,
            .sysclk_hz = hardware.clock_ready && !hardware.fault ? CANVIEW_STM_SYSCLK_HZ : 0U,
            .peripheral_hz = hardware.clock_ready && !hardware.fault ? CANVIEW_STM_PCLK_HZ : 0U,
            .control_capabilities = service_policy_safe ? 0U : service_decision.control_capabilities,
            .tx_permit = service_policy_safe ? false : service_decision.tx_permit,
            .authenticity_known = service_policy_safe ? false : service_decision.authenticity_known,
            .production_debug_lock_known = service_policy_safe
                                               ? false
                                               : service_decision.production_debug_lock_known,
            .build_metadata_valid = metadata_status == CANVIEW_OK && capture_only_contract_valid,
            .build_contract_digest = metadata.build_contract_digest,
            .board_profile = metadata.board_profile,
            .stack_free_bytes = stack_status == CANVIEW_OK ? stack_snapshot.current_free_bytes : 0U,
            .stack_min_free_bytes = stack_status == CANVIEW_OK
                                        ? stack_snapshot.minimum_free_bytes
                                        : 0U,
            .stack_watermark_valid = stack_status == CANVIEW_OK && stack_snapshot.valid,
            .service_reset_erase_pending = service_policy_safe
                                               ? false
                                               : service_decision.erase_on_service_reset,
            .protocol_schema_sha256 = metadata.protocol_schema_sha256,
            .uart_protocol_schema_sha256 = metadata.uart_protocol_schema_sha256,
            .hardware_digest = metadata.hardware_digest,
            .build_mode = metadata.build_mode};
        *diagnostic = snapshot;
    }
}

canview_status_t canview_stm_board_diagnostic_encode(uint8_t *buffer, size_t capacity,
                                                     size_t *written)
{
    canview_stm_diagnostic_t diagnostic;
    canview_stm_board_diagnostic(&diagnostic);
    return canview_stm_diagnostic_encode(&diagnostic, buffer, capacity, written);
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
