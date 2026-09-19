/* SPDX-License-Identifier: GPL-3.0-only */
/* 실제 Arm link 검증 전용. boot_go/정책/handoff가 없는 비배포 image다. */
#include "canview_stm_flash_command.h"
#include "canview_stm_flash_read.h"
#include <stdint.h>

static uint32_t initialized = UINT32_C(0x12345678);
static uint32_t zeroed;

int main(void)
{
    /* linker GC로 제품 호출부가 사라지지 않게 한다. 이 시험 image는 flash하지 않는다. */
    zeroed = initialized;
    const canview_status_t status = canview_stm_flash_read(
        UINT32_C(0x08010200), &initialized, sizeof(initialized));
    if (status == CANVIEW_OK && initialized == zeroed)
    {
        return (int)canview_stm_flash_command(CANVIEW_STM_FLASH_PROGRAM,
            UINT32_C(0x08040800), UINT32_MAX, UINT32_MAX);
    }
    return (int)status;
}
