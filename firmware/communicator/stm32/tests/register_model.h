/* SPDX-License-Identifier: GPL-3.0-only */
/* Host-only named register model. Bit values: CubeG4 v1.6.3 stm32g474xx.h.
 * Not an electrical/peripheral timing emulator or a target CMSIS replacement. */
#ifndef CANVIEW_STM_REGISTER_MODEL_H
#define CANVIEW_STM_REGISTER_MODEL_H
#include <stdint.h>
typedef struct
{
    volatile uint32_t CR, CFGR, PLLCFGR, CSR, APB1ENR1, APB2ENR, CCIPR, CIFR, CICR;
} model_rcc_t;
typedef struct
{
    volatile uint32_t CR1, CR5, SR2;
} model_pwr_t;
typedef struct
{
    volatile uint32_t ACR, SR, OPTR, WRP1AR, WRP1BR, WRP2AR, WRP2BR, CR, KEYR, ECCR;
} model_flash_t;
typedef struct
{
    volatile uint32_t MEMRMP, CFGR2;
} model_syscfg_t;
extern model_syscfg_t model_syscfg;
extern uint16_t model_flash_size_kib;
#define SYSCFG (&model_syscfg)
#define RCC_APB2ENR_SYSCFGEN UINT32_C(1)
#define FLASH_SR_BSY UINT32_C(0x10000)
#define FLASH_SR_OPTVERR UINT32_C(0x8000)
#define FLASH_SR_EOP UINT32_C(1)
#define FLASH_SR_OPERR UINT32_C(2)
#define FLASH_SR_PROGERR UINT32_C(8)
#define FLASH_SR_WRPERR UINT32_C(0x10)
#define FLASH_SR_PGAERR UINT32_C(0x20)
#define FLASH_SR_SIZERR UINT32_C(0x40)
#define FLASH_SR_PGSERR UINT32_C(0x80)
#define FLASH_SR_MISERR UINT32_C(0x100)
#define FLASH_SR_FASTERR UINT32_C(0x200)
#define FLASH_SR_RDERR UINT32_C(0x4000)
#define FLASH_CR_PG UINT32_C(1)
#define FLASH_CR_PER UINT32_C(2)
#define FLASH_CR_PNB UINT32_C(0x3f8)
#define FLASH_CR_PNB_Pos (3U)
#define FLASH_CR_BKER UINT32_C(0x800)
#define FLASH_CR_STRT UINT32_C(0x10000)
#define FLASH_CR_OPTLOCK UINT32_C(0x40000000)
#define FLASH_CR_LOCK UINT32_C(0x80000000)
#define FLASH_ACR_ICEN UINT32_C(0x200)
#define FLASH_ACR_DCEN UINT32_C(0x400)
#define FLASH_ACR_ICRST UINT32_C(0x800)
#define FLASH_ACR_DCRST UINT32_C(0x1000)
#define FLASH_OPTR_RDP UINT32_C(0xff)
#define FLASH_ECCR_ECCIE UINT32_C(0x1000000)
#define FLASH_ECCR_ECCC2 UINT32_C(0x10000000)
#define FLASH_ECCR_ECCD2 UINT32_C(0x20000000)
#define FLASH_ECCR_ECCC UINT32_C(0x40000000)
#define FLASH_ECCR_ECCD UINT32_C(0x80000000)
#define RCC_CIFR_LSECSSF UINT32_C(0x200)
#define SYSCFG_CFGR2_SPF UINT32_C(0x100)
#define FLASH_OPTR_DBANK UINT32_C(0x400000)
#define FLASH_OPTR_BFB2 UINT32_C(0x100000)
#define FLASH_OPTR_NRST_MODE UINT32_C(0x30000000)
#define SYSCFG_MEMRMP_FB_MODE UINT32_C(0x100)
#define FLASH_WRP1AR_WRP1A_STRT UINT32_C(0x7f)
#define FLASH_WRP1AR_WRP1A_END UINT32_C(0x7f0000)
#define FLASH_WRP1AR_WRP1A_END_Pos (16U)
#define FLASH_WRP1BR_WRP1B_STRT UINT32_C(0x7f)
#define FLASH_WRP1BR_WRP1B_END UINT32_C(0x7f0000)
#define FLASH_WRP1BR_WRP1B_END_Pos (16U)
#define FLASH_WRP2AR_WRP2A_STRT UINT32_C(0x7f)
#define FLASH_WRP2AR_WRP2A_END UINT32_C(0x7f0000)
#define FLASH_WRP2AR_WRP2A_END_Pos (16U)
#define FLASH_WRP2BR_WRP2B_STRT UINT32_C(0x7f)
#define FLASH_WRP2BR_WRP2B_END UINT32_C(0x7f0000)
#define FLASH_WRP2BR_WRP2B_END_Pos (16U)
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
#define RCC_CSR_OBLRSTF UINT32_C(0x2000000)
#define RCC_CSR_PINRSTF UINT32_C(0x4000000)
#define RCC_CSR_BORRSTF UINT32_C(0x8000000)
#define RCC_CSR_SFTRSTF UINT32_C(0x10000000)
#define RCC_CSR_IWDGRSTF UINT32_C(0x20000000)
#define RCC_CSR_WWDGRSTF UINT32_C(0x40000000)
#define RCC_CSR_LPWRRSTF UINT32_C(0x80000000)
#define RCC_CR_HSEON UINT32_C(0x10000)
#define RCC_CR_HSION UINT32_C(0x100)
#define RCC_CR_HSIRDY UINT32_C(0x400)
#define RCC_CR_HSERDY UINT32_C(0x20000)
#define RCC_CR_HSEBYP UINT32_C(0x40000)
#define RCC_CR_CSSON UINT32_C(0x80000)
#define RCC_CR_PLLON UINT32_C(0x1000000)
#define RCC_CR_PLLRDY UINT32_C(0x2000000)
#define RCC_CFGR_SW UINT32_C(3)
#define RCC_CFGR_SW_PLL UINT32_C(3)
#define RCC_CFGR_SW_HSI UINT32_C(1)
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
void canview_stm_boot_test_key(uint32_t value);
void canview_stm_boot_test_reset(void);
void canview_stm_test_reset(void);
void canview_stm_test_before_feed(void);
uint32_t model_systick_config(uint32_t ticks);
void model_wait(void);
uint32_t model_primask(void);
void model_disable_irq(void);
void model_set_primask(uint32_t mask);
void model_reset(void);
#define SysTick_Config(ticks) model_systick_config(ticks)
#define __WFI() model_wait()
#define __get_PRIMASK() model_primask()
#define __get_MSP() canview_stm_test_stack_top()
#define __disable_irq() model_disable_irq()
#define __set_PRIMASK(mask) model_set_primask(mask)
#define __DMB() ((void)0)
#define NVIC_SystemReset() model_reset()
uintptr_t canview_stm_test_stack_top(void);
uintptr_t canview_stm_test_stack_low(void);
void canview_stm_test_set_stack_pointer(uintptr_t stack_pointer);
void canview_stm_test_corrupt_stack(void);

/* Boot Flash command용 register event hook. 제품 target에서는 컴파일하지 않는다. */
typedef struct { volatile uintptr_t VTOR; volatile uint32_t ICSR; } model_scb_t;
typedef struct { volatile uint32_t CTRL, LOAD, VAL; } model_systick_t;
typedef struct { volatile uint32_t ICER[8], ICPR[8]; } model_nvic_t;
typedef struct { volatile uint32_t CTRL; } model_mpu_t;
typedef struct { volatile uint32_t FPCCR; } model_fpu_t;
extern model_systick_t model_systick;
extern model_nvic_t model_nvic;
extern model_mpu_t model_mpu;
extern model_fpu_t model_fpu;
#define SysTick (&model_systick)
#define NVIC (&model_nvic)
#define MPU (&model_mpu)
#define FPU (&model_fpu)
#define FPU_FPCCR_LSPACT_Msk UINT32_C(1)
#define SCB_ICSR_PENDSTCLR_Msk UINT32_C(0x02000000)
#define SCB_ICSR_PENDSVCLR_Msk UINT32_C(0x08000000)
__attribute__((noreturn)) void canview_stm_handoff_test_branch(uint32_t stack, uint32_t entry);
extern model_scb_t model_scb;
extern uint32_t model_ipsr, model_control;
extern uint32_t model_basepri, model_faultmask;
#define SCB (&model_scb)
#define __get_IPSR() (model_ipsr)
#define __get_CONTROL() (model_control)
#define __get_BASEPRI() (model_basepri)
#define __get_FAULTMASK() (model_faultmask)
#if defined(CANVIEW_STM_FLASH_READ_TEST)
void canview_stm_read_test_barrier(void);
#define __DSB() canview_stm_read_test_barrier()
#else
#define __DSB() ((void)0)
#endif
#define __ISB() ((void)0)
uint32_t canview_stm_flash_test_load(uint32_t address);
void canview_stm_flash_test_store(uint32_t address, uint32_t value);
void canview_stm_flash_test_key(uint32_t value);
void canview_stm_flash_test_clear(uint32_t value);
void canview_stm_flash_test_ecc_clear(uint32_t value);
void canview_stm_flash_test_start(void);
void canview_stm_flash_test_poll(void);
__attribute__((noreturn)) void canview_stm_flash_test_fatal(void);
_Bool canview_stm_flash_test_ram_ready(void);
#endif
