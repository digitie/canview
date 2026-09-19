/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_BOOT_FLASH_H
#define CANVIEW_BOOT_FLASH_H
#include <stdint.h>

/** 동기식 boot 전용 IO. task/ISR/reentry 호출 금지, buffer는 반환까지 유효하다.
 * 성공 0, 실패 음수. 실제 backend는 profile 검증 이후에만 write/erase를 허용하고
 * ECC/전원 중단/중복 doubleword를 처리해야 한다. G474 primitive 연결은 flash_io.c이며
 * 최종 SRAM 배치/boot executable/복구 정책은 별도 연결한다. host 모형과 구분한다.
 * address는 BSP 검사를 통과한 절대 주소다. backend도 범위를 독립 검사한다.
 * 실패한 read의 전체 출력은 사용 불가다. 256B 초과 요청은 앞선 chunk가 반영될 수 있다.
 * write/erase 실패도 앞선 단위의 변경이 남을 수 있다. 자동 retry/erase를 하지 않는다.
 * source는 호출 중 불변인 유효 buffer이며 destination은 caller 소유 SRAM1/2다.
 */
int canview_boot_flash_read(uint32_t address, void *destination, uint32_t length);
/** 쓰기 직전 고정 Flash 배치/보호의 필수 조건을 새로 읽는다. 0 또는 음수 오류.
 * option byte를 변경하지 않는다. 성공은 production/RDP·서명·ECC·SRAM 실행 승인과
 * 다르며 실제 backend는 나머지 gate도 검사해야 한다. 실패하면 write/erase 호출 금지.
 */
int canview_boot_flash_check(void);
int canview_boot_flash_write(uint32_t address, const void *source, uint32_t length);
int canview_boot_flash_erase(uint32_t address, uint32_t length);
/** 진행 지점 알림. 실제 target의 시간/진행 조건을 만족한 경우에만 watchdog feed 허용. */
void canview_boot_progress(void);
#endif
