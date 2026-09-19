/* SPDX-License-Identifier: GPL-3.0-only */
/* 실제 guard C를 register 모형에 연결한다. 물리 option-byte/WRP 시험이 아니다. */
#include <stdio.h>
#include <string.h>
#include "canview_boot_flash.h"
#include "register_model.h"

model_rcc_t model_rcc;
model_flash_t model_flash;
model_syscfg_t model_syscfg;
uint16_t model_flash_size_kib;

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); return 1; } } while (0)

static void valid_profile(void)
{
    (void)memset(&model_flash, 0, sizeof(model_flash));
    (void)memset(&model_rcc, 0, sizeof(model_rcc));
    (void)memset(&model_syscfg, 0, sizeof(model_syscfg));
    model_flash_size_kib = 512U;
    model_rcc.APB2ENR = RCC_APB2ENR_SYSCFGEN;
    model_flash.OPTR = FLASH_OPTR_DBANK | FLASH_OPTR_NRST_MODE;
    model_flash.WRP1AR = 31U << 16U;
    model_flash.WRP1BR = 127U;
    model_flash.WRP2AR = 127U;
    model_flash.WRP2BR = 127U;
}

int main(void)
{
    valid_profile();
    CHECK(canview_boot_flash_check() == 0);
    const model_flash_t original = model_flash;
    const model_rcc_t original_rcc = model_rcc;
    const model_syscfg_t original_syscfg = model_syscfg;
    CHECK(canview_boot_flash_check() == 0);
    CHECK(memcmp(&original, &model_flash, sizeof(original)) == 0);
    CHECK(memcmp(&original_rcc, &model_rcc, sizeof(original_rcc)) == 0);
    CHECK(memcmp(&original_syscfg, &model_syscfg, sizeof(original_syscfg)) == 0);
    for (uint32_t bits = 0U; bits < 16U; ++bits)
    {
        valid_profile();
        model_flash.OPTR = ((bits & 1U) != 0U ? FLASH_OPTR_DBANK : 0U) |
            ((bits & 2U) != 0U ? FLASH_OPTR_BFB2 : 0U) | ((bits >> 2U) << 28U);
        CHECK((canview_boot_flash_check() == 0) == (bits == 13U));
    }
    valid_profile();
    model_syscfg.MEMRMP = SYSCFG_MEMRMP_FB_MODE;
    CHECK(canview_boot_flash_check() != 0);
    valid_profile();
    model_rcc.APB2ENR = 0U;
    CHECK(canview_boot_flash_check() != 0);
    valid_profile();
    model_flash.SR = FLASH_SR_BSY;
    CHECK(canview_boot_flash_check() != 0 && model_flash.SR == FLASH_SR_BSY);
    model_flash.SR = FLASH_SR_OPTVERR;
    CHECK(canview_boot_flash_check() != 0 && model_flash.SR == FLASH_SR_OPTVERR);
    model_flash.SR = 0U;
    for (uint32_t size = 0U; size <= UINT16_MAX; ++size)
    {
        model_flash_size_kib = (uint16_t)size;
        CHECK((canview_boot_flash_check() == 0) == (size == 512U));
    }
    valid_profile();
    volatile uint32_t *const areas[] = {
        &model_flash.WRP1AR, &model_flash.WRP1BR, &model_flash.WRP2AR, &model_flash.WRP2BR};
    for (size_t area = 0U; area < sizeof(areas) / sizeof(areas[0]); ++area)
    {
        for (uint32_t start = 0U; start < 128U; ++start)
        {
            for (uint32_t end = 0U; end < 128U; ++end)
            {
                *areas[area] = start | (end << 16U);
                const int expected = area == 0U ? (start == 0U && end == 31U) : start > end;
                const uint32_t before = *areas[area];
                CHECK((canview_boot_flash_check() == 0) == expected);
                CHECK(*areas[area] == before); /* 실패도 보호 register 수정 금지 */
            }
        }
        valid_profile();
    }
    puts("PASS: Flash guard option16/size65536/WRP65536, read-only; physical NOT_RUN");
    return 0;
}
