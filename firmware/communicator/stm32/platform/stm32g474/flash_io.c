/* SPDX-License-Identifier: GPL-3.0-only */
/** @file flash_io.c @brief MCUboot IO를 G474 bounded read/단일 명령에 연결한다. */
#include "canview_boot_flash.h"
#include "canview_stm_flash_read.h"
#include "canview_stm_flash_command.h"
#include "flash_layout.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static bool io_range(uint32_t address, uint32_t length, canview_stm_flash_operation_t operation)
{
    const bool secondary = address >= CANVIEW_STM_SECONDARY_ADDRESS;
    const uint32_t begin = secondary ? CANVIEW_STM_SECONDARY_ADDRESS : CANVIEW_STM_PRIMARY_ADDRESS;
    uint32_t checked;
    return address >= begin && canview_stm_flash_range(
        secondary ? CANVIEW_STM_FLASH_SECONDARY : CANVIEW_STM_FLASH_PRIMARY,
        operation, address - begin, length, &checked) == CANVIEW_OK && checked == address;
}

static bool erased(const uint8_t *bytes, uint32_t length)
{
    for (uint32_t index = 0U; index < length; ++index)
    {
        if (bytes[index] != UINT8_MAX) { return false; }
    }
    return true;
}

static uint32_t word_le(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) |
        ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}

int canview_boot_flash_read(uint32_t address, void *destination, uint32_t length)
{
    if (destination == NULL || !io_range(address, length, CANVIEW_STM_FLASH_READ)) { return -1; }
    uint8_t *const output = destination;
    uint32_t offset = 0U;
    while (offset < length)
    {
        const uint32_t remaining = length - offset;
        const uint32_t chunk = remaining < CANVIEW_STM_FLASH_READ_MAX ? remaining : CANVIEW_STM_FLASH_READ_MAX;
        if (canview_stm_flash_read(address + offset, output + offset, chunk) != CANVIEW_OK) { return -1; }
        offset += chunk;
    }
    return 0;
}

int canview_boot_flash_write(uint32_t address, const void *source, uint32_t length)
{
    if (source == NULL || !io_range(address, length, CANVIEW_STM_FLASH_PROGRAM) ||
        canview_boot_flash_check() != 0) { return -1; }
    const uint8_t *const input = source;
    uint8_t current[CANVIEW_STM_FLASH_WRITE_BYTES];
    for (uint32_t offset = 0U; offset < length; offset += CANVIEW_STM_FLASH_WRITE_BYTES)
    {
        /* FF 값을 다시 program하지 않는다. 이전 실패의 retry/erase 결정은 상위 책임이다. */
        if (canview_stm_flash_read(address + offset, current, sizeof(current)) != CANVIEW_OK ||
            !erased(current, sizeof(current))) { return -1; }
        if (erased(input + offset, sizeof(current))) { continue; }
        if (canview_stm_flash_command(CANVIEW_STM_FLASH_PROGRAM, address + offset,
            word_le(input + offset), word_le(input + offset + 4U)) != CANVIEW_OK) { return -1; }
        if (canview_stm_flash_read(address + offset, current, sizeof(current)) != CANVIEW_OK ||
            memcmp(current, input + offset, sizeof(current)) != 0) { return -1; }
        canview_boot_progress();
    }
    return 0;
}

int canview_boot_flash_erase(uint32_t address, uint32_t length)
{
    if (!io_range(address, length, CANVIEW_STM_FLASH_ERASE) || canview_boot_flash_check() != 0) { return -1; }
    uint8_t current[CANVIEW_STM_FLASH_READ_MAX];
    for (uint32_t offset = 0U; offset < length; offset += CANVIEW_STM_FLASH_PAGE_BYTES)
    {
        if (canview_stm_flash_command(CANVIEW_STM_FLASH_ERASE, address + offset, 0U, 0U) != CANVIEW_OK)
        {
            return -1;
        }
        for (uint32_t part = 0U; part < CANVIEW_STM_FLASH_PAGE_BYTES; part += sizeof(current))
        {
            if (canview_stm_flash_read(address + offset + part, current, sizeof(current)) != CANVIEW_OK ||
                !erased(current, sizeof(current))) { return -1; }
        }
        canview_boot_progress();
    }
    return 0;
}
