/* SPDX-License-Identifier: GPL-3.0-only */
#include "fake_hardware.h"

#include <string.h>

#include "core_hw.h"
#include "safe_gpio.h"

GPIO_TypeDef fake_gpio_a;
GPIO_TypeDef fake_gpio_b;
RCC_TypeDef fake_rcc;
TIM_TypeDef fake_tim2;
FDCAN_GlobalTypeDef fake_fdcan1;
FDCAN_GlobalTypeDef fake_fdcan2;
FDCAN_GlobalTypeDef fake_fdcan3;
USART_TypeDef fake_usart2;
DMA_TypeDef fake_dma1;
DMA_Channel_TypeDef fake_dma1_channel1;
DMA_Channel_TypeDef fake_dma1_channel2;
DMAMUX_Channel_TypeDef fake_dmamux1_channel0;
DMAMUX_Channel_TypeDef fake_dmamux1_channel1;
RNG_TypeDef fake_rng;
uint8_t fake_sramcan[4096];
uint32_t fake_now_us;
uint32_t fake_output_calls;
uint32_t fake_output_fail_call;
uint32_t fake_output_fail_call_2;
uint32_t fake_wait_calls;
uint32_t fake_wait_fail_call;
uint32_t fake_wait_poll_mismatch_call;
uint64_t fake_now_us64;
volatile uint32_t canview_test_stm_uid[3];
bool fake_outputs[2][16];

void fake_hardware_reset(void)
{
    memset(&fake_gpio_a, 0, sizeof(fake_gpio_a));
    memset(&fake_gpio_b, 0, sizeof(fake_gpio_b));
    memset(&fake_rcc, 0, sizeof(fake_rcc));
    memset(&fake_tim2, 0, sizeof(fake_tim2));
    memset(&fake_fdcan1, 0, sizeof(fake_fdcan1));
    memset(&fake_fdcan2, 0, sizeof(fake_fdcan2));
    memset(&fake_fdcan3, 0, sizeof(fake_fdcan3));
    memset(&fake_usart2, 0, sizeof(fake_usart2));
    memset(&fake_dma1, 0, sizeof(fake_dma1));
    memset(&fake_dma1_channel1, 0, sizeof(fake_dma1_channel1));
    memset(&fake_dma1_channel2, 0, sizeof(fake_dma1_channel2));
    memset(&fake_dmamux1_channel0, 0, sizeof(fake_dmamux1_channel0));
    memset(&fake_dmamux1_channel1, 0, sizeof(fake_dmamux1_channel1));
    memset(&fake_rng, 0, sizeof(fake_rng));
    memset(fake_sramcan, 0, sizeof(fake_sramcan));
    memset(fake_outputs, 0, sizeof(fake_outputs));
    fake_now_us = 1U;
    fake_now_us64 = 1000U;
    memset((void *)canview_test_stm_uid, 0, sizeof(canview_test_stm_uid));
    fake_output_calls = 0U;
    fake_output_fail_call = 0U;
    fake_output_fail_call_2 = 0U;
    fake_wait_calls = 0U;
    fake_wait_fail_call = 0U;
    fake_wait_poll_mismatch_call = 0U;
}

void fake_hardware_ready(void)
{
    fake_rcc.CR = RCC_CR_HSERDY | RCC_CR_PLLRDY;
    fake_rcc.CFGR = RCC_CFGR_SWS_PLL;
    fake_rcc.CCIPR = RCC_CCIPR_FDCANSEL_0;
    fake_tim2.CR1 = TIM_CR1_CEN;
    fake_tim2.PSC = 159U;
    fake_tim2.ARR = UINT32_MAX;
}

bool canview_stm_fdcan_test_wait_should_timeout(void)
{
    ++fake_wait_calls;
    if (fake_wait_poll_mismatch_call != 0U &&
        fake_wait_calls == fake_wait_poll_mismatch_call)
    {
        fake_fdcan1.CCCR = 0U;
        fake_fdcan2.CCCR = 0U;
        fake_fdcan3.CCCR = 0U;
    }
    return fake_wait_fail_call != 0U && fake_wait_calls == fake_wait_fail_call;
}

uint32_t canview_stm_now_us(void *context)
{
    (void)context;
    return fake_now_us++;
}

uint64_t canview_stm_now_us64(void *context)
{
    (void)context;
    return fake_now_us64;
}

uint64_t canview_stm_now_ms64(void *context)
{
    return canview_stm_now_us64(context) / UINT64_C(1000);
}

uint32_t canview_stm_critical_enter(void *context)
{
    (void)context;
    return 0U;
}

void canview_stm_critical_leave(void *context, uint32_t saved_mask)
{
    (void)context;
    (void)saved_mask;
}

canview_status_t canview_stm_output(uint8_t port, uint8_t pin, bool high)
{
    ++fake_output_calls;
    if ((fake_output_fail_call != 0U && fake_output_calls == fake_output_fail_call) ||
        (fake_output_fail_call_2 != 0U && fake_output_calls == fake_output_fail_call_2))
    {
        return CANVIEW_TIMEOUT;
    }
    if (port >= 2U || pin >= 16U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    fake_outputs[port][pin] = high;
    return CANVIEW_OK;
}
