/* SPDX-License-Identifier: GPL-3.0-only */
#include <stddef.h>
#include "canview_stm_flash_layout.h"
#include "flash_layout.h"

static const canview_stm_flash_area_t flash_areas[CANVIEW_STM_FLASH_REGION_COUNT] =
{
    {CANVIEW_STM_FLASH_BASE, CANVIEW_STM_BOOT_BYTES},
    {CANVIEW_STM_PRIMARY_ADDRESS, CANVIEW_STM_PRIMARY_BYTES},
    {CANVIEW_STM_SECONDARY_ADDRESS, CANVIEW_STM_SECONDARY_BYTES},
    {CANVIEW_STM_POLICY_A_ADDRESS, CANVIEW_STM_POLICY_BYTES},
    {CANVIEW_STM_POLICY_B_ADDRESS, CANVIEW_STM_POLICY_BYTES},
    {CANVIEW_STM_CONFIG_A_ADDRESS, CANVIEW_STM_CONFIG_BYTES},
    {CANVIEW_STM_CONFIG_B_ADDRESS, CANVIEW_STM_CONFIG_BYTES},
    {CANVIEW_STM_RESERVED_ADDRESS, CANVIEW_STM_RESERVED_BYTES}
};

canview_status_t canview_stm_flash_area(canview_stm_flash_region_t region,
    canview_stm_flash_area_t *area)
{
    if (area == NULL || (uint32_t)region >= (uint32_t)CANVIEW_STM_FLASH_REGION_COUNT)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *area = flash_areas[(uint32_t)region];
    return CANVIEW_OK;
}

canview_status_t canview_stm_flash_range(canview_stm_flash_region_t region,
    canview_stm_flash_operation_t operation, uint32_t offset, uint32_t length,
    uint32_t *address)
{
    canview_stm_flash_area_t area;
    uint32_t alignment;
    if (address == NULL || length == 0U ||
        canview_stm_flash_area(region, &area) != CANVIEW_OK)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    switch (operation)
    {
        case CANVIEW_STM_FLASH_READ:
            alignment = 1U;
            break;
        case CANVIEW_STM_FLASH_PROGRAM:
            alignment = CANVIEW_STM_FLASH_WRITE_BYTES;
            break;
        case CANVIEW_STM_FLASH_ERASE:
            alignment = CANVIEW_STM_FLASH_PAGE_BYTES;
            break;
        default:
            return CANVIEW_INVALID_ARGUMENT;
    }
    if (operation != CANVIEW_STM_FLASH_READ &&
        (region == CANVIEW_STM_FLASH_BOOT || region == CANVIEW_STM_FLASH_RESERVED))
    {
        return CANVIEW_AUTH_FAILED;
    }
    /* 덧셈 전에 빼기로 확인한다. UINT32_MAX 길이도 wrap되어 허용되지 않는다. */
    if (offset > area.size || length > area.size - offset)
    {
        return CANVIEW_OVERSIZE;
    }
    if (offset % alignment != 0U || length % alignment != 0U)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *address = area.address + offset;
    return CANVIEW_OK;
}
