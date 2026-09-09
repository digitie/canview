/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_board_core.h"
#include "canview_stm_build.h"
#include "canview_stm_uart.h"
#include "canview_build_mode.h"
#include "canview_board.h"
#include "core_hw.h"
#include "uart_dma.h"

static canview_stm_boot_t boot;
static canview_stm_scheduler_t scheduler;
static canview_stm_uart_context_t uart_runtime;
static canview_stm_uart_platform_t uart_platform;
static uint8_t diagnostic_record[CANVIEW_STM_DIAGNOSTIC_ENCODED_BYTES];
static uint8_t uart_rx_buffer[CANVIEW_STM_UART_PLATFORM_RX_CAPACITY]
    __attribute__((aligned(4)));

static canview_status_t uart_worker(void *context)
{
    return canview_stm_uart_platform_service(
        (canview_stm_uart_platform_t *)context, canview_stm_now_ms64(NULL),
        canview_stm_now_us64(NULL), 512U);
}

int main(void)
{
    const canview_stm_boot_port_t boot_port = canview_stm_board_boot_port();
    if (canview_stm_boot_start(&boot, &boot_port) != CANVIEW_OK)
    {
        canview_stm_board_wait_reset();
    }
    canview_stm_diagnostic_t diagnostic;
    size_t diagnostic_bytes = 0U;
    if (canview_stm_board_diagnostic_encode(diagnostic_record, sizeof(diagnostic_record),
                                            &diagnostic_bytes) != CANVIEW_OK ||
        diagnostic_bytes != CANVIEW_STM_DIAGNOSTIC_ENCODED_BYTES)
    {
        boot_port.fault(boot_port.context, CANVIEW_STM_FAULT_BOOT);
        canview_stm_board_wait_reset();
    }
    canview_stm_board_diagnostic(&diagnostic);
    if (!diagnostic.build_metadata_valid || !diagnostic.stack_watermark_valid ||
        diagnostic.control_capabilities != CANVIEW_STM_CONTROL_CAPABILITIES ||
        diagnostic.tx_permit != CANVIEW_STM_TX_PERMIT)
    {
        boot_port.fault(boot_port.context, CANVIEW_STM_FAULT_BOOT);
        canview_stm_board_wait_reset();
    }
    const uint64_t local_device_id = canview_stm_uart_platform_device_id();
    const uint64_t local_boot_id = canview_stm_uart_platform_boot_id();
    if (local_device_id == 0U || local_boot_id == 0U)
    {
        boot_port.fault(boot_port.context, CANVIEW_STM_FAULT_BOOT);
        canview_stm_board_wait_reset();
    }
    canview_stm_uart_config_t uart_config = {
        .local_boot_id = local_boot_id,
        .local_device_id = local_device_id,
        .local_safety_revision = CANVIEW_STM_UART_CAPTURE_ONLY_SAFETY_REVISION,
        .authorize = NULL,
        .authorize_context = NULL};
    if (canview_stm_build_id_digest(uart_config.build_id_digest) != CANVIEW_OK)
    {
        boot_port.fault(boot_port.context, CANVIEW_STM_FAULT_BOOT);
        canview_stm_board_wait_reset();
    }
    if (canview_stm_uart_init(&uart_runtime, &uart_config,
                              canview_stm_now_ms64(NULL),
                              canview_stm_now_us64(NULL)) != CANVIEW_OK)
    {
        boot_port.fault(boot_port.context, CANVIEW_STM_FAULT_BOOT);
        canview_stm_board_wait_reset();
    }
    const canview_stm_uart_platform_config_t uart_platform_config = {
        &uart_runtime, uart_rx_buffer, sizeof(uart_rx_buffer)};
    if (canview_stm_uart_platform_init(&uart_platform, &uart_platform_config) != CANVIEW_OK ||
        canview_stm_uart_platform_start(&uart_platform) != CANVIEW_OK)
    {
        boot_port.fault(boot_port.context, CANVIEW_STM_FAULT_BOOT);
        canview_stm_board_wait_reset();
    }
    const canview_stm_scheduler_port_t schedule_port = canview_stm_board_scheduler_port();
    const canview_stm_worker_t workers[] = {
        {uart_worker, &uart_platform, 1U, 10U, 1000U, true},
        {canview_stm_board_health, NULL, 1U, 10U, 100U, true}};
    if (canview_stm_scheduler_init(&scheduler, workers, sizeof(workers) / sizeof(workers[0]),
                                   &schedule_port, canview_stm_board_now_ms()) != CANVIEW_OK)
    {
        boot_port.fault(boot_port.context, CANVIEW_STM_FAULT_BOOT);
        canview_stm_board_wait_reset();
    }
    const canview_platform_port_t board = canview_board_port();
    for (;;)
    {
        if (canview_stm_scheduler_step(&scheduler, canview_stm_board_now_ms()) != CANVIEW_OK)
        {
            canview_stm_board_wait_reset();
        }
        board.idle(board.context);
    }
}
