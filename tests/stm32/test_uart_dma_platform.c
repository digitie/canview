/* SPDX-License-Identifier: GPL-3.0-only */
/** @file test_uart_dma_platform.c
 * @brief Host fake-register tests for the STM32 UART DMA adapter.
 */
#include "uart_dma.h"

#include "fake_hardware.h"
#include "stm32g4xx_ll_dmamux.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expression)                                                                        \
    do                                                                                          \
    {                                                                                           \
        if (!(expression))                                                                       \
        {                                                                                        \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression);              \
            return 1;                                                                            \
        }                                                                                        \
    } while (0)

#define TEST_STM_BOOT_ID (UINT64_C(0x1111222233334444))
#define TEST_STM_DEVICE_ID (UINT64_C(0x5555666677778888))

typedef struct
{
    canview_stm_uart_context_t runtime;
    canview_stm_uart_platform_t platform;
    uint8_t rx_buffer[CANVIEW_STM_UART_PLATFORM_RX_CAPACITY] __attribute__((aligned(4)));
} fixture_t;

static int fixture_start(fixture_t *fixture)
{
    if (fixture == NULL)
    {
        return 1;
    }
    fake_hardware_reset();
    fake_hardware_ready();
    memset(fixture, 0, sizeof(*fixture));
    const canview_stm_uart_config_t runtime_config = {
        TEST_STM_BOOT_ID, TEST_STM_DEVICE_ID, CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION,
        NULL, NULL, {0x42U}};
    CHECK(canview_stm_uart_init(&fixture->runtime, &runtime_config, 0U, 0U) == CANVIEW_OK);
    const canview_stm_uart_platform_config_t platform_config = {
        &fixture->runtime, fixture->rx_buffer, sizeof(fixture->rx_buffer)};
    CHECK(canview_stm_uart_platform_init(&fixture->platform, &platform_config) == CANVIEW_OK);
    CHECK(canview_stm_uart_platform_start(&fixture->platform) == CANVIEW_OK);
    return 0;
}

static int fixture_stop(fixture_t *fixture)
{
    CHECK(fixture != NULL);
    CHECK(canview_stm_uart_platform_stop(&fixture->platform) == CANVIEW_OK);
    CHECK(!fixture->platform.started && !fixture->platform.tx_in_flight);
    CHECK((USART2->CR1 | USART2->CR3) == 0U);
    return 0;
}

static int test_start_cts_and_tx_completion(void)
{
    fixture_t fixture;
    CHECK(fixture_start(&fixture) == 0);
    CHECK(GPIOA->AFR[0] == UINT32_C(0x7777));
    CHECK(GPIOA->MODER == UINT32_C(0x00aa));
    CHECK(DMAMUX1_Channel0->CCR == LL_DMAMUX_REQ_USART2_RX &&
          DMAMUX1_Channel1->CCR == LL_DMAMUX_REQ_USART2_TX);
    CHECK(DMA1_Channel1->CNDTR == CANVIEW_STM_UART_PLATFORM_RX_CAPACITY &&
          (DMA1_Channel1->CCR & DMA_CCR_EN) != 0U);
    CHECK(USART2->BRR == CANVIEW_STM_UART_BRR &&
          (USART2->CR3 & (USART_CR3_DMAR | USART_CR3_DMAT | USART_CR3_RTSE |
                          USART_CR3_CTSE)) ==
              (USART_CR3_DMAR | USART_CR3_DMAT | USART_CR3_RTSE | USART_CR3_CTSE));

    CHECK(canview_stm_uart_platform_service(&fixture.platform, 2U, 2000U, 64U) == CANVIEW_OK);
    CHECK(fixture.platform.tx_in_flight && fixture.runtime.tx_current.valid);
    GPIOA->IDR = UINT32_C(1) << CANVIEW_BOARD_ESP_RTS_PIN;
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 3U, 3000U, 64U) == CANVIEW_OK);
    CHECK(fixture.runtime.cts_known && fixture.runtime.cts_blocked);
    GPIOA->IDR = 0U;
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 4U, 4000U, 64U) == CANVIEW_OK);
    CHECK(!fixture.runtime.cts_blocked);

    DMA1->ISR = DMA_ISR_TCIF2;
    DMA1_Channel2_IRQHandler();
    CHECK(fixture.platform.tx_done_pending);
    GPIOA->IDR = UINT32_C(1) << CANVIEW_BOARD_ESP_RTS_PIN;
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 5U, 5000U, 64U) == CANVIEW_OK);
    CHECK(!fixture.platform.tx_done_pending);
    CHECK(fixture.runtime.stats.tx_completed >= 1U);
    CHECK(fixture_stop(&fixture) == 0);
    return 0;
}

static int test_runtime_reset_quiesces_dma(void)
{
    fixture_t fixture;
    CHECK(fixture_start(&fixture) == 0);
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 2U, 2000U, 64U) == CANVIEW_OK);
    CHECK(fixture.platform.tx_in_flight && fixture.runtime.tx_current.valid);
    DMA1->ISR = DMA_ISR_TCIF2 | DMA_ISR_TEIF2;
    DMA1_Channel2_IRQHandler();
    DMA1->ISR = 0U;
    CHECK(fixture.platform.tx_done_pending && fixture.platform.tx_error_pending);
    fixture.platform.rx_error_pending = true;
    fixture.platform.events |= CANVIEW_STM_UART_PLATFORM_EVENT_RX_ERROR;
    CHECK(canview_stm_uart_reset(&fixture.runtime, 3U, 3000U) == CANVIEW_OK);
    CHECK(!fixture.platform.tx_done_pending && !fixture.platform.tx_error_pending);
    CHECK((fixture.platform.events & (CANVIEW_STM_UART_PLATFORM_EVENT_TX_DONE |
                                     CANVIEW_STM_UART_PLATFORM_EVENT_TX_ERROR)) == 0U);
    CHECK(fixture.platform.rx_error_pending &&
          (fixture.platform.events & CANVIEW_STM_UART_PLATFORM_EVENT_RX_ERROR) != 0U);
    fixture.platform.rx_error_pending = false;
    fixture.platform.events = 0U;
    CHECK(!fixture.platform.tx_in_flight && !fixture.runtime.tx_current.valid &&
          (DMA1_Channel2->CCR & DMA_CCR_EN) == 0U &&
          (USART2->CR3 & USART_CR3_DMAT) == 0U);
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 4U, 4000U, 64U) == CANVIEW_OK);
    CHECK(fixture.platform.tx_in_flight);
    const uint32_t completed = fixture.runtime.stats.tx_completed;
    const uint32_t remaining = DMA1_Channel2->CNDTR;
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 5U, 5000U, 64U) == CANVIEW_OK);
    CHECK(fixture.platform.tx_in_flight && fixture.runtime.stats.tx_completed == completed &&
          DMA1_Channel2->CNDTR == remaining);
    CHECK(fixture_stop(&fixture) == 0);
    return 0;
}

static int test_rx_recovery_accounting(void)
{
    fixture_t fixture;
    CHECK(fixture_start(&fixture) == 0);
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 2U, 2000U, 64U) == CANVIEW_OK);
    fixture.platform.rx_wrap_count_isr = 2U;
    DMA1_Channel1->CNDTR = CANVIEW_STM_UART_PLATFORM_RX_CAPACITY;
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 3U, 3000U, 64U) == CANVIEW_OK);
    CHECK(fixture.platform.rx_overrun_recovery_count == 1U &&
          fixture.platform.last_rx_recovery_reason == CANVIEW_STM_UART_RX_RECOVERY_RING_OVERRUN &&
          fixture.platform.rx_discarded_bytes >= CANVIEW_STM_UART_PLATFORM_RX_CAPACITY);

    DMA1_Channel1->CNDTR = CANVIEW_STM_UART_PLATFORM_RX_CAPACITY + 1U;
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 4U, 4000U, 64U) == CANVIEW_OK);
    CHECK(fixture.platform.last_rx_recovery_reason == CANVIEW_STM_UART_RX_RECOVERY_CNDTR_INVALID);
    CHECK(fixture.platform.rx_unknown_loss_count == 1U);

    const uint64_t discarded_before = fixture.platform.rx_discarded_bytes;
    DMA1_Channel1->CNDTR = CANVIEW_STM_UART_PLATFORM_RX_CAPACITY - 7U;
    fixture.platform.rx_error_pending = true;
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 5U, 5000U, 64U) == CANVIEW_OK);
    CHECK(fixture.platform.rx_error_recovery_count >= 1U &&
          fixture.platform.last_rx_recovery_reason ==
              CANVIEW_STM_UART_RX_RECOVERY_DMA_OR_USART_ERROR);
    CHECK(fixture.platform.rx_discarded_bytes == discarded_before + 7U);
    CHECK(fixture.platform.rx_unknown_loss_count == 1U);
    fixture.platform.rx_unknown_loss_count = UINT32_MAX;
    DMA1_Channel1->CNDTR = CANVIEW_STM_UART_PLATFORM_RX_CAPACITY + 1U;
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 6U, 6000U, 64U) == CANVIEW_OK);
    CHECK(fixture.platform.rx_unknown_loss_count == UINT32_MAX);
    CHECK(fixture_stop(&fixture) == 0);
    return 0;
}

static int test_rx_data_and_fault_events(void)
{
    fixture_t fixture;
    CHECK(fixture_start(&fixture) == 0);
    fixture.rx_buffer[0] = 0U;
    DMA1_Channel1->CNDTR = CANVIEW_STM_UART_PLATFORM_RX_CAPACITY - 1U;
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 2U, 2000U, 1U) == CANVIEW_OK);
    CHECK(fixture.platform.rx_read_total == 1U);

    fixture.platform.rx_wrap_count_isr = UINT32_MAX;
    DMA1->ISR = DMA_ISR_TCIF1;
    DMA1_Channel1_IRQHandler();
    CHECK(fixture.platform.rx_error_pending);
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 3U, 3000U, 64U) == CANVIEW_OK);
    CHECK(fixture.platform.last_rx_recovery_reason ==
          CANVIEW_STM_UART_RX_RECOVERY_DMA_OR_USART_ERROR);

    CHECK(canview_stm_uart_platform_service(&fixture.platform, 4U, 4000U, 64U) == CANVIEW_OK);
    DMA1->ISR = DMA_ISR_TEIF2;
    DMA1_Channel2_IRQHandler();
    CHECK(fixture.platform.tx_error_pending);
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 5U, 5000U, 64U) == CANVIEW_OK);

    USART2->ISR = USART_ISR_ORE;
    USART2_IRQHandler();
    CHECK(fixture.platform.rx_error_pending);
    CHECK(canview_stm_uart_platform_service(&fixture.platform, 6U, 6000U, 64U) == CANVIEW_OK);
    CHECK(fixture.platform.rx_error_recovery_count >= 2U);
    CHECK(fixture_stop(&fixture) == 0);
    return 0;
}

static int test_irq_events_and_singleton(void)
{
    fixture_t first;
    fixture_t second;
    CHECK(fixture_start(&first) == 0);
    memset(&second, 0, sizeof(second));
    const canview_stm_uart_config_t runtime_config = {
        TEST_STM_BOOT_ID + 1U, TEST_STM_DEVICE_ID + 1U,
        CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION, NULL, NULL, {0x42U}};
    CHECK(canview_stm_uart_init(&second.runtime, &runtime_config, 0U, 0U) == CANVIEW_OK);
    const canview_stm_uart_platform_config_t platform_config = {
        &second.runtime, second.rx_buffer, sizeof(second.rx_buffer)};
    CHECK(canview_stm_uart_platform_init(&second.platform, &platform_config) ==
          CANVIEW_RESOURCE_BUSY);

    DMA1->ISR = DMA_ISR_TCIF1 | DMA_ISR_HTIF1;
    DMA1_Channel1_IRQHandler();
    CHECK(first.platform.rx_wrap_count_isr == 1U && first.platform.events != 0U);
    USART2->ISR = USART_ISR_IDLE | USART_ISR_CTSIF;
    USART2_IRQHandler();
    CHECK((first.platform.events & CANVIEW_STM_UART_PLATFORM_EVENT_CTS) != 0U);
    CHECK(fixture_stop(&first) == 0);

    CHECK(canview_stm_uart_platform_init(&second.platform, &platform_config) == CANVIEW_OK);
    CHECK(canview_stm_uart_platform_start(&second.platform) == CANVIEW_OK);
    CHECK(fixture_stop(&second) == 0);
    return 0;
}

static int test_boot_id_rng_contract(void)
{
    fake_hardware_reset();
    fake_rcc.CRRCR = RCC_CRRCR_HSI48RDY;
    fake_rng.SR = RNG_SR_DRDY;
    fake_rng.DR = UINT32_C(0xabcdef01);
    CHECK(canview_stm_uart_platform_boot_id() == UINT64_C(0xabcdef01abcdef01));

    fake_rng.SR = RNG_SR_CECS;
    CHECK(canview_stm_uart_platform_boot_id() == 0U);

    fake_hardware_reset();
    CHECK(canview_stm_uart_platform_boot_id() == 0U);
    return 0;
}

static int test_device_id_contract(void)
{
    fake_hardware_reset();
    canview_test_stm_uid[0] = UINT32_C(0x12345678);
    canview_test_stm_uid[1] = UINT32_C(0x9abcdef0);
    canview_test_stm_uid[2] = UINT32_C(0x0fedcba9);
    const uint64_t device_id = canview_stm_uart_platform_device_id();
    CHECK(device_id != 0U);
    CHECK((device_id >> 32U) != (device_id & UINT64_C(0xffffffff)));
    return 0;
}

int main(void)
{
    CHECK(test_start_cts_and_tx_completion() == 0);
    CHECK(test_runtime_reset_quiesces_dma() == 0);
    CHECK(test_rx_recovery_accounting() == 0);
    CHECK(test_rx_data_and_fault_events() == 0);
    CHECK(test_irq_events_and_singleton() == 0);
    CHECK(test_boot_id_rng_contract() == 0);
    CHECK(test_device_id_contract() == 0);
    (void)puts("STM32 UART DMA platform tests passed");
    return 0;
}
