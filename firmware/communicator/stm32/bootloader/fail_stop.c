/* SPDX-License-Identifier: GPL-3.0-only */
#include "fail_stop.h"
#include "bootutil/fault_injection_hardening.h"

#if !defined(FIH_ENABLE_GLOBAL_FAIL)
#error "Boot libc failure requires the existing MCUboot FIH panic"
#endif

void __wrap___assert_func(const char *file, int line,
    const char *function, const char *expression)
{
    (void)file;
    (void)line;
    (void)function;
    (void)expression;
    FIH_PANIC;
}

void __wrap_abort(void)
{
    FIH_PANIC;
}
