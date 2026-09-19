/* SPDX-License-Identifier: GPL-3.0-only */
/** @file boot_identity.c @brief 이미지가 바꿀 수 없는 Communicator 부트 identity/공개키. */
#include "canview_boot_identity.h"
#include "canview_boot_trust_generated.h"
#include "bootutil/sign_key.h"
#include <stddef.h>

/* OTA 전체 보드 ID는 ADR-007/OTA §6을 따른다. MCU pin profile ID와 구별한다. */
static const canview_ota_identity_t boot_identity = {
    CANVIEW_OTA_ROLE_COMMUNICATOR, "comm-r2-n16r8", "communicator-ota-layout-v1",
    CANVIEW_BOOT_SECURITY_EPOCH, CANVIEW_BOOT_MANIFEST_KEY_ID};
static const uint8_t boot_public_der[CANVIEW_BOOT_PUBLIC_DER_BYTES] = CANVIEW_BOOT_PUBLIC_DER;
/* 이 두 export와 unsigned int 길이는 변경할 수 없는 upstream 공개키 ABI다. */
static const unsigned int boot_public_length = sizeof(boot_public_der);
const struct bootutil_key bootutil_keys[] = {{boot_public_der, &boot_public_length}};
const int bootutil_key_cnt = 1;

canview_status_t canview_boot_identity_read(canview_ota_identity_t *identity, uint32_t *abi)
{
    if (identity == NULL || abi == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    *identity = boot_identity;
    *abi = CANVIEW_BOOT_STM_ABI;
    return CANVIEW_OK;
}
