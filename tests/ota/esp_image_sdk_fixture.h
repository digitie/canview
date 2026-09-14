/* SPDX-License-Identifier: GPL-3.0-only */
/** @file esp_image_sdk_fixture.h @brief SDK 호출/실패 모형. 실제 암호 검증이 아니다. */
#ifndef CANVIEW_ESP_IMAGE_SDK_FIXTURE_H
#define CANVIEW_ESP_IMAGE_SDK_FIXTURE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef int32_t esp_err_t;
#define ESP_OK (0)
#define ESP_FAIL (-1)
#define ESP_ERR_INVALID_ARG (0x102)
#define ESP_ERR_INVALID_SIZE (0x104)
#define ESP_ERR_INVALID_STATE (0x103)
#define ESP_ERR_NOT_FOUND (0x105)
#define ESP_ERR_NOT_SUPPORTED (0x106)
#define ESP_ERR_NO_MEM (0x101)
#define ESP_ERR_IMAGE_INVALID (0x2002)
#define ESP_PARTITION_TYPE_APP (0)
#define ESP_PARTITION_TYPE_DATA (1)
#define ESP_APP_DESC_MAGIC_WORD (UINT32_C(0xABCD5432))
#define ESP_IMAGE_VERIFY_SILENT (1)
typedef struct { uint32_t unused; } esp_flash_t;
extern esp_flash_t *esp_flash_default_chip;
typedef struct
{
    esp_flash_t *flash_chip;
    uint32_t address;
    uint32_t size;
    uint8_t type;
    uint8_t subtype;
    bool encrypted;
} esp_partition_t;
typedef struct { uint32_t offset; uint32_t size; } esp_partition_pos_t;
typedef struct { uint8_t segment_count; uint8_t hash_appended; uint8_t unused[22]; } esp_image_header_t;
typedef struct { uint32_t load_addr; uint32_t data_len; } esp_image_segment_header_t;
typedef struct
{
    uint32_t start_addr;
    uint32_t image_len;
    uint32_t mmu_page_size;
    esp_image_header_t image;
    esp_image_segment_header_t segments[16];
    uint32_t segment_data[16];
} esp_image_metadata_t;
typedef struct { uint32_t magic_word; uint8_t unused0[12]; char version[32]; uint8_t unused1[208]; } esp_app_desc_t;
const esp_partition_t *esp_partition_find_first(uint8_t type, uint8_t subtype, const char *label);
const esp_partition_t *esp_partition_verify(const esp_partition_t *partition);
bool esp_efuse_is_flash_encryption_enabled(void);
uint32_t esp_image_bootloader_offset_get(void);
esp_err_t bootloader_common_get_sha256_of_partition(uint32_t address, uint32_t size, int type, uint8_t *digest);
esp_err_t esp_image_verify(int mode, const esp_partition_pos_t *position, esp_image_metadata_t *out);
esp_err_t esp_partition_read(const esp_partition_t *partition, size_t offset, void *out, size_t size);
#endif
