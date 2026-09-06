/* SPDX-License-Identifier: GPL-3.0-only */
/**
 * @file syscalls.c
 * @brief Deliberately unavailable newlib system calls for the bare-metal target.
 */

/* These declarations satisfy the project's missing-prototypes warning while
 * keeping the ABI expected by newlib's reentrant wrappers. */
int _close(int file);
int _lseek(int file, int offset, int whence);
int _read(int file, void *buffer, int length);
int _write(int file, const void *buffer, int length);

int _close(int file)
{
    (void)file;
    return -1;
}

int _lseek(int file, int offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
    return -1;
}

int _read(int file, void *buffer, int length)
{
    (void)file;
    (void)buffer;
    (void)length;
    return -1;
}

int _write(int file, const void *buffer, int length)
{
    (void)file;
    (void)buffer;
    (void)length;
    return -1;
}
