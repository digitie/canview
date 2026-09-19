/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_BOOT_FLASH_H
#define CANVIEW_BOOT_FLASH_H
#include <stdint.h>

/** 동기식 boot 전용 IO. task/ISR/reentry 호출 금지, buffer는 반환까지 유효하다.
 * 성공 0, 실패 음수. 실제 backend는 profile 검증 이후에만 write/erase를 허용하고
 * ECC/전원 중단/중복 doubleword를 처리해야 한다. 현재 backend는 host 모형뿐이다.
 * address는 BSP 검사를 통과한 절대 주소다. backend도 범위를 독립 검사한다.
 */
int canview_boot_flash_read(uint32_t address, void *destination, uint32_t length);
int canview_boot_flash_write(uint32_t address, const void *source, uint32_t length);
int canview_boot_flash_erase(uint32_t address, uint32_t length);
/** 진행 지점 알림. 실제 target의 시간/진행 조건을 만족한 경우에만 watchdog feed 허용. */
void canview_boot_progress(void);
#endif
