/* SPDX-License-Identifier: GPL-3.0-only */
#include "dns_server.h"
#include "canview_bridge_web.h"
#include <errno.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

#define CANVIEW_BRIDGE_DNS_PORT (53U)
#define CANVIEW_BRIDGE_DNS_BUFFER_BYTES (512U)
#define CANVIEW_BRIDGE_DNS_STACK_BYTES (4096U)
#define CANVIEW_BRIDGE_DNS_PRIORITY (4U)
#define CANVIEW_BRIDGE_DNS_RETRY_MS (1000U)
#define CANVIEW_BRIDGE_DNS_RECV_TIMEOUT_MS (1000U)
#define CANVIEW_BRIDGE_DNS_TTL_SECONDS (60U)
#define CANVIEW_BRIDGE_DNS_MAX_QUERIES_PER_SLICE (16U)

typedef struct
{
    uint8_t rx[CANVIEW_BRIDGE_DNS_BUFFER_BYTES];
    uint8_t tx[CANVIEW_BRIDGE_DNS_BUFFER_BYTES];
    StaticTask_t task_buffer;
    StackType_t task_stack[CANVIEW_BRIDGE_DNS_STACK_BYTES];
    TaskHandle_t volatile task;
    volatile int socket_fd;
    volatile bool stop_requested;
    volatile bool stopped;
    volatile bool started;
} canview_bridge_dns_state_t;

static canview_bridge_dns_state_t dns_state;

static uint16_t read_u16_be(const uint8_t *data)
{
    return (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

static void write_u16_be(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)value;
}

static void write_u32_be(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24U);
    data[1] = (uint8_t)(value >> 16U);
    data[2] = (uint8_t)(value >> 8U);
    data[3] = (uint8_t)value;
}

static bool build_captive_response(const uint8_t *query, size_t query_length, uint8_t *response,
                                   size_t response_capacity, size_t *response_length)
{
    if (query == NULL || response == NULL || response_length == NULL || query_length < 12U ||
        query_length > CANVIEW_BRIDGE_DNS_BUFFER_BYTES || response_capacity < 12U)
    {
        return false;
    }
    const uint16_t flags = read_u16_be(query + 2U);
    if (read_u16_be(query + 4U) != 1U || (flags & 0x8000U) != 0U || (flags & 0x7800U) != 0U)
    {
        return false;
    }

    size_t offset = 12U;
    unsigned label_count = 0U;
    while (offset < query_length)
    {
        const uint8_t label_length = query[offset];
        ++offset;
        if (label_length == 0U)
        {
            break;
        }
        if ((label_length & 0xC0U) != 0U || label_length > 63U ||
            label_count >= 127U || offset > query_length - label_length)
        {
            return false;
        }
        offset += label_length;
        ++label_count;
    }
    if (offset < 13U || offset > query_length || offset > query_length - 4U ||
        read_u16_be(query + offset) != 1U || read_u16_be(query + offset + 2U) != 1U)
    {
        return false;
    }
    const size_t question_end = offset + 4U;
    const size_t answer_bytes = 2U + 2U + 2U + 4U + 2U + 4U;
    if (question_end > response_capacity || answer_bytes > response_capacity - question_end)
    {
        return false;
    }
    memcpy(response, query, question_end);
    write_u16_be(response + 2U, 0x8180U);
    write_u16_be(response + 4U, 1U);
    write_u16_be(response + 6U, 1U);
    write_u16_be(response + 8U, 0U);
    write_u16_be(response + 10U, 0U);

    size_t answer_offset = question_end;
    write_u16_be(response + answer_offset, 0xC00CU);
    answer_offset += 2U;
    write_u16_be(response + answer_offset, 1U);
    answer_offset += 2U;
    write_u16_be(response + answer_offset, 1U);
    answer_offset += 2U;
    write_u32_be(response + answer_offset, CANVIEW_BRIDGE_DNS_TTL_SECONDS);
    answer_offset += 4U;
    write_u16_be(response + answer_offset, 4U);
    answer_offset += 2U;
    response[answer_offset++] = 192U;
    response[answer_offset++] = 168U;
    response[answer_offset++] = 4U;
    response[answer_offset++] = 1U;
    *response_length = answer_offset;
    return true;
}

static void close_socket(int socket_fd)
{
    if (socket_fd >= 0)
    {
        (void)shutdown(socket_fd, SHUT_RDWR);
        (void)close(socket_fd);
    }
}

static void dns_task(void *context)
{
    canview_bridge_dns_state_t *state = context;
    while (!state->stop_requested)
    {
        const int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_fd < 0)
        {
            vTaskDelay(pdMS_TO_TICKS(CANVIEW_BRIDGE_DNS_RETRY_MS));
            continue;
        }
        state->socket_fd = socket_fd;
        const struct timeval timeout = {.tv_sec = CANVIEW_BRIDGE_DNS_RECV_TIMEOUT_MS / 1000U,
                                         .tv_usec = (CANVIEW_BRIDGE_DNS_RECV_TIMEOUT_MS % 1000U) * 1000U};
        if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0)
        {
            close_socket(socket_fd);
            state->socket_fd = -1;
            vTaskDelay(pdMS_TO_TICKS(CANVIEW_BRIDGE_DNS_RETRY_MS));
            continue;
        }
        const struct sockaddr_in address = {.sin_len = sizeof(struct sockaddr_in),
                                            .sin_family = AF_INET,
                                            .sin_port = htons(CANVIEW_BRIDGE_DNS_PORT),
                                            .sin_addr = {.s_addr = htonl(INADDR_ANY)}};
        if (bind(socket_fd, (const struct sockaddr *)&address, sizeof(address)) != 0)
        {
            close_socket(socket_fd);
            state->socket_fd = -1;
            vTaskDelay(pdMS_TO_TICKS(CANVIEW_BRIDGE_DNS_RETRY_MS));
            continue;
        }
        unsigned queries_in_slice = 0U;
        for (;;)
        {
            if (state->stop_requested)
            {
                break;
            }
            if (queries_in_slice >= CANVIEW_BRIDGE_DNS_MAX_QUERIES_PER_SLICE)
            {
                vTaskDelay(1U);
                queries_in_slice = 0U;
            }
            struct sockaddr_storage source;
            socklen_t source_length = sizeof(source);
            const int received = recvfrom(socket_fd, state->rx, sizeof(state->rx), 0,
                                          (struct sockaddr *)&source, &source_length);
            if (received < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                {
                    continue;
                }
                break;
            }
            if (received == 0)
            {
                continue;
            }
            ++queries_in_slice;
            size_t response_length = 0U;
            if (!build_captive_response(state->rx, (size_t)received, state->tx, sizeof(state->tx),
                                        &response_length))
            {
                continue;
            }
            const int sent = sendto(socket_fd, state->tx, response_length, 0,
                                    (const struct sockaddr *)&source, source_length);
            if (sent < 0 || (size_t)sent != response_length)
            {
                break;
            }
        }
        close_socket(socket_fd);
        state->socket_fd = -1;
        if (!state->stop_requested)
        {
            vTaskDelay(pdMS_TO_TICKS(CANVIEW_BRIDGE_DNS_RETRY_MS));
        }
    }
    state->socket_fd = -1;
    state->stopped = true;
    for (;;)
    {
        vTaskSuspend(NULL);
    }
}

esp_err_t canview_bridge_dns_start(void)
{
    if (dns_state.task != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    memset(&dns_state, 0, sizeof(dns_state));
    dns_state.socket_fd = -1;
    dns_state.task = xTaskCreateStatic(dns_task, "bridge_dns", CANVIEW_BRIDGE_DNS_STACK_BYTES,
                                       &dns_state, CANVIEW_BRIDGE_DNS_PRIORITY,
                                       dns_state.task_stack, &dns_state.task_buffer);
    if (dns_state.task == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    dns_state.started = true;
    return ESP_OK;
}

esp_err_t canview_bridge_dns_stop(void)
{
    if (!dns_state.started)
    {
        return ESP_OK;
    }
    dns_state.stop_requested = true;
    const int socket_fd = dns_state.socket_fd;
    if (socket_fd >= 0)
    {
        (void)shutdown(socket_fd, SHUT_RDWR);
    }
    const TickType_t wait_ticks = pdMS_TO_TICKS(CANVIEW_BRIDGE_DNS_RECV_TIMEOUT_MS + 500U);
    TickType_t waited = 0U;
    while (!dns_state.stopped && waited < wait_ticks)
    {
        vTaskDelay(1U);
        ++waited;
    }
    if (!dns_state.stopped)
    {
        return ESP_ERR_TIMEOUT;
    }
    TaskHandle_t task = dns_state.task;
    if (task != NULL)
    {
        vTaskDelete(task);
    }
    dns_state.task = NULL;
    memset(&dns_state, 0, sizeof(dns_state));
    dns_state.socket_fd = -1;
    return ESP_OK;
}
