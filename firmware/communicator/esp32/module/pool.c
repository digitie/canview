/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_esp_pool.h"
#include <string.h>

static bool overlap(const void *a, size_t a_size, const void *b, size_t b_size)
{
    const uintptr_t left = (uintptr_t)a;
    const uintptr_t right = (uintptr_t)b;
    if (a_size > UINTPTR_MAX - left || b_size > UINTPTR_MAX - right)
    {
        return true;
    }
    return left < right + b_size && right < left + a_size;
}

static bool ready(const canview_esp_pool_t *pool)
{
    return pool != NULL && pool->initialized;
}

static void increment(uint32_t *value)
{
    if (*value != UINT32_MAX)
    {
        ++*value;
    }
}

static canview_esp_pool_slot_t *lookup(canview_esp_pool_t *pool, canview_esp_pool_token_t token)
{
    if (token.owner == (uintptr_t)pool && token.slot < pool->capacity && token.generation != 0U)
    {
        canview_esp_pool_slot_t *slot = &pool->slots[token.slot];
        if (slot->used && slot->generation == token.generation)
        {
            return slot;
        }
    }
    increment(&pool->stats.stale);
    return NULL;
}

canview_status_t canview_esp_pool_init(canview_esp_pool_t *pool, uint16_t capacity,
                                       const canview_esp_pool_port_t *port)
{
    if (pool == NULL || port == NULL || port->enter == NULL || port->leave == NULL ||
        capacity == 0U || capacity > CANVIEW_ESP_POOL_SLOTS)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (pool->initialized)
    {
        return CANVIEW_RESOURCE_BUSY;
    }
    const canview_esp_pool_port_t saved = *port;
    memset(pool, 0, sizeof(*pool));
    pool->port = saved;
    pool->capacity = capacity;
    pool->initialized = true;
    return CANVIEW_OK;
}

canview_status_t canview_esp_pool_acquire(canview_esp_pool_t *pool, const void *data, size_t length,
                                          canview_esp_pool_token_t *token)
{
    if (!ready(pool) || data == NULL || token == NULL || length == 0U ||
        length > CANVIEW_ESP_POOL_BYTES || overlap(data, length, pool, sizeof(*pool)) ||
        overlap(token, sizeof(*token), pool, sizeof(*pool)) ||
        overlap(token, sizeof(*token), data, length))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    pool->port.enter(pool->port.context);
    canview_status_t status = CANVIEW_RESOURCE_BUSY;
    for (uint16_t index = 0U; index < pool->capacity; ++index)
    {
        canview_esp_pool_slot_t *slot = &pool->slots[index];
        if (slot->used || slot->generation == UINT32_MAX)
        {
            continue;
        }
        memcpy(slot->data, data, length);
        slot->length = (uint16_t)length;
        ++slot->generation;
        slot->used = true;
        token->owner = (uintptr_t)pool;
        token->slot = index;
        token->generation = slot->generation;
        ++pool->stats.used;
        if (pool->stats.used > pool->stats.high_water)
        {
            pool->stats.high_water = pool->stats.used;
        }
        status = CANVIEW_OK;
        break;
    }
    if (status != CANVIEW_OK)
    {
        increment(&pool->stats.exhausted);
    }
    pool->port.leave(pool->port.context);
    return status;
}

canview_status_t canview_esp_pool_copy(canview_esp_pool_t *pool, canview_esp_pool_token_t token,
                                       void *data, size_t capacity, size_t *length)
{
    if (!ready(pool) || data == NULL || length == NULL ||
        overlap(data, capacity, pool, sizeof(*pool)) ||
        overlap(length, sizeof(*length), pool, sizeof(*pool)) ||
        overlap(data, capacity, length, sizeof(*length)))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    pool->port.enter(pool->port.context);
    const canview_esp_pool_slot_t *slot = lookup(pool, token);
    canview_status_t status = CANVIEW_STALE;
    if (slot != NULL)
    {
        status = CANVIEW_BUFFER_TOO_SMALL;
        if (capacity >= slot->length)
        {
            memcpy(data, slot->data, slot->length);
            *length = slot->length;
            status = CANVIEW_OK;
        }
    }
    pool->port.leave(pool->port.context);
    return status;
}

canview_status_t canview_esp_pool_release(canview_esp_pool_t *pool, canview_esp_pool_token_t token)
{
    if (!ready(pool))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    pool->port.enter(pool->port.context);
    canview_esp_pool_slot_t *slot = lookup(pool, token);
    canview_status_t status = CANVIEW_STALE;
    if (slot != NULL)
    {
        memset(slot->data, 0, sizeof(slot->data));
        slot->length = 0U;
        slot->used = false;
        --pool->stats.used;
        if (slot->generation == UINT32_MAX)
        {
            ++pool->stats.retired;
        }
        status = CANVIEW_OK;
    }
    pool->port.leave(pool->port.context);
    return status;
}

canview_status_t canview_esp_pool_stats(canview_esp_pool_t *pool, canview_esp_pool_stats_t *stats)
{
    if (!ready(pool) || stats == NULL || overlap(stats, sizeof(*stats), pool, sizeof(*pool)))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    pool->port.enter(pool->port.context);
    *stats = pool->stats;
    pool->port.leave(pool->port.context);
    return CANVIEW_OK;
}
