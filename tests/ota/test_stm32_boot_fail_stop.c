/* SPDX-License-Identifier: GPL-3.0-only */
#include "fail_stop.h"
#include "bootutil/fault_injection_hardening.h"
#include <limits.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static jmp_buf panic_return;
static uint32_t panic_count;

/* 테스트에서만 panic의 non-return을 longjmp로 관찰한다. 제품 FIH는 그대로다. */
void fih_panic_loop(void)
{
    ++panic_count;
    longjmp(panic_return, 1);
}

int main(void)
{
    if (setjmp(panic_return) == 0)
    {
        __wrap___assert_func(NULL, INT_MIN, NULL, NULL);
    }
    if (panic_count != 1U) { return 1; }
    /* NUL 종료가 없는 인자도 역참조/printf하지 않아야 한다. */
    const char opaque = 'x';
    if (setjmp(panic_return) == 0)
    {
        __wrap___assert_func(&opaque, INT_MAX, &opaque, &opaque);
    }
    if (panic_count != 2U) { return 1; }
    if (setjmp(panic_return) == 0) { __wrap_abort(); }
    if (panic_count != 3U) { return 1; }
    (void)puts("PASS: newlib failure wrappers enter FIH panic; hardware reset NOT_RUN");
    return 0;
}
