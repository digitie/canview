/* SPDX-License-Identifier: GPL-3.0-only */
/** @file envelope.c @brief OTA 수신 prefix 검사. Flash writer를 호출하지 않는다. */
#include <string.h>
#include "envelope.h"
#include "cbor_document.h"

#define ENVELOPE_VERSION_OFFSET (8U)
#define ENVELOPE_HEADER_SIZE_OFFSET (10U)
#define ENVELOPE_MANIFEST_SIZE_OFFSET (12U)
#define ENVELOPE_SIGNATURE_SIZE_OFFSET (16U)
#define ENVELOPE_IMAGE_COUNT_OFFSET (18U)
#define ENVELOPE_TOTAL_SIZE_OFFSET (20U)
#define ENVELOPE_VERSION (1U)
#define BYTE_BITS (8U)

static uint16_t envelope_u16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << BYTE_BITS));
}

static uint32_t envelope_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << BYTE_BITS) |
           ((uint32_t)bytes[2] << (2U * BYTE_BITS)) |
           ((uint32_t)bytes[3] << (3U * BYTE_BITS));
}

canview_status_t canview_ota_envelope_check(
    const uint8_t *prefix, size_t size, canview_ota_manifest_verify_fn verify,
    void *context, canview_ota_envelope_t *out)
{
    static const uint8_t magic[] = {'C', 'V', 'O', 'T', 'A', '0', '0', '1'};
    canview_ota_envelope_t candidate = {0};
    canview_status_t status;
    uint32_t manifest_size;
    size_t expected_size;

    if (out == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    (void)memset(out, 0, sizeof(*out));
    if (prefix == NULL || verify == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (size > CANVIEW_OTA_ENVELOPE_PREFIX_MAX)
    {
        return CANVIEW_OVERSIZE;
    }
    if (size < CANVIEW_OTA_ENVELOPE_HEADER_BYTES)
    {
        return CANVIEW_INCOMPLETE;
    }
    if (memcmp(prefix, magic, sizeof(magic)) != 0)
    {
        return CANVIEW_MALFORMED;
    }
    if (envelope_u16(prefix + ENVELOPE_VERSION_OFFSET) != ENVELOPE_VERSION)
    {
        return CANVIEW_UNSUPPORTED_VERSION;
    }
    if (envelope_u16(prefix + ENVELOPE_HEADER_SIZE_OFFSET) != CANVIEW_OTA_ENVELOPE_HEADER_BYTES ||
        envelope_u16(prefix + ENVELOPE_SIGNATURE_SIZE_OFFSET) != CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES)
    {
        return CANVIEW_MALFORMED;
    }
    manifest_size = envelope_u32(prefix + ENVELOPE_MANIFEST_SIZE_OFFSET);
    if (manifest_size == 0U)
    {
        return CANVIEW_MALFORMED;
    }
    if (manifest_size > CANVIEW_OTA_ENVELOPE_MANIFEST_MAX)
    {
        return CANVIEW_OVERSIZE;
    }
    expected_size = CANVIEW_OTA_ENVELOPE_HEADER_BYTES + (size_t)manifest_size +
                    CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES;
    if (size != expected_size)
    {
        return size < expected_size ? CANVIEW_INCOMPLETE : CANVIEW_MALFORMED;
    }
    candidate.declared_image_count = envelope_u16(prefix + ENVELOPE_IMAGE_COUNT_OFFSET);
    candidate.declared_total_size = envelope_u32(prefix + ENVELOPE_TOTAL_SIZE_OFFSET);
    if (candidate.declared_image_count == 0U ||
        candidate.declared_image_count > CANVIEW_OTA_ENVELOPE_IMAGE_MAX ||
        candidate.declared_total_size < expected_size + candidate.declared_image_count)
    {
        return CANVIEW_MALFORMED;
    }
    if (candidate.declared_total_size > CANVIEW_OTA_ENVELOPE_BUNDLE_MAX)
    {
        return CANVIEW_OVERSIZE;
    }
    candidate.manifest_offset = CANVIEW_OTA_ENVELOPE_HEADER_BYTES;
    candidate.manifest_size = manifest_size;
    candidate.images_offset = expected_size;
    status = canview_ota_cbor_validate(prefix + candidate.manifest_offset, candidate.manifest_size);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    status = verify(context, prefix + candidate.manifest_offset, candidate.manifest_size,
                    prefix + expected_size - CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES);
    if (status != CANVIEW_OK)
    {
        return status;
    }
    *out = candidate;
    return CANVIEW_OK;
}
