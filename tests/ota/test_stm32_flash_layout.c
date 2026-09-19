/* SPDX-License-Identifier: GPL-3.0-only */
/** @file test_stm32_flash_layout.c @brief 고정 배치·전체 byte offset·overflow 거절 시험. */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "canview_stm_flash_layout.h"
#include "flash_layout.h"

#define CHECK(c) do { if (!(c)) { (void)fprintf(stderr, "flash layout line %d\n", __LINE__); return 1; } } while (0)
#define SENTINEL (0xDEADBEEFU)

int main(void)
{
    /* 정본 문서의 literal oracle: production 상수만 되풀이하여 drift를 놓치지 않는다. */
    static const canview_stm_flash_area_t expected[] =
    {
        {0x08000000U, 0x10000U}, {0x08010000U, 0x30000U},
        {0x08040000U, 0x30800U}, {0x08070800U, 0x1000U},
        {0x08071800U, 0x1000U}, {0x08072800U, 0x4000U},
        {0x08076800U, 0x4000U}, {0x0807A800U, 0x5800U}
    };
    static const uint32_t bad_regions[] = {8U, 9U, 255U, 0x7FFFFFFFU, UINT32_MAX};
    static const uint32_t bad_operations[] = {3U, 4U, 255U, 0x7FFFFFFFU, UINT32_MAX};
    uint32_t address = SENTINEL;
    canview_stm_flash_area_t area = {SENTINEL, SENTINEL};
    CHECK(CANVIEW_STM_FLASH_REGION_COUNT == 8);
    CHECK(CANVIEW_STM_FLASH_BASE == 0x08000000U);
    CHECK(CANVIEW_STM_FLASH_BYTES == 0x80000U);
    CHECK(CANVIEW_STM_FLASH_PAGE_BYTES == 2048U);
    CHECK(CANVIEW_STM_FLASH_WRITE_BYTES == 8U);
    CHECK(CANVIEW_STM_PRIMARY_VECTOR == 0x08010200U);
    CHECK(CANVIEW_STM_SECONDARY_IMAGE == 0x08040800U);
    CHECK(CANVIEW_STM_SIGNED_IMAGE_MAX_BYTES == 184320U);
    CHECK(canview_stm_flash_area(CANVIEW_STM_FLASH_BOOT, NULL) == CANVIEW_INVALID_ARGUMENT);
    CHECK(canview_stm_flash_range(CANVIEW_STM_FLASH_PRIMARY, CANVIEW_STM_FLASH_READ,
        0U, 1U, NULL) == CANVIEW_INVALID_ARGUMENT);
    for (size_t index = 0U; index < sizeof(bad_regions) / sizeof(bad_regions[0]); ++index)
    {
        const canview_stm_flash_region_t region = (canview_stm_flash_region_t)bad_regions[index];
        CHECK(canview_stm_flash_area(region, &area) == CANVIEW_INVALID_ARGUMENT);
        CHECK(area.address == SENTINEL && area.size == SENTINEL);
        CHECK(canview_stm_flash_range(region, CANVIEW_STM_FLASH_READ, 0U, 1U, &address) == CANVIEW_INVALID_ARGUMENT);
        CHECK(address == SENTINEL);
    }
    for (size_t index = 0U; index < sizeof(bad_operations) / sizeof(bad_operations[0]); ++index)
    {
        CHECK(canview_stm_flash_range(CANVIEW_STM_FLASH_PRIMARY,
            (canview_stm_flash_operation_t)bad_operations[index], 0U, 8U, &address) == CANVIEW_INVALID_ARGUMENT);
        CHECK(address == SENTINEL);
    }
    for (uint32_t index = 0U; index < 8U; ++index)
    {
        const canview_stm_flash_region_t region = (canview_stm_flash_region_t)index;
        const bool read_only = index == 0U || index == 7U;
        CHECK(canview_stm_flash_area(region, &area) == CANVIEW_OK);
        CHECK(area.address == expected[index].address && area.size == expected[index].size);
        CHECK(area.address % 2048U == 0U && area.size % 2048U == 0U);
        CHECK(area.address + area.size == (index < 7U ? expected[index + 1U].address : 0x08080000U));
        for (uint32_t op = 0U; op <= 2U; ++op)
        {
            const canview_stm_flash_operation_t operation = (canview_stm_flash_operation_t)op;
            const uint32_t unit = op == 0U ? 1U : (op == 1U ? 8U : 2048U);
            const uint32_t bad_offsets[] = {area.size, area.size + 1U, UINT32_MAX};
            const uint32_t bad_lengths[] = {area.size + 1U, UINT32_MAX};
            const canview_status_t whole = read_only && op != 0U ? CANVIEW_AUTH_FAILED : CANVIEW_OK;
            address = SENTINEL;
            CHECK(canview_stm_flash_range(region, operation, 0U, 0U, &address) == CANVIEW_INVALID_ARGUMENT);
            CHECK(address == SENTINEL);
            CHECK(canview_stm_flash_range(region, operation, 0U, area.size, &address) == whole);
            CHECK(address == (whole == CANVIEW_OK ? area.address : SENTINEL));
            for (uint32_t offset = 0U; offset < area.size; ++offset)
            {
                canview_status_t status = CANVIEW_OK;
                if (read_only && op != 0U) { status = CANVIEW_AUTH_FAILED; }
                else if ((uint64_t)offset + unit > area.size) { status = CANVIEW_OVERSIZE; }
                else if (offset % unit != 0U) { status = CANVIEW_INVALID_ARGUMENT; }
                address = SENTINEL;
                CHECK(canview_stm_flash_range(region, operation, offset, unit, &address) == status);
                CHECK(address == (status == CANVIEW_OK ? area.address + offset : SENTINEL));
            }
            for (uint32_t length = 1U; length <= 2049U; ++length)
            {
                const canview_status_t status = read_only && op != 0U ? CANVIEW_AUTH_FAILED :
                    (length % unit == 0U ? CANVIEW_OK : CANVIEW_INVALID_ARGUMENT);
                address = SENTINEL;
                CHECK(canview_stm_flash_range(region, operation, 0U, length, &address) == status);
                CHECK(address == (status == CANVIEW_OK ? area.address : SENTINEL));
            }
            for (size_t bad = 0U; bad < sizeof(bad_offsets) / sizeof(bad_offsets[0]); ++bad)
            {
                address = SENTINEL;
                CHECK(canview_stm_flash_range(region, operation, bad_offsets[bad], unit, &address) ==
                    (read_only && op != 0U ? CANVIEW_AUTH_FAILED : CANVIEW_OVERSIZE));
                CHECK(address == SENTINEL);
            }
            for (size_t bad = 0U; bad < sizeof(bad_lengths) / sizeof(bad_lengths[0]); ++bad)
            {
                address = SENTINEL;
                CHECK(canview_stm_flash_range(region, operation, unit, bad_lengths[bad], &address) ==
                    (read_only && op != 0U ? CANVIEW_AUTH_FAILED : CANVIEW_OVERSIZE));
                CHECK(address == SENTINEL);
            }
        }
    }
    return 0;
}
