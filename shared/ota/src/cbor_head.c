/* SPDX-License-Identifier: GPL-3.0-only */
/** @file cbor_head.c
 * @brief 최대 8 byte argument를 읽는 bounded CBOR head decoder.
 */
#include "cbor_head.h"

#define CBOR_MAJOR_SHIFT (5U)
#define CBOR_ADDITIONAL_MASK (0x1FU)
#define CBOR_INLINE_LIMIT (24U)
#define CBOR_EXTENDED_LAST (27U)
#define CBOR_BYTE_BITS (8U)

canview_status_t canview_ota_cbor_read_head(const uint8_t *data, size_t length,
                                         canview_ota_cbor_head_t *out)
{
    static const uint64_t minimum_arguments[] = {
        UINT64_C(24), UINT64_C(256), UINT64_C(65536), UINT64_C(4294967296)
    };
    uint64_t argument = 0U;
    size_t extra_size = 0U;
    uint8_t major;
    uint8_t additional;

    if (out == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    out->argument = 0U;
    out->encoded_size = 0U;
    out->type = CANVIEW_OTA_CBOR_UINT;
    if (data == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (length == 0U)
    {
        return CANVIEW_INCOMPLETE;
    }
    major = (uint8_t)(data[0] >> CBOR_MAJOR_SHIFT);
    additional = (uint8_t)(data[0] & CBOR_ADDITIONAL_MASK);
    if (major != CANVIEW_OTA_CBOR_UINT && major != CANVIEW_OTA_CBOR_BYTES &&
        major != CANVIEW_OTA_CBOR_TEXT && major != CANVIEW_OTA_CBOR_ARRAY &&
        major != CANVIEW_OTA_CBOR_MAP)
    {
        return CANVIEW_UNSUPPORTED_MESSAGE;
    }
    if (additional < CBOR_INLINE_LIMIT)
    {
        argument = additional;
    }
    else
    {
        size_t width_index;

        if (additional > CBOR_EXTENDED_LAST)
        {
            return CANVIEW_MALFORMED;
        }
        width_index = (size_t)additional - CBOR_INLINE_LIMIT;
        extra_size = (size_t)1U << width_index;
        if (length - 1U < extra_size)
        {
            return CANVIEW_INCOMPLETE;
        }
        for (size_t index = 0U; index < extra_size; ++index)
        {
            argument = (argument << CBOR_BYTE_BITS) | data[index + 1U];
        }
        if (argument < minimum_arguments[width_index])
        {
            return CANVIEW_MALFORMED;
        }
    }
    out->argument = argument;
    out->encoded_size = extra_size + 1U;
    out->type = (canview_ota_cbor_type_t)major;
    return CANVIEW_OK;
}
