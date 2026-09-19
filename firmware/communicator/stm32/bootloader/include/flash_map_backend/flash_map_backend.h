/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_BOOT_FLASH_MAP_H
#define CANVIEW_BOOT_FLASH_MAP_H
#include <stddef.h>
#include <stdint.h>

/* MCUboot 외부 ABI의 이름/타입을 유지한다. fa_off는 장치 기준 상대 offset이다. */
struct flash_area { uint8_t fa_id; uint8_t fa_device_id; uint32_t fa_off; uint32_t fa_size; };
struct flash_sector { uint32_t fs_off; uint32_t fs_size; };
int flash_device_base(uint8_t device_id, uintptr_t *base);
int flash_area_open(uint8_t id, const struct flash_area **area);
void flash_area_close(const struct flash_area *area);
int flash_area_read(const struct flash_area *area, uint32_t offset, void *data, uint32_t length);
int flash_area_write(const struct flash_area *area, uint32_t offset, const void *data, uint32_t length);
int flash_area_erase(const struct flash_area *area, uint32_t offset, uint32_t length);
uint32_t flash_area_align(const struct flash_area *area);
uint8_t flash_area_erased_val(const struct flash_area *area);
int flash_area_get_sectors(int id, uint32_t *count, struct flash_sector *sectors);
int flash_area_get_sector(const struct flash_area *area, uint32_t offset, struct flash_sector *sector);
int flash_area_id_from_image_slot(int slot);
int flash_area_id_from_multi_image_slot(int image, int slot);
int flash_area_id_to_multi_image_slot(int image, int id);
static inline uint32_t flash_area_get_off(const struct flash_area *area) { return area->fa_off; }
static inline uint32_t flash_area_get_size(const struct flash_area *area) { return area->fa_size; }
static inline uint8_t flash_area_get_device_id(const struct flash_area *area) { return area->fa_device_id; }
static inline uint8_t flash_area_get_id(const struct flash_area *area) { return area->fa_id; }
static inline uint32_t flash_sector_get_off(const struct flash_sector *sector) { return sector->fs_off; }
static inline uint32_t flash_sector_get_size(const struct flash_sector *sector) { return sector->fs_size; }
#endif
