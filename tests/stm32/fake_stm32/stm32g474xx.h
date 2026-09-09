/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_TEST_STM32G474XX_H
#define CANVIEW_TEST_STM32G474XX_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t MODER;
    volatile uint32_t PUPDR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CCIPR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t APB1ENR1;
} RCC_TypeDef;

typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
} TIM_TypeDef;

typedef struct
{
    /* STM32G4 uses fixed Message RAM; RXF0C/RXESC are intentionally absent. */
    volatile uint32_t CCCR;
    volatile uint32_t NBTP;
    volatile uint32_t DBTP;
    volatile uint32_t TSCC;
    volatile uint32_t TSCV;
    volatile uint32_t TOCC;
    volatile uint32_t RXGFC;
    volatile uint32_t XIDAM;
    volatile uint32_t IR;
    volatile uint32_t IE;
    volatile uint32_t ILS;
    volatile uint32_t ILE;
    volatile uint32_t RXF0S;
    volatile uint32_t RXF0A;
    volatile uint32_t PSR;
    volatile uint32_t ECR;
} FDCAN_GlobalTypeDef;

extern GPIO_TypeDef fake_gpio_a;
extern GPIO_TypeDef fake_gpio_b;
extern RCC_TypeDef fake_rcc;
extern TIM_TypeDef fake_tim2;
extern FDCAN_GlobalTypeDef fake_fdcan1;
extern FDCAN_GlobalTypeDef fake_fdcan2;
extern FDCAN_GlobalTypeDef fake_fdcan3;
extern uint8_t fake_sramcan[4096];

#define GPIOA (&fake_gpio_a)
#define GPIOB (&fake_gpio_b)
#define RCC (&fake_rcc)
#define TIM2 (&fake_tim2)
#define FDCAN1 (&fake_fdcan1)
#define FDCAN2 (&fake_fdcan2)
#define FDCAN3 (&fake_fdcan3)
#define SRAMCAN_BASE ((uintptr_t)fake_sramcan)

#define RCC_CR_HSERDY (UINT32_C(1) << 17U)
#define RCC_CR_PLLRDY (UINT32_C(1) << 24U)
#define RCC_CFGR_SWS (UINT32_C(3) << 2U)
#define RCC_CFGR_SWS_PLL (UINT32_C(3) << 2U)
#define RCC_CCIPR_FDCANSEL (UINT32_C(3) << 24U)
#define RCC_CCIPR_FDCANSEL_0 (UINT32_C(1) << 24U)
#define RCC_AHB2ENR_GPIOAEN (UINT32_C(1) << 0U)
#define RCC_AHB2ENR_GPIOBEN (UINT32_C(1) << 1U)
#define RCC_APB1ENR1_FDCANEN (UINT32_C(1) << 9U)
#define TIM_CR1_CEN (UINT32_C(1) << 0U)

#define FDCAN_CCCR_INIT (UINT32_C(1) << 0U)
#define FDCAN_CCCR_CCE (UINT32_C(1) << 1U)
#define FDCAN_CCCR_MON (UINT32_C(1) << 5U)
#define FDCAN_CCCR_DAR (UINT32_C(1) << 6U)
#define FDCAN_ILE_EINT0 (UINT32_C(1) << 0U)
#define FDCAN_XIDAM_EIDM (UINT32_C(0x1fffffff))
#define FDCAN_NBTP_NSJW_Pos (25U)
#define FDCAN_NBTP_NTSEG1_Pos (8U)
#define FDCAN_NBTP_NBRP_Pos (0U)
#define FDCAN_NBTP_NTSEG2_Pos (16U)
#define FDCAN_RXF0S_F0FL (UINT32_C(0xf))
#define FDCAN_RXF0S_F0GI (UINT32_C(0x3) << 8U)
#define FDCAN_RXF0S_F0GI_Pos (8U)
#define FDCAN_RXF0S_RF0L (UINT32_C(1) << 25U)
#define FDCAN_PSR_LEC (UINT32_C(0x7))
#define FDCAN_PSR_LEC_Pos (0U)
#define FDCAN_PSR_EP (UINT32_C(1) << 5U)
#define FDCAN_PSR_BO (UINT32_C(1) << 7U)

#define FDCAN_IR_RF0N (UINT32_C(1) << 0U)
#define FDCAN_IR_RF0F (UINT32_C(1) << 1U)
#define FDCAN_IR_RF0L (UINT32_C(1) << 2U)
#define FDCAN_IR_EP (UINT32_C(1) << 3U)
#define FDCAN_IR_EW (UINT32_C(1) << 4U)
#define FDCAN_IR_BO (UINT32_C(1) << 5U)
#define FDCAN_IR_MRAF (UINT32_C(1) << 6U)
#define FDCAN_IR_PEA (UINT32_C(1) << 7U)
#define FDCAN_IR_PED (UINT32_C(1) << 8U)

#define FDCAN_IE_RF0NE FDCAN_IR_RF0N
#define FDCAN_IE_RF0FE FDCAN_IR_RF0F
#define FDCAN_IE_RF0LE FDCAN_IR_RF0L
#define FDCAN_IE_EPE FDCAN_IR_EP
#define FDCAN_IE_EWE FDCAN_IR_EW
#define FDCAN_IE_BOE FDCAN_IR_BO
#define FDCAN_IE_MRAFE FDCAN_IR_MRAF
#define FDCAN_IE_PEAE FDCAN_IR_PEA
#define FDCAN_IE_PEDE FDCAN_IR_PED

typedef int IRQn_Type;
#define FDCAN1_IT0_IRQn (0)
#define FDCAN2_IT0_IRQn (1)
#define FDCAN3_IT0_IRQn (2)

static inline void NVIC_ClearPendingIRQ(IRQn_Type irq)
{
    (void)irq;
}

static inline void NVIC_SetPriority(IRQn_Type irq, uint32_t priority)
{
    (void)irq;
    (void)priority;
}

static inline void NVIC_EnableIRQ(IRQn_Type irq)
{
    (void)irq;
}

static inline void NVIC_DisableIRQ(IRQn_Type irq)
{
    (void)irq;
}

static inline void __DMB(void)
{
}

#endif
