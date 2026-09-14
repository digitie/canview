/* SPDX-License-Identifier: GPL-3.0-only */
/** @file main.c @brief SDK 실제 compile/link 전용. Flash/provisioning을 실행하지 않는다. */
#include "ota_image.h"
#include "native_metadata.h"

_Static_assert(CANVIEW_ESP_IMAGE_CUSTOM_BYTES == CANVIEW_OTA_NATIVE_METADATA_BYTES, "custom metadata size drift");
_Static_assert(sizeof(((esp_app_desc_t *)0)->version) == CANVIEW_OTA_ESP_VERSION_BYTES, "SDK version size drift");

void app_main(void);
void app_main(void)
{
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
