/* SPDX-License-Identifier: GPL-3.0-only */
/** @file test_esp_image_sdk.c @brief 실제 adapter에 SDK fault를 주입하는 host 시험. */
#include <stdio.h>
#include <string.h>
#include "ota_image.h"

#define CHECK(c) do { if (!(c)) { (void)fprintf(stderr, "ESP image SDK line %d\n", __LINE__); return 1; } } while (0)
static esp_flash_t internal_chip;
esp_flash_t *esp_flash_default_chip = &internal_chip;
static esp_partition_t partition;
static esp_partition_pos_t expected_position;
static uint32_t fault;
static uint32_t calls;
static bool encrypted;
static bool correct;

const esp_partition_t *esp_partition_verify(const esp_partition_t *input)
{
    correct = correct && calls++ == 0U && input == &partition;
    return fault == 1U ? NULL : &partition;
}
bool esp_efuse_is_flash_encryption_enabled(void) { return encrypted; }
uint32_t esp_image_bootloader_offset_get(void) { return fault == 15U ? expected_position.offset : UINT32_MAX; }
esp_err_t bootloader_common_get_sha256_of_partition(uint32_t address, uint32_t size, int type, uint8_t *digest)
{
    correct = correct && calls++ == 1U && address == expected_position.offset &&
        size == expected_position.size && type == ESP_PARTITION_TYPE_DATA;
    (void)memset(digest, fault == 3U ? 0xAA : 0x55, 32U);
    return fault == 2U ? ESP_ERR_NO_MEM : ESP_OK;
}
esp_err_t esp_image_verify(int mode, const esp_partition_pos_t *position, esp_image_metadata_t *out)
{
    correct = correct && calls++ == 2U && mode == ESP_IMAGE_VERIFY_SILENT &&
        position->offset == expected_position.offset && position->size == expected_position.size;
    out->start_addr = position->offset;
    out->image_len = position->size;
    out->mmu_page_size = 65536U;
    out->image.segment_count = 1U;
    out->image.hash_appended = 1U;
    out->segment_data[0] = position->offset + 32U;
    out->segments[0].data_len = 424U;
    switch (fault)
    {
        case 5U: ++out->start_addr; break;
        case 6U: --out->image_len; break;
        case 7U: out->image.segment_count = 0U; break;
        case 8U: out->image.hash_appended = 0U; break;
        case 9U: out->mmu_page_size = 32768U; break;
        case 10U: ++out->segment_data[0]; break;
        case 11U: --out->segments[0].data_len; break;
        default: break;
    }
    return fault == 4U ? ESP_ERR_IMAGE_INVALID : ESP_OK;
}
esp_err_t esp_partition_read(const esp_partition_t *source, size_t offset, void *out, size_t size)
{
    correct = correct && source == &partition;
    (void)memset(out, 0x5A, size);
    if (calls++ == 3U)
    {
        correct = correct && offset == expected_position.offset - partition.address + 32U && size == 256U;
        ((esp_app_desc_t *)out)->magic_word = fault == 14U ? 0U : ESP_APP_DESC_MAGIC_WORD;
        return fault == 12U ? ESP_FAIL : ESP_OK;
    }
    correct = correct && calls == 5U && offset == expected_position.offset - partition.address + 288U && size == 168U;
    return fault == 13U ? ESP_FAIL : ESP_OK;
}
static void reset_fixture(void)
{
    partition = (esp_partition_t){&internal_chip, 0xA40000U, 4718592U, 0x40U, 3U, false};
    expected_position = (esp_partition_pos_t){0xA50000U, 8192U};
    fault = 0U;
    calls = 0U;
    encrypted = false;
    correct = true;
}
static bool cleared(const canview_esp_image_info_t *out)
{
    const uint8_t *bytes = (const uint8_t *)out;
    for (size_t index = 0U; index < sizeof(*out); ++index)
    {
        if (bytes[index] != 0U) { return false; }
    }
    return true;
}
int main(void)
{
    canview_esp_image_info_t out;
    uint8_t digest[32];
    (void)memset(digest, 0x55, sizeof(digest));
    reset_fixture();
    CHECK(canview_esp_image_verify(&partition, 65536U, 8192U, digest, NULL) == ESP_ERR_INVALID_ARG);
    (void)memset(&out, 0xA5, sizeof(out));
    CHECK(canview_esp_image_verify(NULL, 0U, 0U, digest, &out) == ESP_ERR_INVALID_ARG && cleared(&out));
    CHECK(canview_esp_image_verify(&partition, 0U, 0U, NULL, &out) == ESP_ERR_INVALID_ARG && cleared(&out));
#if !defined(CONFIG_SECURE_SIGNED_ON_UPDATE) || !CONFIG_SECURE_SIGNED_ON_UPDATE || \
    !defined(CONFIG_SECURE_SIGNED_APPS_RSA_SCHEME) || !CONFIG_SECURE_SIGNED_APPS_RSA_SCHEME || defined(CONFIG_IDF_ENV_FPGA)
    CHECK(canview_esp_image_verify(&partition, 65536U, 8192U, digest, &out) == ESP_ERR_NOT_SUPPORTED);
    CHECK(calls == 0U && cleared(&out));
#else
    esp_flash_t external_chip;
    for (uint32_t failure = 0U; failure <= 15U; ++failure)
    {
        reset_fixture();
        fault = failure;
        const esp_err_t result = canview_esp_image_verify(&partition, 65536U, 8192U, digest, &out);
        const esp_err_t expected = failure == 0U ? ESP_OK : failure == 1U ? ESP_ERR_NOT_FOUND :
            failure == 2U ? ESP_ERR_NO_MEM : failure == 15U ? ESP_ERR_INVALID_STATE :
            (failure == 12U || failure == 13U) ? ESP_FAIL : ESP_ERR_IMAGE_INVALID;
        CHECK(result == expected && correct);
        if (failure == 0U) { CHECK(calls == 5U && out.app.magic_word == ESP_APP_DESC_MAGIC_WORD && out.custom[167] == 0x5AU); }
        else { CHECK(cleared(&out)); }
    }
    const uint32_t lengths[] = {0U, 1U, 4096U, 8191U, 8193U, 4194305U, UINT32_MAX};
    for (size_t index = 0U; index < sizeof(lengths) / sizeof(lengths[0]); ++index)
    {
        reset_fixture();
        CHECK(canview_esp_image_verify(&partition, 65536U, lengths[index], digest, &out) == ESP_ERR_INVALID_SIZE);
        CHECK(calls == 1U && correct && cleared(&out));
    }
    const uint32_t offsets[] = {1U, 4096U, 65535U, 65537U, 4718592U, 4784128U, UINT32_MAX};
    for (size_t index = 0U; index < sizeof(offsets) / sizeof(offsets[0]); ++index)
    {
        reset_fixture();
        CHECK(canview_esp_image_verify(&partition, offsets[index], 8192U, digest, &out) == ESP_ERR_INVALID_SIZE);
        CHECK(calls == 1U && cleared(&out));
    }
    for (uint32_t mutation = 0U; mutation < 9U; ++mutation)
    {
        reset_fixture();
        esp_err_t expected = ESP_ERR_INVALID_SIZE;
        switch (mutation)
        {
            case 0U: partition.flash_chip = &external_chip; expected = ESP_ERR_NOT_SUPPORTED; break;
            case 1U: partition.type = 1U; expected = ESP_ERR_NOT_SUPPORTED; break;
            case 2U: partition.subtype = 4U; expected = ESP_ERR_NOT_SUPPORTED; break;
            case 3U: partition.address = 0U; break;
            case 4U: ++partition.address; break;
            case 5U: partition.size = 65535U; break;
            case 6U: partition.address = 0xFFFF0000U; break;
            case 7U: partition.address = 0xFFFE0000U; expected_position.size = 131072U; break;
            default: encrypted = true; expected = ESP_ERR_INVALID_STATE; break;
        }
        CHECK(canview_esp_image_verify(&partition, 65536U, expected_position.size, digest, &out) == expected);
        CHECK(calls == 1U && cleared(&out));
    }
    for (uint32_t mode = 0U; mode < 4U; ++mode)
    {
        reset_fixture();
        partition.type = mode < 2U ? ESP_PARTITION_TYPE_APP : 0x40U;
        partition.encrypted = (mode & 1U) != 0U;
        encrypted = partition.encrypted;
        expected_position.offset = partition.address;
        expected_position.size = 4194304U;
        CHECK(canview_esp_image_verify(&partition, 0U, expected_position.size, digest, &out) == ESP_OK);
        CHECK(calls == 5U && correct);
    }
#endif
    (void)puts("PASS: ESP image SDK call/bounds/failure model; no actual signature or hardware execution");
    return 0;
}
