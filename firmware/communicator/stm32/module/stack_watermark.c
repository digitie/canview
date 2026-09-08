/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_stack.h"

canview_status_t canview_stm_stack_watermark_arm(canview_stm_stack_watermark_t *watermark,
                                                  volatile uint8_t *region, size_t region_size)
{
    if (watermark == NULL || region == NULL || region_size < CANVIEW_STM_STACK_WATERMARK_MIN_BYTES)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (region_size > CANVIEW_STM_STACK_WATERMARK_MAX_BYTES)
    {
        return CANVIEW_OVERSIZE;
    }
    if (watermark->busy || watermark->armed)
    {
        return CANVIEW_RESOURCE_BUSY;
    }

    watermark->busy = true;
    for (size_t index = 0U; index < region_size; ++index)
    {
        region[index] = CANVIEW_STM_STACK_WATERMARK_PATTERN;
    }
    watermark->region = region;
    watermark->region_size = region_size;
    watermark->minimum_free_bytes = region_size;
    watermark->armed = true;
    watermark->busy = false;
    return CANVIEW_OK;
}

canview_status_t canview_stm_stack_watermark_sample(
    canview_stm_stack_watermark_t *watermark,
    canview_stm_stack_watermark_snapshot_t *snapshot)
{
    if (watermark == NULL || snapshot == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *snapshot = (const canview_stm_stack_watermark_snapshot_t){0U, 0U, false};
    if (!watermark->armed || watermark->region == NULL ||
        watermark->region_size < CANVIEW_STM_STACK_WATERMARK_MIN_BYTES ||
        watermark->region_size > CANVIEW_STM_STACK_WATERMARK_MAX_BYTES ||
        watermark->minimum_free_bytes > watermark->region_size)
    {
        return watermark->armed ? CANVIEW_MALFORMED : CANVIEW_INCOMPLETE;
    }
    if (watermark->busy)
    {
        return CANVIEW_RESOURCE_BUSY;
    }

    watermark->busy = true;
    size_t cursor = watermark->minimum_free_bytes;
    size_t inspected = 0U;
    while (cursor > 0U && inspected < CANVIEW_STM_STACK_SAMPLE_MAX_BYTES)
    {
        --cursor;
        ++inspected;
        if (watermark->region[cursor] == CANVIEW_STM_STACK_WATERMARK_PATTERN)
        {
            const size_t current_free = cursor + 1U;
            if (current_free < watermark->minimum_free_bytes)
            {
                watermark->minimum_free_bytes = current_free;
            }
            const canview_stm_stack_watermark_snapshot_t result = {
                current_free, watermark->minimum_free_bytes, true};
            watermark->busy = false;
            *snapshot = result;
            return CANVIEW_OK;
        }
    }
    if (cursor == 0U)
    {
        watermark->minimum_free_bytes = 0U;
        const canview_stm_stack_watermark_snapshot_t result = {0U, 0U, true};
        watermark->busy = false;
        *snapshot = result;
        return CANVIEW_OK;
    }

    watermark->busy = false;
    return CANVIEW_TIMEOUT;
}
