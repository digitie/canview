/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_esp_core.h"
#include <stddef.h>

static canview_status_t fail(canview_esp_core_t *core, canview_esp_core_fault_t fault,
                             canview_status_t status)
{
    if (core->fault == CANVIEW_ESP_FAULT_NONE)
    {
        core->fault = fault;
    }
    core->state = CANVIEW_ESP_CORE_FAULT;
    core->busy = false;
    return status == CANVIEW_OK ? CANVIEW_TIMEOUT : status;
}

static bool memory_valid(const canview_esp_core_sample_t *sample)
{
    return sample->flash_bytes == CANVIEW_ESP_CORE_FLASH_BYTES &&
           sample->psram_bytes == CANVIEW_ESP_CORE_PSRAM_BYTES &&
           sample->heap_free_bytes >= CANVIEW_ESP_CORE_HEAP_MIN &&
           sample->heap_free_bytes <= CANVIEW_ESP_CORE_INTERNAL_MAX &&
           sample->largest_block_bytes >= CANVIEW_ESP_CORE_BLOCK_MIN &&
           sample->largest_block_bytes <= sample->heap_free_bytes &&
           sample->stack_free_bytes >= CANVIEW_ESP_CORE_STACK_MIN &&
           sample->stack_free_bytes <= CANVIEW_ESP_CORE_STACK_BYTES;
}

static void increment(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        ++*value;
    }
}

canview_status_t canview_esp_core_boot(canview_esp_core_t *core,
                                       const canview_esp_core_port_t *port)
{
    if (core == NULL || port == NULL || port->safe_gpio == NULL || port->watchdog_start == NULL ||
        port->now_us == NULL || port->sample == NULL || port->feed == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (core->busy)
    {
        return fail(core, CANVIEW_ESP_FAULT_REENTRY, CANVIEW_RESOURCE_BUSY);
    }
    if (core->state != CANVIEW_ESP_CORE_UNINITIALIZED)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    core->port = *port;
    core->state = CANVIEW_ESP_CORE_STARTING;
    core->busy = true;
    canview_status_t status = core->port.safe_gpio(core->port.context);
    if (status != CANVIEW_OK || core->state == CANVIEW_ESP_CORE_FAULT)
    {
        return fail(core, CANVIEW_ESP_FAULT_SAFE_GPIO, status);
    }
    status = core->port.watchdog_start(core->port.context);
    if (status != CANVIEW_OK || core->state == CANVIEW_ESP_CORE_FAULT)
    {
        return fail(core, CANVIEW_ESP_FAULT_WATCHDOG, status);
    }
    core->watchdog_ready = true;
    uint64_t started = 0U;
    status = core->port.now_us(core->port.context, &started);
    if (status != CANVIEW_OK || core->state == CANVIEW_ESP_CORE_FAULT ||
        started > UINT64_MAX - CANVIEW_ESP_CORE_DEADLINE_US)
    {
        return fail(core, CANVIEW_ESP_FAULT_CLOCK, status);
    }
    canview_esp_core_sample_t sample = {0};
    status = core->port.sample(core->port.context, &sample);
    if (status != CANVIEW_OK || core->state == CANVIEW_ESP_CORE_FAULT || !memory_valid(&sample))
    {
        return fail(core, CANVIEW_ESP_FAULT_MEMORY, status);
    }
    uint64_t finished = 0U;
    status = core->port.now_us(core->port.context, &finished);
    if (status != CANVIEW_OK || core->state == CANVIEW_ESP_CORE_FAULT || finished < started)
    {
        return fail(core, CANVIEW_ESP_FAULT_CLOCK, status);
    }
    if (finished - started > CANVIEW_ESP_CORE_BUDGET_US)
    {
        return fail(core, CANVIEW_ESP_FAULT_BUDGET, CANVIEW_TIMEOUT);
    }
    core->sample = sample;
    core->last_progress_us = finished;
    core->state = CANVIEW_ESP_CORE_SAFE_BENCH;
    core->busy = false;
    return CANVIEW_OK;
}

canview_status_t canview_esp_core_step(canview_esp_core_t *core)
{
    if (core == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (core->busy)
    {
        return fail(core, CANVIEW_ESP_FAULT_REENTRY, CANVIEW_RESOURCE_BUSY);
    }
    if (core->state != CANVIEW_ESP_CORE_SAFE_BENCH)
    {
        return core->state == CANVIEW_ESP_CORE_FAULT ? CANVIEW_TIMEOUT : CANVIEW_INVALID_ARGUMENT;
    }
    core->busy = true;
    uint64_t started = 0U;
    canview_status_t status = core->port.now_us(core->port.context, &started);
    if (status != CANVIEW_OK || core->state == CANVIEW_ESP_CORE_FAULT ||
        started <= core->last_progress_us || started > UINT64_MAX - CANVIEW_ESP_CORE_DEADLINE_US)
    {
        return fail(core, CANVIEW_ESP_FAULT_CLOCK, status);
    }
    if (started - core->last_progress_us > CANVIEW_ESP_CORE_DEADLINE_US)
    {
        return fail(core, CANVIEW_ESP_FAULT_DEADLINE, CANVIEW_TIMEOUT);
    }
    canview_esp_core_sample_t sample = {0};
    status = core->port.sample(core->port.context, &sample);
    if (status != CANVIEW_OK || core->state == CANVIEW_ESP_CORE_FAULT || !memory_valid(&sample))
    {
        return fail(core, CANVIEW_ESP_FAULT_MEMORY, status);
    }
    uint64_t finished = 0U;
    status = core->port.now_us(core->port.context, &finished);
    if (status != CANVIEW_OK || core->state == CANVIEW_ESP_CORE_FAULT || finished < started)
    {
        return fail(core, CANVIEW_ESP_FAULT_CLOCK, status);
    }
    if (finished - core->last_progress_us > CANVIEW_ESP_CORE_DEADLINE_US)
    {
        return fail(core, CANVIEW_ESP_FAULT_DEADLINE, CANVIEW_TIMEOUT);
    }
    if (finished - started > CANVIEW_ESP_CORE_BUDGET_US)
    {
        return fail(core, CANVIEW_ESP_FAULT_BUDGET, CANVIEW_TIMEOUT);
    }
    const uint64_t progress_limit = core->last_progress_us + CANVIEW_ESP_CORE_DEADLINE_US;
    const uint64_t budget_limit = started + CANVIEW_ESP_CORE_BUDGET_US;
    const uint64_t deadline = progress_limit < budget_limit ? progress_limit : budget_limit;
    uint64_t fed_at = 0U;
    status = core->port.feed(core->port.context, finished, deadline, &fed_at);
    if (status != CANVIEW_OK || core->state == CANVIEW_ESP_CORE_FAULT)
    {
        return fail(core, CANVIEW_ESP_FAULT_WATCHDOG, status);
    }
    if (fed_at < finished || fed_at > deadline)
    {
        return fail(core, CANVIEW_ESP_FAULT_CLOCK, CANVIEW_TIMEOUT);
    }
    core->sample = sample;
    core->last_progress_us = fed_at;
    increment(&core->checks);
    increment(&core->feeds);
    core->busy = false;
    return CANVIEW_OK;
}
