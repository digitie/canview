/* SPDX-License-Identifier: GPL-3.0-only */
/** @file flash_guard.c @brief MCUboot 쓰기 전 G474 고정 배치의 읽기 전용 guard. */
#include "canview_boot_flash.h"
#include "flash_layout.h"
#include <stdbool.h>
#if defined(CANVIEW_STM_REGISTER_TEST)
#if defined(__arm__) || defined(__thumb__)
#error Host_register_model_must_not_be_built_for_target
#endif
#include "register_model.h"
#define FLASH_SIZE_KIB() (model_flash_size_kib)
#else
#include "stm32g474xx.h"
#define FLASH_SIZE_KIB() (*(volatile const uint16_t *)FLASHSIZE_BASE)
#endif

#define FLASH_EXPECTED_KIB (CANVIEW_STM_FLASH_BYTES / 1024U)
#define FLASH_BOOT_LAST_PAGE (CANVIEW_STM_BOOT_BYTES / CANVIEW_STM_FLASH_PAGE_BYTES - 1U)

static bool wrp_disabled(uint32_t value, uint32_t start_mask, uint32_t end_mask, uint32_t end_shift)
{
    return (value & start_mask) > ((value & end_mask) >> end_shift);
}

int canview_boot_flash_check(void)
{
    /* SYSCFG 미클럭 상태의0을 bank1 정상값으로 믿지 않는다. 호출자 BSP가 clock 소유. */
    if ((RCC->APB2ENR & RCC_APB2ENR_SYSCFGEN) == 0U ||
        (FLASH->SR & (FLASH_SR_BSY | FLASH_SR_OPTVERR)) != 0U ||
        FLASH_SIZE_KIB() != FLASH_EXPECTED_KIB)
    {
        return -1;
    }
    const uint32_t option = FLASH->OPTR;
    const uint32_t required_mask = FLASH_OPTR_DBANK | FLASH_OPTR_BFB2 | FLASH_OPTR_NRST_MODE;
    const uint32_t required_value = FLASH_OPTR_DBANK | FLASH_OPTR_NRST_MODE;
    if ((option & required_mask) != required_value ||
        (SYSCFG->MEMRMP & SYSCFG_MEMRMP_FB_MODE) != 0U)
    {
        return -1;
    }
    const uint32_t first = FLASH->WRP1AR;
    if ((first & FLASH_WRP1AR_WRP1A_STRT) != 0U ||
        ((first & FLASH_WRP1AR_WRP1A_END) >> FLASH_WRP1AR_WRP1A_END_Pos) != FLASH_BOOT_LAST_PAGE ||
        !wrp_disabled(FLASH->WRP1BR, FLASH_WRP1BR_WRP1B_STRT, FLASH_WRP1BR_WRP1B_END,
            FLASH_WRP1BR_WRP1B_END_Pos) ||
        !wrp_disabled(FLASH->WRP2AR, FLASH_WRP2AR_WRP2A_STRT, FLASH_WRP2AR_WRP2A_END,
            FLASH_WRP2AR_WRP2A_END_Pos) ||
        !wrp_disabled(FLASH->WRP2BR, FLASH_WRP2BR_WRP2B_STRT, FLASH_WRP2BR_WRP2B_END,
            FLASH_WRP2BR_WRP2B_END_Pos))
    {
        return -1;
    }
    return 0;
}
