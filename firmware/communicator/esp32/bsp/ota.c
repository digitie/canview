/* SPDX-License-Identifier: GPL-3.0-only */
/** @file ota.c @brief BSP 고정 partition과 SDK/core 검사의 연결만 담당한다. */
#include <string.h>
#include "ota.h"
#include "board_pins.h"
#include "ota_image.h"
#include "native_metadata.h"

#define STAGING_TYPE (0x40U)
#define STAGING_SUBTYPE (3U)
#define BUNDLE_IMAGE_ALIGNMENT (UINT32_C(65536))

typedef char ota_board_id_fits[(sizeof(CANVIEW_BOARD_ID) <= CANVIEW_OTA_TEXT_BYTES) ? 1 : -1];
typedef char ota_metadata_size_matches[(CANVIEW_ESP_IMAGE_CUSTOM_BYTES == CANVIEW_OTA_NATIVE_METADATA_BYTES) ? 1 : -1];
typedef char ota_version_size_matches[(sizeof(((esp_app_desc_t *)0)->version) == CANVIEW_OTA_ESP_VERSION_BYTES) ? 1 : -1];

canview_status_t canview_comm_ota_image_check(const canview_ota_identity_t *identity,
    const canview_ota_image_t *expected)
{
    if (identity == NULL || expected == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    if (identity->role != CANVIEW_OTA_ROLE_COMMUNICATOR ||
        memcmp(identity->board_revision, CANVIEW_BOARD_ID, sizeof(CANVIEW_BOARD_ID)) != 0 ||
        expected->target != CANVIEW_OTA_TARGET_COMM_ESP ||
        expected->signature != CANVIEW_OTA_IMAGE_ESP_SECURE_BOOT_V2)
    {
        return CANVIEW_AUTH_FAILED;
    }
    /* prefix가 차지하는 첫64KiB에는 native image가 올 수 없다. */
    if (expected->offset < BUNDLE_IMAGE_ALIGNMENT || expected->offset % BUNDLE_IMAGE_ALIGNMENT != 0U)
    {
        return CANVIEW_MALFORMED;
    }
    const esp_partition_t *staging = esp_partition_find_first(STAGING_TYPE, STAGING_SUBTYPE, "bundle_stage");
    if (staging == NULL) { return CANVIEW_INCOMPLETE; }
    if (staging->type != STAGING_TYPE || staging->subtype != STAGING_SUBTYPE ||
        staging->address != CANVIEW_BOARD_OTA_STAGING_OFFSET ||
        staging->size != CANVIEW_BOARD_OTA_STAGING_BYTES)
    {
        return CANVIEW_MALFORMED;
    }
    canview_esp_image_info_t info = {0};
    const esp_err_t error = canview_esp_image_verify(staging, expected->offset,
        expected->length, expected->sha256, &info);
    if (error == ESP_ERR_NO_MEM) { return CANVIEW_RESOURCE_BUSY; }
    if (error == ESP_ERR_NOT_SUPPORTED) { return CANVIEW_NOT_IMPLEMENTED; }
    if (error != ESP_OK) { return CANVIEW_AUTH_FAILED; }
    return canview_ota_esp_metadata_check(info.custom, sizeof(info.custom), info.app.version, identity, expected);
}
