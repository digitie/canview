/* SPDX-License-Identifier: GPL-3.0-only */
/* Host-only named register model. Bit values: CubeG4 v1.6.3 stm32g474xx.h.
 * Not an electrical/peripheral timing emulator or a target CMSIS replacement. */
#ifndef CANVIEW_STM_REGISTER_MODEL_H
#define CANVIEW_STM_REGISTER_MODEL_H
#include <stdint.h>
typedef struct
{
    volatile uint32_t CR, CFGR, PLLCFGR, CSR, APB1ENR1, CCIPR, CIFR, CICR;
} model_rcc_t;
typedef struct
{
    volatile uint32_t CR1, CR5, SR2;
} model_pwr_t;
typedef struct
{
    volatile uint32_t ACR;
} model_flash_t;
typedef struct
{
    volatile uint32_t KR, PR, RLR, WINR, SR;
} model_iwdg_t;
typedef struct
{
    volatile uint32_t CR1, DIER, PSC, ARR, EGR, SR, CNT;
} model_timer_t;
typedef struct
{
    volatile uint32_t DEMCR;
} model_debug_t;
typedef struct
{
    volatile uint32_t CTRL, CYCCNT;
} model_dwt_t;
extern model_rcc_t model_rcc;
extern model_pwr_t model_pwr;
extern model_flash_t model_flash;
extern model_iwdg_t model_iwdg;
extern model_timer_t model_timer;
extern model_debug_t model_debug;
extern model_dwt_t model_dwt;
extern uint32_t SystemCoreClock;
#define RCC (&model_rcc)
#define PWR (&model_pwr)
#define FLASH (&model_flash)
#define IWDG (&model_iwdg)
#define TIM2 (&model_timer)
#define CoreDebug (&model_debug)
#define DWT (&model_dwt)
#define RCC_CSR_LSION UINT32_C(1)
#define RCC_CSR_LSIRDY UINT32_C(2)
#define RCC_CSR_RMVF UINT32_C(0x800000)
#define RCC_CR_HSEON UINT32_C(0x10000)
#define RCC_CR_HSERDY UINT32_C(0x20000)
#define RCC_CR_HSEBYP UINT32_C(0x40000)
#define RCC_CR_CSSON UINT32_C(0x80000)
#define RCC_CR_PLLON UINT32_C(0x1000000)
#define RCC_CR_PLLRDY UINT32_C(0x2000000)
#define RCC_CFGR_SW UINT32_C(3)
#define RCC_CFGR_SW_PLL UINT32_C(3)
#define RCC_CFGR_SWS UINT32_C(12)
#define RCC_CFGR_SWS_PLL UINT32_C(12)
#define RCC_CFGR_SWS_HSI UINT32_C(4)
#define RCC_CFGR_HPRE UINT32_C(0xf0)
#define RCC_CFGR_HPRE_DIV2 UINT32_C(0x80)
#define RCC_CFGR_PPRE1 UINT32_C(0x700)
#define RCC_CFGR_PPRE1_DIV2 UINT32_C(0x400)
#define RCC_CFGR_PPRE2 UINT32_C(0x3800)
#define RCC_CFGR_PPRE2_DIV2 UINT32_C(0x2000)
#define RCC_APB1ENR1_PWREN UINT32_C(0x10000000)
#define RCC_APB1ENR1_TIM2EN UINT32_C(1)
#define PWR_CR1_VOS UINT32_C(0x600)
#define PWR_CR1_VOS_0 UINT32_C(0x200)
#define PWR_SR2_VOSF UINT32_C(0x400)
#define PWR_CR5_R1MODE UINT32_C(0x100)
#define FLASH_ACR_LATENCY UINT32_C(15)
#define FLASH_ACR_LATENCY_4WS UINT32_C(4)
#define RCC_PLLCFGR_PLLSRC_HSE UINT32_C(3)
#define RCC_PLLCFGR_PLLM_Pos (4U)
#define RCC_PLLCFGR_PLLN_Pos (8U)
#define RCC_PLLCFGR_PLLQ_Pos (21U)
#define RCC_PLLCFGR_PLLR_Pos (25U)
#define RCC_PLLCFGR_PLLQEN UINT32_C(0x100000)
#define RCC_PLLCFGR_PLLREN UINT32_C(0x1000000)
#define RCC_CCIPR_USART2SEL UINT32_C(0xc)
#define RCC_CCIPR_FDCANSEL UINT32_C(0x3000000)
#define RCC_CCIPR_FDCANSEL_0 UINT32_C(0x1000000)
#define CoreDebug_DEMCR_TRCENA_Msk UINT32_C(0x1000000)
#define DWT_CTRL_CYCCNTENA_Msk UINT32_C(1)
#define IWDG_SR_PVU UINT32_C(1)
#define IWDG_SR_RVU UINT32_C(2)
#define IWDG_SR_WVU UINT32_C(4)
#define TIM_EGR_UG UINT32_C(1)
#define TIM_CR1_CEN UINT32_C(1)
#define RCC_CIFR_CSSF UINT32_C(0x100)
#define RCC_CICR_CSSC UINT32_C(0x100)
void canview_stm_test_poll(void);
void canview_stm_test_reset(void);
uint32_t model_systick_config(uint32_t ticks);
void model_wait(void);
uint32_t model_primask(void);
void model_disable_irq(void);
void model_set_primask(uint32_t mask);
void model_reset(void);
#define SysTick_Config(ticks) model_systick_config(ticks)
#define __WFI() model_wait()
#define __get_PRIMASK() model_primask()
#define __disable_irq() model_disable_irq()
#define __set_PRIMASK(mask) model_set_primask(mask)
#define __DMB() ((void)0)
#define NVIC_SystemReset() model_reset()
#endif
