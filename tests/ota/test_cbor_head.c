/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include "cbor_head.h"

#define CHECK(condition) do { if (!(condition)) { \
    (void)fprintf(stderr, "CBOR check failed at line %d\n", __LINE__); \
    return 1; } } while (0)

static int test_unsigned_boundaries(void)
{
    static const struct
    {
        uint8_t bytes[9];
        size_t length;
        uint64_t value;
    } vectors[] = {
        {{0x00U}, 1U, UINT64_C(0)},
        {{0x17U}, 1U, UINT64_C(23)},
        {{0x18U, 0x18U}, 2U, UINT64_C(24)},
        {{0x18U, 0xFFU}, 2U, UINT64_C(255)},
        {{0x19U, 0x01U, 0x00U}, 3U, UINT64_C(256)},
        {{0x19U, 0xFFU, 0xFFU}, 3U, UINT64_C(65535)},
        {{0x1AU, 0x00U, 0x01U, 0x00U, 0x00U}, 5U, UINT64_C(65536)},
        {{0x1AU, 0xFFU, 0xFFU, 0xFFU, 0xFFU}, 5U, UINT64_C(4294967295)},
        {{0x1BU, 0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U},
         9U, UINT64_C(4294967296)},
        {{0x1BU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU},
         9U, UINT64_MAX}
    };
    for (size_t index = 0U; index < sizeof(vectors) / sizeof(vectors[0]); ++index)
    {
        canview_ota_cbor_head_t head;
        for (size_t prefix = 0U; prefix < vectors[index].length; ++prefix)
        {
            CHECK(canview_ota_cbor_read_head(vectors[index].bytes, prefix, &head) ==
                  CANVIEW_INCOMPLETE);
            CHECK(head.encoded_size == 0U && head.argument == 0U);
        }
        CHECK(canview_ota_cbor_read_head(vectors[index].bytes, vectors[index].length,
                                       &head) == CANVIEW_OK);
        CHECK(head.argument == vectors[index].value);
        CHECK(head.encoded_size == vectors[index].length);
        CHECK(head.type == CANVIEW_OTA_CBOR_UINT);
    }
    return 0;
}

static int test_rejections(void)
{
    static const uint8_t nonminimal[][9] = {
        {0x18U, 0x17U}, {0x19U, 0x00U, 0xFFU},
        {0x1AU, 0x00U, 0x00U, 0xFFU, 0xFFU},
        {0x1BU, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU}
    };
    static const uint8_t unsupported[] = {0x20U, 0xC0U, 0xF4U, 0xF9U, 0xFFU};
    canview_ota_cbor_head_t head = {UINT64_MAX, 9U, CANVIEW_OTA_CBOR_MAP};
    CHECK(canview_ota_cbor_read_head(NULL, 0U, &head) == CANVIEW_INVALID_ARGUMENT);
    CHECK(head.encoded_size == 0U && head.argument == 0U && head.type == CANVIEW_OTA_CBOR_UINT);
    CHECK(canview_ota_cbor_read_head(unsupported, sizeof(unsupported), NULL) ==
          CANVIEW_INVALID_ARGUMENT);
    for (size_t index = 0U; index < sizeof(nonminimal) / sizeof(nonminimal[0]); ++index)
    {
        CHECK(canview_ota_cbor_read_head(nonminimal[index], sizeof(nonminimal[index]),
                                       &head) == CANVIEW_MALFORMED);
        CHECK(head.argument == 0U && head.encoded_size == 0U);
    }
    for (size_t index = 0U; index < sizeof(unsupported); ++index)
    {
        CHECK(canview_ota_cbor_read_head(&unsupported[index], 1U, &head) ==
              CANVIEW_UNSUPPORTED_MESSAGE);
    }
    for (uint8_t value = 28U; value <= 31U; ++value)
    {
        CHECK(canview_ota_cbor_read_head(&value, 1U, &head) == CANVIEW_MALFORMED);
    }
    return 0;
}

static int test_major_types(void)
{
    static const uint8_t major_types[] = {0U, 2U, 3U, 4U, 5U};
    canview_ota_cbor_head_t head;
    for (size_t index = 0U; index < sizeof(major_types); ++index)
    {
        for (uint8_t argument = 0U; argument < 24U; ++argument)
        {
            uint8_t bytes[] = {(uint8_t)((major_types[index] << 5U) | argument), 0xFFU};
            CHECK(canview_ota_cbor_read_head(bytes, sizeof(bytes), &head) == CANVIEW_OK);
            CHECK(head.type == (canview_ota_cbor_type_t)major_types[index]);
            CHECK(head.argument == argument && head.encoded_size == 1U);
        }
        for (uint8_t additional = 28U; additional <= 31U; ++additional)
        {
            const uint8_t value = (uint8_t)((major_types[index] << 5U) | additional);
            CHECK(canview_ota_cbor_read_head(&value, 1U, &head) == CANVIEW_MALFORMED);
        }
    }
    /* 완전한 item이 아닌 head만 읽으므로 payload 부재는 여기서 판정하지 않는다. */
    {
        static const uint8_t bytes[] = {0x58U, 0x18U};
        CHECK(canview_ota_cbor_read_head(bytes, sizeof(bytes), &head) == CANVIEW_OK);
        CHECK(head.type == CANVIEW_OTA_CBOR_BYTES && head.argument == 24U);
    }
    return 0;
}

int main(void)
{
    CHECK(test_unsigned_boundaries() == 0);
    CHECK(test_rejections() == 0);
    CHECK(test_major_types() == 0);
    return 0;
}
