/* SPDX-License-Identifier: GPL-3.0-only */
/** @file boot_runtime.c @brief reset HSI16의 bounded boot progress와 IWDG. */
#include "canview_boot_runtime.h"
#include "canview_boot_flash.h"
#include "flash_layout.h"
#include <stdint.h>
#if defined(CANVIEW_STM_REGISTER_TEST)
#if defined(__arm__) || defined(__thumb__)
#error Host_register_model_must_not_be_built_for_target
#endif
#include "register_model.h"
#define BOOT_POLL() canview_stm_test_poll()
#define BOOT_IWDG_KEY(value) canview_stm_boot_test_key(value)
#else
#include "stm32g474xx.h"
#define BOOT_POLL() ((void)0)
#define BOOT_IWDG_KEY(value) (IWDG->KR = (value))
#endif

#define BOOT_HSI_HZ (UINT32_C(16000000))
#define BOOT_DEADLINE_CYCLES (BOOT_HSI_HZ * UINT32_C(30))
#define BOOT_FEED_INTERVAL_CYCLES (BOOT_HSI_HZ / UINT32_C(10))
#define BOOT_POLL_LIMIT (UINT32_C(1000000))
#define BOOT_TICK_POLL_LIMIT (UINT32_C(1024))
#define BOOT_IWDG_ENABLE (UINT32_C(0xcccc))
#define BOOT_IWDG_WRITE (UINT32_C(0x5555))
#define BOOT_IWDG_FEED (UINT32_C(0xaaaa))
#define BOOT_IWDG_DIV256 (UINT32_C(6))
#define BOOT_IWDG_RELOAD (UINT32_C(4095))
#define BOOT_CLOCK_MASK (RCC_CFGR_SW | RCC_CFGR_SWS | RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)
#define BOOT_CLOCK_VALUE (RCC_CFGR_SW_HSI | RCC_CFGR_SWS_HSI)

/* MCU IWDG 단일 owner. 일반 앱 core_hw와 함께 link하지 않는다. ISR 공유 상태 없음. */
typedef struct
{
    uint32_t start;
    uint32_t last_feed;
    uint32_t last_sample;
    bool attempted;
    bool ready;
} boot_runtime_t;
static boot_runtime_t runtime;

#if defined(CANVIEW_STM_REGISTER_TEST)
void canview_stm_boot_test_reset(void)
{
    const boot_runtime_t cleared = {0};
    runtime = cleared;
}
#endif

static bool boot_clock_valid(void)
{
    return (RCC->CFGR & BOOT_CLOCK_MASK) == BOOT_CLOCK_VALUE &&
        (RCC->CR & (RCC_CR_HSION | RCC_CR_HSIRDY)) == (RCC_CR_HSION | RCC_CR_HSIRDY) &&
        (RCC->CR & (RCC_CR_HSEON | RCC_CR_PLLON | RCC_CR_CSSON)) == 0U;
}

static bool boot_context_valid(void)
{
    return __get_IPSR() == 0U && __get_CONTROL() == 0U &&
        __get_BASEPRI() == 0U && __get_FAULTMASK() == 0U &&
        __get_PRIMASK() == 0U && SCB->VTOR == CANVIEW_STM_FLASH_BASE;
}

static bool boot_counter_ticking(void)
{
    const uint32_t initial = DWT->CYCCNT;
    for (uint32_t count = 0U; count < BOOT_TICK_POLL_LIMIT; ++count)
    {
        BOOT_POLL();
        if (DWT->CYCCNT != initial) { return true; }
    }
    return false;
}

canview_status_t canview_boot_runtime_start(void)
{
    if (runtime.attempted) { return CANVIEW_RESOURCE_BUSY; }
    runtime.attempted = true;
    if (!boot_context_valid() || !boot_clock_valid() || IWDG->WINR != BOOT_IWDG_RELOAD)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB();
    __ISB();
    if (!boot_counter_ticking()) { return CANVIEW_TIMEOUT; }
    RCC->CSR |= RCC_CSR_LSION;
    bool oscillator_ready = false;
    for (uint32_t count = 0U; count < BOOT_POLL_LIMIT; ++count)
    {
        BOOT_POLL();
        if ((RCC->CSR & RCC_CSR_LSIRDY) != 0U) { oscillator_ready = true; break; }
    }
    if (!oscillator_ready) { return CANVIEW_TIMEOUT; }
    BOOT_IWDG_KEY(BOOT_IWDG_ENABLE);
    BOOT_IWDG_KEY(BOOT_IWDG_WRITE);
    IWDG->PR = BOOT_IWDG_DIV256;
    IWDG->RLR = BOOT_IWDG_RELOAD;
    /* WINR reset값을 유지한다. WINR 쓰기는 암묵적으로 reload하므로 하지 않는다. */
    bool watchdog_ready = false;
    for (uint32_t count = 0U; count < BOOT_POLL_LIMIT; ++count)
    {
        BOOT_POLL();
        if (IWDG->SR == 0U) { watchdog_ready = true; break; }
    }
    if (!watchdog_ready) { return CANVIEW_TIMEOUT; }
    runtime.start = DWT->CYCCNT;
    runtime.last_sample = runtime.start;
    runtime.ready = true;
    if (!canview_boot_runtime_ready()) { return CANVIEW_TIMEOUT; }
    runtime.last_feed = DWT->CYCCNT;
    BOOT_IWDG_KEY(BOOT_IWDG_FEED); /* 설정 완료 시 한 번. 이후 검증된 progress에서만 feed. */
    return CANVIEW_OK;
}

bool canview_boot_runtime_ready(void)
{
    if (!runtime.ready) { return false; }
    if (!boot_context_valid() || !boot_clock_valid() ||
        (CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0U ||
        (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U ||
        (RCC->CSR & (RCC_CSR_LSION | RCC_CSR_LSIRDY)) != (RCC_CSR_LSION | RCC_CSR_LSIRDY) || IWDG->SR != 0U ||
        IWDG->PR != BOOT_IWDG_DIV256 || IWDG->RLR != BOOT_IWDG_RELOAD || IWDG->WINR != BOOT_IWDG_RELOAD ||
        !boot_counter_ticking() ||
        (uint32_t)(DWT->CYCCNT - runtime.start) >= BOOT_DEADLINE_CYCLES ||
        (uint32_t)(DWT->CYCCNT - runtime.last_sample) >= BOOT_DEADLINE_CYCLES)
    {
        runtime.ready = false;
    }
    if (runtime.ready) { runtime.last_sample = DWT->CYCCNT; }
    return runtime.ready;
}

void canview_boot_progress(void)
{
    if (!canview_boot_runtime_ready()) { return; }
    const uint32_t now = DWT->CYCCNT;
    if ((uint32_t)(now - runtime.start) >= BOOT_DEADLINE_CYCLES)
    {
        runtime.ready = false;
        return;
    }
    if ((uint32_t)(now - runtime.last_feed) >= BOOT_FEED_INTERVAL_CYCLES)
    {
        BOOT_IWDG_KEY(BOOT_IWDG_FEED);
        runtime.last_feed = now;
    }
}
