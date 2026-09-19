/* SPDX-License-Identifier: GPL-3.0-only */
#include "flash_map_backend/flash_map_backend.h"
#include "canview_boot_flash.h"
#include "canview_stm_flash_layout.h"
#include "flash_layout.h"

static const struct flash_area primary = {1U, 0U,
    CANVIEW_STM_PRIMARY_ADDRESS - CANVIEW_STM_FLASH_BASE, CANVIEW_STM_PRIMARY_BYTES};
static const struct flash_area secondary = {2U, 0U,
    CANVIEW_STM_SECONDARY_ADDRESS - CANVIEW_STM_FLASH_BASE, CANVIEW_STM_SECONDARY_BYTES};

static int area_valid(const struct flash_area *area)
{
    return area == &primary || area == &secondary;
}

static int area_range(const struct flash_area *area, canview_stm_flash_operation_t operation,
    uint32_t offset, uint32_t length, uint32_t *address)
{
    canview_stm_flash_region_t region;
    if (area_valid(area) == 0)
    {
        return -1;
    }
    region = area == &primary ? CANVIEW_STM_FLASH_PRIMARY : CANVIEW_STM_FLASH_SECONDARY;
    return canview_stm_flash_range(region, operation, offset, length, address) == CANVIEW_OK ? 0 : -1;
}

int flash_device_base(uint8_t device_id, uintptr_t *base)
{
    if (device_id != 0U || base == NULL)
    {
        return -1;
    }
    *base = CANVIEW_STM_FLASH_BASE;
    return 0;
}

int flash_area_open(uint8_t id, const struct flash_area **area)
{
    if (area == NULL || (id != primary.fa_id && id != secondary.fa_id))
    {
        return -1;
    }
    *area = id == primary.fa_id ? &primary : &secondary;
    return 0;
}

void flash_area_close(const struct flash_area *area)
{
    /* 정적 불변 descriptor이므로 open 자원/참조계수는 없다. */
    (void)area;
}

int flash_area_read(const struct flash_area *area, uint32_t offset, void *data, uint32_t length)
{
    uint32_t address;
    if (data == NULL || area_range(area, CANVIEW_STM_FLASH_READ, offset, length, &address) != 0)
    {
        return -1;
    }
    return canview_boot_flash_read(address, data, length);
}

int flash_area_write(const struct flash_area *area, uint32_t offset, const void *data, uint32_t length)
{
    uint32_t address;
    if (data == NULL || area_range(area, CANVIEW_STM_FLASH_PROGRAM, offset, length, &address) != 0)
    {
        return -1;
    }
    if (canview_boot_flash_check() != 0) { return -1; }
    return canview_boot_flash_write(address, data, length);
}

int flash_area_erase(const struct flash_area *area, uint32_t offset, uint32_t length)
{
    uint32_t address;
    if (area_range(area, CANVIEW_STM_FLASH_ERASE, offset, length, &address) != 0)
    {
        return -1;
    }
    if (canview_boot_flash_check() != 0) { return -1; }
    return canview_boot_flash_erase(address, length);
}

uint32_t flash_area_align(const struct flash_area *area)
{
    return area_valid(area) != 0 ? CANVIEW_STM_FLASH_WRITE_BYTES : 0U;
}

uint8_t flash_area_erased_val(const struct flash_area *area)
{
    (void)area; /* 유일한 internal Flash 장치는 erased=0xff다. IO 권한은 부여하지 않는다. */
    return UINT8_MAX;
}

int flash_area_get_sectors(int id, uint32_t *count, struct flash_sector *sectors)
{
    const struct flash_area *area;
    uint32_t required;
    uint32_t index;
    if (id < 0 || id > UINT8_MAX || count == NULL || sectors == NULL ||
        flash_area_open((uint8_t)id, &area) != 0)
    {
        return -1;
    }
    required = area->fa_size / CANVIEW_STM_FLASH_PAGE_BYTES;
    if (*count < required)
    {
        return -1;
    }
    for (index = 0U; index < required; ++index)
    {
        sectors[index].fs_off = index * CANVIEW_STM_FLASH_PAGE_BYTES;
        sectors[index].fs_size = CANVIEW_STM_FLASH_PAGE_BYTES;
    }
    *count = required;
    return 0;
}

int flash_area_get_sector(const struct flash_area *area, uint32_t offset, struct flash_sector *sector)
{
    if (area_valid(area) == 0 || sector == NULL || offset >= area->fa_size)
    {
        return -1;
    }
    sector->fs_off = offset - offset % CANVIEW_STM_FLASH_PAGE_BYTES;
    sector->fs_size = CANVIEW_STM_FLASH_PAGE_BYTES;
    return 0;
}

int flash_area_id_from_multi_image_slot(int image, int slot)
{
    return image == 0 && (slot == 0 || slot == 1) ? slot + 1 : -1;
}

int flash_area_id_from_image_slot(int slot)
{
    return flash_area_id_from_multi_image_slot(0, slot);
}

int flash_area_id_to_multi_image_slot(int image, int id)
{
    return image == 0 && (id == 1 || id == 2) ? id - 1 : -1;
}
