/* SPDX-License-Identifier: GPL-3.0-only */
/** @file native_metadata.c @brief 기존 CVIMG001 byte 계약의 공통 대조. */
#include <string.h>
#include "native_metadata.h"

enum
{
    META_VERSION = 8, META_SIZE = 10, META_ROLE = 12, META_TARGET = 16,
    META_EPOCH = 20, META_ABI = 24, META_RESERVED = 28, META_SEQUENCE = 32,
    META_BOARD = 40, META_LAYOUT = 104
};

static uint16_t metadata_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8U));
}
static uint32_t metadata_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) | ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U);
}
static bool metadata_text_equal(const uint8_t *wire, size_t size, const char *local)
{
    size_t end = 0U;
    while (end < size && wire[end] != 0U)
    {
        if (wire[end] < 0x20U || wire[end] > 0x7EU || wire[end] != (uint8_t)local[end]) { return false; }
        ++end;
    }
    if (end == 0U || end == size || local[end] != '\0') { return false; }
    for (; end < size; ++end)
    {
        if (wire[end] != 0U) { return false; }
    }
    return true;
}
static bool metadata_target(const canview_ota_identity_t *identity, const canview_ota_image_t *expected)
{
    const bool stm = expected->target == CANVIEW_OTA_TARGET_COMM_STM;
    if (expected->signature != (stm ? CANVIEW_OTA_IMAGE_MCUBOOT_P256 : CANVIEW_OTA_IMAGE_ESP_SECURE_BOOT_V2))
    {
        return false;
    }
    switch (identity->role)
    {
        case CANVIEW_OTA_ROLE_COMMUNICATOR: return stm || expected->target == CANVIEW_OTA_TARGET_COMM_ESP;
        case CANVIEW_OTA_ROLE_CONTROLLER: return expected->target == CANVIEW_OTA_TARGET_CONTROLLER;
        case CANVIEW_OTA_ROLE_BRIDGE: return expected->target == CANVIEW_OTA_TARGET_BRIDGE;
        default: return false;
    }
}
canview_status_t canview_ota_native_metadata_check(const uint8_t *data, size_t size,
    const canview_ota_identity_t *identity, const canview_ota_image_t *expected)
{
    static const uint8_t magic[8] = {'C', 'V', 'I', 'M', 'G', '0', '0', '1'};
    if (data == NULL || identity == NULL || expected == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    if (size != CANVIEW_OTA_NATIVE_METADATA_BYTES) { return CANVIEW_MALFORMED; }
    if (!metadata_target(identity, expected)) { return CANVIEW_AUTH_FAILED; }
    const uint64_t sequence = (uint64_t)metadata_u32(data + META_SEQUENCE) |
        ((uint64_t)metadata_u32(data + META_SEQUENCE + 4U) << 32U);
    return memcmp(data, magic, sizeof(magic)) == 0 && metadata_u16(data + META_VERSION) == 1U &&
        metadata_u16(data + META_SIZE) == CANVIEW_OTA_NATIVE_METADATA_BYTES &&
        metadata_u32(data + META_ROLE) == (uint32_t)identity->role &&
        metadata_u32(data + META_TARGET) == (uint32_t)expected->target &&
        metadata_u32(data + META_EPOCH) == identity->security_epoch &&
        metadata_u32(data + META_ABI) == expected->abi && metadata_u32(data + META_RESERVED) == 0U &&
        sequence == expected->release_sequence &&
        metadata_text_equal(data + META_BOARD, CANVIEW_OTA_TEXT_BYTES, identity->board_revision) &&
        metadata_text_equal(data + META_LAYOUT, CANVIEW_OTA_TEXT_BYTES, identity->layout_id)
        ? CANVIEW_OK : CANVIEW_AUTH_FAILED;
}
canview_status_t canview_ota_esp_metadata_check(const uint8_t *data, size_t size,
    const char version[CANVIEW_OTA_ESP_VERSION_BYTES],
    const canview_ota_identity_t *identity, const canview_ota_image_t *expected)
{
    if (version == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    const canview_status_t status = canview_ota_native_metadata_check(data, size, identity, expected);
    if (status != CANVIEW_OK) { return status; }
    return expected->target != CANVIEW_OTA_TARGET_COMM_STM &&
        metadata_text_equal((const uint8_t *)version, CANVIEW_OTA_ESP_VERSION_BYTES, expected->version)
        ? CANVIEW_OK : CANVIEW_AUTH_FAILED;
}
