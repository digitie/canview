/* SPDX-License-Identifier: GPL-3.0-only */
#include "safe_gpio.h"
#include "stm32g474xx.h"

canview_status_t canview_stm_output(uint8_t port, uint8_t pin, bool high)
{
    if (port > 1U || pin > 15U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    (void)RCC->AHB2ENR;
    GPIO_TypeDef *const gpio = port == 0U ? GPIOA : GPIOB;
    gpio->BSRR = UINT32_C(1) << (high ? pin : (uint32_t)pin + 16U);
    gpio->OTYPER &= ~(UINT32_C(1) << pin);
    gpio->PUPDR &= ~(UINT32_C(3) << ((uint32_t)pin * 2U));
    gpio->MODER = (gpio->MODER & ~(UINT32_C(3) << ((uint32_t)pin * 2U))) |
                  (UINT32_C(1) << ((uint32_t)pin * 2U));
    return CANVIEW_OK;
}

void canview_stm_idle(void *context)
{
    (void)context;
    __WFI();
}
