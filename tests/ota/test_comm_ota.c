/* SPDX-License-Identifier: GPL-3.0-only */
/** @file test_comm_ota.c @brief BSP partition 선택→SDK→실제 metadata 검사 연결 모형. */
#include <stdio.h>
#include <string.h>
#include "ota.h"
#include "ota_image.h"
#include "board_pins.h"

#define CHECK(c) do { if (!(c)) { (void)fprintf(stderr, "comm OTA line %d\n", __LINE__); return 1; } } while (0)
static esp_partition_t staging;
static canview_esp_image_info_t signed_info;
static esp_err_t sdk_error;
static bool missing;
static bool correct;
static uint32_t calls;
static uint32_t sdk_calls;

const esp_partition_t *esp_partition_find_first(uint8_t type, uint8_t subtype, const char *label)
{
    correct = correct && type == 0x40U && subtype == 3U && strcmp(label, "bundle_stage") == 0;
    ++calls;
    return missing ? NULL : &staging;
}
esp_err_t canview_esp_image_verify(const esp_partition_t *partition, uint32_t offset, uint32_t size,
    const uint8_t digest[CANVIEW_ESP_IMAGE_DIGEST_BYTES], canview_esp_image_info_t *out)
{
    ++sdk_calls;
    correct = correct && partition == &staging && offset == 65536U && size == 8192U;
    for (size_t index = 0U; index < 32U; ++index) { correct = correct && digest[index] == 0x55U; }
    *out = signed_info;
    return sdk_error;
}
static void put32(uint8_t *out, uint32_t value)
{
    for (size_t index = 0U; index < 4U; ++index) { out[index] = (uint8_t)(value >> (index * 8U)); }
}
static void fixture(canview_ota_identity_t *identity, canview_ota_image_t *expected)
{
    *identity = (canview_ota_identity_t){CANVIEW_OTA_ROLE_COMMUNICATOR, CANVIEW_BOARD_ID, "synthetic-layout", 7U, 1U};
    *expected = (canview_ota_image_t){0};
    expected->target = CANVIEW_OTA_TARGET_COMM_ESP;
    expected->signature = CANVIEW_OTA_IMAGE_ESP_SECURE_BOOT_V2;
    expected->offset = 65536U; expected->length = 8192U; expected->abi = 2U;
    expected->release_sequence = UINT64_MAX;
    (void)memset(expected->sha256, 0x55, 32U);
    (void)memcpy(expected->version, "1.2.3+4", 8U);
    staging = (esp_partition_t){NULL, CANVIEW_BOARD_OTA_STAGING_OFFSET, CANVIEW_BOARD_OTA_STAGING_BYTES, 0x40U, 3U, true};
    signed_info = (canview_esp_image_info_t){0};
    (void)memcpy(signed_info.app.version, expected->version, 32U);
    (void)memcpy(signed_info.custom, "CVIMG001", 8U);
    signed_info.custom[8] = 1U; signed_info.custom[10] = 168U;
    put32(signed_info.custom + 12U, 1U); put32(signed_info.custom + 16U, 1U);
    put32(signed_info.custom + 20U, 7U); put32(signed_info.custom + 24U, 2U);
    put32(signed_info.custom + 32U, UINT32_MAX); put32(signed_info.custom + 36U, UINT32_MAX);
    (void)memcpy(signed_info.custom + 40U, identity->board_revision, 64U);
    (void)memcpy(signed_info.custom + 104U, identity->layout_id, 64U);
    sdk_error = ESP_OK; missing = false; correct = true; calls = 0U; sdk_calls = 0U;
}
int main(void)
{
    canview_ota_identity_t identity;
    canview_ota_image_t expected;
    fixture(&identity, &expected);
    CHECK(canview_comm_ota_image_check(NULL, &expected) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_comm_ota_image_check(&identity, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(calls == 0U && sdk_calls == 0U);
    CHECK(canview_comm_ota_image_check(&identity, &expected) == CANVIEW_OK && correct && calls == 1U && sdk_calls == 1U);
    for (uint32_t fault = 0U; fault < 11U; ++fault)
    {
        fixture(&identity, &expected);
        canview_status_t status = CANVIEW_AUTH_FAILED;
        switch (fault)
        {
            case 0U: identity.role = CANVIEW_OTA_ROLE_BRIDGE; break;
            case 1U: identity.board_revision[0] ^= 1; break;
            case 2U: expected.target = CANVIEW_OTA_TARGET_COMM_STM; break;
            case 3U: expected.signature = CANVIEW_OTA_IMAGE_MCUBOOT_P256; break;
            case 4U: expected.offset = 0U; status = CANVIEW_MALFORMED; break;
            case 5U: ++expected.offset; status = CANVIEW_MALFORMED; break;
            case 6U: missing = true; status = CANVIEW_INCOMPLETE; break;
            case 7U: ++staging.type; status = CANVIEW_MALFORMED; break;
            case 8U: ++staging.subtype; status = CANVIEW_MALFORMED; break;
            case 9U: ++staging.address; status = CANVIEW_MALFORMED; break;
            default: --staging.size; status = CANVIEW_MALFORMED; break;
        }
        CHECK(canview_comm_ota_image_check(&identity, &expected) == status && sdk_calls == 0U && correct);
    }
    const esp_err_t errors[] = {ESP_ERR_NO_MEM, ESP_ERR_NOT_SUPPORTED, ESP_FAIL, ESP_ERR_IMAGE_INVALID};
    for (size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); ++index)
    {
        fixture(&identity, &expected);
        sdk_error = errors[index];
        const canview_status_t status = index == 0U ? CANVIEW_RESOURCE_BUSY : index == 1U ? CANVIEW_NOT_IMPLEMENTED : CANVIEW_AUTH_FAILED;
        CHECK(canview_comm_ota_image_check(&identity, &expected) == status && correct && sdk_calls == 1U);
    }
    for (size_t index = 0U; index < 168U; ++index)
    {
        fixture(&identity, &expected);
        signed_info.custom[index] ^= 1U;
        CHECK(canview_comm_ota_image_check(&identity, &expected) == CANVIEW_AUTH_FAILED && sdk_calls == 1U && correct);
    }
    fixture(&identity, &expected);
    signed_info.app.version[0] ^= 1;
    CHECK(canview_comm_ota_image_check(&identity, &expected) == CANVIEW_AUTH_FAILED && sdk_calls == 1U);
    fixture(&identity, &expected);
    CHECK(canview_comm_ota_image_check(&identity, &expected) == CANVIEW_OK && correct);
    (void)puts("PASS: Communicator fixed staging/SDK status/metadata integration model; no actual RSA or Flash writes");
    return 0;
}
