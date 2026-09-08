/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_stack.h"

static uint8_t watermark_pattern(size_t index)
{
    return (index & 1U) == 0U ? CANVIEW_STM_STACK_WATERMARK_PATTERN
                              : CANVIEW_STM_STACK_WATERMARK_PATTERN_INVERTED;
}

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
        region[index] = watermark_pattern(index);
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
    const size_t scan_limit = watermark->region_size < CANVIEW_STM_STACK_SAMPLE_MAX_BYTES
                                  ? watermark->region_size
                                  : CANVIEW_STM_STACK_SAMPLE_MAX_BYTES;
    size_t current_free = 0U;
    while (current_free < scan_limit &&
           watermark->region[current_free] == watermark_pattern(current_free))
    {
        ++current_free;
    }
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
