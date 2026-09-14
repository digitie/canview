/* SPDX-License-Identifier: GPL-3.0-only */
/** @file ota_image.c @brief ESP-IDF6.0.3 native verifier를 재사용하는 BSP adapter. */
#include <string.h>
#include "ota_image.h"
#if !defined(CANVIEW_ESP_IMAGE_SDK_TEST)
#include "sdkconfig.h"
#include "bootloader_common.h"
#include "esp_flash.h"
#include "esp_efuse.h"
#include "esp_image_format.h"
#endif

#define IMAGE_ALIGNMENT (UINT32_C(65536))
#define IMAGE_SECTOR_BYTES (UINT32_C(4096))
#define IMAGE_MAX_BYTES (UINT32_C(4194304))
#define IMAGE_STAGE_TYPE (0x40U)
#define IMAGE_STAGE_SUBTYPE (3U)

esp_err_t canview_esp_image_verify(const esp_partition_t *partition, uint32_t offset,
    uint32_t size, const uint8_t digest[CANVIEW_ESP_IMAGE_DIGEST_BYTES], canview_esp_image_info_t *out)
{
    if (out == NULL) { return ESP_ERR_INVALID_ARG; }
    (void)memset(out, 0, sizeof(*out));
    if (partition == NULL || digest == NULL) { return ESP_ERR_INVALID_ARG; }
#if !defined(CONFIG_SECURE_SIGNED_ON_UPDATE) || !CONFIG_SECURE_SIGNED_ON_UPDATE || \
    !defined(CONFIG_SECURE_SIGNED_APPS_RSA_SCHEME) || !CONFIG_SECURE_SIGNED_APPS_RSA_SCHEME || \
    defined(CONFIG_IDF_ENV_FPGA) || defined(CONFIG_IDF_ENV_BRINGUP)
    (void)offset;
    (void)size;
    return ESP_ERR_NOT_SUPPORTED;
#else
    const esp_partition_t *source = esp_partition_verify(partition);
    if (source == NULL) { return ESP_ERR_NOT_FOUND; }
    if (source->flash_chip != esp_flash_default_chip ||
        (source->type != ESP_PARTITION_TYPE_APP &&
         !(source->type == IMAGE_STAGE_TYPE && source->subtype == IMAGE_STAGE_SUBTYPE)))
    {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (source->address < IMAGE_ALIGNMENT || source->address % IMAGE_ALIGNMENT != 0U ||
        offset % IMAGE_ALIGNMENT != 0U || offset > source->size || size > source->size - offset ||
        size < 2U * IMAGE_SECTOR_BYTES || size > IMAGE_MAX_BYTES || size % IMAGE_SECTOR_BYTES != 0U ||
        offset > UINT32_MAX - source->address || size > UINT32_MAX - source->address - offset)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    if (esp_efuse_is_flash_encryption_enabled() && !source->encrypted)
    {
        /* SDK bootloader reader는 partition flag와 무관하게 암호화 Flash를 복호화한다. */
        return ESP_ERR_INVALID_STATE;
    }
    const esp_partition_pos_t position = {.offset = source->address + offset, .size = size};
    if (position.offset == esp_image_bootloader_offset_get()) { return ESP_ERR_INVALID_STATE; }
    uint8_t actual[CANVIEW_ESP_IMAGE_DIGEST_BYTES];
    /* APP 모드 hash는 appended hash만 반환한다. DATA 모드로 signature sector까지 읽는다. */
    esp_err_t error = bootloader_common_get_sha256_of_partition(position.offset, size, ESP_PARTITION_TYPE_DATA, actual);
    if (error != ESP_OK) { return error; }
    if (memcmp(actual, digest, sizeof(actual)) != 0) { return ESP_ERR_IMAGE_INVALID; }
    esp_image_metadata_t metadata = {0};
    error = esp_image_verify(ESP_IMAGE_VERIFY_SILENT, &position, &metadata);
    if (error != ESP_OK) { return error; }
    const uint32_t descriptor_offset = (uint32_t)(sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t));
    if (metadata.start_addr != position.offset || metadata.image_len != size ||
        metadata.image.segment_count == 0U || metadata.image.hash_appended != 1U ||
        metadata.mmu_page_size != IMAGE_ALIGNMENT ||
        metadata.segment_data[0] != position.offset + descriptor_offset ||
        metadata.segments[0].data_len < sizeof(out->app) + sizeof(out->custom))
    {
        return ESP_ERR_IMAGE_INVALID;
    }
    error = esp_partition_read(source, offset + descriptor_offset, &out->app, sizeof(out->app));
    if (error == ESP_OK)
    {
        error = esp_partition_read(source, offset + descriptor_offset + sizeof(out->app), out->custom, sizeof(out->custom));
    }
    if (error == ESP_OK && out->app.magic_word != ESP_APP_DESC_MAGIC_WORD)
    {
        error = ESP_ERR_IMAGE_INVALID;
    }
    if (error != ESP_OK) { (void)memset(out, 0, sizeof(*out)); }
    return error;
#endif
}
