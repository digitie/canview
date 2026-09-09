/* SPDX-License-Identifier: GPL-3.0-only */
/** @file uart_link.c
 * @brief Portable STM32 owner for the Communicator UART link.
 *
 * The module is deliberately independent from CMSIS, HAL and FreeRTOS.  One
 * cooperative worker owns the decoder, link state, queues, cache and pending
 * command slots.  The platform adapter only supplies bytes, CTS samples and
 * completed DMA transfers.  No function in this file creates or transmits a
 * CAN frame.
 */
#include "canview_stm_uart.h"

#include <limits.h>
#include <string.h>

#define UART_HELLO_SIZE (72U)
#define UART_HELLO_ACK_SIZE (40U)
#define UART_HEARTBEAT_SIZE (48U)
#define UART_ACK_SIZE (20U)
#define UART_ERROR_SIZE (20U)
#define UART_COMMAND_RESULT_SIZE (82U)
#define UART_CONTROL_LEASE_SIZE (52U)
#define UART_CONTROL_TIME_SYNC_SIZE (80U)
#define UART_SAFETY_SNAPSHOT_SIZE (48U)

#define UART_PROTOCOL_HARDWARE_REVISION (UINT16_C(0))
#define UART_FIRMWARE_MAJOR (UINT16_C(0))
#define UART_FIRMWARE_MINOR (UINT16_C(1))
#define UART_FIRMWARE_PATCH (UINT16_C(0))
#define UART_CAPTURE_ONLY_CAPABILITY (UINT64_C(0))

#define UART_LEASE_ACQUIRE (UINT8_C(1))
#define UART_LEASE_RENEW (UINT8_C(2))
#define UART_LEASE_RELEASE (UINT8_C(3))
#define UART_LEASE_MAX_MS (UINT32_C(30000))

#define UART_ERROR_ORIGIN_STM32 (UINT8_C(1))
#define UART_ERROR_SEVERITY_REQUEST (UINT8_C(1))
#define UART_ERROR_SEVERITY_BUSY (UINT8_C(2))
#define UART_ERROR_RATE_LIMIT (CANVIEW_STM_UART_ERROR_RATE_LIMIT)
#define UART_ERROR_RATE_WINDOW_MS (CANVIEW_STM_UART_ERROR_RATE_WINDOW_MS)
#define UART_RETRY_BUSY_MS (UINT32_C(100))
#define UART_RETRY_TIMEOUT_MS (UINT32_C(1000))
#define UART_SAFETY_FLAG_CAPTURE_ONLY (UINT32_C(1))
#define UART_SAFETY_INHIBIT_CAPTURE_ONLY (UINT16_C(1))
#define UART_SAFETY_TX_GATE_CLOSED (UINT8_C(0))

static uint64_t read_le(const uint8_t *bytes, size_t width)
{
    uint64_t value = 0U;
    for (size_t index = 0U; index < width; ++index)
    {
        value |= (uint64_t)bytes[index] << (index * 8U);
    }
    return value;
}

static void write_le(uint8_t *bytes, size_t width, uint64_t value)
{
    for (size_t index = 0U; index < width; ++index)
    {
        bytes[index] = (uint8_t)(value >> (index * 8U));
    }
}

static void increment_saturating(uint32_t *value)
{
    if (value != NULL && *value != UINT32_MAX)
    {
        ++*value;
    }
}

static void add_saturating(uint32_t *value, uint32_t amount)
{
    if (value == NULL)
    {
        return;
    }
    if (amount > UINT32_MAX - *value)
    {
        *value = UINT32_MAX;
    }
    else
    {
        *value += amount;
    }
}

static uint64_t elapsed_ms(uint64_t now_ms, uint64_t then_ms)
{
    return now_ms < then_ms ? UINT64_MAX : now_ms - then_ms;
}

static bool context_ready(const canview_stm_uart_context_t *context)
{
    return context != NULL && context->initialized;
}

static bool reject_reentry(canview_stm_uart_context_t *context)
{
    if (context != NULL && context->servicing)
    {
        increment_saturating(&context->stats.callback_reentry);
        return true;
    }
    return false;
}

static bool queue_class_valid(canview_stm_uart_tx_class_t queue_class)
{
    return queue_class < CANVIEW_STM_UART_TX_CLASS_COUNT;
}

static bool p0_available(const canview_stm_uart_context_t *context)
{
    return context != NULL && context->tx_queues[CANVIEW_STM_UART_TX_P0].count <
                                  CANVIEW_STM_UART_TX_QUEUE_CAPACITY;
}

static bool p1_available(const canview_stm_uart_context_t *context)
{
    return context != NULL && context->tx_queues[CANVIEW_STM_UART_TX_P1].count <
                                  CANVIEW_STM_UART_TX_QUEUE_CAPACITY;
}

static uint32_t next_sequence(canview_stm_uart_context_t *context)
{
    uint32_t sequence = context->next_tx_sequence;
    if (sequence == 0U)
    {
        sequence = 1U;
    }
    context->next_tx_sequence = sequence == UINT32_MAX ? 1U : sequence + 1U;
    return sequence;
}

static canview_status_t make_tx_item(canview_stm_uart_context_t *context,
                                     uint8_t message_type, uint8_t flags,
                                     uint32_t correlation_id, uint64_t sender_time_us,
                                     const uint8_t *payload, size_t payload_size,
                                     canview_stm_uart_tx_item_t *item)
{
    if (context == NULL || item == NULL || payload_size > CANVIEW_STM_UART_COMMAND_PAYLOAD_MAX ||
        (payload == NULL && payload_size != 0U))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(item, 0, sizeof(*item));
    item->valid = true;
    item->message_type = message_type;
    item->flags = flags;
    item->payload_size = (uint16_t)payload_size;
    item->sequence = next_sequence(context);
    item->correlation_id = correlation_id;
    item->sender_time_us = sender_time_us;
    if (payload_size != 0U)
    {
        memcpy(item->payload, payload, payload_size);
    }
    return CANVIEW_OK;
}

static void mark_queue_full(canview_stm_uart_context_t *context,
                            canview_stm_uart_tx_class_t queue_class)
{
    increment_saturating(&context->stats.tx_queue_full);
    if (queue_class == CANVIEW_STM_UART_TX_P0 || queue_class == CANVIEW_STM_UART_TX_P1)
    {
        context->safety_inhibited = true;
        increment_saturating(&context->stats.tx_safety_inhibited);
    }
}

static canview_status_t publish_tx_item(canview_stm_uart_context_t *context,
                                        canview_stm_uart_tx_class_t queue_class,
                                        const canview_stm_uart_tx_item_t *item)
{
    if (!context_ready(context) || !queue_class_valid(queue_class) || item == NULL ||
        !item->valid)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_stm_uart_tx_queue_t *queue = &context->tx_queues[queue_class];
    if (queue_class == CANVIEW_STM_UART_TX_STATE)
    {
        for (size_t index = 0U; index < CANVIEW_STM_UART_TX_QUEUE_CAPACITY; ++index)
        {
            canview_stm_uart_tx_item_t *existing = &queue->items[index];
            if (existing->valid && existing->message_type == item->message_type &&
                existing->correlation_id == item->correlation_id)
            {
                *existing = *item;
                increment_saturating(&context->stats.tx_enqueued);
                return CANVIEW_OK;
            }
        }
    }
    if (queue->count >= CANVIEW_STM_UART_TX_QUEUE_CAPACITY)
    {
        if (queue_class == CANVIEW_STM_UART_TX_RAW)
        {
            memset(&queue->items[queue->head], 0, sizeof(queue->items[queue->head]));
            queue->head = (uint8_t)((queue->head + 1U) % CANVIEW_STM_UART_TX_QUEUE_CAPACITY);
            --queue->count;
            increment_saturating(&context->stats.tx_raw_dropped);
        }
        else
        {
            mark_queue_full(context, queue_class);
            return CANVIEW_RESOURCE_BUSY;
        }
    }
    queue->items[queue->tail] = *item;
    queue->tail = (uint8_t)((queue->tail + 1U) % CANVIEW_STM_UART_TX_QUEUE_CAPACITY);
    ++queue->count;
    increment_saturating(&context->stats.tx_enqueued);
    return CANVIEW_OK;
}

static canview_status_t enqueue_validated(canview_stm_uart_context_t *context,
                                          canview_stm_uart_tx_class_t queue_class,
                                          uint8_t message_type, uint8_t flags,
                                          uint32_t correlation_id, uint64_t sender_time_us,
                                          const uint8_t *payload, size_t payload_size)
{
    canview_stm_uart_tx_item_t item;
    const canview_status_t item_status =
        make_tx_item(context, message_type, flags, correlation_id, sender_time_us, payload,
                     payload_size, &item);
    if (item_status != CANVIEW_OK)
    {
        return item_status;
    }
    return publish_tx_item(context, queue_class, &item);
}

static bool tx_contains_message(const canview_stm_uart_context_t *context, uint8_t message_type)
{
    if (context == NULL)
    {
        return false;
    }
    if (context->tx_current.valid && context->tx_current.message_type == message_type)
    {
        return true;
    }
    for (size_t queue_index = 0U; queue_index < CANVIEW_STM_UART_TX_CLASS_COUNT; ++queue_index)
    {
        const canview_stm_uart_tx_queue_t *queue = &context->tx_queues[queue_index];
        for (size_t item_index = 0U; item_index < CANVIEW_STM_UART_TX_QUEUE_CAPACITY;
             ++item_index)
        {
            if (queue->items[item_index].valid &&
                queue->items[item_index].message_type == message_type)
            {
                return true;
            }
        }
    }
    return false;
}

static canview_status_t enqueue_hello(canview_stm_uart_context_t *context)
{
    uint8_t payload[UART_HELLO_SIZE] = {0};
    write_le(payload, 8U, context->local_boot_id);
    payload[8U] = CANVIEW_UART_PROTOCOL_MAJOR;
    payload[9U] = CANVIEW_UART_PROTOCOL_MINOR;
    payload[10U] = CANVIEW_UART_PROTOCOL_MAJOR;
    payload[11U] = CANVIEW_UART_PROTOCOL_MINOR;
    write_le(payload + 12U, 2U, UART_PROTOCOL_HARDWARE_REVISION);
    write_le(payload + 14U, 2U, UART_FIRMWARE_MAJOR);
    write_le(payload + 16U, 2U, UART_FIRMWARE_MINOR);
    write_le(payload + 18U, 2U, UART_FIRMWARE_PATCH);
    write_le(payload + 20U, 8U, UART_CAPTURE_ONLY_CAPABILITY);
    write_le(payload + 28U, 2U, CANVIEW_UART_MAX_FRAME_SIZE);
    write_le(payload + 48U, 8U, context->local_device_id);
    return enqueue_validated(context, CANVIEW_STM_UART_TX_P0, CANVIEW_UART_MSG_LINK_HELLO, 0U, 0U,
                             context->last_now_us, payload, sizeof(payload));
}

static canview_status_t enqueue_hello_if_missing(canview_stm_uart_context_t *context)
{
    if (!tx_contains_message(context, CANVIEW_UART_MSG_LINK_HELLO))
    {
        return enqueue_hello(context);
    }
    return CANVIEW_OK;
}

static canview_status_t enqueue_hello_ack(canview_stm_uart_context_t *context,
                                          uint64_t peer_boot_id, uint32_t correlation_id)
{
    uint8_t payload[UART_HELLO_ACK_SIZE] = {0};
    write_le(payload, 8U, context->local_boot_id);
    write_le(payload + 8U, 8U, peer_boot_id);
    payload[16U] = CANVIEW_UART_PROTOCOL_MAJOR;
    payload[17U] = CANVIEW_UART_PROTOCOL_MINOR;
    payload[18U] = 0U;
    write_le(payload + 20U, 8U, UART_CAPTURE_ONLY_CAPABILITY);
    write_le(payload + 28U, 2U, CANVIEW_UART_MAX_FRAME_SIZE);
    return enqueue_validated(context, CANVIEW_STM_UART_TX_P0, CANVIEW_UART_MSG_LINK_HELLO_ACK,
                             CANVIEW_UART_FLAG_RESPONSE, correlation_id, context->last_now_us,
                             payload, sizeof(payload));
}

static canview_status_t enqueue_ack(canview_stm_uart_context_t *context, uint32_t sequence,
                                    uint32_t correlation_id, uint64_t request_token,
                                    canview_status_t status, uint16_t detail, uint64_t now_ms,
                                    uint64_t now_us)
{
    uint8_t payload[UART_ACK_SIZE] = {0};
    write_le(payload, 4U, sequence);
    write_le(payload + 4U, 2U, detail);
    write_le(payload + 6U, 2U, (uint16_t)status);
    write_le(payload + 8U, 8U, request_token);
    write_le(payload + 16U, 4U, (uint32_t)now_ms);
    return enqueue_validated(context, CANVIEW_STM_UART_TX_P0, CANVIEW_UART_MSG_ACK,
                             CANVIEW_UART_FLAG_RESPONSE, correlation_id, now_us, payload,
                             sizeof(payload));
}

static uint16_t reason_for_status(canview_status_t status)
{
    switch (status)
    {
    case CANVIEW_AUTH_FAILED:
        return CANVIEW_STM_UART_REASON_AUTH_FAILED;
    case CANVIEW_RESOURCE_BUSY:
        return CANVIEW_STM_UART_REASON_QUEUE_FULL;
    case CANVIEW_TIMEOUT:
        return CANVIEW_STM_UART_REASON_TIMEOUT;
    case CANVIEW_DUPLICATE:
        return CANVIEW_STM_UART_REASON_DUPLICATE;
    case CANVIEW_STALE:
        return CANVIEW_STM_UART_REASON_TOKEN_CONFLICT;
    case CANVIEW_NOT_IMPLEMENTED:
    case CANVIEW_UNSUPPORTED_MESSAGE:
        return CANVIEW_STM_UART_REASON_UNSUPPORTED;
    default:
        return CANVIEW_STM_UART_REASON_INVALID;
    }
}

static canview_status_t enqueue_error(canview_stm_uart_context_t *context, uint8_t message_type,
                                      uint32_t sequence, uint32_t correlation_id,
                                      canview_status_t status, uint16_t detail, uint64_t now_us)
{
    if (!context_ready(context))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (elapsed_ms(context->last_now_ms, context->error_window_started_ms) >=
        UART_ERROR_RATE_WINDOW_MS)
    {
        context->error_window_started_ms = context->last_now_ms;
        context->error_responses_in_window = 0U;
    }
    if (context->error_responses_in_window >= UART_ERROR_RATE_LIMIT)
    {
        increment_saturating(&context->stats.error_responses_rate_limited);
        return CANVIEW_RESOURCE_BUSY;
    }
    ++context->error_responses_in_window;
    uint8_t payload[UART_ERROR_SIZE] = {0};
    const bool busy = status == CANVIEW_RESOURCE_BUSY;
    write_le(payload, 2U, (uint16_t)status);
    payload[2U] = busy ? UART_ERROR_SEVERITY_BUSY : UART_ERROR_SEVERITY_REQUEST;
    payload[3U] = UART_ERROR_ORIGIN_STM32;
    payload[4U] = message_type;
    write_le(payload + 6U, 2U, detail == 0U ? reason_for_status(status) : detail);
    write_le(payload + 8U, 4U, sequence);
    write_le(payload + 12U, 4U,
             status == CANVIEW_TIMEOUT ? UART_RETRY_TIMEOUT_MS
                                       : (busy ? UART_RETRY_BUSY_MS : 0U));
    return enqueue_validated(context, CANVIEW_STM_UART_TX_P1, CANVIEW_UART_MSG_ERROR,
                             (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_ERROR),
                             correlation_id, now_us, payload, sizeof(payload));
}

static canview_status_t enqueue_heartbeat(canview_stm_uart_context_t *context, uint64_t now_us,
                                          uint64_t now_ms)
{
    uint8_t payload[UART_HEARTBEAT_SIZE] = {0};
    const canview_stm_uart_tx_queue_t *p0 = &context->tx_queues[CANVIEW_STM_UART_TX_P0];
    const canview_stm_uart_tx_queue_t *p1 = &context->tx_queues[CANVIEW_STM_UART_TX_P1];
    const canview_stm_uart_tx_queue_t *state = &context->tx_queues[CANVIEW_STM_UART_TX_STATE];
    const canview_stm_uart_tx_queue_t *raw = &context->tx_queues[CANVIEW_STM_UART_TX_RAW];
    uint64_t cts_duration = 0U;
    if (context->cts_known && context->cts_blocked)
    {
        cts_duration = elapsed_ms(now_ms, context->link.cts_blocked_since_ms);
    }
    write_le(payload, 8U, context->local_boot_id);
    write_le(payload + 8U, 8U, now_us);
    payload[16U] = (uint8_t)context->link.state;
    payload[17U] = context->safety_inhibited ? UINT8_C(1) : UINT8_C(0);
    write_le(payload + 18U, 2U, p0->count + p1->count);
    write_le(payload + 20U, 2U, state->count);
    write_le(payload + 22U, 2U, raw->count);
    write_le(payload + 24U, 4U, context->stats.tx_raw_dropped);
    write_le(payload + 28U, 4U, context->stats.protocol_errors);
    write_le(payload + 32U, 4U, cts_duration > UINT32_MAX ? UINT32_MAX : cts_duration);
    write_le(payload + 36U, 2U, context->safety_inhibited ? UINT16_C(1) : UINT16_C(0));
    write_le(payload + 40U, 4U, context->local_safety_revision);
    return enqueue_validated(context, CANVIEW_STM_UART_TX_STATE, CANVIEW_UART_MSG_HEARTBEAT, 0U,
                             0U, now_us, payload, sizeof(payload));
}

static canview_status_t enqueue_safety_snapshot(canview_stm_uart_context_t *context,
                                                uint64_t now_us)
{
    if (!context_ready(context))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    uint8_t payload[UART_SAFETY_SNAPSHOT_SIZE] = {0};
    write_le(payload, 8U, context->local_boot_id);
    write_le(payload + 8U, 4U, context->local_safety_revision);
    write_le(payload + 12U, 4U, context->local_safety_revision);
    write_le(payload + 16U, 4U, UART_SAFETY_FLAG_CAPTURE_ONLY);
    write_le(payload + 20U, 2U, UART_SAFETY_INHIBIT_CAPTURE_ONLY);
    payload[22U] = context->lease.valid ? UINT8_C(1) : UINT8_C(0);
    payload[23U] = UART_SAFETY_TX_GATE_CLOSED;
    write_le(payload + 32U, 8U, now_us);
    return enqueue_validated(context, CANVIEW_STM_UART_TX_STATE,
                             CANVIEW_UART_MSG_SAFETY_SNAPSHOT, CANVIEW_UART_FLAG_SNAPSHOT, 0U,
                             now_us, payload, sizeof(payload));
}

static bool signed_difference(uint64_t left, uint64_t right, int64_t *difference)
{
    if (difference == NULL)
    {
        return false;
    }
    if (left >= right)
    {
        const uint64_t magnitude = left - right;
        if (magnitude > (uint64_t)INT64_MAX)
        {
            return false;
        }
        *difference = (int64_t)magnitude;
        return true;
    }
    const uint64_t magnitude = right - left;
    if (magnitude > (uint64_t)INT64_MAX)
    {
        return false;
    }
    *difference = -(int64_t)magnitude;
    return true;
}

static bool signed_sum(int64_t left, int64_t right, int64_t *sum)
{
    if (sum == NULL || (right > 0 && left > INT64_MAX - right) ||
        (right < 0 && left < INT64_MIN - right))
    {
        return false;
    }
    *sum = left + right;
    return true;
}

static bool apply_offset(uint64_t base, int64_t offset, uint64_t *mapped)
{
    if (mapped == NULL)
    {
        return false;
    }
    if (offset >= 0)
    {
        const uint64_t positive = (uint64_t)offset;
        if (positive > UINT64_MAX - base)
        {
            return false;
        }
        *mapped = base + positive;
        return true;
    }
    const uint64_t magnitude = (uint64_t)(-(offset + INT64_C(1))) + UINT64_C(1);
    if (magnitude > base)
    {
        return false;
    }
    *mapped = base - magnitude;
    return true;
}

static bool command_deadline(const canview_stm_uart_context_t *context,
                             const uint8_t *payload, uint64_t *issued_stm_us,
                             uint64_t *deadline_stm_us)
{
    if (context == NULL || payload == NULL || issued_stm_us == NULL || deadline_stm_us == NULL ||
        !context->time_mapping.valid)
    {
        return false;
    }
    const uint64_t issued_controller_ms = read_le(payload + 36U, 4U);
    const uint64_t ttl_ms = read_le(payload + 10U, 2U);
    if (issued_controller_ms > UINT64_MAX / UINT64_C(1000) ||
        ttl_ms > UINT64_MAX / UINT64_C(1000))
    {
        return false;
    }
    const uint64_t issued_controller_us = issued_controller_ms * UINT64_C(1000);
    const uint64_t ttl_us = ttl_ms * UINT64_C(1000);
    if (issued_controller_us > UINT64_MAX - ttl_us)
    {
        return false;
    }
    if (!apply_offset(issued_controller_us, context->time_mapping.offset_stm_minus_controller_us,
                      issued_stm_us))
    {
        return false;
    }
    const uint64_t deadline_controller_us = issued_controller_us + ttl_us;
    return apply_offset(deadline_controller_us,
                        context->time_mapping.offset_stm_minus_controller_us,
                        deadline_stm_us);
}

static bool command_time_valid(const canview_stm_uart_context_t *context, const uint8_t *payload,
                               uint64_t now_ms, uint64_t now_us)
{
    if (context == NULL || payload == NULL || !context->time_mapping.valid ||
        elapsed_ms(now_ms, context->time_mapping.updated_ms) >=
            CANVIEW_UART_CONTROL_TIME_MAX_AGE_MS ||
        read_le(payload + 20U, 8U) != context->time_mapping.controller_boot_id ||
        read_le(payload + 40U, 4U) != context->time_mapping.generation ||
        context->time_mapping.stm_boot_id != context->local_boot_id ||
        context->time_mapping.uncertainty_us > CANVIEW_UART_CONTROL_TIME_MAX_UNCERTAINTY_US)
    {
        return false;
    }
    uint64_t issued_stm_us = 0U;
    uint64_t deadline_stm_us = 0U;
    if (!command_deadline(context, payload, &issued_stm_us, &deadline_stm_us) ||
        now_us < issued_stm_us)
    {
        return false;
    }
    const uint64_t uncertainty = context->time_mapping.uncertainty_us;
    if (deadline_stm_us <= uncertainty)
    {
        return false;
    }
    return now_us < deadline_stm_us - uncertainty;
}

static bool runtime_authorize(const canview_uart_message_view_t *view, uint64_t now_ms,
                              void *opaque)
{
    canview_stm_uart_context_t *context = (canview_stm_uart_context_t *)opaque;
    if (!context_ready(context) || view == NULL || view->policy == NULL ||
        !canview_uart_link_command_admission_allowed(&context->link, now_ms) ||
        context->link.safety_revision != context->local_safety_revision ||
        context->authorize == NULL)
    {
        return false;
    }
    if (view->wire.header.message_type == CANVIEW_UART_MSG_COMMAND_REQUEST &&
        !command_time_valid(context, view->wire.payload, now_ms, context->last_now_us))
    {
        return false;
    }
    return context->authorize(view, now_ms, context->authorize_context);
}

static canview_status_t prepare_candidate(const canview_stm_uart_context_t *context,
                                           const canview_uart_message_view_t *view,
                                           uint64_t now_ms,
                                           canview_uart_replay_context_t *candidate)
{
    if (!context_ready(context) || view == NULL || candidate == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_uart_command_admission_context_t authorization = {
        runtime_authorize, (void *)context};
    *candidate = context->replay;
    return canview_uart_message_admit(view, CANVIEW_UART_ENDPOINT_STM32,
                                      CANVIEW_UART_FLOW_INBOUND, candidate, &authorization,
                                      now_ms);
}

static void clear_pending_slots(canview_stm_uart_context_t *context)
{
    if (context->pending_count != 0U)
    {
        add_saturating(&context->stats.pending_cancelled, context->pending_count);
    }
    memset(context->pending, 0, sizeof(context->pending));
    context->pending_head = 0U;
    context->pending_count = 0U;
}

static canview_status_t clear_local_session(canview_stm_uart_context_t *context, uint64_t now_ms,
                                            bool reset_codec)
{
    if (!context_ready(context))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    clear_pending_slots(context);
    memset(context->tx_queues, 0, sizeof(context->tx_queues));
    if (context->tx_current.valid)
    {
        increment_saturating(&context->stats.tx_dropped);
    }
    memset(&context->tx_current, 0, sizeof(context->tx_current));
    context->tx_current_class = CANVIEW_STM_UART_TX_P0;
    memset(&context->time_mapping, 0, sizeof(context->time_mapping));
    memset(&context->lease, 0, sizeof(context->lease));
    context->pending_sync_token = 0U;
    context->pending_sync_controller_boot_id = 0U;
    context->pending_sync_generation = 0U;
    context->pending_sync_t1_controller_us = 0U;
    context->pending_sync_valid = false;
    memset(context->tx_serial, 0, sizeof(context->tx_serial));
    memset(context->tx_scratch, 0, sizeof(context->tx_scratch));
    context->tx_serial_size = 0U;
    context->last_heartbeat_tx_ms = now_ms;
    context->error_window_started_ms = now_ms;
    context->error_responses_in_window = 0U;
    if (!reset_codec)
    {
        return CANVIEW_OK;
    }
    return canview_uart_codec_reset(&context->codec, CANVIEW_UART_ENDPOINT_STM32,
                                    CANVIEW_UART_FLOW_INBOUND);
}

static canview_status_t begin_session_recovery(canview_stm_uart_context_t *context,
                                               uint64_t now_ms, bool reset_codec)
{
    increment_saturating(&context->stats.session_invalidations);
    increment_saturating(&context->stats.link_resets);
    const canview_status_t clear_status = clear_local_session(context, now_ms, reset_codec);
    if (clear_status != CANVIEW_OK)
    {
        return clear_status;
    }
    if (context->cts_known)
    {
        const canview_status_t cts_status =
            canview_uart_link_set_cts_blocked(&context->link, context->cts_blocked, now_ms);
        if (cts_status != CANVIEW_OK && cts_status != CANVIEW_TIMEOUT)
        {
            return cts_status;
        }
    }
    return enqueue_hello(context);
}

static bool pending_handle_equal(const canview_stm_uart_pending_command_t *pending,
                                 const canview_uart_command_handle_t *handle)
{
    return pending != NULL && handle != NULL && pending->valid &&
           pending->cache_handle.slot == handle->slot &&
           pending->cache_handle.generation == handle->generation &&
           pending->cache_handle.reserved == handle->reserved;
}

static size_t find_pending(const canview_stm_uart_context_t *context,
                           const canview_uart_command_handle_t *handle)
{
    if (!context_ready(context) || handle == NULL)
    {
        return CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY;
    }
    for (size_t index = 0U; index < CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY; ++index)
    {
        if (pending_handle_equal(&context->pending[index], handle))
        {
            return index;
        }
    }
    return CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY;
}

static void remove_pending(canview_stm_uart_context_t *context, size_t index)
{
    const size_t last =
        (context->pending_head + (size_t)context->pending_count - 1U) %
        CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY;
    size_t current = index;
    while (current != last)
    {
        const size_t next = (current + 1U) % CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY;
        context->pending[current] = context->pending[next];
        current = next;
    }
    memset(&context->pending[last], 0, sizeof(context->pending[last]));
    --context->pending_count;
    if (context->pending_count == 0U)
    {
        context->pending_head = 0U;
    }
}

static size_t pending_tail_index(const canview_stm_uart_context_t *context)
{
    return (context->pending_head + (size_t)context->pending_count) %
           CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY;
}

static canview_status_t build_command_result(const canview_stm_uart_pending_command_t *pending,
                                             uint8_t stage, uint16_t reason,
                                             uint32_t state_revision, uint32_t feedback_revision,
                                             uint32_t feedback_time_ms, uint64_t now_ms,
                                             uint8_t payload[UART_COMMAND_RESULT_SIZE])
{
    if (pending == NULL || !pending->valid || payload == NULL || stage <
            CANVIEW_STM_UART_RESULT_STAGE_COMPLETED ||
        stage > CANVIEW_STM_UART_RESULT_STAGE_BUSY)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(payload, 0, UART_COMMAND_RESULT_SIZE);
    write_le(payload, 8U, pending->key.request_token);
    write_le(payload + 8U, 2U, pending->key.command_id);
    payload[10U] = stage;
    write_le(payload + 12U, 2U, reason);
    write_le(payload + 14U, 4U, state_revision);
    write_le(payload + 18U, 4U, (uint32_t)now_ms);
    write_le(payload + 22U, 4U, pending->request_sequence);
    memcpy(payload + 26U, pending->payload + 88U, CANVIEW_UART_CONTROL_TAG_SIZE);
    write_le(payload + 42U, 4U, feedback_revision);
    write_le(payload + 46U, 4U, feedback_time_ms);
    memcpy(payload + 50U, pending->key.canonical_argument_digest,
           CANVIEW_UART_COMMAND_DIGEST_SIZE);
    return CANVIEW_OK;
}

static canview_status_t complete_pending_internal(
    canview_stm_uart_context_t *context, const canview_uart_command_handle_t *handle,
    uint8_t stage, uint16_t reason, uint32_t state_revision, uint32_t feedback_revision,
    uint32_t feedback_time_ms, uint64_t now_ms, uint64_t now_us)
{
    if (!context_ready(context) || handle == NULL || stage <
            CANVIEW_STM_UART_RESULT_STAGE_COMPLETED ||
        stage > CANVIEW_STM_UART_RESULT_STAGE_BUSY)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const size_t pending_index = find_pending(context, handle);
    if (pending_index >= CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY)
    {
        return CANVIEW_STALE;
    }
    if (!p0_available(context))
    {
        mark_queue_full(context, CANVIEW_STM_UART_TX_P0);
        return CANVIEW_RESOURCE_BUSY;
    }
    uint8_t result[UART_COMMAND_RESULT_SIZE];
    const canview_status_t build_status =
        build_command_result(&context->pending[pending_index], stage, reason, state_revision,
                             feedback_revision, feedback_time_ms, now_ms, result);
    if (build_status != CANVIEW_OK)
    {
        return build_status;
    }
    const canview_status_t cache_status = canview_uart_command_cache_record_result(
        &context->command_cache, &context->pending[pending_index].cache_handle, result,
        sizeof(result), now_ms);
    if (cache_status != CANVIEW_OK)
    {
        if (stage != CANVIEW_STM_UART_RESULT_STAGE_EXPIRED ||
            (cache_status != CANVIEW_TIMEOUT && cache_status != CANVIEW_STALE))
        {
            return cache_status;
        }
    }
    const canview_status_t queue_status = enqueue_validated(
        context, CANVIEW_STM_UART_TX_P0, CANVIEW_UART_MSG_COMMAND_RESULT,
        CANVIEW_UART_FLAG_RESPONSE, context->pending[pending_index].correlation_id, now_us, result,
        sizeof(result));
    if (queue_status != CANVIEW_OK)
    {
        return queue_status;
    }
    if (stage == CANVIEW_STM_UART_RESULT_STAGE_COMPLETED)
    {
        increment_saturating(&context->stats.execution_count);
    }
    if (stage == CANVIEW_STM_UART_RESULT_STAGE_EXPIRED)
    {
        increment_saturating(&context->stats.commands_expired);
    }
    increment_saturating(&context->stats.commands_completed);
    remove_pending(context, pending_index);
    return CANVIEW_OK;
}

static canview_status_t handle_hello(canview_stm_uart_context_t *context,
                                     const canview_uart_message_view_t *view, uint64_t now_ms)
{
    const uint8_t *payload = view->wire.payload;
    const uint64_t peer_boot_id = read_le(payload, 8U);
    const bool version_supported =
        payload[8U] <= CANVIEW_UART_PROTOCOL_MAJOR && payload[10U] >= CANVIEW_UART_PROTOCOL_MAJOR;
    if (!version_supported)
    {
        increment_saturating(&context->stats.protocol_errors);
        return enqueue_error(context, CANVIEW_UART_MSG_LINK_HELLO, view->wire.header.sequence,
                             view->wire.header.correlation_id, CANVIEW_UNSUPPORTED_VERSION,
                             CANVIEW_STM_UART_REASON_UNSUPPORTED, context->last_now_us);
    }
    const bool new_epoch = context->link.peer_boot_id == 0U || !context->link.hello_complete ||
                           context->link.peer_boot_id != peer_boot_id;
    if (new_epoch)
    {
        bool boot_changed = false;
        const canview_status_t session_status = canview_uart_session_note_hello(
            &context->link, &context->plan, &context->command_cache, &context->replay,
            peer_boot_id, now_ms, &boot_changed);
        (void)boot_changed;
        if (session_status != CANVIEW_OK)
        {
            return session_status;
        }
        const canview_status_t recovery_status = begin_session_recovery(context, now_ms, false);
        if (recovery_status != CANVIEW_OK)
        {
            return recovery_status;
        }
    }
    canview_uart_replay_context_t candidate;
    const canview_status_t admission = prepare_candidate(context, view, now_ms, &candidate);
    if (admission != CANVIEW_OK)
    {
        return admission;
    }
    if (!p0_available(context))
    {
        mark_queue_full(context, CANVIEW_STM_UART_TX_P0);
        return CANVIEW_RESOURCE_BUSY;
    }
    const canview_status_t ack_status =
        enqueue_hello_ack(context, peer_boot_id, view->wire.header.correlation_id);
    if (ack_status != CANVIEW_OK)
    {
        return ack_status;
    }
    context->replay = candidate;
    return CANVIEW_OK;
}

static canview_status_t handle_hello_ack(canview_stm_uart_context_t *context,
                                         const canview_uart_message_view_t *view,
                                         uint64_t now_ms)
{
    const uint8_t *payload = view->wire.payload;
    if (!context->link.hello_complete || read_le(payload + 8U, 8U) != context->local_boot_id)
    {
        return CANVIEW_AUTH_FAILED;
    }
    canview_uart_replay_context_t candidate;
    const canview_status_t admission = prepare_candidate(context, view, now_ms, &candidate);
    if (admission != CANVIEW_OK)
    {
        return admission;
    }
    const canview_status_t link_status = canview_uart_link_note_hello_ack(
        &context->link, read_le(payload, 8U), payload[16U], payload[17U], payload[18U], now_ms);
    if (link_status != CANVIEW_OK)
    {
        return link_status;
    }
    const canview_status_t snapshot_status =
        enqueue_safety_snapshot(context, context->last_now_us);
    if (snapshot_status != CANVIEW_OK)
    {
        return snapshot_status;
    }
    const canview_status_t snapshot_note_status = canview_uart_link_note_safety_snapshot(
        &context->link, read_le(payload, 8U), context->local_safety_revision, now_ms);
    if (snapshot_note_status != CANVIEW_OK)
    {
        return snapshot_note_status;
    }
    context->replay = candidate;
    return CANVIEW_OK;
}

static canview_status_t handle_heartbeat(canview_stm_uart_context_t *context,
                                         const canview_uart_message_view_t *view,
                                         uint64_t now_ms)
{
    const uint8_t *payload = view->wire.payload;
    const uint64_t peer_boot_id = read_le(payload, 8U);
    /* HEARTBEAT calls this field state_revision; T-105 binds it to safety state. */
    const uint32_t peer_state_revision = (uint32_t)read_le(payload + 40U, 4U);
    const bool new_epoch = context->link.peer_boot_id == 0U || !context->link.hello_complete ||
                           context->link.peer_boot_id != peer_boot_id;
    if (new_epoch)
    {
        bool boot_changed = false;
        const canview_status_t note_status = canview_uart_session_note_heartbeat(
            &context->link, &context->plan, &context->command_cache, &context->replay,
            peer_boot_id, peer_state_revision, now_ms, &boot_changed);
        (void)boot_changed;
        if (note_status != CANVIEW_OK)
        {
            return note_status;
        }
        /* A heartbeat cannot establish a new epoch; HELLO must be observed first. */
        const canview_status_t recovery_status = begin_session_recovery(context, now_ms, false);
        if (recovery_status != CANVIEW_OK)
        {
            return recovery_status;
        }
        return CANVIEW_TIMEOUT;
    }
    canview_uart_replay_context_t candidate;
    const canview_status_t admission = prepare_candidate(context, view, now_ms, &candidate);
    if (admission != CANVIEW_OK)
    {
        return admission;
    }
    bool boot_changed = false;
    const canview_status_t note_status = canview_uart_session_note_heartbeat(
        &context->link, &context->plan, &context->command_cache, &context->replay, peer_boot_id,
        peer_state_revision, now_ms, &boot_changed);
    if (note_status == CANVIEW_TIMEOUT || boot_changed)
    {
        const canview_status_t recovery_status = begin_session_recovery(context, now_ms, false);
        if (recovery_status != CANVIEW_OK)
        {
            return recovery_status;
        }
        return CANVIEW_TIMEOUT;
    }
    if (note_status != CANVIEW_OK)
    {
        return note_status;
    }
    if (peer_state_revision == context->local_safety_revision && context->link.hello_ack_complete)
    {
        const canview_status_t snapshot_status = enqueue_safety_snapshot(context, context->last_now_us);
        if (snapshot_status == CANVIEW_OK)
        {
            (void)canview_uart_link_note_safety_snapshot(&context->link, peer_boot_id,
                                                         context->local_safety_revision, now_ms);
        }
    }
    (void)canview_uart_link_tick(&context->link, now_ms);
    context->replay = candidate;
    return CANVIEW_OK;
}

static canview_status_t handle_time_sync(canview_stm_uart_context_t *context,
                                         const canview_uart_message_view_t *view,
                                         uint64_t now_ms, uint64_t now_us)
{
    const uint8_t *payload = view->wire.payload;
    if (!context->link.hello_complete || !context->link.hello_ack_complete)
    {
        increment_saturating(&context->stats.time_sync_rejected);
        return CANVIEW_TIMEOUT;
    }
    canview_uart_replay_context_t candidate;
    const canview_status_t admission = prepare_candidate(context, view, now_ms, &candidate);
    if (admission != CANVIEW_OK)
    {
        increment_saturating(&context->stats.time_sync_rejected);
        return admission;
    }
    const uint8_t phase = payload[28U];
    const uint64_t request_token = read_le(payload, 8U);
    const uint64_t controller_boot_id = read_le(payload + 8U, 8U);
    const uint64_t stm_boot_id = read_le(payload + 16U, 8U);
    const uint32_t generation = (uint32_t)read_le(payload + 24U, 4U);
    if (stm_boot_id != 0U && stm_boot_id != context->local_boot_id)
    {
        increment_saturating(&context->stats.time_sync_rejected);
        return CANVIEW_AUTH_FAILED;
    }
    if (phase == CANVIEW_UART_TIME_SYNC_REQUEST)
    {
        if (context->pending_sync_valid &&
            (context->pending_sync_token != request_token ||
             context->pending_sync_controller_boot_id != controller_boot_id ||
             context->pending_sync_generation != generation))
        {
            increment_saturating(&context->stats.time_sync_rejected);
            return CANVIEW_RESOURCE_BUSY;
        }
        if (!p1_available(context))
        {
            mark_queue_full(context, CANVIEW_STM_UART_TX_P1);
            increment_saturating(&context->stats.time_sync_rejected);
            return CANVIEW_RESOURCE_BUSY;
        }
        uint8_t response[UART_CONTROL_TIME_SYNC_SIZE] = {0};
        write_le(response, 8U, request_token);
        write_le(response + 8U, 8U, controller_boot_id);
        write_le(response + 16U, 8U, context->local_boot_id);
        write_le(response + 24U, 4U, generation);
        response[28U] = CANVIEW_UART_TIME_SYNC_RESPONSE;
        write_le(response + 32U, 8U, read_le(payload + 32U, 8U));
        write_le(response + 40U, 8U, now_us);
        write_le(response + 48U, 8U, now_us);
        const canview_status_t response_status = enqueue_validated(
            context, CANVIEW_STM_UART_TX_P1, CANVIEW_UART_MSG_CONTROL_TIME_SYNC,
            (uint8_t)(CANVIEW_UART_FLAG_RESPONSE | CANVIEW_UART_FLAG_HIGH_PRIORITY),
            view->wire.header.correlation_id, now_us, response, sizeof(response));
        if (response_status != CANVIEW_OK)
        {
            increment_saturating(&context->stats.time_sync_rejected);
            return response_status;
        }
        context->pending_sync_valid = true;
        context->pending_sync_token = request_token;
        context->pending_sync_controller_boot_id = controller_boot_id;
        context->pending_sync_generation = generation;
        context->pending_sync_t1_controller_us = read_le(payload + 32U, 8U);
        context->replay = candidate;
        increment_saturating(&context->stats.time_sync_requests);
        increment_saturating(&context->stats.time_sync_responses);
        return CANVIEW_OK;
    }
    if (phase == CANVIEW_UART_TIME_SYNC_COMMIT)
    {
        const uint64_t t1 = read_le(payload + 32U, 8U);
        const uint64_t t2 = read_le(payload + 40U, 8U);
        const uint64_t t3 = read_le(payload + 48U, 8U);
        const uint64_t t4 = read_le(payload + 56U, 8U);
        if (!context->pending_sync_valid || context->pending_sync_token != request_token ||
            context->pending_sync_controller_boot_id != controller_boot_id ||
            context->pending_sync_generation != generation ||
            context->pending_sync_t1_controller_us != t1 ||
            t1 == 0U || t2 == 0U || t3 == 0U || t4 == 0U || t2 < t1 || t3 < t2 || t4 < t3)
        {
            increment_saturating(&context->stats.time_sync_rejected);
            return CANVIEW_STALE;
        }
        int64_t first_leg = 0;
        int64_t second_leg = 0;
        int64_t round_trip = 0;
        int64_t stm_turnaround = 0;
        int64_t offset_sum = 0;
        if (!signed_difference(t2, t1, &first_leg) || !signed_difference(t3, t4, &second_leg) ||
            !signed_difference(t4, t1, &round_trip) || !signed_difference(t3, t2, &stm_turnaround) ||
            !signed_sum(first_leg, second_leg, &offset_sum) || round_trip < stm_turnaround ||
            round_trip - stm_turnaround > (int64_t)CANVIEW_STM_UART_TIME_SYNC_MAX_DELAY_US)
        {
            increment_saturating(&context->stats.time_sync_rejected);
            return CANVIEW_TIMEOUT;
        }
        const uint64_t computed_uncertainty =
            (uint64_t)(round_trip - stm_turnaround) / UINT64_C(2) + UINT64_C(1);
        const uint64_t reported_uncertainty = read_le(payload + 72U, 4U);
        const uint64_t uncertainty = computed_uncertainty > reported_uncertainty
                                         ? computed_uncertainty
                                         : reported_uncertainty;
        if (uncertainty > CANVIEW_UART_CONTROL_TIME_MAX_UNCERTAINTY_US)
        {
            increment_saturating(&context->stats.time_sync_rejected);
            return CANVIEW_TIMEOUT;
        }
        context->time_mapping.valid = true;
        context->time_mapping.generation = generation;
        context->time_mapping.uncertainty_us = (uint32_t)uncertainty;
        context->time_mapping.offset_stm_minus_controller_us = offset_sum / INT64_C(2);
        context->time_mapping.updated_ms = now_ms;
        context->time_mapping.controller_boot_id = controller_boot_id;
        context->time_mapping.stm_boot_id = context->local_boot_id;
        context->pending_sync_valid = false;
        context->pending_sync_token = 0U;
        context->pending_sync_controller_boot_id = 0U;
        context->pending_sync_generation = 0U;
        context->pending_sync_t1_controller_us = 0U;
        context->replay = candidate;
        increment_saturating(&context->stats.time_sync_commits);
        return CANVIEW_OK;
    }
    increment_saturating(&context->stats.time_sync_rejected);
    return CANVIEW_UNSUPPORTED_MESSAGE;
}

static canview_status_t command_key_from_view(const canview_uart_message_view_t *view,
                                              canview_uart_command_key_t *key)
{
    if (view == NULL || key == NULL || view->wire.payload == NULL || view->wire.payload_size <
            104U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const uint8_t *payload = view->wire.payload;
    memset(key, 0, sizeof(*key));
    key->origin_device_id = read_le(payload + 12U, 8U);
    key->origin_boot_id = read_le(payload + 20U, 8U);
    key->wireless_session_id = (uint32_t)read_le(payload + 28U, 4U);
    key->control_generation = (uint32_t)read_le(payload + 32U, 4U);
    key->request_token = read_le(payload, 8U);
    key->command_id = (uint16_t)read_le(payload + 8U, 2U);
    memcpy(key->canonical_argument_digest, payload + 56U, CANVIEW_UART_COMMAND_DIGEST_SIZE);
    return CANVIEW_OK;
}

static canview_status_t respond_to_duplicate_command(
    canview_stm_uart_context_t *context, const canview_uart_message_view_t *view,
    const canview_uart_command_key_t *key, uint64_t now_ms, uint64_t now_us)
{
    if (!context_ready(context) || view == NULL || key == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    canview_uart_command_handle_t cached_handle;
    const uint8_t *cached_result = NULL;
    size_t cached_result_size = 0U;
    const canview_status_t lookup_status = canview_uart_command_cache_lookup(
        &context->command_cache, key, now_ms, &cached_handle, &cached_result, &cached_result_size);
    if (lookup_status != CANVIEW_DUPLICATE)
    {
        return lookup_status;
    }
    if (cached_result != NULL && cached_result_size != 0U)
    {
        return enqueue_validated(context, CANVIEW_STM_UART_TX_P0, CANVIEW_UART_MSG_COMMAND_RESULT,
                                 CANVIEW_UART_FLAG_RESPONSE, view->wire.header.correlation_id,
                                 now_us, cached_result, cached_result_size);
    }
    return enqueue_ack(context, view->wire.header.sequence, view->wire.header.correlation_id,
                       key->request_token, CANVIEW_DUPLICATE,
                       CANVIEW_STM_UART_REASON_DUPLICATE, now_ms, now_us);
}

static canview_status_t report_command_conflict(canview_stm_uart_context_t *context,
                                                const canview_uart_message_view_t *view,
                                                uint64_t now_us)
{
    if (!context_ready(context) || view == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    increment_saturating(&context->stats.commands_conflict);
    const canview_status_t error_status = enqueue_error(
        context, CANVIEW_UART_MSG_COMMAND_REQUEST, view->wire.header.sequence,
        view->wire.header.correlation_id, CANVIEW_MALFORMED,
        CANVIEW_STM_UART_REASON_TOKEN_CONFLICT, now_us);
    return error_status == CANVIEW_OK ? CANVIEW_MALFORMED : error_status;
}

static canview_status_t respond_to_duplicate_ack(canview_stm_uart_context_t *context,
                                                 const canview_uart_message_view_t *view,
                                                 size_t token_offset, uint64_t now_ms,
                                                 uint64_t now_us)
{
    if (!context_ready(context) || view == NULL || view->wire.payload == NULL ||
        token_offset > view->wire.payload_size || 8U > view->wire.payload_size - token_offset)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    return enqueue_ack(context, view->wire.header.sequence, view->wire.header.correlation_id,
                       read_le(view->wire.payload + token_offset, 8U), CANVIEW_DUPLICATE,
                       CANVIEW_STM_UART_REASON_DUPLICATE, now_ms, now_us);
}

static canview_status_t handle_command(canview_stm_uart_context_t *context,
                                       const canview_uart_message_view_t *view, uint64_t now_ms,
                                       uint64_t now_us)
{
    if (!canview_uart_link_command_admission_allowed(&context->link, now_ms))
    {
        (void)enqueue_error(context, CANVIEW_UART_MSG_COMMAND_REQUEST, view->wire.header.sequence,
                            view->wire.header.correlation_id, CANVIEW_TIMEOUT,
                            CANVIEW_STM_UART_REASON_LINK_OFFLINE, now_us);
        return CANVIEW_TIMEOUT;
    }
    canview_uart_replay_context_t candidate;
    const canview_status_t admission = prepare_candidate(context, view, now_ms, &candidate);
    if (admission != CANVIEW_OK)
    {
        if (admission == CANVIEW_AUTH_FAILED)
        {
            canview_uart_replay_context_t duplicate_candidate = context->replay;
            const canview_status_t sequence_status = canview_sequence_window_accept(
                &duplicate_candidate.sequence, view->wire.header.sequence);
            if (sequence_status == CANVIEW_DUPLICATE)
            {
                canview_uart_command_key_t duplicate_key;
                const canview_status_t key_status = command_key_from_view(view, &duplicate_key);
                if (key_status != CANVIEW_OK)
                {
                    return key_status;
                }
                const canview_status_t response_status = respond_to_duplicate_command(
                    context, view, &duplicate_key, now_ms, now_us);
                if (response_status == CANVIEW_OK)
                {
                    context->replay = duplicate_candidate;
                    increment_saturating(&context->stats.commands_duplicate);
                    return CANVIEW_DUPLICATE;
                }
                if (response_status == CANVIEW_RESOURCE_BUSY)
                {
                    return response_status;
                }
                if (response_status == CANVIEW_MALFORMED)
                {
                    return report_command_conflict(context, view, now_us);
                }
            }
        }
        if (admission == CANVIEW_DUPLICATE)
        {
            canview_uart_command_key_t duplicate_key;
            const canview_status_t key_status = command_key_from_view(view, &duplicate_key);
            if (key_status != CANVIEW_OK)
            {
                return key_status;
            }
            const canview_status_t response_status = respond_to_duplicate_command(
                context, view, &duplicate_key, now_ms, now_us);
            if (response_status == CANVIEW_MALFORMED)
            {
                return report_command_conflict(context, view, now_us);
            }
            if (response_status != CANVIEW_OK)
            {
                return response_status;
            }
            context->replay = candidate;
            increment_saturating(&context->stats.commands_duplicate);
            return CANVIEW_DUPLICATE;
        }
        if (admission != CANVIEW_DUPLICATE && admission != CANVIEW_STALE)
        {
            (void)enqueue_error(context, CANVIEW_UART_MSG_COMMAND_REQUEST,
                                view->wire.header.sequence, view->wire.header.correlation_id,
                                admission, reason_for_status(admission), now_us);
        }
        if (admission == CANVIEW_AUTH_FAILED)
        {
            increment_saturating(&context->stats.commands_authorization_rejected);
        }
        return admission;
    }
    canview_uart_command_key_t key;
    const canview_status_t key_status = command_key_from_view(view, &key);
    if (key_status != CANVIEW_OK)
    {
        return key_status;
    }
    canview_uart_command_handle_t cached_handle;
    const uint8_t *cached_result = NULL;
    size_t cached_result_size = 0U;
    const canview_status_t lookup_status = canview_uart_command_cache_lookup(
        &context->command_cache, &key, now_ms, &cached_handle, &cached_result, &cached_result_size);
    if (lookup_status == CANVIEW_DUPLICATE)
    {
        canview_status_t response_status;
        if (cached_result != NULL && cached_result_size != 0U)
        {
            response_status = enqueue_validated(
                context, CANVIEW_STM_UART_TX_P0, CANVIEW_UART_MSG_COMMAND_RESULT,
                CANVIEW_UART_FLAG_RESPONSE, view->wire.header.correlation_id, now_us, cached_result,
                cached_result_size);
        }
        else
        {
            response_status = enqueue_ack(context, view->wire.header.sequence,
                                          view->wire.header.correlation_id, key.request_token,
                                          CANVIEW_DUPLICATE, CANVIEW_STM_UART_REASON_DUPLICATE,
                                          now_ms, now_us);
        }
        if (response_status != CANVIEW_OK)
        {
            return response_status;
        }
        context->replay = candidate;
        increment_saturating(&context->stats.commands_duplicate);
        return CANVIEW_DUPLICATE;
    }
    if (lookup_status == CANVIEW_MALFORMED)
    {
        increment_saturating(&context->stats.commands_conflict);
        (void)enqueue_error(context, CANVIEW_UART_MSG_COMMAND_REQUEST,
                            view->wire.header.sequence, view->wire.header.correlation_id,
                            CANVIEW_MALFORMED, CANVIEW_STM_UART_REASON_TOKEN_CONFLICT, now_us);
        return CANVIEW_MALFORMED;
    }
    if (lookup_status == CANVIEW_TIMEOUT)
    {
        increment_saturating(&context->stats.commands_expired);
        (void)enqueue_error(context, CANVIEW_UART_MSG_COMMAND_REQUEST,
                            view->wire.header.sequence, view->wire.header.correlation_id,
                            CANVIEW_TIMEOUT, CANVIEW_STM_UART_REASON_TIMEOUT, now_us);
        return CANVIEW_TIMEOUT;
    }
    if (lookup_status != CANVIEW_STALE)
    {
        return lookup_status;
    }
    if (!p0_available(context) || context->pending_count >= CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY)
    {
        if (p0_available(context))
        {
            (void)enqueue_ack(context, view->wire.header.sequence, view->wire.header.correlation_id,
                              key.request_token, CANVIEW_RESOURCE_BUSY,
                              CANVIEW_STM_UART_REASON_QUEUE_FULL, now_ms, now_us);
        }
        else
        {
            mark_queue_full(context, CANVIEW_STM_UART_TX_P0);
        }
        return CANVIEW_RESOURCE_BUSY;
    }
    canview_uart_command_handle_t handle;
    const canview_status_t cache_status = canview_uart_command_cache_admit(
        &context->command_cache, &key, (uint16_t)read_le(view->wire.payload + 10U, 2U), now_ms,
        &handle);
    if (cache_status == CANVIEW_RESOURCE_BUSY)
    {
        increment_saturating(&context->stats.commands_cache_busy);
        (void)enqueue_ack(context, view->wire.header.sequence, view->wire.header.correlation_id,
                          key.request_token, CANVIEW_RESOURCE_BUSY,
                          CANVIEW_STM_UART_REASON_CACHE_FULL, now_ms, now_us);
        return cache_status;
    }
    if (cache_status != CANVIEW_OK)
    {
        if (cache_status == CANVIEW_MALFORMED)
        {
            increment_saturating(&context->stats.commands_conflict);
        }
        return cache_status;
    }
    const size_t pending_index = pending_tail_index(context);
    canview_stm_uart_pending_command_t *pending = &context->pending[pending_index];
    memset(pending, 0, sizeof(*pending));
    pending->valid = true;
    pending->payload_size = (uint16_t)view->wire.payload_size;
    pending->request_sequence = view->wire.header.sequence;
    pending->correlation_id = view->wire.header.correlation_id;
    pending->cache_handle = handle;
    pending->key = key;
    memcpy(pending->payload, view->wire.payload, view->wire.payload_size);
    ++context->pending_count;
    const canview_status_t cache_ack_status =
        canview_uart_command_cache_mark_ack(&context->command_cache, &handle, now_ms);
    if (cache_ack_status != CANVIEW_OK)
    {
        return cache_ack_status;
    }
    const canview_status_t ack_status =
        enqueue_ack(context, view->wire.header.sequence, view->wire.header.correlation_id,
                    key.request_token, CANVIEW_OK, CANVIEW_STM_UART_REASON_NONE, now_ms, now_us);
    if (ack_status != CANVIEW_OK)
    {
        context->safety_inhibited = true;
        increment_saturating(&context->stats.tx_safety_inhibited);
        return ack_status;
    }
    context->replay = candidate;
    increment_saturating(&context->stats.commands_admitted);
    return CANVIEW_OK;
}

static canview_status_t handle_lease(canview_stm_uart_context_t *context,
                                     const canview_uart_message_view_t *view, uint64_t now_ms,
                                     uint64_t now_us)
{
    canview_uart_replay_context_t candidate;
    const canview_status_t admission = prepare_candidate(context, view, now_ms, &candidate);
    if (admission != CANVIEW_OK)
    {
        if (admission == CANVIEW_DUPLICATE)
        {
            const canview_status_t duplicate_status =
                respond_to_duplicate_ack(context, view, 0U, now_ms, now_us);
            return duplicate_status == CANVIEW_OK ? CANVIEW_DUPLICATE : duplicate_status;
        }
        if (admission != CANVIEW_DUPLICATE && admission != CANVIEW_STALE)
        {
            (void)enqueue_error(context, CANVIEW_UART_MSG_CONTROL_LEASE,
                                view->wire.header.sequence, view->wire.header.correlation_id,
                                admission, reason_for_status(admission), now_us);
        }
        return admission;
    }
    if (!p0_available(context))
    {
        mark_queue_full(context, CANVIEW_STM_UART_TX_P0);
        return CANVIEW_RESOURCE_BUSY;
    }
    const uint8_t *payload = view->wire.payload;
    const uint8_t action = payload[8U];
    const uint64_t request_token = read_le(payload, 8U);
    const uint64_t lease_id = read_le(payload + 16U, 8U);
    const uint16_t scope = (uint16_t)read_le(payload + 24U, 2U);
    const uint32_t generation = (uint32_t)read_le(payload + 32U, 4U);
    const uint32_t requested_ms = (uint32_t)read_le(payload + 12U, 4U);
    canview_status_t result = CANVIEW_OK;
    uint16_t reason = CANVIEW_STM_UART_REASON_NONE;
    if (action == UART_LEASE_RELEASE)
    {
        if (!context->lease.valid || context->lease.lease_id != lease_id)
        {
            result = CANVIEW_STALE;
            reason = CANVIEW_STM_UART_REASON_TOKEN_CONFLICT;
        }
        else
        {
            memset(&context->lease, 0, sizeof(context->lease));
        }
    }
    else if (action == UART_LEASE_ACQUIRE || action == UART_LEASE_RENEW)
    {
#if defined(CANVIEW_STM_CAPTURE_ONLY_CONTRACT)
        (void)requested_ms;
        (void)generation;
        (void)scope;
        (void)lease_id;
        result = CANVIEW_NOT_IMPLEMENTED;
        reason = CANVIEW_STM_UART_REASON_NOT_READY;
#else
        if (requested_ms == 0U || requested_ms > UART_LEASE_MAX_MS || lease_id == 0U ||
            scope == 0U || generation == 0U ||
            (action == UART_LEASE_RENEW &&
             (!context->lease.valid || context->lease.lease_id != lease_id)))
        {
            result = CANVIEW_MALFORMED;
            reason = CANVIEW_STM_UART_REASON_INVALID;
        }
        else if (now_ms > UINT64_MAX - requested_ms)
        {
            result = CANVIEW_TIMEOUT;
            reason = CANVIEW_STM_UART_REASON_TIMEOUT;
        }
        else
        {
            context->lease.valid = true;
            context->lease.lease_id = lease_id;
            context->lease.scope = scope;
            context->lease.control_generation = generation;
            context->lease.expires_at_ms = now_ms + requested_ms;
            memcpy(context->lease.control_tag, payload + 36U, CANVIEW_UART_CONTROL_TAG_SIZE);
        }
#endif
    }
    else
    {
        result = CANVIEW_MALFORMED;
        reason = CANVIEW_STM_UART_REASON_INVALID;
    }
    const canview_status_t ack_status = enqueue_ack(
        context, view->wire.header.sequence, view->wire.header.correlation_id, request_token,
        result, reason, now_ms, now_us);
    if (ack_status != CANVIEW_OK)
    {
        return ack_status;
    }
    context->replay = candidate;
    return result;
}

static canview_status_t handle_plan(canview_stm_uart_context_t *context,
                                    const canview_uart_message_view_t *view, uint64_t now_ms,
                                    uint64_t now_us)
{
    canview_uart_replay_context_t candidate;
    const canview_status_t admission = prepare_candidate(context, view, now_ms, &candidate);
    if (admission != CANVIEW_OK)
    {
        if (admission == CANVIEW_DUPLICATE)
        {
            const canview_status_t duplicate_status =
                respond_to_duplicate_ack(context, view, 4U, now_ms, now_us);
            return duplicate_status == CANVIEW_OK ? CANVIEW_DUPLICATE : duplicate_status;
        }
        if (admission != CANVIEW_DUPLICATE && admission != CANVIEW_STALE)
        {
            (void)enqueue_error(context, CANVIEW_UART_MSG_CAN_OBSERVER_PLAN,
                                view->wire.header.sequence, view->wire.header.correlation_id,
                                admission, reason_for_status(admission), now_us);
        }
        return admission;
    }
    if (!p0_available(context))
    {
        mark_queue_full(context, CANVIEW_STM_UART_TX_P0);
        return CANVIEW_RESOURCE_BUSY;
    }
    const canview_status_t plan_status = canview_uart_plan_apply(
        &context->plan, view->wire.payload, view->wire.payload_size, now_ms);
    if (plan_status != CANVIEW_OK && plan_status != CANVIEW_INCOMPLETE)
    {
        (void)enqueue_error(context, CANVIEW_UART_MSG_CAN_OBSERVER_PLAN,
                            view->wire.header.sequence, view->wire.header.correlation_id,
                            plan_status, reason_for_status(plan_status), now_us);
        return plan_status;
    }
    const uint64_t request_token = read_le(view->wire.payload + 4U, 8U);
    const canview_status_t ack_status = enqueue_ack(
        context, view->wire.header.sequence, view->wire.header.correlation_id, request_token,
        plan_status, CANVIEW_STM_UART_REASON_NONE, now_ms, now_us);
    if (ack_status != CANVIEW_OK)
    {
        return ack_status;
    }
    context->replay = candidate;
    return plan_status;
}

static canview_status_t handle_unsupported_control(canview_stm_uart_context_t *context,
                                                   const canview_uart_message_view_t *view,
                                                   uint64_t now_ms, uint64_t now_us)
{
    canview_uart_replay_context_t candidate;
    const canview_status_t admission = prepare_candidate(context, view, now_ms, &candidate);
    if (admission != CANVIEW_OK)
    {
        if (admission == CANVIEW_DUPLICATE)
        {
            const canview_status_t duplicate_status = enqueue_error(
                context, view->wire.header.message_type, view->wire.header.sequence,
                view->wire.header.correlation_id, CANVIEW_NOT_IMPLEMENTED,
                CANVIEW_STM_UART_REASON_UNSUPPORTED, now_us);
            return duplicate_status == CANVIEW_OK ? CANVIEW_DUPLICATE : duplicate_status;
        }
        if (admission != CANVIEW_DUPLICATE && admission != CANVIEW_STALE)
        {
            (void)enqueue_error(context, view->wire.header.message_type,
                                view->wire.header.sequence, view->wire.header.correlation_id,
                                admission, reason_for_status(admission), now_us);
        }
        return admission;
    }
    const canview_status_t error_status = enqueue_error(
        context, view->wire.header.message_type, view->wire.header.sequence,
        view->wire.header.correlation_id, CANVIEW_NOT_IMPLEMENTED,
        CANVIEW_STM_UART_REASON_UNSUPPORTED, now_us);
    if (error_status != CANVIEW_OK)
    {
        return error_status;
    }
    context->replay = candidate;
    return CANVIEW_NOT_IMPLEMENTED;
}

static canview_status_t handle_simple_message(canview_stm_uart_context_t *context,
                                              const canview_uart_message_view_t *view,
                                              uint64_t now_ms)
{
    canview_uart_replay_context_t candidate;
    const canview_status_t admission = prepare_candidate(context, view, now_ms, &candidate);
    if (admission == CANVIEW_OK)
    {
        context->replay = candidate;
    }
    return admission;
}

static canview_status_t dispatch_message(canview_stm_uart_context_t *context,
                                         const canview_uart_message_view_t *view,
                                         uint64_t now_ms, uint64_t now_us)
{
    if (view == NULL || view->policy == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    switch (view->wire.header.message_type)
    {
    case CANVIEW_UART_MSG_LINK_HELLO:
        return handle_hello(context, view, now_ms);
    case CANVIEW_UART_MSG_LINK_HELLO_ACK:
        return handle_hello_ack(context, view, now_ms);
    case CANVIEW_UART_MSG_HEARTBEAT:
        return handle_heartbeat(context, view, now_ms);
    case CANVIEW_UART_MSG_CONTROL_TIME_SYNC:
        return handle_time_sync(context, view, now_ms, now_us);
    case CANVIEW_UART_MSG_COMMAND_REQUEST:
        return handle_command(context, view, now_ms, now_us);
    case CANVIEW_UART_MSG_CONTROL_LEASE:
        return handle_lease(context, view, now_ms, now_us);
    case CANVIEW_UART_MSG_CAN_OBSERVER_PLAN:
        return handle_plan(context, view, now_ms, now_us);
    case CANVIEW_UART_MSG_CAN_CAPTURE_CONTROL:
    case CANVIEW_UART_MSG_CAN_EVENT_MARKER:
    case CANVIEW_UART_MSG_CONFIG_GET:
    case CANVIEW_UART_MSG_CONFIG_SET:
        return handle_unsupported_control(context, view, now_ms, now_us);
    default:
        return handle_simple_message(context, view, now_ms);
    }
}

static void account_ingest_status(canview_stm_uart_context_t *context, canview_status_t status)
{
    switch (status)
    {
    case CANVIEW_INCOMPLETE:
        increment_saturating(&context->stats.rx_incomplete);
        break;
    case CANVIEW_MALFORMED:
        increment_saturating(&context->stats.rx_malformed);
        increment_saturating(&context->stats.protocol_errors);
        break;
    case CANVIEW_UNSUPPORTED_MESSAGE:
        increment_saturating(&context->stats.rx_unsupported);
        increment_saturating(&context->stats.protocol_errors);
        break;
    case CANVIEW_CRC_MISMATCH:
        increment_saturating(&context->stats.rx_crc_failures);
        increment_saturating(&context->stats.protocol_errors);
        break;
    case CANVIEW_OVERSIZE:
        increment_saturating(&context->stats.rx_oversize);
        increment_saturating(&context->stats.protocol_errors);
        break;
    default:
        break;
    }
}

canview_status_t canview_stm_uart_init(canview_stm_uart_context_t *context,
                                       const canview_stm_uart_config_t *config,
                                       uint64_t now_ms, uint64_t now_us)
{
    if (context == NULL || config == NULL || config->local_boot_id == 0U ||
        config->local_device_id == 0U || config->local_safety_revision == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(context, 0, sizeof(*context));
    context->local_boot_id = config->local_boot_id;
    context->local_device_id = config->local_device_id;
    context->local_safety_revision = config->local_safety_revision;
    context->authorize = config->authorize;
    context->authorize_context = config->authorize_context;
    context->next_tx_sequence = 1U;
    context->last_now_ms = now_ms;
    context->last_now_us = now_us;
    context->last_heartbeat_tx_ms = now_ms;
    context->initialized = true;
    canview_status_t status = canview_uart_codec_reset(
        &context->codec, CANVIEW_UART_ENDPOINT_STM32, CANVIEW_UART_FLOW_INBOUND);
    if (status == CANVIEW_OK)
    {
        status = canview_uart_link_reset(&context->link);
    }
    if (status == CANVIEW_OK)
    {
        status = canview_uart_plan_reset(&context->plan);
    }
    if (status == CANVIEW_OK)
    {
        status = canview_uart_command_cache_reset(&context->command_cache);
    }
    if (status == CANVIEW_OK)
    {
        status = canview_uart_replay_reset(&context->replay);
    }
    if (status != CANVIEW_OK)
    {
        context->initialized = false;
        return status;
    }
    return enqueue_hello(context);
}

canview_status_t canview_stm_uart_reset(canview_stm_uart_context_t *context,
                                        uint64_t now_ms, uint64_t now_us)
{
    if (!context_ready(context))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (reject_reentry(context))
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    const uint64_t local_boot_id = context->local_boot_id;
    const uint64_t local_device_id = context->local_device_id;
    const uint32_t local_safety_revision = context->local_safety_revision;
    canview_stm_uart_authorize_fn *authorize = context->authorize;
    void *authorize_context = context->authorize_context;
    const canview_status_t session_status = canview_uart_session_reset(
        &context->link, &context->plan, &context->command_cache, &context->replay);
    if (session_status != CANVIEW_OK)
    {
        return session_status;
    }
    memset(&context->codec, 0, sizeof(context->codec));
    memset(&context->time_mapping, 0, sizeof(context->time_mapping));
    memset(&context->lease, 0, sizeof(context->lease));
    clear_pending_slots(context);
    if (context->tx_current.valid)
    {
        increment_saturating(&context->stats.tx_dropped);
    }
    memset(context->tx_queues, 0, sizeof(context->tx_queues));
    memset(&context->tx_current, 0, sizeof(context->tx_current));
    memset(context->tx_serial, 0, sizeof(context->tx_serial));
    memset(context->tx_scratch, 0, sizeof(context->tx_scratch));
    context->tx_current_class = CANVIEW_STM_UART_TX_P0;
    context->tx_serial_size = 0U;
    context->pending_sync_valid = false;
    context->pending_sync_token = 0U;
    context->pending_sync_controller_boot_id = 0U;
    context->pending_sync_generation = 0U;
    context->pending_sync_t1_controller_us = 0U;
    context->local_boot_id = local_boot_id;
    context->local_device_id = local_device_id;
    context->local_safety_revision = local_safety_revision;
    context->authorize = authorize;
    context->authorize_context = authorize_context;
    increment_saturating(&context->stats.link_resets);
    context->next_tx_sequence = 1U;
    context->last_now_ms = now_ms;
    context->last_now_us = now_us;
    context->last_heartbeat_tx_ms = now_ms;
    context->error_window_started_ms = now_ms;
    context->error_responses_in_window = 0U;
    context->cts_blocked = false;
    context->cts_known = false;
    context->servicing = false;
    const canview_status_t codec_status = canview_uart_codec_reset(
        &context->codec, CANVIEW_UART_ENDPOINT_STM32, CANVIEW_UART_FLOW_INBOUND);
    if (codec_status != CANVIEW_OK)
    {
        return codec_status;
    }
    return enqueue_hello(context);
}

canview_status_t canview_stm_uart_ingest_byte(canview_stm_uart_context_t *context, uint8_t byte,
                                              uint64_t now_ms, uint64_t now_us)
{
    if (!context_ready(context))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (context->servicing)
    {
        increment_saturating(&context->stats.callback_reentry);
        return CANVIEW_RESOURCE_BUSY;
    }
    context->servicing = true;
    context->last_now_ms = now_ms;
    context->last_now_us = now_us;
    increment_saturating(&context->stats.rx_bytes);
    canview_uart_message_view_t view;
    const canview_status_t codec_status = canview_uart_codec_feed(&context->codec, byte, &view);
    account_ingest_status(context, codec_status);
    canview_status_t result = codec_status;
    if (codec_status == CANVIEW_OK)
    {
        increment_saturating(&context->stats.rx_frames);
        result = dispatch_message(context, &view, now_ms, now_us);
        if (result != CANVIEW_OK && result != CANVIEW_INCOMPLETE && result != CANVIEW_DUPLICATE &&
            result != CANVIEW_STALE && result != CANVIEW_RESOURCE_BUSY &&
            result != CANVIEW_TIMEOUT && result != CANVIEW_AUTH_FAILED &&
            result != CANVIEW_NOT_IMPLEMENTED)
        {
            increment_saturating(&context->stats.protocol_errors);
        }
    }
    context->servicing = false;
    return result;
}

canview_status_t canview_stm_uart_set_cts_blocked(canview_stm_uart_context_t *context,
                                                  bool blocked, uint64_t now_ms)
{
    if (!context_ready(context))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (reject_reentry(context))
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    const bool was_blocked = context->cts_blocked;
    context->cts_blocked = blocked;
    context->cts_known = true;
    if (blocked && !was_blocked)
    {
        increment_saturating(&context->stats.cts_stalls);
    }
    const canview_status_t status =
        canview_uart_link_set_cts_blocked(&context->link, blocked, now_ms);
    if (status == CANVIEW_TIMEOUT)
    {
        const canview_status_t recovery_status = begin_session_recovery(context, now_ms, true);
        if (recovery_status != CANVIEW_OK)
        {
            return recovery_status;
        }
        return status;
    }
    if (status != CANVIEW_OK)
    {
        return status;
    }
    if (context->lease.valid && context->link.state != CANVIEW_UART_LINK_ONLINE)
    {
        memset(&context->lease, 0, sizeof(context->lease));
    }
    return CANVIEW_OK;
}

canview_status_t canview_stm_uart_enqueue(canview_stm_uart_context_t *context,
                                          canview_stm_uart_tx_class_t queue_class,
                                          uint8_t message_type, uint8_t flags,
                                          uint32_t correlation_id, uint64_t sender_time_us,
                                          const uint8_t *payload, size_t payload_size)
{
    if (!context_ready(context) || !queue_class_valid(queue_class) ||
        payload_size > CANVIEW_STM_UART_COMMAND_PAYLOAD_MAX ||
        (payload == NULL && payload_size != 0U))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (reject_reentry(context))
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    canview_wire_view_t wire = {0};
    wire.header.message_type = message_type;
    wire.header.flags = flags;
    wire.header.correlation_id = correlation_id;
    wire.header.sender_time = sender_time_us;
    wire.payload = payload;
    wire.payload_size = payload_size;
    canview_uart_message_view_t view;
    const canview_status_t validation = canview_uart_message_validate_for_endpoint(
        &wire, CANVIEW_UART_ENDPOINT_STM32, CANVIEW_UART_FLOW_OUTBOUND, &view);
    if (validation != CANVIEW_OK)
    {
        return validation;
    }
    return enqueue_validated(context, queue_class, message_type, flags, correlation_id,
                             sender_time_us, payload, payload_size);
}

canview_status_t canview_stm_uart_tx_begin(canview_stm_uart_context_t *context,
                                           const uint8_t **data, size_t *size,
                                           uint32_t *sequence)
{
    if (!context_ready(context) || data == NULL || size == NULL || sequence == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *data = NULL;
    *size = 0U;
    *sequence = 0U;
    if (reject_reentry(context))
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    if (context->tx_current.valid)
    {
        if (context->tx_serial_size == 0U)
        {
            return CANVIEW_MALFORMED;
        }
        *data = context->tx_serial;
        *size = context->tx_serial_size;
        *sequence = context->tx_current.sequence;
        return CANVIEW_OK;
    }
    for (size_t class_index = 0U; class_index < CANVIEW_STM_UART_TX_CLASS_COUNT; ++class_index)
    {
        canview_stm_uart_tx_queue_t *queue = &context->tx_queues[class_index];
        if (queue->count == 0U)
        {
            continue;
        }
        context->tx_current = queue->items[queue->head];
        context->tx_current_class = (canview_stm_uart_tx_class_t)class_index;
        memset(&queue->items[queue->head], 0, sizeof(queue->items[queue->head]));
        queue->head = (uint8_t)((queue->head + 1U) % CANVIEW_STM_UART_TX_QUEUE_CAPACITY);
        --queue->count;
        size_t written = 0U;
        const canview_status_t encode_status = canview_uart_message_encode(
            context->tx_current.message_type, context->tx_current.flags,
            context->tx_current.sequence, context->tx_current.correlation_id,
            context->tx_current.sender_time_us, context->tx_current.payload,
            context->tx_current.payload_size, CANVIEW_UART_ENDPOINT_STM32,
            CANVIEW_UART_FLOW_OUTBOUND, context->tx_scratch, sizeof(context->tx_scratch),
            context->tx_serial, sizeof(context->tx_serial), &written);
        if (encode_status == CANVIEW_OK)
        {
            context->tx_serial_size = (uint16_t)written;
            *data = context->tx_serial;
            *size = context->tx_serial_size;
            *sequence = context->tx_current.sequence;
            return CANVIEW_OK;
        }
        increment_saturating(&context->stats.tx_dropped);
        if (context->tx_current_class == CANVIEW_STM_UART_TX_RAW)
        {
            increment_saturating(&context->stats.tx_raw_dropped);
        }
        if (context->tx_current_class == CANVIEW_STM_UART_TX_P0 ||
            context->tx_current_class == CANVIEW_STM_UART_TX_P1)
        {
            context->safety_inhibited = true;
            increment_saturating(&context->stats.tx_safety_inhibited);
        }
        memset(&context->tx_current, 0, sizeof(context->tx_current));
        context->tx_serial_size = 0U;
    }
    return CANVIEW_INCOMPLETE;
}

canview_status_t canview_stm_uart_tx_finish(canview_stm_uart_context_t *context,
                                            canview_status_t transfer_status, uint64_t now_ms)
{
    if (!context_ready(context) || !context->tx_current.valid)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (reject_reentry(context))
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    context->last_now_ms = now_ms;
    if (transfer_status == CANVIEW_RESOURCE_BUSY)
    {
        return transfer_status;
    }
    if (transfer_status == CANVIEW_OK)
    {
        increment_saturating(&context->stats.tx_completed);
    }
    else
    {
        increment_saturating(&context->stats.tx_dropped);
        if (context->tx_current_class == CANVIEW_STM_UART_TX_RAW)
        {
            increment_saturating(&context->stats.tx_raw_dropped);
        }
        if (context->tx_current_class == CANVIEW_STM_UART_TX_P0 ||
            context->tx_current_class == CANVIEW_STM_UART_TX_P1)
        {
            context->safety_inhibited = true;
            increment_saturating(&context->stats.tx_safety_inhibited);
        }
    }
    memset(&context->tx_current, 0, sizeof(context->tx_current));
    context->tx_serial_size = 0U;
    return transfer_status;
}

canview_status_t canview_stm_uart_pending_peek(const canview_stm_uart_context_t *context,
                                               canview_stm_uart_pending_command_t *command)
{
    if (!context_ready(context) || command == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    memset(command, 0, sizeof(*command));
    if (context->pending_count == 0U)
    {
        return CANVIEW_INCOMPLETE;
    }
    *command = context->pending[context->pending_head];
    return CANVIEW_OK;
}

canview_status_t canview_stm_uart_pending_complete(canview_stm_uart_context_t *context,
                                                   const canview_uart_command_handle_t *handle,
                                                   uint8_t stage, uint16_t reason,
                                                   uint32_t state_revision,
                                                   uint32_t feedback_revision,
                                                   uint32_t feedback_time_ms, uint64_t now_ms,
                                                   uint64_t now_us)
{
    if (!context_ready(context))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (reject_reentry(context))
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    return complete_pending_internal(context, handle, stage, reason, state_revision,
                                     feedback_revision, feedback_time_ms, now_ms, now_us);
}

static bool pending_is_expired(const canview_stm_uart_context_t *context,
                               const canview_stm_uart_pending_command_t *pending,
                               uint64_t now_ms, uint64_t now_us)
{
    if (!context_ready(context) || pending == NULL || !pending->valid)
    {
        return false;
    }
    if (!context->time_mapping.valid ||
        elapsed_ms(now_ms, context->time_mapping.updated_ms) >=
            CANVIEW_UART_CONTROL_TIME_MAX_AGE_MS)
    {
        return true;
    }
    uint64_t issued_stm_us = 0U;
    uint64_t deadline_stm_us = 0U;
    if (!command_deadline(context, pending->payload, &issued_stm_us, &deadline_stm_us) ||
        now_us < issued_stm_us)
    {
        return false;
    }
    const uint64_t uncertainty = context->time_mapping.uncertainty_us;
    return deadline_stm_us <= uncertainty || now_us >= deadline_stm_us - uncertainty;
}

canview_status_t canview_stm_uart_tick(canview_stm_uart_context_t *context, uint64_t now_ms,
                                       uint64_t now_us)
{
    if (!context_ready(context))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (context->servicing)
    {
        increment_saturating(&context->stats.callback_reentry);
        return CANVIEW_RESOURCE_BUSY;
    }
    context->servicing = true;
    context->last_now_ms = now_ms;
    context->last_now_us = now_us;
    const bool had_session = context->link.peer_boot_id != 0U || context->link.hello_complete;
    const canview_status_t session_status = canview_uart_session_tick(
        &context->link, &context->plan, &context->command_cache, &context->replay, now_ms);
    if (session_status != CANVIEW_OK)
    {
        context->servicing = false;
        return session_status;
    }
    if (had_session && !context->link.hello_complete)
    {
        const canview_status_t recovery_status = begin_session_recovery(context, now_ms, true);
        if (recovery_status != CANVIEW_OK)
        {
            context->servicing = false;
            return recovery_status;
        }
    }
    if (context->link.state != CANVIEW_UART_LINK_ONLINE && context->lease.valid)
    {
        memset(&context->lease, 0, sizeof(context->lease));
    }
    if (context->time_mapping.valid &&
        elapsed_ms(now_ms, context->time_mapping.updated_ms) >=
            CANVIEW_UART_CONTROL_TIME_MAX_AGE_MS)
    {
        memset(&context->time_mapping, 0, sizeof(context->time_mapping));
    }
    if (context->lease.valid && now_ms >= context->lease.expires_at_ms)
    {
        memset(&context->lease, 0, sizeof(context->lease));
    }
    const canview_status_t cache_status =
        canview_uart_command_cache_expire(&context->command_cache, now_ms);
    if (cache_status != CANVIEW_OK)
    {
        context->servicing = false;
        return cache_status;
    }
    for (size_t index = 0U; index < CANVIEW_STM_UART_PENDING_COMMAND_CAPACITY &&
                              context->pending_count != 0U;
         ++index)
    {
        const canview_stm_uart_pending_command_t pending = context->pending[context->pending_head];
        if (!pending_is_expired(context, &pending, now_ms, now_us))
        {
            break;
        }
        const canview_status_t expired_status = complete_pending_internal(
            context, &pending.cache_handle, CANVIEW_STM_UART_RESULT_STAGE_EXPIRED,
            CANVIEW_STM_UART_REASON_TIMEOUT, context->local_safety_revision,
            context->local_safety_revision, (uint32_t)now_ms, now_ms, now_us);
        if (expired_status != CANVIEW_OK)
        {
            break;
        }
    }
    if (elapsed_ms(now_ms, context->last_heartbeat_tx_ms) >=
        CANVIEW_STM_UART_HEARTBEAT_PERIOD_MS)
    {
        const canview_status_t heartbeat_status = enqueue_heartbeat(context, now_us, now_ms);
        if (heartbeat_status == CANVIEW_OK)
        {
            context->last_heartbeat_tx_ms = now_ms;
        }
    }
    if (!context->link.hello_complete)
    {
        (void)enqueue_hello_if_missing(context);
    }
    context->servicing = false;
    return CANVIEW_OK;
}

canview_status_t canview_stm_uart_get_stats(const canview_stm_uart_context_t *context,
                                            canview_stm_uart_stats_t *stats)
{
    if (!context_ready(context) || stats == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *stats = context->stats;
    return CANVIEW_OK;
}

canview_status_t canview_stm_uart_get_link(const canview_stm_uart_context_t *context,
                                           canview_uart_link_t *link)
{
    if (!context_ready(context) || link == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *link = context->link;
    return CANVIEW_OK;
}

canview_status_t canview_stm_uart_get_time_mapping(
    const canview_stm_uart_context_t *context, canview_stm_uart_time_mapping_t *mapping)
{
    if (!context_ready(context) || mapping == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *mapping = context->time_mapping;
    return CANVIEW_OK;
}

canview_status_t canview_stm_uart_get_lease(const canview_stm_uart_context_t *context,
                                            canview_stm_uart_lease_t *lease)
{
    if (!context_ready(context) || lease == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *lease = context->lease;
    return CANVIEW_OK;
}
