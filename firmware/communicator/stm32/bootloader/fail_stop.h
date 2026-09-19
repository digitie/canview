/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_BOOT_FAIL_STOP_H
#define CANVIEW_BOOT_FAIL_STOP_H

/* newlib/GNU --wrap 내부 ABI. int는 newlib의 line 인자와 일치해야 한다.
 * 인자를 읽거나 출력하지 않는다. 기존 FIH panic으로 진입하며 복귀/할당/
 * watchdog feed/Flash 변경을 하지 않는다. IWDG reset 실측은 별도 gate다.
 * 제품 loader에만 link한다. host 모형의 libc assert/abort는 대체하지 않는다. */
__attribute__((noreturn)) void __wrap___assert_func(const char *file, int line,
    const char *function, const char *expression);
__attribute__((noreturn)) void __wrap_abort(void);

#endif
