/* SPDX-License-Identifier: GPL-3.0-only */
/** @file main.c @brief SDK 실제 compile/link 전용. Flash/provisioning을 실행하지 않는다. */
#include "ota_image.h"
#include "native_metadata.h"
#include "ota.h"
#include "ota_crypto.h"

_Static_assert(CANVIEW_ESP_IMAGE_CUSTOM_BYTES == CANVIEW_OTA_NATIVE_METADATA_BYTES, "custom metadata size drift");
_Static_assert(sizeof(((esp_app_desc_t *)0)->version) == CANVIEW_OTA_ESP_VERSION_BYTES, "SDK version size drift");
_Static_assert(PSA_KEY_USAGE_VERIFY_MESSAGE == UINT32_C(0x00000800), "PSA verify-message usage drift");

void app_main(void);
void app_main(void)
{
    if (canview_esp_ota_crypto_init(NULL, NULL) != CANVIEW_INVALID_ARGUMENT) { return; }
    if (canview_esp_ota_crypto_close(NULL) != CANVIEW_INVALID_ARGUMENT) { return; }
    if (canview_esp_ota_manifest_verify(NULL, NULL, 0U, NULL) != CANVIEW_INVALID_ARGUMENT) { return; }
    const canview_ota_hash_t hash = canview_esp_ota_hash_provider(NULL);
    if (hash.start(NULL) != CANVIEW_INVALID_ARGUMENT) { return; }
    if (canview_comm_ota_image_check(NULL, NULL) != CANVIEW_INVALID_ARGUMENT) { return; }
    canview_esp_image_info_t info;
    /* NULL negative만 호출한다. 실제 검증 경로의 symbol도 linker가 해석해야 한다. */
    if (canview_esp_image_verify(NULL, 0U, 0U, NULL, &info) != ESP_ERR_INVALID_ARG)
    {
        return;
    }
    if (canview_ota_esp_metadata_check(info.custom, sizeof(info.custom), info.app.version,
            NULL, NULL) != CANVIEW_INVALID_ARGUMENT)
    {
        return;
    }
}
