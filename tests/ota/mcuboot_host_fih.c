/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdlib.h>
#include "bootutil/fault_injection_hardening.h"
#if !defined(CANVIEW_MCUBOOT_HOST_MODEL) || defined(__arm__) || defined(__thumb__)
#error "Only the x86 host model may replace the Arm panic loop"
#endif
/* 고정 upstream의 FIH 상태/CFI 구현은 그대로 컴파일한다. Arm branch loop만 제외. */
#undef FIH_ENABLE_GLOBAL_FAIL
#include CANVIEW_UPSTREAM_FIH_SOURCE
void fih_panic_loop(void)
{
    abort();
}
