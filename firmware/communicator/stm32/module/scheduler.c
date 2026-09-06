/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_core.h"
#include <string.h>

static canview_status_t latch_fault(canview_stm_scheduler_t *scheduler, canview_stm_fault_t fault)
{
    if (scheduler->fault == CANVIEW_STM_FAULT_NONE)
    {
        scheduler->fault = fault;
        scheduler->votes = 0U;
        scheduler->port.fault(scheduler->port.context, fault);
    }
    return CANVIEW_TIMEOUT;
}

canview_status_t canview_stm_scheduler_init(canview_stm_scheduler_t *scheduler,
                                            const canview_stm_worker_t *workers, size_t count,
                                            const canview_stm_scheduler_port_t *port,
                                            uint32_t now_ms)
{
    if (scheduler == NULL || workers == NULL || port == NULL || count == 0U ||
        count > CANVIEW_STM_WORKERS_MAX || port->now_us == NULL || port->feed == NULL ||
        port->fault == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (scheduler->initialized)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    uint16_t required_mask = 0U;
    for (size_t index = 0U; index < count; ++index)
    {
        const canview_stm_worker_t *const worker = &workers[index];
        if (worker->run == NULL || worker->period_ms == 0U ||
            worker->period_ms > CANVIEW_STM_PERIOD_MAX_MS ||
            worker->deadline_ms < worker->period_ms ||
            worker->deadline_ms > CANVIEW_STM_PERIOD_MAX_MS || worker->budget_us == 0U ||
            worker->budget_us > CANVIEW_STM_WORKER_BUDGET_MAX_US ||
            (worker->required && worker->deadline_ms > CANVIEW_STM_HEALTH_WINDOW_MS))
        {
            return CANVIEW_INVALID_ARGUMENT;
        }
        if (worker->required)
        {
            required_mask |= (uint16_t)(UINT16_C(1) << index);
        }
    }
    if (required_mask == 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_stm_scheduler_port_t copied_port = *port;
    memmove(scheduler->workers, workers, count * sizeof(*workers));
    scheduler->port = copied_port;
    scheduler->count = count;
    scheduler->required_mask = required_mask;
    scheduler->last_step_ms = now_ms;
    scheduler->last_feed_ms = now_ms;
    scheduler->votes = 0U;
    scheduler->started_mask = 0U;
    scheduler->feeds = 0U;
    scheduler->fault = CANVIEW_STM_FAULT_NONE;
    scheduler->running = true;
    scheduler->initialized = true;
    const uint32_t initialized_us = copied_port.now_us(copied_port.context);
    scheduler->running = false;
    if (scheduler->fault != CANVIEW_STM_FAULT_NONE)
    {
        return CANVIEW_TIMEOUT;
    }
    for (size_t index = 0U; index < count; ++index)
    {
        scheduler->last_run_ms[index] = now_ms;
        scheduler->last_progress_ms[index] = now_ms;
        scheduler->last_progress_us[index] = initialized_us;
    }
    return CANVIEW_OK;
}

static canview_status_t run_due(canview_stm_scheduler_t *scheduler, uint32_t now_ms)
{
    for (size_t index = 0U; index < scheduler->count; ++index)
    {
        if (scheduler->fault != CANVIEW_STM_FAULT_NONE)
        {
            return CANVIEW_TIMEOUT;
        }
        const canview_stm_worker_t *const worker = &scheduler->workers[index];
        const uint16_t mask = (uint16_t)(UINT16_C(1) << index);
        const uint32_t started_us = scheduler->port.now_us(scheduler->port.context);
        if (scheduler->fault != CANVIEW_STM_FAULT_NONE)
        {
            return CANVIEW_TIMEOUT;
        }
        if (worker->required && (uint32_t)(started_us - scheduler->last_progress_us[index]) >
                                    worker->deadline_ms * UINT32_C(1000))
        {
            return latch_fault(scheduler, CANVIEW_STM_FAULT_DEADLINE);
        }
        if ((scheduler->started_mask & mask) != 0U &&
            (uint32_t)(now_ms - scheduler->last_run_ms[index]) < worker->period_ms)
        {
            continue;
        }
        const canview_status_t status = worker->run(worker->context);
        const uint32_t finished_us = scheduler->port.now_us(scheduler->port.context);
        const uint32_t elapsed_us = finished_us - started_us;
        if (scheduler->fault != CANVIEW_STM_FAULT_NONE)
        {
            return CANVIEW_TIMEOUT;
        }
        if (elapsed_us > worker->budget_us)
        {
            return latch_fault(scheduler, CANVIEW_STM_FAULT_BUDGET);
        }
        /* 이전 진척의 deadline을 확인한 뒤에만 새 성공/vote로 덮어쓴다. */
        if (worker->required && (uint32_t)(finished_us - scheduler->last_progress_us[index]) >
                                    worker->deadline_ms * UINT32_C(1000))
        {
            return latch_fault(scheduler, CANVIEW_STM_FAULT_DEADLINE);
        }
        scheduler->last_run_ms[index] = now_ms;
        scheduler->started_mask |= mask;
        if (status == CANVIEW_OK)
        {
            scheduler->last_progress_ms[index] = now_ms;
            scheduler->last_progress_us[index] = finished_us;
            scheduler->votes |= mask;
        }
        else if (status != CANVIEW_RESOURCE_BUSY)
        {
            return latch_fault(scheduler, CANVIEW_STM_FAULT_WORKER);
        }
    }
    return CANVIEW_OK;
}

canview_status_t canview_stm_scheduler_step(canview_stm_scheduler_t *scheduler, uint32_t now_ms)
{
    if (scheduler == NULL || !scheduler->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (scheduler->fault != CANVIEW_STM_FAULT_NONE)
    {
        return CANVIEW_TIMEOUT;
    }
    if (scheduler->running)
    {
        return latch_fault(scheduler, CANVIEW_STM_FAULT_REENTRY);
    }
    if ((uint32_t)(now_ms - scheduler->last_step_ms) > CANVIEW_STM_HEALTH_WINDOW_MS)
    {
        return latch_fault(scheduler, CANVIEW_STM_FAULT_GAP);
    }
    scheduler->running = true;
    scheduler->last_step_ms = now_ms;
    const uint32_t started_us = scheduler->port.now_us(scheduler->port.context);
    canview_status_t result = run_due(scheduler, now_ms);
    const uint32_t finished_us = scheduler->port.now_us(scheduler->port.context);
    const uint32_t elapsed_us = finished_us - started_us;
    if (scheduler->fault != CANVIEW_STM_FAULT_NONE)
    {
        result = CANVIEW_TIMEOUT;
    }
    if (result == CANVIEW_OK && elapsed_us > CANVIEW_STM_HEALTH_WINDOW_MS * UINT32_C(1000))
    {
        result = latch_fault(scheduler, CANVIEW_STM_FAULT_BUDGET);
    }
    if (result == CANVIEW_OK)
    {
        for (size_t index = 0U; index < scheduler->count; ++index)
        {
            if (scheduler->workers[index].required &&
                (uint32_t)(finished_us - scheduler->last_progress_us[index]) >
                    scheduler->workers[index].deadline_ms * UINT32_C(1000))
            {
                result = latch_fault(scheduler, CANVIEW_STM_FAULT_DEADLINE);
                break;
            }
        }
    }
    if (result == CANVIEW_OK &&
        (scheduler->votes & scheduler->required_mask) == scheduler->required_mask &&
        (uint32_t)(now_ms - scheduler->last_feed_ms) >= CANVIEW_STM_FEED_PERIOD_MS)
    {
        const canview_status_t fed = scheduler->port.feed(scheduler->port.context);
        if (scheduler->fault != CANVIEW_STM_FAULT_NONE)
        {
            result = CANVIEW_TIMEOUT;
        }
        else if (fed != CANVIEW_OK)
        {
            result = latch_fault(scheduler, CANVIEW_STM_FAULT_PORT);
        }
        else
        {
            scheduler->last_feed_ms = now_ms;
            scheduler->votes = 0U;
            if (scheduler->feeds != UINT32_MAX)
            {
                ++scheduler->feeds;
            }
        }
    }
    scheduler->running = false;
    return result;
}
