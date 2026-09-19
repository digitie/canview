/* SPDX-License-Identifier: GPL-3.0-only */
/** @file idf_receiver_host.c @brief SDK fixture와 같은 C 수신 흐름의 CNG host 실행. */
#include <stdio.h>
#include "receiver.h"

#define GOLDEN_BYTES (394310U)

int main(int argc, char **argv)
{
    static uint8_t data[GOLDEN_BYTES + 1U];
    if (argc != 2) { return 1; }
    FILE *input = NULL;
    if (fopen_s(&input, argv[1], "rb") != 0 || input == NULL) { return 1; }
    const size_t size = fread(data, 1U, sizeof(data), input);
    const bool valid = ferror(input) == 0 && size == GOLDEN_BYTES;
    if (fclose(input) != 0 || !valid) { return 1; }
    if (!canview_ota_fixture_receiver_test(data, size)) { return 1; }
    (void)puts("PASS: SDK fixture receiver 4 cases with actual CNG; PSA device execution NOT_RUN");
    return 0;
}
