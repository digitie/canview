/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_queue.h"
#include <string.h>

static bool overlaps(const void *first, size_t first_size, const void *second, size_t second_size)
{
    const uintptr_t a = (uintptr_t)first;
    const uintptr_t b = (uintptr_t)second;
    return a <= b ? b - a < first_size : a - b < second_size;
}

canview_status_t canview_stm_queue_init(canview_stm_queue_t *queue, uint8_t *storage, size_t bytes,
                                        size_t capacity, size_t record_size,
                                        const canview_stm_critical_t *critical)
{
    if (queue == NULL || storage == NULL || critical == NULL || critical->enter == NULL ||
        critical->leave == NULL || capacity == 0U || capacity > CANVIEW_STM_QUEUE_CAPACITY_MAX ||
        record_size == 0U || record_size > CANVIEW_STM_QUEUE_RECORD_MAX ||
        bytes / record_size < capacity || overlaps(queue, sizeof(*queue), storage, bytes))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (queue->initialized)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    const canview_stm_critical_t copied_port = *critical;
    memset(queue, 0, sizeof(*queue));
    queue->storage = storage;
    queue->capacity = capacity;
    queue->record_size = record_size;
    queue->critical = copied_port;
    queue->initialized = true;
    return CANVIEW_OK;
}

static bool valid_record(const canview_stm_queue_t *queue, const void *record, size_t bytes)
{
    return queue != NULL && queue->initialized && record != NULL && bytes == queue->record_size &&
           !overlaps(queue, sizeof(*queue), record, bytes) &&
           !overlaps(queue->storage, queue->capacity * queue->record_size, record, bytes);
}

canview_status_t canview_stm_queue_push(canview_stm_queue_t *queue, const void *record,
                                        size_t bytes)
{
    if (!valid_record(queue, record, bytes))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const uint32_t mask = queue->critical.enter(queue->critical.context);
    canview_status_t status = CANVIEW_RESOURCE_BUSY;
    if (queue->count < queue->capacity)
    {
        memcpy(&queue->storage[queue->write_index * queue->record_size], record, bytes);
        queue->write_index = (queue->write_index + 1U) % queue->capacity;
        ++queue->count;
        if (queue->count > queue->high_water)
        {
            queue->high_water = queue->count;
        }
        status = CANVIEW_OK;
    }
    else if (queue->dropped != UINT32_MAX)
    {
        ++queue->dropped;
    }
    queue->critical.leave(queue->critical.context, mask);
    return status;
}

canview_status_t canview_stm_queue_pop(canview_stm_queue_t *queue, void *record, size_t bytes)
{
    if (!valid_record(queue, record, bytes))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const uint32_t mask = queue->critical.enter(queue->critical.context);
    canview_status_t status = CANVIEW_INCOMPLETE;
    if (queue->count != 0U)
    {
        memcpy(record, &queue->storage[queue->read_index * queue->record_size], bytes);
        queue->read_index = (queue->read_index + 1U) % queue->capacity;
        --queue->count;
        status = CANVIEW_OK;
    }
    queue->critical.leave(queue->critical.context, mask);
    return status;
}
