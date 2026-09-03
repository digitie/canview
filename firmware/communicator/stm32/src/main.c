#include "stm32g474xx.h"

static void transceivers_enter_safe_state(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    /* Set output levels before switching PA4..PA6 to output mode.
     * TCAN STB1/STB2 high: standby.
     * MAX3055 STB is fixed high in hardware; EN low: Power-On Standby.
     */
    GPIOA->BSRR = GPIO_BSRR_BS4 | GPIO_BSRR_BS5 | GPIO_BSRR_BR6;

    GPIOA->MODER = (GPIOA->MODER &
                    ~(GPIO_MODER_MODE4_Msk |
                      GPIO_MODER_MODE5_Msk |
                      GPIO_MODER_MODE6_Msk)) |
                   (GPIO_MODER_MODE4_0 |
                    GPIO_MODER_MODE5_0 |
                    GPIO_MODER_MODE6_0);
}

int main(void)
{
    transceivers_enter_safe_state();

    /* Deliberately no CAN/UART enable here. This scaffold proves the CMake,
     * startup, linker, and reset-safe GPIO path before generated peripheral
     * initialization is reviewed and added. A production boot must validate
     * HSE and start all FDCAN instances before changing PA4, PA5, or PA6.
     */
    for (;;) {
        __WFI();
    }
}
