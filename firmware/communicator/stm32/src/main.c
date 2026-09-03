#include "stm32g474xx.h"

static void transceivers_enter_safe_state(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    /* Set output levels before switching PA4..PA7 to output mode.
     * TCAN STB1/STB2 high: standby.
     * MAX3055 STB/EN low: sleep/standby family, no normal transmission.
     */
    GPIOA->BSRR = GPIO_BSRR_BS4 | GPIO_BSRR_BS5 |
                  GPIO_BSRR_BR6 | GPIO_BSRR_BR7;

    GPIOA->MODER = (GPIOA->MODER &
                    ~(GPIO_MODER_MODE4_Msk |
                      GPIO_MODER_MODE5_Msk |
                      GPIO_MODER_MODE6_Msk |
                      GPIO_MODER_MODE7_Msk)) |
                   (GPIO_MODER_MODE4_0 |
                    GPIO_MODER_MODE5_0 |
                    GPIO_MODER_MODE6_0 |
                    GPIO_MODER_MODE7_0);
}
int main(void)
{
    transceivers_enter_safe_state();

    /* Deliberately no CAN/UART enable here. This scaffold proves the CMake,
     * startup, linker, and reset-safe GPIO path before generated peripheral
     * initialization is reviewed and added.
     */
    for (;;) {
        __WFI();
    }
}
