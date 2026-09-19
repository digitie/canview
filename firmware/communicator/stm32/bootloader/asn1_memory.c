/* SPDX-License-Identifier: GPL-3.0-only */
#include <stddef.h>
#include "mbedtls/platform.h"
#include <stdlib.h>

/* 사용하는 SPKI/DER 읽기는 할당이 없다. 다른 ASN1 API 도입은 fail-closed다. */
void *mbedtls_calloc(size_t count, size_t size)
{
    (void)count;
    (void)size;
    return NULL;
}

void mbedtls_free(void *pointer)
{
    if (pointer != NULL)
    {
        abort();
    }
}
