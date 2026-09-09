/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_stm_build.h"

/* Explicit host fixture; never included by the STM32 target project. */
const uint8_t canview_stm_link_build_id[CANVIEW_STM_BUILD_ID_DIGEST_SIZE] = {
    0x12U, 0x34U, 0x56U, 0x78U, 0x9aU, 0xbcU, 0xdeU, 0xf0U,
    0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU};
