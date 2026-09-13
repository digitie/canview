/* SPDX-License-Identifier: GPL-3.0-only */
/** @file manifest.c @brief 고정 schema를 순서대로 읽는 bounded manifest 검사. */
#include <stdbool.h>
#include <string.h>
#include "manifest.h"
#include "cbor_head.h"

#define MANIFEST_FIELDS (12U)
#define IMAGE_FIELDS (7U)
#define COMPATIBILITY_FIELDS (4U)
#define RANGE_FIELDS (2U)
#define CONFIG_FIELDS (3U)
#define REQUIRES_FIELDS (3U)
#define FORMAT_VERSION (1U)
#define STM_IMAGE_MAX (UINT32_C(184320))
#define BRIDGE_IMAGE_MAX (UINT32_C(2621440))
#define ASCII_FIRST (0x20U)
#define ASCII_LAST (0x7EU)

/* 미배포 내부 key 배정. 최종 wire schema와 함께 T-007에서 확정한다. */
typedef enum
{
    MANIFEST_KEY_FORMAT = 0,
    MANIFEST_KEY_PACKAGE = 1,
    MANIFEST_KEY_ROLE = 2,
    MANIFEST_KEY_BOARD = 3,
    MANIFEST_KEY_LAYOUT = 4,
    MANIFEST_KEY_RELEASE = 5,
    MANIFEST_KEY_EPOCH = 6,
    MANIFEST_KEY_KEY_ID = 7,
    MANIFEST_KEY_IMAGES = 8,
    MANIFEST_KEY_COMPATIBILITY = 9,
    MANIFEST_KEY_CONFIG = 10,
    MANIFEST_KEY_REQUIRES = 11
} manifest_key_t;

typedef enum
{
    IMAGE_KEY_TARGET = 0,
    IMAGE_KEY_LENGTH = 1,
    IMAGE_KEY_DIGEST = 2,
    IMAGE_KEY_VERSION = 3,
    IMAGE_KEY_SEQUENCE = 4,
    IMAGE_KEY_SIGNATURE = 5,
    IMAGE_KEY_ABI = 6
} image_key_t;

typedef enum
{
    COMPATIBILITY_KEY_ESP = 0,
    COMPATIBILITY_KEY_STM = 1,
    COMPATIBILITY_KEY_PEER = 2,
    COMPATIBILITY_KEY_COMBINATIONS = 3
} compatibility_key_t;

typedef struct
{
    const uint8_t *data;
    size_t size;
    size_t offset;
    canview_status_t status;
} manifest_cursor_t;

/* Sticky status는 첫 오류를 유지한다. 실패 후 helper는 입력을 더 읽지 않는다. */
static uint64_t manifest_head(manifest_cursor_t *cursor, canview_ota_cbor_type_t type)
{
    canview_ota_cbor_head_t head;
    if (cursor->status != CANVIEW_OK)
    {
        return 0U;
    }
    cursor->status = canview_ota_cbor_read_head(cursor->data + cursor->offset,
                                              cursor->size - cursor->offset, &head);
    if (cursor->status != CANVIEW_OK)
    {
        return 0U;
    }
    if (head.type != type)
    {
        cursor->status = CANVIEW_MALFORMED;
        return 0U;
    }
    cursor->offset += head.encoded_size;
    return head.argument;
}

static void manifest_expect(manifest_cursor_t *cursor, canview_ota_cbor_type_t type,
                            uint64_t expected)
{
    const uint64_t value = manifest_head(cursor, type);
    if (cursor->status == CANVIEW_OK && value != expected)
    {
        cursor->status = CANVIEW_MALFORMED;
    }
}

static uint32_t manifest_u32(manifest_cursor_t *cursor)
{
    const uint64_t value = manifest_head(cursor, CANVIEW_OTA_CBOR_UINT);
    if (value > UINT32_MAX)
    {
        cursor->status = CANVIEW_OVERSIZE;
        return 0U;
    }
    return (uint32_t)value;
}

static void manifest_bytes(manifest_cursor_t *cursor, uint8_t *out, size_t expected)
{
    const uint64_t size = manifest_head(cursor, CANVIEW_OTA_CBOR_BYTES);
    if (cursor->status != CANVIEW_OK)
    {
        return;
    }
    if (size != expected || size > cursor->size - cursor->offset)
    {
        cursor->status = CANVIEW_MALFORMED;
        return;
    }
    (void)memcpy(out, cursor->data + cursor->offset, expected);
    cursor->offset += expected;
}

static void manifest_text(manifest_cursor_t *cursor, char out[CANVIEW_OTA_TEXT_BYTES])
{
    const uint64_t size = manifest_head(cursor, CANVIEW_OTA_CBOR_TEXT);
    if (cursor->status != CANVIEW_OK)
    {
        return;
    }
    if (size == 0U || size >= CANVIEW_OTA_TEXT_BYTES || size > cursor->size - cursor->offset)
    {
        cursor->status = CANVIEW_MALFORMED;
        return;
    }
    for (size_t index = 0U; index < (size_t)size; ++index)
    {
        const uint8_t byte = cursor->data[cursor->offset + index];
        if (byte < ASCII_FIRST || byte > ASCII_LAST)
        {
            cursor->status = CANVIEW_MALFORMED;
            return;
        }
        out[index] = (char)byte;
    }
    out[(size_t)size] = '\0';
    cursor->offset += (size_t)size;
}

static bool manifest_local_text(const char text[CANVIEW_OTA_TEXT_BYTES])
{
    for (size_t index = 0U; index < CANVIEW_OTA_TEXT_BYTES; ++index)
    {
        const uint8_t byte = (uint8_t)text[index];
        if (byte == 0U)
        {
            return index != 0U;
        }
        if (byte < ASCII_FIRST || byte > ASCII_LAST)
        {
            return false;
        }
    }
    return false;
}

static void manifest_range(manifest_cursor_t *cursor, canview_ota_abi_range_t *range)
{
    manifest_expect(cursor, CANVIEW_OTA_CBOR_ARRAY, RANGE_FIELDS);
    range->minimum = manifest_u32(cursor);
    range->maximum = manifest_u32(cursor);
    if (cursor->status == CANVIEW_OK && range->minimum > range->maximum)
    {
        cursor->status = CANVIEW_MALFORMED;
    }
}

static void manifest_image(manifest_cursor_t *cursor, canview_ota_image_t *image)
{
    manifest_expect(cursor, CANVIEW_OTA_CBOR_MAP, IMAGE_FIELDS);
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, IMAGE_KEY_TARGET);
    const uint32_t target = manifest_u32(cursor);
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, IMAGE_KEY_LENGTH);
    image->length = manifest_u32(cursor);
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, IMAGE_KEY_DIGEST);
    manifest_bytes(cursor, image->sha256, sizeof(image->sha256));
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, IMAGE_KEY_VERSION);
    manifest_text(cursor, image->version);
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, IMAGE_KEY_SEQUENCE);
    image->release_sequence = manifest_head(cursor, CANVIEW_OTA_CBOR_UINT);
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, IMAGE_KEY_SIGNATURE);
    const uint32_t signature = manifest_u32(cursor);
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, IMAGE_KEY_ABI);
    image->abi = manifest_u32(cursor);
    if (cursor->status == CANVIEW_OK)
    {
        if (target < CANVIEW_OTA_TARGET_COMM_ESP || target > CANVIEW_OTA_TARGET_BRIDGE ||
            signature < CANVIEW_OTA_IMAGE_ESP_SECURE_BOOT_V2 || signature > CANVIEW_OTA_IMAGE_MCUBOOT_P256)
        {
            cursor->status = CANVIEW_MALFORMED;
        }
        else
        {
            image->target = (canview_ota_target_t)target;
            image->signature = (canview_ota_image_signature_t)signature;
        }
    }
}

static void manifest_compatibility(manifest_cursor_t *cursor, canview_ota_manifest_t *out)
{
    manifest_expect(cursor, CANVIEW_OTA_CBOR_MAP, COMPATIBILITY_FIELDS);
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, COMPATIBILITY_KEY_ESP);
    manifest_range(cursor, &out->esp);
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, COMPATIBILITY_KEY_STM);
    manifest_range(cursor, &out->stm);
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, COMPATIBILITY_KEY_PEER);
    manifest_range(cursor, &out->peer);
    manifest_expect(cursor, CANVIEW_OTA_CBOR_UINT, COMPATIBILITY_KEY_COMBINATIONS);
    const uint64_t count = manifest_head(cursor, CANVIEW_OTA_CBOR_ARRAY);
    if (cursor->status != CANVIEW_OK)
    {
        return;
    }
    if (count > CANVIEW_OTA_COMBINATIONS_MAX)
    {
        cursor->status = CANVIEW_OVERSIZE;
        return;
    }
    out->combination_count = (uint32_t)count;
    for (uint32_t index = 0U; index < out->combination_count && cursor->status == CANVIEW_OK; ++index)
    {
        canview_ota_abi_pair_t *pair = &out->combinations[index];
        manifest_expect(cursor, CANVIEW_OTA_CBOR_ARRAY, RANGE_FIELDS);
        pair->esp = manifest_u32(cursor);
        pair->stm = manifest_u32(cursor);
    }
}

static canview_status_t manifest_decode(const uint8_t *data, size_t size, canview_ota_manifest_t *out)
{
    manifest_cursor_t cursor = {data, size, 0U, CANVIEW_OK};
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_MAP, MANIFEST_FIELDS);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_FORMAT);
    const uint32_t version = manifest_u32(&cursor);
    if (cursor.status == CANVIEW_OK && version != FORMAT_VERSION)
    {
        return CANVIEW_UNSUPPORTED_VERSION;
    }
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_PACKAGE);
    manifest_bytes(&cursor, out->package_id, sizeof(out->package_id));
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_ROLE);
    const uint32_t role = manifest_u32(&cursor);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_BOARD);
    manifest_text(&cursor, out->board_revision);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_LAYOUT);
    manifest_text(&cursor, out->layout_id);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_RELEASE);
    manifest_text(&cursor, out->release);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_EPOCH);
    out->security_epoch = manifest_u32(&cursor);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_KEY_ID);
    out->key_id = manifest_u32(&cursor);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_IMAGES);
    const uint64_t count = manifest_head(&cursor, CANVIEW_OTA_CBOR_ARRAY);
    if (cursor.status != CANVIEW_OK)
    {
        return cursor.status;
    }
    if (count == 0U || count > CANVIEW_OTA_ENVELOPE_IMAGE_MAX ||
        role < CANVIEW_OTA_ROLE_COMMUNICATOR || role > CANVIEW_OTA_ROLE_BRIDGE)
    {
        return CANVIEW_MALFORMED;
    }
    out->role = (canview_ota_role_t)role;
    out->image_count = (uint32_t)count;
    for (uint32_t index = 0U; index < out->image_count && cursor.status == CANVIEW_OK; ++index)
    {
        manifest_image(&cursor, &out->images[index]);
    }
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_COMPATIBILITY);
    manifest_compatibility(&cursor, out);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_CONFIG);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_ARRAY, CONFIG_FIELDS);
    out->config_minimum = manifest_u32(&cursor);
    out->config_maximum = manifest_u32(&cursor);
    out->config_snapshot = manifest_u32(&cursor);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_UINT, MANIFEST_KEY_REQUIRES);
    manifest_expect(&cursor, CANVIEW_OTA_CBOR_ARRAY, REQUIRES_FIELDS);
    out->minimum_bootloader = manifest_u32(&cursor);
    out->minimum_recovery = manifest_u32(&cursor);
    out->hardware_capabilities = manifest_head(&cursor, CANVIEW_OTA_CBOR_UINT);
    if (cursor.status == CANVIEW_OK && cursor.offset != cursor.size)
    {
        return CANVIEW_MALFORMED;
    }
    return cursor.status;
}

static canview_status_t manifest_constraints(canview_ota_manifest_t *out,
                                             const canview_ota_envelope_t *envelope)
{
    uint32_t total = (uint32_t)envelope->images_offset;
    if (out->image_count != envelope->declared_image_count ||
        out->image_count > (out->role == CANVIEW_OTA_ROLE_COMMUNICATOR ? 2U : 1U) ||
        out->config_minimum > out->config_snapshot || out->config_snapshot > out->config_maximum ||
        (out->role == CANVIEW_OTA_ROLE_COMMUNICATOR ? out->combination_count == 0U : out->combination_count != 0U))
    {
        return CANVIEW_MALFORMED;
    }
    for (uint32_t index = 0U; index < out->image_count; ++index)
    {
        canview_ota_image_t *image = &out->images[index];
        const bool stm = image->target == CANVIEW_OTA_TARGET_COMM_STM;
        const uint32_t maximum = stm ? STM_IMAGE_MAX :
            (image->target == CANVIEW_OTA_TARGET_BRIDGE ? BRIDGE_IMAGE_MAX : CANVIEW_OTA_ENVELOPE_IMAGE_BYTES_MAX);
        const canview_ota_abi_range_t *range = stm ? &out->stm : &out->esp;
        if ((out->role == CANVIEW_OTA_ROLE_COMMUNICATOR && image->target != CANVIEW_OTA_TARGET_COMM_ESP && !stm) ||
            (out->role == CANVIEW_OTA_ROLE_CONTROLLER && image->target != CANVIEW_OTA_TARGET_CONTROLLER) ||
            (out->role == CANVIEW_OTA_ROLE_BRIDGE && image->target != CANVIEW_OTA_TARGET_BRIDGE) ||
            image->length == 0U || image->abi < range->minimum || image->abi > range->maximum ||
            image->signature != (stm ? CANVIEW_OTA_IMAGE_MCUBOOT_P256 : CANVIEW_OTA_IMAGE_ESP_SECURE_BOOT_V2))
        {
            return CANVIEW_MALFORMED;
        }
        for (uint32_t previous = 0U; previous < index; ++previous)
        {
            if (image->target == out->images[previous].target)
            {
                return CANVIEW_DUPLICATE;
            }
        }
        if (image->length > maximum || image->length > UINT32_MAX - total)
        {
            return CANVIEW_OVERSIZE;
        }
        image->offset = total;
        total += image->length;
    }
    for (uint32_t index = 0U; index < out->combination_count; ++index)
    {
        const canview_ota_abi_pair_t *pair = &out->combinations[index];
        if (pair->esp < out->esp.minimum || pair->esp > out->esp.maximum ||
            pair->stm < out->stm.minimum || pair->stm > out->stm.maximum)
        {
            return CANVIEW_MALFORMED;
        }
        for (uint32_t previous = 0U; previous < index; ++previous)
        {
            if (pair->esp == out->combinations[previous].esp && pair->stm == out->combinations[previous].stm)
            {
                return CANVIEW_DUPLICATE;
            }
        }
    }
    if (total != envelope->declared_total_size)
    {
        return CANVIEW_MALFORMED;
    }
    out->total_size = total;
    return CANVIEW_OK;
}

canview_status_t canview_ota_manifest_check(
    const uint8_t *prefix, size_t size, const canview_ota_identity_t *identity,
    canview_ota_manifest_verify_fn verify, void *context, canview_ota_manifest_t *out)
{
    canview_ota_envelope_t envelope;
    canview_ota_manifest_t candidate = {0};
    canview_status_t status;
    if (out == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    (void)memset(out, 0, sizeof(*out));
    if (identity == NULL || identity->role < CANVIEW_OTA_ROLE_COMMUNICATOR ||
        identity->role > CANVIEW_OTA_ROLE_BRIDGE || !manifest_local_text(identity->board_revision) ||
        !manifest_local_text(identity->layout_id))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    status = canview_ota_envelope_check(prefix, size, verify, context, &envelope);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    status = manifest_decode(prefix + envelope.manifest_offset, envelope.manifest_size, &candidate);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    if (candidate.role != identity->role || candidate.security_epoch != identity->security_epoch ||
        candidate.key_id != identity->key_id || strcmp(candidate.board_revision, identity->board_revision) != 0 ||
        strcmp(candidate.layout_id, identity->layout_id) != 0)
    {
        return CANVIEW_AUTH_FAILED;
    }
    status = manifest_constraints(&candidate, &envelope);
    if (status == CANVIEW_OK)
    {
        *out = candidate;
    }
    return status;
}
