/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_BOOT_OS_MALLOC_H
#define CANVIEW_BOOT_OS_MALLOC_H
#include <stddef.h>
#include <stdlib.h>
/* 사용하지 않는 upstream split-image API도 heap을 열지 않는다. */
static inline void *canview_boot_noalloc(size_t size)
{
    (void)size;
    return NULL;
}
static inline void canview_boot_nofree(void *pointer)
{
    if (pointer != NULL)
    {
        abort();
    }
}
#define malloc(size) canview_boot_noalloc(size)
#define free(pointer) canview_boot_nofree(pointer)
#endif
