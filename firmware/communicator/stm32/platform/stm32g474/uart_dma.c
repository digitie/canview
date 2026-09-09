/* SPDX-License-Identifier: GPL-3.0-only */
/** @file uart_dma.c
 * @brief STM32G474 USART2 DMA/IRQ adapter for the T-104 UART owner.
 */
#include "uart_dma.h"

#include "board_pins.h"
#include "core_hw.h"
#include "stm32g474xx.h"
#include "stm32g4xx_ll_dmamux.h"

#include <string.h>

#if CANVIEW_BOARD_ESP_RTS_PORT != 0U || CANVIEW_BOARD_ESP_RTS_PIN != 0U || \
    CANVIEW_BOARD_STM_RTS_SRC_PORT != 0U || CANVIEW_BOARD_STM_RTS_SRC_PIN != 1U || \
    CANVIEW_BOARD_STM_TX_SRC_PORT != 0U || CANVIEW_BOARD_STM_TX_SRC_PIN != 2U || \
    CANVIEW_BOARD_ESP_TX_PORT != 0U || CANVIEW_BOARD_ESP_TX_PIN != 3U
#error Invalid_R1_USART2_pin_contract
#endif

#if CANVIEW_STM_UART_PLATFORM_RX_CAPACITY > UINT16_MAX
#error USART2_DMA_RX_capacity_must_fit_CNDTR
#endif

#define CANVIEW_STM_UART_GPIO_PIN_MASK (UINT32_C(0x0000000f))
#define CANVIEW_STM_UART_GPIO_MODE_MASK (UINT32_C(0x000000ff))
#define CANVIEW_STM_UART_GPIO_ALTERNATE_MASK (UINT32_C(0x0000ffff))
#define CANVIEW_STM_UART_GPIO_ALTERNATE_VALUE (UINT32_C(0x00007777))
#define CANVIEW_STM_UART_GPIO_ALTERNATE_MODE (UINT32_C(0x000000aa))
#define CANVIEW_STM_UART_GPIO_HIGH_SPEED (UINT32_C(0x000000ff))

#define CANVIEW_STM_UART_DMA_RX_REQUEST (LL_DMAMUX_REQ_USART2_RX)
#define CANVIEW_STM_UART_DMA_TX_REQUEST (LL_DMAMUX_REQ_USART2_TX)
#define CANVIEW_STM_UART_DMA_PRIORITY (DMA_CCR_PL_0 | DMA_CCR_PL_1)

#define CANVIEW_STM_UART_USART_ERROR_FLAGS \
    (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE)
#define CANVIEW_STM_UART_USART_ERROR_CLEAR \
    (USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_ORECF)

#define CANVIEW_STM_UART_RX_DMA_FLAGS \
    (DMA_ISR_TCIF1 | DMA_ISR_HTIF1 | DMA_ISR_TEIF1)
#define CANVIEW_STM_UART_TX_DMA_FLAGS \
    (DMA_ISR_TCIF2 | DMA_ISR_TEIF2)
#define CANVIEW_STM_UART_RX_DMA_CLEAR \
    (DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1 | DMA_IFCR_CGIF1)
#define CANVIEW_STM_UART_TX_DMA_CLEAR \
    (DMA_IFCR_CTCIF2 | DMA_IFCR_CTEIF2 | DMA_IFCR_CGIF2)

#if defined(CANVIEW_STM_UART_PLATFORM_TEST)
extern volatile uint32_t canview_test_stm_uid[3];
#define CANVIEW_STM_UART_ID_UID_BASE ((uintptr_t)canview_test_stm_uid)
#else
#define CANVIEW_STM_UART_ID_UID_BASE (0x1fff7590UL)
#endif
#define CANVIEW_STM_UART_POLL_LIMIT (UINT32_C(100000))

/* This adapter is a hardware singleton: there is one USART2 and one DMA pair. */
static canview_stm_uart_platform_t *volatile active_platform;

static bool wait_register(volatile const uint32_t *address, uint32_t mask,
                          uint32_t expected)
{
    if (address == NULL)
    {
        return false;
    }
    for (uint32_t attempt = 0U; attempt < CANVIEW_STM_UART_POLL_LIMIT; ++attempt)
    {
        __DMB();
        if ((*address & mask) == expected)
        {
            return true;
        }
    }
    return false;
}

static void post_event(canview_stm_uart_platform_t *platform, uint32_t event)
{
    if (platform != NULL && event != 0U)
    {
        __DMB();
        platform->events |= event;
    }
}

static bool clock_ready(void)
{
    return (RCC->CR & (RCC_CR_HSERDY | RCC_CR_PLLRDY)) ==
               (RCC_CR_HSERDY | RCC_CR_PLLRDY) &&
           (RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL &&
           (RCC->CCIPR & RCC_CCIPR_USART2SEL) == 0U;
}

static void configure_uart_gpio(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~CANVIEW_STM_UART_GPIO_ALTERNATE_MASK) |
                    CANVIEW_STM_UART_GPIO_ALTERNATE_VALUE;
    GPIOA->PUPDR &= ~CANVIEW_STM_UART_GPIO_MODE_MASK;
    GPIOA->OTYPER &= ~CANVIEW_STM_UART_GPIO_PIN_MASK;
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~CANVIEW_STM_UART_GPIO_MODE_MASK) |
                     CANVIEW_STM_UART_GPIO_HIGH_SPEED;
    GPIOA->MODER = (GPIOA->MODER & ~CANVIEW_STM_UART_GPIO_MODE_MASK) |
                   CANVIEW_STM_UART_GPIO_ALTERNATE_MODE;
}

static void configure_uart_gpio_input(void)
{
    GPIOA->MODER &= ~CANVIEW_STM_UART_GPIO_MODE_MASK;
    GPIOA->PUPDR &= ~CANVIEW_STM_UART_GPIO_MODE_MASK;
    GPIOA->OTYPER &= ~CANVIEW_STM_UART_GPIO_PIN_MASK;
    GPIOA->AFR[0] &= ~CANVIEW_STM_UART_GPIO_ALTERNATE_MASK;
}

static void clear_dma_flags(void)
{
    DMA1->IFCR = CANVIEW_STM_UART_RX_DMA_CLEAR | CANVIEW_STM_UART_TX_DMA_CLEAR;
}

static void disable_dma(void)
{
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DMA1->IFCR = CANVIEW_STM_UART_RX_DMA_CLEAR | CANVIEW_STM_UART_TX_DMA_CLEAR;
}

static void disable_usart(void)
{
    USART2->CR1 = 0U;
    USART2->CR3 = 0U;
    USART2->ICR = CANVIEW_STM_UART_USART_ERROR_CLEAR | USART_ICR_IDLECF | USART_ICR_CTSCF;
    USART2->RQR = USART_RQR_RXFRQ | USART_RQR_TXFRQ;
}

static void reset_adapter_counters(canview_stm_uart_platform_t *platform)
{
    platform->rx_read_total = 0U;
    platform->rx_wrap_count_isr = 0U;
    platform->events = 0U;
    platform->rx_error_pending = false;
    platform->tx_error_pending = false;
    platform->tx_done_pending = false;
    platform->tx_in_flight = false;
    platform->servicing = false;
}

static void configure_dma_requests(void)
{
    DMAMUX1_Channel0->CCR = CANVIEW_STM_UART_DMA_RX_REQUEST;
    DMAMUX1_Channel1->CCR = CANVIEW_STM_UART_DMA_TX_REQUEST;
}

static void configure_dma_channels(const uint8_t *rx_buffer)
{
    DMA1_Channel1->CCR = 0U;
    DMA1_Channel2->CCR = 0U;
    DMA1_Channel1->CPAR = (uint32_t)(uintptr_t)&USART2->RDR;
    DMA1_Channel1->CMAR = (uint32_t)(uintptr_t)rx_buffer;
    DMA1_Channel1->CNDTR = (uint32_t)CANVIEW_STM_UART_PLATFORM_RX_CAPACITY;
    DMA1_Channel1->CCR = DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_HTIE | DMA_CCR_TCIE |
                         DMA_CCR_TEIE | CANVIEW_STM_UART_DMA_PRIORITY;

    DMA1_Channel2->CPAR = (uint32_t)(uintptr_t)&USART2->TDR;
    DMA1_Channel2->CMAR = 0U;
    DMA1_Channel2->CNDTR = 0U;
    DMA1_Channel2->CCR = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE |
                         CANVIEW_STM_UART_DMA_PRIORITY;
}

static void configure_usart(void)
{
    USART2->CR1 = 0U;
    USART2->CR2 = 0U;
    USART2->CR3 = 0U;
    USART2->BRR = CANVIEW_STM_UART_BRR;
    USART2->PRESC = 0U;
    USART2->ICR = CANVIEW_STM_UART_USART_ERROR_CLEAR | USART_ICR_IDLECF | USART_ICR_CTSCF;
    USART2->RQR = USART_RQR_RXFRQ | USART_RQR_TXFRQ;
    USART2->CR3 = USART_CR3_EIE | USART_CR3_DMAR | USART_CR3_DMAT | USART_CR3_RTSE |
                  USART_CR3_CTSE | USART_CR3_CTSIE | USART_CR3_DDRE;
    USART2->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_IDLEIE;
}

static void enable_interrupts(void)
{
    NVIC_ClearPendingIRQ(DMA1_Channel1_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Channel2_IRQn);
    NVIC_ClearPendingIRQ(USART2_IRQn);
    NVIC_SetPriority(DMA1_Channel1_IRQn, UINT32_C(5));
    NVIC_SetPriority(DMA1_Channel2_IRQn, UINT32_C(5));
    NVIC_SetPriority(USART2_IRQn, UINT32_C(5));
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    NVIC_EnableIRQ(USART2_IRQn);
}

static void disable_interrupts(void)
{
    NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    NVIC_DisableIRQ(DMA1_Channel2_IRQn);
    NVIC_DisableIRQ(USART2_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Channel1_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Channel2_IRQn);
    NVIC_ClearPendingIRQ(USART2_IRQn);
}

static bool cts_is_blocked(void)
{
    /* The external ESP32 RTS output is active-low and pulled high at reset. */
    return (GPIOA->IDR & (UINT32_C(1) << CANVIEW_BOARD_ESP_RTS_PIN)) != 0U;
}

static bool sample_rx_total(const canview_stm_uart_platform_t *platform,
                            uint64_t *producer_total)
{
    if (platform == NULL || producer_total == NULL)
    {
        return false;
    }
    for (size_t attempt = 0U; attempt < 3U; ++attempt)
    {
        const uint32_t wraps_before = platform->rx_wrap_count_isr;
        const uint32_t remaining = DMA1_Channel1->CNDTR;
        __DMB();
        const uint32_t wraps_after = platform->rx_wrap_count_isr;
        if (wraps_before != wraps_after)
        {
            continue;
        }
        if (remaining > CANVIEW_STM_UART_PLATFORM_RX_CAPACITY)
        {
            return false;
        }
        const uint32_t position = remaining == 0U
                                       ? (uint32_t)CANVIEW_STM_UART_PLATFORM_RX_CAPACITY
                                       : (uint32_t)CANVIEW_STM_UART_PLATFORM_RX_CAPACITY - remaining;
        const uint32_t capacity = (uint32_t)CANVIEW_STM_UART_PLATFORM_RX_CAPACITY;
        if ((uint64_t)wraps_before > UINT64_MAX / (uint64_t)capacity)
        {
            return false;
        }
        uint64_t total = (uint64_t)wraps_before * (uint64_t)capacity + position;
        /* A transfer-complete IRQ can be pending while CNDTR already wrapped. */
        if (total < platform->rx_read_total)
        {
            const uint64_t lag = platform->rx_read_total - total;
            if (lag > capacity || total > UINT64_MAX - capacity)
            {
                return false;
            }
            total += capacity;
        }
        *producer_total = total;
        return true;
    }
    return false;
}

static uint32_t take_events(canview_stm_uart_platform_t *platform, bool *rx_error,
                            bool *tx_error, bool *tx_done)
{
    const uint32_t saved_mask = canview_stm_critical_enter(NULL);
    const uint32_t events = platform->events;
    *rx_error = platform->rx_error_pending;
    *tx_error = platform->tx_error_pending;
    *tx_done = platform->tx_done_pending;
    platform->events = 0U;
    platform->rx_error_pending = false;
    platform->tx_error_pending = false;
    platform->tx_done_pending = false;
    canview_stm_critical_leave(NULL, saved_mask);
    return events;
}

static canview_status_t abort_tx_dma(canview_stm_uart_platform_t *platform)
{
    if (platform == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    USART2->CR3 &= ~USART_CR3_DMAT;
    __DSB();
    if (!wait_register(&DMA1_Channel2->CCR, DMA_CCR_EN, 0U))
    {
        return CANVIEW_TIMEOUT;
    }
    DMA1->IFCR = CANVIEW_STM_UART_TX_DMA_CLEAR;
    __DSB();
    platform->tx_in_flight = false;
    return CANVIEW_OK;
}

static canview_status_t runtime_reset_hook(void *opaque)
{
    canview_stm_uart_platform_t *const platform =
        (canview_stm_uart_platform_t *)opaque;
    return abort_tx_dma(platform);
}

static void record_rx_recovery(canview_stm_uart_platform_t *platform,
                               canview_stm_uart_rx_recovery_reason_t reason,
                               uint64_t discarded_bytes)
{
    if (platform == NULL)
    {
        return;
    }
    if (platform->rx_recovery_count != UINT32_MAX)
    {
        ++platform->rx_recovery_count;
    }
    if (reason == CANVIEW_STM_UART_RX_RECOVERY_DMA_OR_USART_ERROR &&
        platform->rx_error_recovery_count != UINT32_MAX)
    {
        ++platform->rx_error_recovery_count;
    }
    if (reason == CANVIEW_STM_UART_RX_RECOVERY_RING_OVERRUN &&
        platform->rx_overrun_recovery_count != UINT32_MAX)
    {
        ++platform->rx_overrun_recovery_count;
    }
    if (discarded_bytes > UINT64_MAX - platform->rx_discarded_bytes)
    {
        platform->rx_discarded_bytes = UINT64_MAX;
    }
    else
    {
        platform->rx_discarded_bytes += discarded_bytes;
    }
    platform->last_rx_recovery_reason = reason;
}

static canview_status_t reset_rx_dma(canview_stm_uart_platform_t *platform,
                                     uint64_t now_ms, uint64_t now_us,
                                     canview_stm_uart_rx_recovery_reason_t reason,
                                     uint64_t discarded_bytes)
{
    canview_status_t result = CANVIEW_OK;
    record_rx_recovery(platform, reason, discarded_bytes);
    NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    NVIC_DisableIRQ(DMA1_Channel2_IRQn);
    NVIC_DisableIRQ(USART2_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Channel1_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Channel2_IRQn);
    NVIC_ClearPendingIRQ(USART2_IRQn);
    const canview_status_t tx_abort_status = abort_tx_dma(platform);
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    USART2->CR3 &= ~USART_CR3_DMAR;
    DMA1->IFCR = CANVIEW_STM_UART_RX_DMA_CLEAR;
    USART2->ICR = CANVIEW_STM_UART_USART_ERROR_CLEAR | USART_ICR_IDLECF | USART_ICR_CTSCF;
    USART2->RQR = USART_RQR_RXFRQ;
    if (tx_abort_status != CANVIEW_OK)
    {
        result = tx_abort_status;
    }
    else
    {
        result = canview_stm_uart_reset(platform->config.runtime, now_ms, now_us);
    }
    if (result == CANVIEW_OK)
    {
        memset(platform->config.rx_buffer, 0, platform->config.rx_capacity);
        DMA1_Channel1->CPAR = (uint32_t)(uintptr_t)&USART2->RDR;
        DMA1_Channel1->CMAR = (uint32_t)(uintptr_t)platform->config.rx_buffer;
        DMA1_Channel1->CNDTR = (uint32_t)CANVIEW_STM_UART_PLATFORM_RX_CAPACITY;
        DMA1_Channel1->CCR = DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_HTIE | DMA_CCR_TCIE |
                             DMA_CCR_TEIE | CANVIEW_STM_UART_DMA_PRIORITY;
        DMA1_Channel1->CCR |= DMA_CCR_EN;
        USART2->CR3 |= USART_CR3_DMAR;
        platform->rx_read_total = 0U;
        platform->rx_wrap_count_isr = 0U;
        platform->events = 0U;
        platform->rx_error_pending = false;
        platform->tx_error_pending = false;
        platform->tx_done_pending = false;
    }
    NVIC_ClearPendingIRQ(DMA1_Channel1_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Channel2_IRQn);
    NVIC_ClearPendingIRQ(USART2_IRQn);
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    NVIC_EnableIRQ(USART2_IRQn);
    return result;
}

static bool input_status_nonfatal(canview_status_t status)
{
    switch (status)
    {
    case CANVIEW_OK:
    case CANVIEW_INCOMPLETE:
    case CANVIEW_MALFORMED:
    case CANVIEW_UNSUPPORTED_VERSION:
    case CANVIEW_UNSUPPORTED_MESSAGE:
    case CANVIEW_CRC_MISMATCH:
    case CANVIEW_OVERSIZE:
    case CANVIEW_DUPLICATE:
    case CANVIEW_STALE:
    case CANVIEW_NOT_IMPLEMENTED:
    case CANVIEW_AUTH_FAILED:
    case CANVIEW_RESOURCE_BUSY:
    case CANVIEW_TIMEOUT:
        return true;
    default:
        return false;
    }
}

static canview_status_t consume_rx(canview_stm_uart_platform_t *platform, uint64_t producer_total,
                                   uint64_t now_ms, uint64_t now_us, size_t rx_budget)
{
    const uint64_t available = producer_total - platform->rx_read_total;
    if (available > (uint64_t)CANVIEW_STM_UART_PLATFORM_RX_CAPACITY)
    {
        return CANVIEW_OVERSIZE;
    }
    const uint64_t bounded_budget = (uint64_t)rx_budget;
    const uint64_t count = available < bounded_budget ? available : bounded_budget;
    __DMB();
    for (uint64_t index = 0U; index < count; ++index)
    {
        const uint32_t buffer_index =
            (uint32_t)(platform->rx_read_total &
                       ((uint64_t)CANVIEW_STM_UART_PLATFORM_RX_CAPACITY - UINT64_C(1)));
        const uint8_t byte = platform->config.rx_buffer[buffer_index];
        const canview_status_t status = canview_stm_uart_ingest_byte(
            platform->config.runtime, byte, now_ms, now_us);
        platform->rx_read_total++;
        if (!input_status_nonfatal(status))
        {
            return status;
        }
    }
    return CANVIEW_OK;
}

static canview_status_t start_tx_dma(canview_stm_uart_platform_t *platform)
{
    const uint8_t *data = NULL;
    size_t size = 0U;
    uint32_t sequence = 0U;
    const canview_status_t begin_status =
        canview_stm_uart_tx_begin(platform->config.runtime, &data, &size, &sequence);
    (void)sequence;
    if (begin_status == CANVIEW_INCOMPLETE)
    {
        return CANVIEW_OK;
    }
    if (begin_status != CANVIEW_OK || data == NULL || size == 0U || size > UINT16_MAX)
    {
        return begin_status == CANVIEW_OK ? CANVIEW_MALFORMED : begin_status;
    }
    const canview_status_t quiesce_status = abort_tx_dma(platform);
    if (quiesce_status != CANVIEW_OK)
    {
        return quiesce_status;
    }
    DMA1_Channel2->CMAR = (uint32_t)(uintptr_t)data;
    DMA1_Channel2->CNDTR = (uint32_t)size;
    DMA1_Channel2->CCR = DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_TEIE |
                         CANVIEW_STM_UART_DMA_PRIORITY;
    __DMB();
    USART2->CR3 |= USART_CR3_DMAT;
    DMA1_Channel2->CCR |= DMA_CCR_EN;
    platform->tx_in_flight = true;
    return CANVIEW_OK;
}

canview_status_t canview_stm_uart_platform_init(
    canview_stm_uart_platform_t *platform,
    const canview_stm_uart_platform_config_t *config)
{
    if (platform == NULL || config == NULL || config->runtime == NULL ||
        !config->runtime->initialized || config->rx_buffer == NULL ||
        config->rx_capacity != CANVIEW_STM_UART_PLATFORM_RX_CAPACITY ||
        (((uintptr_t)config->rx_buffer & (uintptr_t)3U) != 0U))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (platform->initialized || active_platform != NULL)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    memset(platform, 0, sizeof(*platform));
    platform->config = *config;
    platform->initialized = true;
    return CANVIEW_OK;
}

canview_status_t canview_stm_uart_platform_start(canview_stm_uart_platform_t *platform)
{
    if (platform == NULL || !platform->initialized || platform->config.runtime == NULL ||
        !platform->config.runtime->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (platform->started || platform->servicing || active_platform != NULL || !clock_ready())
    {
        return platform->started || platform->servicing || active_platform != NULL
                   ? CANVIEW_RESOURCE_BUSY
                   : CANVIEW_INVALID_ARGUMENT;
    }
    const canview_status_t reset_status = canview_stm_uart_reset(
        platform->config.runtime, canview_stm_now_ms64(NULL),
        canview_stm_now_us64(NULL));
    if (reset_status != CANVIEW_OK)
    {
        return reset_status;
    }
    const canview_status_t hook_status = canview_stm_uart_set_reset_hook(
        platform->config.runtime, runtime_reset_hook, platform);
    if (hook_status != CANVIEW_OK)
    {
        return hook_status;
    }
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    (void)RCC->AHB1ENR;
    (void)RCC->APB1ENR1;
    configure_uart_gpio();
    disable_dma();
    disable_usart();
    clear_dma_flags();
    configure_dma_requests();
    configure_dma_channels(platform->config.rx_buffer);
    configure_usart();
    DMA1_Channel1->CCR |= DMA_CCR_EN;
    reset_adapter_counters(platform);
    platform->started = true;
    platform->events = CANVIEW_STM_UART_PLATFORM_EVENT_CTS;
    active_platform = platform;
    enable_interrupts();
    return CANVIEW_OK;
}

canview_status_t canview_stm_uart_platform_stop(canview_stm_uart_platform_t *platform)
{
    if (platform == NULL || !platform->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (active_platform != NULL && active_platform != platform)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    if (platform->servicing)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    if (active_platform == platform)
    {
        active_platform = NULL;
    }
    const canview_status_t abort_status = abort_tx_dma(platform);
    disable_interrupts();
    disable_dma();
    disable_usart();
    configure_uart_gpio_input();
    platform->started = false;
    reset_adapter_counters(platform);
    const canview_status_t hook_status = canview_stm_uart_set_reset_hook(
        platform->config.runtime, NULL, NULL);
    if (abort_status != CANVIEW_OK)
    {
        return abort_status;
    }
    return hook_status;
}

canview_status_t canview_stm_uart_platform_service(canview_stm_uart_platform_t *platform,
                                                    uint64_t now_ms, uint64_t now_us,
                                                    size_t rx_budget)
{
    if (platform == NULL || !platform->initialized || !platform->started ||
        platform->config.runtime == NULL || !platform->config.runtime->initialized ||
        rx_budget == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (platform->servicing)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    platform->servicing = true;
    bool rx_error = false;
    bool tx_error = false;
    bool tx_done = false;
    const uint32_t events = take_events(platform, &rx_error, &tx_error, &tx_done);
    (void)events;

    uint64_t producer_total = 0U;
    const bool producer_valid = sample_rx_total(platform, &producer_total);
    canview_status_t result = CANVIEW_OK;
    bool reset_happened = false;
    if (!producer_valid || rx_error)
    {
        if (!producer_valid)
        {
            producer_total = platform->rx_read_total;
        }
        const canview_status_t recovery_status =
            reset_rx_dma(platform, now_ms, now_us,
                         rx_error ? CANVIEW_STM_UART_RX_RECOVERY_DMA_OR_USART_ERROR
                                  : CANVIEW_STM_UART_RX_RECOVERY_CNDTR_INVALID,
                         0U);
        reset_happened = true;
        if (recovery_status != CANVIEW_OK)
        {
            result = recovery_status;
            goto service_done;
        }
        producer_total = platform->rx_read_total;
    }
    else if (producer_total - platform->rx_read_total >
             (uint64_t)CANVIEW_STM_UART_PLATFORM_RX_CAPACITY)
    {
        const canview_status_t recovery_status =
            reset_rx_dma(platform, now_ms, now_us,
                         CANVIEW_STM_UART_RX_RECOVERY_RING_OVERRUN,
                         producer_total - platform->rx_read_total);
        reset_happened = true;
        if (recovery_status != CANVIEW_OK)
        {
            result = recovery_status;
            goto service_done;
        }
        producer_total = platform->rx_read_total;
    }

    if (tx_error)
    {
        if (platform->tx_in_flight)
        {
            const canview_status_t abort_status = abort_tx_dma(platform);
            if (abort_status != CANVIEW_OK)
            {
                result = abort_status;
                goto service_done;
            }
            const canview_status_t finish_status = canview_stm_uart_tx_finish(
                platform->config.runtime, CANVIEW_TIMEOUT, now_ms);
            if (finish_status != CANVIEW_TIMEOUT)
            {
                result = finish_status;
                goto service_done;
            }
        }
        const canview_status_t reset_status =
            canview_stm_uart_reset(platform->config.runtime, now_ms, now_us);
        reset_happened = true;
        if (reset_status != CANVIEW_OK)
        {
            result = reset_status;
            goto service_done;
        }
    }
    else if (tx_done && platform->tx_in_flight)
    {
        platform->tx_in_flight = false;
        USART2->CR3 &= ~USART_CR3_DMAT;
        __DSB();
        const canview_status_t finish_status =
            canview_stm_uart_tx_finish(platform->config.runtime, CANVIEW_OK, now_ms);
        if (finish_status != CANVIEW_OK)
        {
            result = finish_status;
            goto service_done;
        }
    }

    if (!reset_happened)
    {
        const canview_status_t rx_status =
            consume_rx(platform, producer_total, now_ms, now_us, rx_budget);
        if (rx_status != CANVIEW_OK)
        {
            result = rx_status;
            goto service_done;
        }
    }

    const bool sampled_cts_blocked = cts_is_blocked();
    if (reset_happened || !platform->config.runtime->cts_known ||
        platform->config.runtime->cts_blocked != sampled_cts_blocked ||
        (events & CANVIEW_STM_UART_PLATFORM_EVENT_CTS) != 0U)
    {
        const canview_status_t cts_status = canview_stm_uart_set_cts_blocked(
            platform->config.runtime, sampled_cts_blocked, now_ms);
        if (cts_status != CANVIEW_OK && cts_status != CANVIEW_TIMEOUT)
        {
            result = cts_status;
            goto service_done;
        }
    }

    const canview_status_t tick_status =
        canview_stm_uart_tick(platform->config.runtime, now_ms, now_us);
    if (tick_status != CANVIEW_OK)
    {
        result = tick_status;
        goto service_done;
    }
    if (!platform->tx_in_flight)
    {
        const canview_status_t tx_status = start_tx_dma(platform);
        if (tx_status != CANVIEW_OK)
        {
            result = tx_status;
            goto service_done;
        }
    }

service_done:
    platform->servicing = false;
    return result;
}

uint64_t canview_stm_uart_platform_device_id(void)
{
    const volatile uint32_t *const uid = (const volatile uint32_t *)CANVIEW_STM_UART_ID_UID_BASE;
    uint32_t hash = UINT32_C(2166136261);
    for (size_t index = 0U; index < 3U; ++index)
    {
        const uint32_t word = uid[index];
        for (size_t byte = 0U; byte < 4U; ++byte)
        {
            hash ^= (word >> (byte * 8U)) & UINT32_C(0xff);
            hash *= UINT32_C(16777619);
        }
    }
    return hash == 0U ? UINT64_C(0) : ((uint64_t)hash << 32U) | (uint64_t)(hash ^ UINT32_C(0xa5a5a5a5));
}

uint64_t canview_stm_uart_platform_boot_id(void)
{
    RCC->CRRCR |= RCC_CRRCR_HSI48ON;
    if (!wait_register(&RCC->CRRCR, RCC_CRRCR_HSI48RDY, RCC_CRRCR_HSI48RDY))
    {
        return UINT64_C(0);
    }
    RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_CLK48SEL);
    (void)RCC->CCIPR;
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    (void)RCC->AHB2ENR;
    RNG->CR |= RNG_CR_RNGEN;
    uint32_t words[2] = {0U, 0U};
    for (size_t index = 0U; index < 2U; ++index)
    {
        bool ready = false;
        for (uint32_t attempt = 0U; attempt < CANVIEW_STM_UART_POLL_LIMIT; ++attempt)
        {
            const uint32_t status = RNG->SR;
            if ((status & (RNG_SR_CECS | RNG_SR_SECS)) != 0U)
            {
                return UINT64_C(0);
            }
            if ((status & RNG_SR_DRDY) != 0U)
            {
                words[index] = RNG->DR;
                ready = true;
                break;
            }
        }
        if (!ready)
        {
            return UINT64_C(0);
        }
    }
    const uint64_t boot_id = ((uint64_t)words[0] << 32U) | (uint64_t)words[1];
    return boot_id == 0U ? UINT64_C(0) : boot_id;
}

void DMA1_Channel1_IRQHandler(void)
{
    canview_stm_uart_platform_t *const platform = active_platform;
    const uint32_t flags = DMA1->ISR & CANVIEW_STM_UART_RX_DMA_FLAGS;
    if (flags != 0U)
    {
        DMA1->IFCR = CANVIEW_STM_UART_RX_DMA_CLEAR;
    }
    if (platform == NULL || !platform->started)
    {
        return;
    }
    if ((flags & (DMA_ISR_HTIF1 | DMA_ISR_TCIF1)) != 0U)
    {
        if ((flags & DMA_ISR_TCIF1) != 0U)
        {
            if (platform->rx_wrap_count_isr == UINT32_MAX)
            {
                platform->rx_error_pending = true;
                post_event(platform, CANVIEW_STM_UART_PLATFORM_EVENT_RX_ERROR);
            }
            else
            {
                ++platform->rx_wrap_count_isr;
            }
        }
        post_event(platform, CANVIEW_STM_UART_PLATFORM_EVENT_RX);
    }
    if ((flags & DMA_ISR_TEIF1) != 0U)
    {
        platform->rx_error_pending = true;
        post_event(platform, CANVIEW_STM_UART_PLATFORM_EVENT_RX_ERROR);
    }
}

void DMA1_Channel2_IRQHandler(void)
{
    canview_stm_uart_platform_t *const platform = active_platform;
    const uint32_t flags = DMA1->ISR & CANVIEW_STM_UART_TX_DMA_FLAGS;
    if (flags != 0U)
    {
        DMA1->IFCR = CANVIEW_STM_UART_TX_DMA_CLEAR;
    }
    if (platform == NULL || !platform->started)
    {
        return;
    }
    if ((flags & DMA_ISR_TEIF2) != 0U)
    {
        platform->tx_error_pending = true;
        post_event(platform, CANVIEW_STM_UART_PLATFORM_EVENT_TX_ERROR);
    }
    if ((flags & DMA_ISR_TCIF2) != 0U)
    {
        platform->tx_done_pending = true;
        post_event(platform, CANVIEW_STM_UART_PLATFORM_EVENT_TX_DONE);
    }
}

void USART2_IRQHandler(void)
{
    canview_stm_uart_platform_t *const platform = active_platform;
    const uint32_t status = USART2->ISR;
    if ((status & CANVIEW_STM_UART_USART_ERROR_FLAGS) != 0U)
    {
        USART2->ICR = CANVIEW_STM_UART_USART_ERROR_CLEAR;
        USART2->RQR = USART_RQR_RXFRQ;
        if (platform != NULL && platform->started)
        {
            platform->rx_error_pending = true;
            post_event(platform, CANVIEW_STM_UART_PLATFORM_EVENT_RX_ERROR |
                                      CANVIEW_STM_UART_PLATFORM_EVENT_RX);
        }
    }
    if ((status & USART_ISR_IDLE) != 0U)
    {
        USART2->ICR = USART_ICR_IDLECF;
        if (platform != NULL && platform->started)
        {
            post_event(platform, CANVIEW_STM_UART_PLATFORM_EVENT_RX);
        }
    }
    if ((status & USART_ISR_CTSIF) != 0U)
    {
        USART2->ICR = USART_ICR_CTSCF;
        if (platform != NULL && platform->started)
        {
            post_event(platform, CANVIEW_STM_UART_PLATFORM_EVENT_CTS);
        }
    }
}
