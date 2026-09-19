/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include "tinycrypt/ecc_platform_specific.h"

/* 이 포트는 공개키 검증 전용이다. 서명/키 생성에 가짜 entropy를 주지 않는다. */
int default_CSPRNG(uint8_t *destination, unsigned int size)
{
    (void)destination;
    (void)size;
    return 0;
}
