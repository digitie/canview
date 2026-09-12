/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <string.h>
#include "cbor_document.h"

#define CHECK(condition) do { if (!(condition)) { \
    (void)fprintf(stderr, "CBOR document check failed at line %d\n", __LINE__); \
    return 1; } } while (0)

int main(void)
{
    static const uint8_t valid[] = {0xA2U, 0U, 0xA1U, 0U, 0x80U, 1U, 0x62U, 0xC2U, 0x80U};
    static const uint8_t duplicate[] = {0xA2U, 0U, 1U, 0U, 2U};
    static const uint8_t reverse[] = {0xA2U, 1U, 1U, 0U, 2U};
    static const uint8_t key_string[] = {0xA1U, 0x60U, 1U};
    uint8_t nested[2U + CANVIEW_OTA_CBOR_MAX_DEPTH + 1U] = {0xA1U, 0U};
    uint8_t oversize[CANVIEW_OTA_CBOR_MAX_BYTES + 1U] = {0};
    CHECK(canview_ota_cbor_validate(NULL, 0U) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_cbor_validate(NULL, SIZE_MAX) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_ota_cbor_validate(valid, SIZE_MAX) == CANVIEW_OVERSIZE);
    CHECK(canview_ota_cbor_validate(valid, sizeof(valid)) == CANVIEW_OK);
    for (size_t length = 0U; length < sizeof(valid); ++length)
    {
        CHECK(canview_ota_cbor_validate(valid, length) == CANVIEW_INCOMPLETE);
    }
    CHECK(canview_ota_cbor_validate(duplicate, sizeof(duplicate)) == CANVIEW_DUPLICATE);
    CHECK(canview_ota_cbor_validate(reverse, sizeof(reverse)) == CANVIEW_MALFORMED);
    CHECK(canview_ota_cbor_validate(key_string, sizeof(key_string)) == CANVIEW_MALFORMED);
    CHECK(canview_ota_cbor_validate(oversize, sizeof(oversize)) == CANVIEW_OVERSIZE);
    (void)memset(nested + 2U, 0x81, CANVIEW_OTA_CBOR_MAX_DEPTH - 1U);
    nested[2U + CANVIEW_OTA_CBOR_MAX_DEPTH - 1U] = 0U;
    CHECK(canview_ota_cbor_validate(nested, 2U + CANVIEW_OTA_CBOR_MAX_DEPTH) == CANVIEW_OK);
    nested[2U + CANVIEW_OTA_CBOR_MAX_DEPTH - 1U] = 0x81U;
    CHECK(canview_ota_cbor_validate(nested, sizeof(nested)) == CANVIEW_OVERSIZE);
    return 0;
}
