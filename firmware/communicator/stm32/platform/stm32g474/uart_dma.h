/* SPDX-License-Identifier: GPL-3.0-only */
/** @file uart_dma.h
 * @brief STM32G474 USART2 4 Mbps DMA UART adapter.
 *
 * This is the only layer in the T-104 STM32 image that touches USART2, DMA1,
 * DMAMUX1 or the UART GPIO registers.  Interrupt handlers publish bounded
 * events only; `service()` is the single worker-context owner of the portable
 * UART runtime and of all byte parsing.
 */
#ifndef CANVIEW_STM_UART_DMA_H
#define CANVIEW_STM_UART_DMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canview_stm_uart.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief UART adapter requires a four-byte aligned circular RX buffer. */
#define CANVIEW_STM_UART_PLATFORM_RX_CAPACITY (CANVIEW_STM_UART_RX_DMA_CAPACITY)

/** @brief Hardware adapter configuration; all pointed objects are caller-owned. */
typedef struct
{
    canview_stm_uart_context_t *runtime;
    uint8_t *rx_buffer;
    size_t rx_capacity;
} canview_stm_uart_platform_config_t;

/** @brief RX recovery cause retained for diagnostics and post-mortem evidence. */
typedef enum
{
    CANVIEW_STM_UART_RX_RECOVERY_NONE = 0,
    CANVIEW_STM_UART_RX_RECOVERY_DMA_OR_USART_ERROR = 1,
    CANVIEW_STM_UART_RX_RECOVERY_CNDTR_INVALID = 2,
    CANVIEW_STM_UART_RX_RECOVERY_RING_OVERRUN = 3
} canview_stm_uart_rx_recovery_reason_t;

/** @brief ISR event bits.  They are internal to the adapter and not wire data. */
enum
{
    CANVIEW_STM_UART_PLATFORM_EVENT_RX = UINT32_C(1) << 0,
    CANVIEW_STM_UART_PLATFORM_EVENT_CTS = UINT32_C(1) << 1,
    CANVIEW_STM_UART_PLATFORM_EVENT_TX_DONE = UINT32_C(1) << 2,
    CANVIEW_STM_UART_PLATFORM_EVENT_TX_ERROR = UINT32_C(1) << 3,
    CANVIEW_STM_UART_PLATFORM_EVENT_RX_ERROR = UINT32_C(1) << 4
};

/** @brief Static adapter state.  No memory is allocated by init/start. */
typedef struct
{
    canview_stm_uart_platform_config_t config;
    /* The DMA position is 11-bit, but the software byte cursor must survive
     * the full 24-hour 4 Mbps acceptance window without wrapping. */
    uint64_t rx_read_total;
    volatile uint32_t rx_wrap_count_isr;
    volatile uint32_t events;
    volatile bool rx_error_pending;
    volatile bool tx_error_pending;
    volatile bool tx_done_pending;
    bool tx_in_flight;
    bool servicing;
    volatile bool started;
    bool initialized;
    uint32_t rx_recovery_count;
    uint32_t rx_error_recovery_count;
    uint32_t rx_overrun_recovery_count;
    uint64_t rx_discarded_bytes;
    uint32_t rx_unknown_loss_count;
    canview_stm_uart_rx_recovery_reason_t last_rx_recovery_reason;
    bool irq_quiesced;
} canview_stm_uart_platform_t;

/** @cond INTERNAL */
typedef char canview_stm_uart_platform_rx_capacity_is_power_of_two[
    (CANVIEW_STM_UART_PLATFORM_RX_CAPACITY != 0U &&
     (CANVIEW_STM_UART_PLATFORM_RX_CAPACITY &
      (CANVIEW_STM_UART_PLATFORM_RX_CAPACITY - 1U)) == 0U)
        ? 1
        : -1];
/** @endcond */

/** @brief Validate config and initialize the caller-owned zero-init adapter context.
 * @param platform caller-owned zero-initialized adapter context.
 * @param config caller-owned runtime, aligned RX buffer and capacity contract.
 * @return `CANVIEW_OK` or invalid configuration.
 */
canview_status_t canview_stm_uart_platform_init(
    canview_stm_uart_platform_t *platform,
    const canview_stm_uart_platform_config_t *config);

/** @brief Reset runtime state and start USART2, RX circular DMA and TX DMA.
 * @param platform initialized adapter context.
 * @return `CANVIEW_OK`, or an invalid/startup failure.
 */
canview_status_t canview_stm_uart_platform_start(canview_stm_uart_platform_t *platform);

/** @brief Stop IRQ/DMA/USART and return PA0..PA3 to input/no-pull state.
 * @param platform initialized adapter context.
 * @return `CANVIEW_OK` or invalid argument.
 */
canview_status_t canview_stm_uart_platform_stop(canview_stm_uart_platform_t *platform);

/** @brief Run a bounded worker service step; never parses bytes in interrupt context.
 * @param platform started adapter context.
 * @param now_ms current monotonic millisecond timestamp.
 * @param now_us current monotonic microsecond timestamp.
 * @param rx_budget maximum number of RX bytes parsed in this step.
 * @return `CANVIEW_OK`, maintenance failure, or invalid argument.
 */
canview_status_t canview_stm_uart_platform_service(canview_stm_uart_platform_t *platform,
                                                    uint64_t now_ms, uint64_t now_us,
                                                    size_t rx_budget);

/** @brief Derive a stable nonzero device identity from the STM32 factory UID.
 * @return stable nonzero device identity, or zero when the UID is unavailable.
 */
uint64_t canview_stm_uart_platform_device_id(void);

/** @brief Read a fresh nonzero boot identity from the hardware RNG, or return zero on failure.
 * @return fresh nonzero boot identity, or zero when the RNG cannot provide one.
 */
uint64_t canview_stm_uart_platform_boot_id(void);

/** @cond INTERNAL */
void DMA1_Channel1_IRQHandler(void);
void DMA1_Channel2_IRQHandler(void);
void USART2_IRQHandler(void);
/** @endcond */

#ifdef __cplusplus
}
#endif
#endif
