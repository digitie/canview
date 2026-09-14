/* SPDX-License-Identifier: GPL-3.0-only */
/** @file floor.c @brief 영속 policy snapshot의 순수 비교. Flash/부팅 상태를 변경하지 않는다. */
#include <string.h>
#include "floor.h"

#define FLOOR_TEXT_FIRST (0x20U)
#define FLOOR_TEXT_LAST (0x7EU)

static bool floor_text_equal(const char *left, const char *right)
{
    for (size_t index = 0U; index < CANVIEW_OTA_TEXT_BYTES; ++index)
    {
        const uint8_t value = (uint8_t)left[index];
        if (left[index] != right[index]) { return false; }
        if (value == 0U) { return index != 0U; }
        if (value < FLOOR_TEXT_FIRST || value > FLOOR_TEXT_LAST) { return false; }
    }
    return false;
}

static bool floor_target_allowed(canview_ota_role_t role, canview_ota_target_t target)
{
    switch (role)
    {
        case CANVIEW_OTA_ROLE_COMMUNICATOR:
            return target == CANVIEW_OTA_TARGET_COMM_ESP || target == CANVIEW_OTA_TARGET_COMM_STM;
        case CANVIEW_OTA_ROLE_CONTROLLER:
            return target == CANVIEW_OTA_TARGET_CONTROLLER;
        case CANVIEW_OTA_ROLE_BRIDGE:
            return target == CANVIEW_OTA_TARGET_BRIDGE;
        default:
            return false;
    }
}

static canview_status_t floor_records_check(const canview_ota_manifest_t *manifest,
                                           const canview_ota_floor_t *floor)
{
    if (floor->count == 0U) { return CANVIEW_INCOMPLETE; }
    if (floor->count > CANVIEW_OTA_FLOOR_TARGET_MAX || manifest->image_count == 0U ||
        manifest->image_count > CANVIEW_OTA_FLOOR_TARGET_MAX)
    {
        return CANVIEW_MALFORMED;
    }
    for (uint32_t index = 0U; index < floor->count; ++index)
    {
        const canview_ota_floor_record_t *record = &floor->records[index];
        if (!floor_target_allowed(manifest->role, record->target) ||
            (record->installed != CANVIEW_OTA_INSTALLED_UNKNOWN &&
             record->installed != CANVIEW_OTA_INSTALLED_BOOTABLE &&
             record->installed != CANVIEW_OTA_INSTALLED_DAMAGED))
        {
            return CANVIEW_MALFORMED;
        }
        if (index != 0U && floor->records[0].target == record->target) { return CANVIEW_MALFORMED; }
    }
    for (uint32_t index = 0U; index < manifest->image_count; ++index)
    {
        if (!floor_target_allowed(manifest->role, manifest->images[index].target) ||
            (index != 0U && manifest->images[0].target == manifest->images[index].target))
        {
            return CANVIEW_MALFORMED;
        }
    }
    return CANVIEW_OK;
}

static canview_status_t floor_image_check(const canview_ota_image_t *image,
    const canview_ota_floor_record_t *record, canview_ota_floor_action_t *action)
{
    if (image->release_sequence < record->minimum_sequence) { return CANVIEW_STALE; }
    if (image->release_sequence > record->minimum_sequence)
    {
        *action = CANVIEW_OTA_FLOOR_UPGRADE;
        return CANVIEW_OK;
    }
    if (memcmp(image->sha256, record->confirmed_digest, CANVIEW_OTA_DIGEST_BYTES) != 0)
    {
        return CANVIEW_AUTH_FAILED;
    }
    if (record->installed == CANVIEW_OTA_INSTALLED_DAMAGED)
    {
        *action = CANVIEW_OTA_FLOOR_REPAIR_REQUIRED;
        return CANVIEW_OK;
    }
    if (record->installed != CANVIEW_OTA_INSTALLED_BOOTABLE ||
        record->observed_sequence != record->minimum_sequence ||
        memcmp(record->observed_digest, record->confirmed_digest, CANVIEW_OTA_DIGEST_BYTES) != 0)
    {
        return CANVIEW_INCOMPLETE;
    }
    *action = CANVIEW_OTA_FLOOR_ALREADY_INSTALLED;
    return CANVIEW_OK;
}

canview_status_t canview_ota_floor_check(const canview_ota_manifest_t *manifest,
    const canview_ota_floor_t *floor, canview_ota_floor_result_t *out)
{
    canview_ota_floor_result_t result = {0};
    if (out == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    (void)memset(out, 0, sizeof(*out));
    if (manifest == NULL || floor == NULL) { return CANVIEW_INVALID_ARGUMENT; }
    if (!floor->ready) { return CANVIEW_INCOMPLETE; }
    if (manifest->role != floor->identity.role || manifest->security_epoch != floor->identity.security_epoch ||
        !floor_text_equal(manifest->board_revision, floor->identity.board_revision) ||
        !floor_text_equal(manifest->layout_id, floor->identity.layout_id))
    {
        return CANVIEW_AUTH_FAILED;
    }
    const canview_status_t status = floor_records_check(manifest, floor);
    if (status != CANVIEW_OK) { return status; }
    for (uint32_t index = 0U; index < manifest->image_count; ++index)
    {
        const canview_ota_image_t *image = &manifest->images[index];
        const canview_ota_floor_record_t *record = NULL;
        for (uint32_t local = 0U; local < floor->count; ++local)
        {
            if (floor->records[local].target == image->target) { record = &floor->records[local]; }
        }
        if (record == NULL) { return CANVIEW_INCOMPLETE; }
        const canview_status_t checked = floor_image_check(image, record, &result.images[index]);
        if (checked != CANVIEW_OK) { return checked; }
    }
    *out = result;
    return CANVIEW_OK;
}
