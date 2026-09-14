/* SPDX-License-Identifier: GPL-3.0-only */
/** @file test_native_metadata.c @brief 공통 signed metadata의 경계/전 byte 변이 시험. */
#include <stdio.h>
#include <string.h>
#include "native_metadata.h"

#define CHECK(c) do { if (!(c)) { (void)fprintf(stderr, "metadata line %d\n", __LINE__); return 1; } } while (0)
static void put32(uint8_t *out, uint32_t value)
{
    for (size_t index = 0U; index < 4U; ++index) { out[index] = (uint8_t)(value >> (index * 8U)); }
}
static void encode(uint8_t *out, const canview_ota_identity_t *identity, const canview_ota_image_t *expected)
{
    (void)memset(out, 0, CANVIEW_OTA_NATIVE_METADATA_BYTES);
    (void)memcpy(out, "CVIMG001", 8U);
    out[8] = 1U;
    out[10] = CANVIEW_OTA_NATIVE_METADATA_BYTES;
    put32(out + 12U, (uint32_t)identity->role);
    put32(out + 16U, (uint32_t)expected->target);
    put32(out + 20U, identity->security_epoch);
    put32(out + 24U, expected->abi);
    put32(out + 32U, (uint32_t)expected->release_sequence);
    put32(out + 36U, (uint32_t)(expected->release_sequence >> 32U));
    (void)memcpy(out + 40U, identity->board_revision, 64U);
    (void)memcpy(out + 104U, identity->layout_id, 64U);
}
int main(void)
{
    canview_ota_identity_t identity = {CANVIEW_OTA_ROLE_COMMUNICATOR, "board", "layout", UINT32_MAX, 1U};
    canview_ota_image_t expected = {0};
    uint8_t data[CANVIEW_OTA_NATIVE_METADATA_BYTES + 1U];
    char version[CANVIEW_OTA_ESP_VERSION_BYTES] = "1.2.3+4";
    expected.target = CANVIEW_OTA_TARGET_COMM_ESP;
    expected.signature = CANVIEW_OTA_IMAGE_ESP_SECURE_BOOT_V2;
    expected.abi = UINT32_MAX;
    expected.release_sequence = UINT64_MAX;
    (void)memcpy(expected.version, version, sizeof(version));
    encode(data, &identity, &expected);
    CHECK(canview_ota_native_metadata_check(NULL, 168U, &identity, &expected) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_native_metadata_check(data, 168U, NULL, &expected) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_native_metadata_check(data, 168U, &identity, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_esp_metadata_check(data, 168U, NULL, &identity, &expected) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_esp_metadata_check(data, 168U, version, NULL, &expected) == CANVIEW_INVALID_ARGUMENT);
    for (size_t size = 0U; size <= sizeof(data); ++size)
    {
        CHECK(canview_ota_native_metadata_check(data, size, &identity, &expected) ==
            (size == 168U ? CANVIEW_OK : CANVIEW_MALFORMED));
    }
    CHECK(canview_ota_native_metadata_check(data, SIZE_MAX, &identity, &expected) == CANVIEW_MALFORMED);
    /* 모든 byte의 각 bit: magic/enum/정수/문자열/padding까지 전부 검증 대상이다. */
    for (size_t offset = 0U; offset < 168U; ++offset)
    {
        for (uint32_t bit = 0U; bit < 8U; ++bit)
        {
            const uint8_t mask = (uint8_t)(1U << bit);
            data[offset] ^= mask;
            CHECK(canview_ota_esp_metadata_check(data, 168U, version, &identity, &expected) == CANVIEW_AUTH_FAILED);
            data[offset] ^= mask;
        }
    }
    for (uint32_t role = 0U; role <= 4U; ++role)
    {
        identity.role = (canview_ota_role_t)role;
        for (uint32_t target = 0U; target <= 5U; ++target)
        {
            expected.target = (canview_ota_target_t)target;
            for (uint32_t signature = 0U; signature <= 3U; ++signature)
            {
                expected.signature = (canview_ota_image_signature_t)signature;
                encode(data, &identity, &expected);
                const bool allowed = ((role == 1U && (target == 1U || target == 2U)) ||
                    (role == 2U && target == 3U) || (role == 3U && target == 4U)) &&
                    signature == (target == 2U ? 2U : 1U);
                CHECK(canview_ota_native_metadata_check(data, 168U, &identity, &expected) ==
                    (allowed ? CANVIEW_OK : CANVIEW_AUTH_FAILED));
                CHECK(canview_ota_esp_metadata_check(data, 168U, version, &identity, &expected) ==
                    (allowed && target != 2U ? CANVIEW_OK : CANVIEW_AUTH_FAILED));
            }
        }
    }
    identity.role = CANVIEW_OTA_ROLE_BRIDGE;
    expected.target = CANVIEW_OTA_TARGET_BRIDGE;
    expected.signature = CANVIEW_OTA_IMAGE_ESP_SECURE_BOOT_V2;
    const uint64_t sequences[] = {0U, 1U, UINT32_MAX, UINT64_C(4294967296), UINT64_C(9007199254740993), UINT64_MAX};
    for (size_t index = 0U; index < sizeof(sequences) / sizeof(sequences[0]); ++index)
    {
        expected.release_sequence = sequences[index];
        encode(data, &identity, &expected);
        CHECK(canview_ota_esp_metadata_check(data, 168U, version, &identity, &expected) == CANVIEW_OK);
        expected.release_sequence ^= UINT64_C(1) << 63U;
        CHECK(canview_ota_esp_metadata_check(data, 168U, version, &identity, &expected) == CANVIEW_AUTH_FAILED);
    }
    /* wire/local이 함께 malformed여도 허용하지 않는다. */
    for (size_t length = 0U; length <= 64U; ++length)
    {
        (void)memset(identity.board_revision, 0, 64U);
        (void)memset(identity.board_revision, 'B', length);
        (void)memcpy(identity.layout_id, identity.board_revision, 64U);
        encode(data, &identity, &expected);
        CHECK(canview_ota_native_metadata_check(data, 168U, &identity, &expected) ==
            (length > 0U && length < 64U ? CANVIEW_OK : CANVIEW_AUTH_FAILED));
    }
    (void)memset(identity.board_revision, 0, 64U);
    (void)memcpy(identity.board_revision, "board", 5U);
    (void)memset(identity.layout_id, 0, 64U);
    (void)memcpy(identity.layout_id, "layout", 6U);
    encode(data, &identity, &expected);
    for (size_t length = 0U; length <= 32U; ++length)
    {
        (void)memset(version, 0, sizeof(version));
        (void)memset(version, 'V', length);
        (void)memset(expected.version, 0, sizeof(expected.version));
        (void)memcpy(expected.version, version, sizeof(version));
        CHECK(canview_ota_esp_metadata_check(data, 168U, version, &identity, &expected) ==
            (length > 0U && length < 32U ? CANVIEW_OK : CANVIEW_AUTH_FAILED));
    }
    (void)memset(version, 0, sizeof(version));
    (void)memset(expected.version, 0, sizeof(expected.version));
    version[0] = 'V'; expected.version[0] = 'V';
    for (uint32_t value = 1U; value <= 255U; ++value)
    {
        version[0] = (char)(uint8_t)value; expected.version[0] = version[0];
        CHECK(canview_ota_esp_metadata_check(data, 168U, version, &identity, &expected) ==
            (value >= 0x20U && value <= 0x7EU ? CANVIEW_OK : CANVIEW_AUTH_FAILED));
    }
    version[0] = 'V'; expected.version[0] = 'V';
    version[31] = 'X';
    CHECK(canview_ota_esp_metadata_check(data, 168U, version, &identity, &expected) == CANVIEW_AUTH_FAILED);
    version[31] = 0; expected.version[1] = 'X';
    CHECK(canview_ota_esp_metadata_check(data, 168U, version, &identity, &expected) == CANVIEW_AUTH_FAILED);
    expected.version[1] = 0; version[0] = 'X';
    CHECK(canview_ota_esp_metadata_check(data, 168U, version, &identity, &expected) == CANVIEW_AUTH_FAILED);
    (void)puts("PASS: native metadata boundaries, all-bit mutations, role matrix, u64 and ESP version");
    return 0;
}
