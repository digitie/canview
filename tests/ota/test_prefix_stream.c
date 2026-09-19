/* SPDX-License-Identifier: GPL-3.0-only */
/** @file test_prefix_stream.c @brief Prefix 조각 수신 경계·reset·소유권 회귀. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "envelope.h"

#define CHECK(value) do { if (!(value)) { (void)fprintf(stderr, "%s:%d\n", __FILE__, __LINE__); abort(); } } while (0)
#define TEST_MANIFEST_OFFSET (12U)
#define TEST_TRAILING_BYTES (3U)

static void put_length(uint8_t *data, uint32_t length)
{
    for (size_t index = 0U; index < sizeof(length); ++index)
    {
        data[TEST_MANIFEST_OFFSET + index] = (uint8_t)(length >> (index * 8U));
    }
}

static void test_chunks(canview_ota_prefix_t *prefix, uint8_t *input)
{
    static const size_t chunks[] = {1U, 23U, 24U, 25U, 31U, CANVIEW_OTA_PREFIX_CHUNK_MAX};
    static const uint32_t lengths[] = {1U, CANVIEW_OTA_ENVELOPE_MANIFEST_MAX};
    for (size_t length_index = 0U; length_index < sizeof(lengths) / sizeof(lengths[0]); ++length_index)
    {
        const size_t expected = CANVIEW_OTA_ENVELOPE_HEADER_BYTES + lengths[length_index] +
            CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES;
        put_length(input, lengths[length_index]);
        for (size_t chunk_index = 0U; chunk_index < sizeof(chunks) / sizeof(chunks[0]); ++chunk_index)
        {
            size_t offset = 0U;
            CHECK(canview_ota_prefix_init(prefix) == CANVIEW_OK);
            while (offset < expected)
            {
                const size_t remaining = expected + TEST_TRAILING_BYTES - offset;
                const size_t take = remaining < chunks[chunk_index] ? remaining : chunks[chunk_index];
                size_t consumed = SIZE_MAX;
                const canview_status_t result = canview_ota_prefix_feed(prefix, offset, input + offset, take, &consumed);
                CHECK(consumed > 0U && consumed <= take && consumed <= expected - offset);
                offset += consumed;
                CHECK(result == (offset == expected ? CANVIEW_OK : CANVIEW_INCOMPLETE));
                CHECK(prefix->received == offset);
                CHECK(memcmp(prefix->data, input, offset) == 0);
            }
            CHECK(canview_ota_prefix_finish(prefix) == CANVIEW_OK);
            CHECK(prefix->received == expected);
            size_t consumed = SIZE_MAX;
            CHECK(canview_ota_prefix_feed(prefix, expected, input, 1U, &consumed) == CANVIEW_DUPLICATE);
            CHECK(consumed == 0U && canview_ota_prefix_finish(prefix) == CANVIEW_DUPLICATE);
            CHECK(canview_ota_prefix_init(prefix) == CANVIEW_OK);
            CHECK(prefix->received == 0U && prefix->expected_size == 0U && prefix->data[expected - 1U] == 0U);
        }
    }
}

static void test_errors(canview_ota_prefix_t *prefix, uint8_t *input)
{
    size_t consumed = SIZE_MAX;
    CHECK(canview_ota_prefix_init(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_finish(NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_feed(NULL, 0U, input, 1U, &consumed) == CANVIEW_INVALID_ARGUMENT);
    (void)memset(prefix, 0, sizeof(*prefix));
    CHECK(canview_ota_prefix_finish(prefix) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_feed(prefix, 0U, input, 1U, &consumed) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_init(prefix) == CANVIEW_OK);
    CHECK(canview_ota_prefix_feed(prefix, 0U, NULL, 1U, &consumed) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_feed(prefix, 0U, input, 1U, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_feed(prefix, 0U, prefix->data, 1U, &consumed) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_feed(prefix, 0U, input, 1U, &prefix->received) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_feed(prefix, 0U, (const uint8_t *)&consumed, sizeof(consumed), &consumed) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_feed(prefix, 0U, input, SIZE_MAX, &consumed) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_feed(prefix, 0U, input, 0U, &consumed) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_finish(prefix) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_prefix_init(prefix) == CANVIEW_OK);
    CHECK(canview_ota_prefix_feed(prefix, 0U, input, CANVIEW_OTA_PREFIX_CHUNK_MAX + 1U, &consumed) == CANVIEW_OVERSIZE);
    CHECK(canview_ota_prefix_feed(prefix, 0U, input, 1U, &consumed) == CANVIEW_OVERSIZE);
    CHECK(canview_ota_prefix_init(prefix) == CANVIEW_OK);
    CHECK(canview_ota_prefix_feed(prefix, 1U, input, 1U, &consumed) == CANVIEW_STALE);
    CHECK(canview_ota_prefix_feed(prefix, 0U, input, 1U, &consumed) == CANVIEW_STALE);
    CHECK(canview_ota_prefix_init(prefix) == CANVIEW_OK);
    CHECK(canview_ota_prefix_feed(prefix, 0U, input, 1U, &consumed) == CANVIEW_INCOMPLETE);
    CHECK(canview_ota_prefix_feed(prefix, 0U, input, 1U, &consumed) == CANVIEW_DUPLICATE);
    static const uint32_t invalid_lengths[] = {0U, CANVIEW_OTA_ENVELOPE_MANIFEST_MAX + 1U, UINT32_MAX};
    for (size_t index = 0U; index < sizeof(invalid_lengths) / sizeof(invalid_lengths[0]); ++index)
    {
        put_length(input, invalid_lengths[index]);
        CHECK(canview_ota_prefix_init(prefix) == CANVIEW_OK);
        CHECK(canview_ota_prefix_feed(prefix, 0U, input, CANVIEW_OTA_ENVELOPE_HEADER_BYTES, &consumed) ==
            (index == 0U ? CANVIEW_MALFORMED : CANVIEW_OVERSIZE));
        CHECK(consumed == CANVIEW_OTA_ENVELOPE_HEADER_BYTES);
    }
    put_length(input, 1U);
    for (size_t end = 0U; end < CANVIEW_OTA_ENVELOPE_HEADER_BYTES + 1U + CANVIEW_OTA_ENVELOPE_SIGNATURE_BYTES; ++end)
    {
        CHECK(canview_ota_prefix_init(prefix) == CANVIEW_OK);
        if (end != 0U)
        {
            CHECK(canview_ota_prefix_feed(prefix, 0U, input, end, &consumed) == CANVIEW_INCOMPLETE);
            CHECK(consumed == end);
        }
        CHECK(canview_ota_prefix_finish(prefix) == CANVIEW_INCOMPLETE);
    }
}

int main(void)
{
    static canview_ota_prefix_t prefix;
    static uint8_t input[CANVIEW_OTA_ENVELOPE_PREFIX_MAX + TEST_TRAILING_BYTES];
    (void)memset(input, 0xA5, sizeof(input));
    test_chunks(&prefix, input);
    test_errors(&prefix, input);
    CHECK(puts("PASS: bounded untrusted prefix chunks/reset/duplicate/hole/alias/truncation; not authorization") >= 0);
    return 0;
}
