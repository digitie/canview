/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_BOOT_SYSFLASH_H
#define CANVIEW_BOOT_SYSFLASH_H
#define FLASH_AREA_IMAGE_PRIMARY(image) (((image) == 0) ? 1 : 255)
#define FLASH_AREA_IMAGE_SECONDARY(image) (((image) == 0) ? 2 : 255)
#endif
