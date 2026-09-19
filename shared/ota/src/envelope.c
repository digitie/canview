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

static bool prefix_overlaps(const void *left, size_t left_size, const void *right, size_t right_size)
{
    const uintptr_t start_left = (uintptr_t)left;
    const uintptr_t start_right = (uintptr_t)right;
    if (left_size > UINTPTR_MAX - start_left || right_size > UINTPTR_MAX - start_right)
    {
        return true;
    }
    return start_left < start_right + right_size && start_right < start_left + left_size;
}

canview_status_t canview_ota_prefix_init(canview_ota_prefix_t *prefix)
{
    if (prefix == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    (void)memset(prefix, 0, sizeof(*prefix));
    prefix->initialized = true;
    return CANVIEW_OK;
}

canview_status_t canview_ota_prefix_finish(const canview_ota_prefix_t *prefix)
{
    if (prefix == NULL || !prefix->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (prefix->error != CANVIEW_OK)
    {
        return prefix->error;
    }
    return prefix->expected_size != 0U && prefix->received == prefix->expected_size ?
        CANVIEW_OK : CANVIEW_INCOMPLETE;
}

canview_status_t canview_ota_prefix_feed(canview_ota_prefix_t *prefix, size_t offset,
    const uint8_t *input, size_t size, size_t *consumed)
{
    if (prefix == NULL || input == NULL || consumed == NULL ||
        prefix_overlaps(prefix, sizeof(*prefix), input, size) ||
        prefix_overlaps(prefix, sizeof(*prefix), consumed, sizeof(*consumed)) ||
        prefix_overlaps(input, size, consumed, sizeof(*consumed)))
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    *consumed = 0U;
    if (!prefix->initialized)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (prefix->error != CANVIEW_OK)
    {
        return prefix->error;
    }
    if (size == 0U || size > CANVIEW_OTA_PREFIX_CHUNK_MAX)
    {
        prefix->error = size == 0U ? CANVIEW_INVALID_ARGUMENT : CANVIEW_OVERSIZE;
        return prefix->error;
    }
    if (offset != prefix->received || canview_ota_prefix_finish(prefix) == CANVIEW_OK)
    {
        prefix->error = offset > prefix->received ? CANVIEW_STALE : CANVIEW_DUPLICATE;
        return prefix->error;
    }
    while (*consumed < size)
    {
        const size_t limit = prefix->expected_size == 0U ?
            CANVIEW_OTA_ENVELOPE_HEADER_BYTES : prefix->expected_size;
        if (limit > sizeof(prefix->data) || prefix->received >= limit)
        {
            prefix->error = CANVIEW_INVALID_ARGUMENT;
            return prefix->error;
        }
        const size_t remaining = limit - prefix->received;
        const size_t available = size - *consumed;
        const size_t take = remaining < available ? remaining : available;
        (void)memcpy(prefix->data + prefix->received, input + *consumed, take);
        prefix->received += take;
        *consumed += take;
        if (prefix->expected_size == 0U && prefix->received == CANVIEW_OTA_ENVELOPE_HEADER_BYTES)
        {
            const uint32_t manifest_size = envelope_u32(prefix->data + ENVELOPE_MANIFEST_SIZE_OFFSET);
            if (manifest_size == 0U || manifest_size > CANVIEW_OTA_ENVELOPE_MANIFEST_MAX)
            {
                prefix->error = manifest_size == 0U ? CANVIEW_MALFORMED : CANVIEW_OVERSIZE;
                return prefix->error;
            }
            prefix->expected_size = CANVIEW_OTA_ENVELOPE_HEADER_BYTES + (size_t)manifest_size +
                CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES;
        }
        if (canview_ota_prefix_finish(prefix) == CANVIEW_OK)
        {
            break;
        }
    }
    return canview_ota_prefix_finish(prefix);
}

canview_status_t canview_ota_envelope_check(
    const uint8_t *prefix, size_t size, canview_ota_manifest_verify_fn verify,
    void *context, canview_ota_envelope_t *out)
{
    static const uint8_t magic[] = {'C', 'V', 'O', 'T', 'A', '0', '0', '2'};
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
    if (envelope_u16(prefix + ENVELOPE_VERSION_OFFSET) != CANVIEW_OTA_ENVELOPE_VERSION)
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
    /* expected_size는 PREFIX_MAX로 제한되어 정렬 덧셈이 넘치지 않는다. */
    candidate.images_offset = ((expected_size + CANVIEW_OTA_ENVELOPE_IMAGE_ALIGNMENT - 1U) /
        CANVIEW_OTA_ENVELOPE_IMAGE_ALIGNMENT) * CANVIEW_OTA_ENVELOPE_IMAGE_ALIGNMENT;
    if (candidate.declared_image_count == 0U ||
        candidate.declared_image_count > CANVIEW_OTA_ENVELOPE_IMAGE_MAX ||
        candidate.declared_total_size < candidate.images_offset + candidate.declared_image_count)
    {
        return CANVIEW_MALFORMED;
    }
    if (candidate.declared_total_size > CANVIEW_OTA_ENVELOPE_BUNDLE_MAX)
    {
        return CANVIEW_OVERSIZE;
    }
    candidate.manifest_offset = CANVIEW_OTA_ENVELOPE_HEADER_BYTES;
    candidate.manifest_size = manifest_size;
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
