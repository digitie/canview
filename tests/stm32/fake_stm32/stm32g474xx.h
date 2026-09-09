/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_TEST_STM32G474XX_H
#define CANVIEW_TEST_STM32G474XX_H

#include <stdint.h>

typedef struct
{
    volatile uint32_t IDR;
    volatile uint32_t MODER;
    volatile uint32_t PUPDR;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CCIPR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t APB1ENR1;
    volatile uint32_t AHB1ENR;
    volatile uint32_t CRRCR;
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

typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t BRR;
    volatile uint32_t PRESC;
    volatile uint32_t ICR;
    volatile uint32_t RQR;
    volatile uint32_t ISR;
    volatile uint32_t RDR;
    volatile uint32_t TDR;
} USART_TypeDef;

typedef struct
{
    volatile uint32_t ISR;
    volatile uint32_t IFCR;
} DMA_TypeDef;

typedef struct
{
    volatile uint32_t CCR;
    volatile uint32_t CNDTR;
    volatile uint32_t CPAR;
    volatile uint32_t CMAR;
} DMA_Channel_TypeDef;

typedef struct
{
    volatile uint32_t CCR;
} DMAMUX_Channel_TypeDef;

typedef struct
{
    volatile uint32_t CR;
    volatile uint32_t SR;
    volatile uint32_t DR;
} RNG_TypeDef;

extern GPIO_TypeDef fake_gpio_a;
extern GPIO_TypeDef fake_gpio_b;
extern RCC_TypeDef fake_rcc;
extern TIM_TypeDef fake_tim2;
extern FDCAN_GlobalTypeDef fake_fdcan1;
extern FDCAN_GlobalTypeDef fake_fdcan2;
extern FDCAN_GlobalTypeDef fake_fdcan3;
extern USART_TypeDef fake_usart2;
extern DMA_TypeDef fake_dma1;
extern DMA_Channel_TypeDef fake_dma1_channel1;
extern DMA_Channel_TypeDef fake_dma1_channel2;
extern DMAMUX_Channel_TypeDef fake_dmamux1_channel0;
extern DMAMUX_Channel_TypeDef fake_dmamux1_channel1;
extern RNG_TypeDef fake_rng;
extern uint8_t fake_sramcan[4096];

#define GPIOA (&fake_gpio_a)
#define GPIOB (&fake_gpio_b)
#define RCC (&fake_rcc)
#define TIM2 (&fake_tim2)
#define FDCAN1 (&fake_fdcan1)
#define FDCAN2 (&fake_fdcan2)
#define FDCAN3 (&fake_fdcan3)
#define USART2 (&fake_usart2)
#define DMA1 (&fake_dma1)
#define DMA1_Channel1 (&fake_dma1_channel1)
#define DMA1_Channel2 (&fake_dma1_channel2)
#define DMAMUX1_Channel0 (&fake_dmamux1_channel0)
#define DMAMUX1_Channel1 (&fake_dmamux1_channel1)
#define RNG (&fake_rng)
#define SRAMCAN_BASE ((uintptr_t)fake_sramcan)

#define RCC_CR_HSERDY (UINT32_C(1) << 17U)
#define RCC_CR_PLLRDY (UINT32_C(1) << 24U)
#define RCC_CRRCR_HSI48ON (UINT32_C(1) << 0U)
#define RCC_CRRCR_HSI48RDY (UINT32_C(1) << 1U)
#define RCC_CFGR_SWS (UINT32_C(3) << 2U)
#define RCC_CFGR_SWS_PLL (UINT32_C(3) << 2U)
#define RCC_CCIPR_FDCANSEL (UINT32_C(3) << 24U)
#define RCC_CCIPR_FDCANSEL_0 (UINT32_C(1) << 24U)
#define RCC_CCIPR_USART2SEL (UINT32_C(3) << 2U)
#define RCC_CCIPR_CLK48SEL (UINT32_C(3) << 26U)
#define RCC_AHB2ENR_GPIOAEN (UINT32_C(1) << 0U)
#define RCC_AHB2ENR_GPIOBEN (UINT32_C(1) << 1U)
#define RCC_AHB2ENR_RNGEN (UINT32_C(1) << 18U)
#define RCC_AHB1ENR_DMA1EN (UINT32_C(1) << 0U)
#define RCC_AHB1ENR_DMAMUX1EN (UINT32_C(1) << 1U)
#define RCC_APB1ENR1_USART2EN (UINT32_C(1) << 17U)
#define RCC_APB1ENR1_FDCANEN (UINT32_C(1) << 9U)
#define TIM_CR1_CEN (UINT32_C(1) << 0U)

#define DMA_CCR_EN (UINT32_C(1) << 0U)
#define DMA_CCR_DIR (UINT32_C(1) << 4U)
#define DMA_CCR_MINC (UINT32_C(1) << 7U)
#define DMA_CCR_CIRC (UINT32_C(1) << 5U)
#define DMA_CCR_HTIE (UINT32_C(1) << 2U)
#define DMA_CCR_TCIE (UINT32_C(1) << 1U)
#define DMA_CCR_TEIE (UINT32_C(1) << 3U)
#define DMA_CCR_PL_0 (UINT32_C(1) << 12U)
#define DMA_CCR_PL_1 (UINT32_C(1) << 13U)
#define DMA_ISR_TCIF1 (UINT32_C(1) << 1U)
#define DMA_ISR_HTIF1 (UINT32_C(1) << 2U)
#define DMA_ISR_TEIF1 (UINT32_C(1) << 3U)
#define DMA_ISR_TCIF2 (UINT32_C(1) << 5U)
#define DMA_ISR_TEIF2 (UINT32_C(1) << 7U)
#define DMA_IFCR_CGIF1 (UINT32_C(1) << 0U)
#define DMA_IFCR_CTCIF1 (UINT32_C(1) << 1U)
#define DMA_IFCR_CHTIF1 (UINT32_C(1) << 2U)
#define DMA_IFCR_CTEIF1 (UINT32_C(1) << 3U)
#define DMA_IFCR_CGIF2 (UINT32_C(1) << 4U)
#define DMA_IFCR_CTCIF2 (UINT32_C(1) << 5U)
#define DMA_IFCR_CHTIF2 (UINT32_C(1) << 6U)
#define DMA_IFCR_CTEIF2 (UINT32_C(1) << 7U)

#define USART_CR1_UE (UINT32_C(1) << 0U)
#define USART_CR1_RE (UINT32_C(1) << 2U)
#define USART_CR1_TE (UINT32_C(1) << 3U)
#define USART_CR1_IDLEIE (UINT32_C(1) << 4U)
#define USART_CR3_EIE (UINT32_C(1) << 0U)
#define USART_CR3_DMAR (UINT32_C(1) << 6U)
#define USART_CR3_DMAT (UINT32_C(1) << 7U)
#define USART_CR3_RTSE (UINT32_C(1) << 8U)
#define USART_CR3_CTSE (UINT32_C(1) << 9U)
#define USART_CR3_CTSIE (UINT32_C(1) << 10U)
#define USART_CR3_DDRE (UINT32_C(1) << 13U)
#define USART_ICR_PECF (UINT32_C(1) << 0U)
#define USART_ICR_FECF (UINT32_C(1) << 1U)
#define USART_ICR_NECF (UINT32_C(1) << 2U)
#define USART_ICR_ORECF (UINT32_C(1) << 3U)
#define USART_ICR_IDLECF (UINT32_C(1) << 4U)
#define USART_ICR_CTSCF (UINT32_C(1) << 5U)
#define USART_RQR_RXFRQ (UINT32_C(1) << 3U)
#define USART_RQR_TXFRQ (UINT32_C(1) << 4U)
#define USART_ISR_PE (UINT32_C(1) << 0U)
#define USART_ISR_FE (UINT32_C(1) << 1U)
#define USART_ISR_NE (UINT32_C(1) << 2U)
#define USART_ISR_ORE (UINT32_C(1) << 3U)
#define USART_ISR_IDLE (UINT32_C(1) << 4U)
#define USART_ISR_CTSIF (UINT32_C(1) << 9U)
#define RNG_CR_RNGEN (UINT32_C(1) << 2U)
#define RNG_SR_DRDY (UINT32_C(1) << 0U)
#define RNG_SR_CECS (UINT32_C(1) << 1U)
#define RNG_SR_SECS (UINT32_C(1) << 2U)

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
#define DMA1_Channel1_IRQn (3)
#define DMA1_Channel2_IRQn (4)
#define USART2_IRQn (5)

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

static inline void __DSB(void)
{
}

#endif
