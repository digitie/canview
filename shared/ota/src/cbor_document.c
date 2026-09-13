/* SPDX-License-Identifier: GPL-3.0-only */
/** @file cbor_document.c
 * @brief 재귀 대신 고정 stack으로 순회하는 OTA CBOR 구조 검사.
 */
#include <stdbool.h>
#include "cbor_document.h"
#include "cbor_head.h"

#define UTF8_ASCII_LIMIT (0x80U)
#define UTF8_CONT_MASK (0xC0U)
#define UTF8_CONT_TAG (0x80U)
#define UTF8_PAYLOAD_MASK (0x3FU)
#define UTF8_PAYLOAD_BITS (6U)
#define UTF8_TWO_MIN (0xC2U)
#define UTF8_TWO_LAST (0xDFU)
#define UTF8_THREE_MIN (0xE0U)
#define UTF8_THREE_LAST (0xEFU)
#define UTF8_FOUR_MIN (0xF0U)
#define UTF8_FOUR_LAST (0xF4U)
#define UTF8_TWO_PAYLOAD_MASK (0x1FU)
#define UTF8_THREE_PAYLOAD_MASK (0x0FU)
#define UTF8_FOUR_PAYLOAD_MASK (0x07U)
#define UTF8_TWO_SCALAR_MIN (UINT32_C(0x80))
#define UTF8_THREE_SCALAR_MIN (UINT32_C(0x800))
#define UTF8_FOUR_SCALAR_MIN (UINT32_C(0x10000))
#define UTF8_SCALAR_MAX (UINT32_C(0x10FFFF))
#define UTF8_SURROGATE_FIRST (UINT32_C(0xD800))
#define UTF8_SURROGATE_LAST (UINT32_C(0xDFFF))

typedef struct
{
    uint64_t previous_key;
    size_t remaining;
    bool map;
    bool key_next;
    bool has_previous;
} cbor_frame_t;

static bool cbor_utf8_valid(const uint8_t *data, size_t length)
{
    size_t offset = 0U;
    while (offset < length)
    {
        const uint8_t first = data[offset];
        uint32_t scalar;
        uint32_t minimum;
        size_t continuation;
        ++offset;
        if (first < UTF8_ASCII_LIMIT)
        {
            continue;
        }
        if (first >= UTF8_TWO_MIN && first <= UTF8_TWO_LAST)
        {
            continuation = 1U;
            minimum = UTF8_TWO_SCALAR_MIN;
            scalar = first & UTF8_TWO_PAYLOAD_MASK;
        }
        else if (first >= UTF8_THREE_MIN && first <= UTF8_THREE_LAST)
        {
            continuation = 2U;
            minimum = UTF8_THREE_SCALAR_MIN;
            scalar = first & UTF8_THREE_PAYLOAD_MASK;
        }
        else if (first >= UTF8_FOUR_MIN && first <= UTF8_FOUR_LAST)
        {
            continuation = 3U;
            minimum = UTF8_FOUR_SCALAR_MIN;
            scalar = first & UTF8_FOUR_PAYLOAD_MASK;
        }
        else
        {
            return false;
        }
        if (continuation > length - offset)
        {
            return false;
        }
        for (size_t index = 0U; index < continuation; ++index)
        {
            const uint8_t next = data[offset + index];
            if ((next & UTF8_CONT_MASK) != UTF8_CONT_TAG)
            {
                return false;
            }
            scalar = (scalar << UTF8_PAYLOAD_BITS) | (next & UTF8_PAYLOAD_MASK);
        }
        if (scalar < minimum || scalar > UTF8_SCALAR_MAX ||
            (scalar >= UTF8_SURROGATE_FIRST && scalar <= UTF8_SURROGATE_LAST))
        {
            return false;
        }
        offset += continuation;
    }
    return true;
}

static canview_status_t cbor_key_check(cbor_frame_t *frame,
                                      const canview_ota_cbor_head_t *head)
{
    if (!frame->map)
    {
        return CANVIEW_OK;
    }
    if (frame->key_next)
    {
        if (head->type != CANVIEW_OTA_CBOR_UINT)
        {
            return CANVIEW_MALFORMED;
        }
        if (frame->has_previous && head->argument <= frame->previous_key)
        {
            return head->argument == frame->previous_key ? CANVIEW_DUPLICATE : CANVIEW_MALFORMED;
        }
        frame->previous_key = head->argument;
        frame->has_previous = true;
    }
    frame->key_next = !frame->key_next;
    return CANVIEW_OK;
}

canview_status_t canview_ota_cbor_validate(const uint8_t *data, size_t length)
{
    cbor_frame_t stack[CANVIEW_OTA_CBOR_MAX_DEPTH + 1U] = {{0}};
    size_t depth = 0U;
    size_t offset = 0U;
    size_t items = 0U;

    if (data == NULL)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    if (length > CANVIEW_OTA_CBOR_MAX_BYTES)
    {
        return CANVIEW_OVERSIZE;
    }
    stack[0].remaining = 1U;
    while (depth > 0U || stack[0].remaining > 0U)
    {
        canview_ota_cbor_head_t head;
        canview_status_t status;
        cbor_frame_t *frame = &stack[depth];
        if (frame->remaining == 0U)
        {
            --depth;
            continue;
        }
        if (items >= CANVIEW_OTA_CBOR_MAX_ITEMS)
        {
            return CANVIEW_OVERSIZE;
        }
        status = canview_ota_cbor_read_head(data + offset, length - offset, &head);
        if (status != CANVIEW_OK)
        {
            return status;
        }
        if (depth == 0U && head.type != CANVIEW_OTA_CBOR_MAP)
        {
            return CANVIEW_MALFORMED;
        }
        status = cbor_key_check(frame, &head);
        if (status != CANVIEW_OK)
        {
            return status;
        }
        --frame->remaining;
        ++items;
        offset += head.encoded_size;
        if (head.type == CANVIEW_OTA_CBOR_BYTES || head.type == CANVIEW_OTA_CBOR_TEXT)
        {
            if (head.argument > CANVIEW_OTA_CBOR_MAX_BYTES)
            {
                return CANVIEW_OVERSIZE;
            }
            if (head.argument > length - offset)
            {
                return CANVIEW_INCOMPLETE;
            }
            if (head.type == CANVIEW_OTA_CBOR_TEXT &&
                !cbor_utf8_valid(data + offset, (size_t)head.argument))
            {
                return CANVIEW_MALFORMED;
            }
            offset += (size_t)head.argument;
        }
        else if (head.type == CANVIEW_OTA_CBOR_ARRAY || head.type == CANVIEW_OTA_CBOR_MAP)
        {
            const bool map = head.type == CANVIEW_OTA_CBOR_MAP;
            const size_t factor = map ? 2U : 1U;
            if (depth >= CANVIEW_OTA_CBOR_MAX_DEPTH ||
                head.argument > CANVIEW_OTA_CBOR_MAX_ITEMS / factor)
            {
                return CANVIEW_OVERSIZE;
            }
            ++depth;
            stack[depth].remaining = (size_t)head.argument * factor;
            stack[depth].map = map;
            stack[depth].key_next = map;
            stack[depth].has_previous = false;
            stack[depth].previous_key = 0U;
        }
    }
    return offset == length ? CANVIEW_OK : CANVIEW_MALFORMED;
}
