/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_MCUBOOT_CONFIG_H
#define CANVIEW_MCUBOOT_CONFIG_H

/* MCUboot 고정 ABI 경계. HAL/RTOS와 분리된 단일 image offset-swap 구성. */
#define MCUBOOT_SIGN_EC256
#define MCUBOOT_USE_TINYCRYPT
#define MCUBOOT_SWAP_USING_OFFSET 1
#define MCUBOOT_VALIDATE_PRIMARY_SLOT
#define MCUBOOT_IMAGE_ACCESS_HOOKS
#define MCUBOOT_USE_FLASH_AREA_GET_SECTORS
#define MCUBOOT_MAX_IMG_SECTORS 97
#define MCUBOOT_IMAGE_NUMBER 1
#if defined(CANVIEW_MCUBOOT_HOST_MODEL)
#if defined(__arm__) || defined(__thumb__)
#error "Host model configuration must not enter Arm target"
#endif
#endif
#define MCUBOOT_FIH_PROFILE_MEDIUM
#define MCUBOOT_SUPPORT_DEV_WITH_ERASE
#include "canview_boot_flash.h"
#define MCUBOOT_WATCHDOG_FEED() canview_boot_progress()

#endif
