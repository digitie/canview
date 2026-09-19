/* SPDX-License-Identifier: GPL-3.0-only */
/* IO 연결의 primitive 오류 주입. 실제 Flash/ECC/원자성 시험이 아니다. */
#include "canview_boot_flash.h"
#include "canview_stm_flash_read.h"
#include "canview_stm_flash_command.h"
#include "flash_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t flash[CANVIEW_STM_FLASH_BYTES];
static uint32_t reads, commands, progress, fail_read, fail_command, corrupt_command;
static int guard_result;
#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); return 1; } } while (0)
#define REQUIRE(test) do { if (!(test)) { \
    fprintf(stderr, "MODEL FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); abort(); } } while (0)

int canview_boot_flash_check(void) { return guard_result; }
void canview_boot_progress(void) { ++progress; }

canview_status_t canview_stm_flash_read(uint32_t address, void *destination, uint32_t length)
{
    REQUIRE(destination != NULL && length != 0U && length <= CANVIEW_STM_FLASH_READ_MAX);
    REQUIRE(address >= CANVIEW_STM_PRIMARY_ADDRESS && address <= CANVIEW_STM_POLICY_A_ADDRESS - length);
    if (++reads == fail_read) { return CANVIEW_INCOMPLETE; }
    memcpy(destination, flash + address - CANVIEW_STM_FLASH_BASE, length);
    return CANVIEW_OK;
}

canview_status_t canview_stm_flash_command(canview_stm_flash_operation_t operation,
    uint32_t address, uint32_t low, uint32_t high)
{
    REQUIRE(address >= CANVIEW_STM_PRIMARY_ADDRESS && address < CANVIEW_STM_POLICY_A_ADDRESS);
    if (++commands == fail_command) { return CANVIEW_INCOMPLETE; }
    uint8_t *const bytes = flash + address - CANVIEW_STM_FLASH_BASE;
    if (operation == CANVIEW_STM_FLASH_PROGRAM)
    {
        REQUIRE(address % 8U == 0U && (low != UINT32_MAX || high != UINT32_MAX));
        for (uint32_t index = 0U; index < 8U; ++index) { REQUIRE(bytes[index] == UINT8_MAX); }
        for (uint32_t index = 0U; index < 4U; ++index)
        {
            bytes[index] = (uint8_t)(low >> (index * 8U));
            bytes[index + 4U] = (uint8_t)(high >> (index * 8U));
        }
    }
    else
    {
        REQUIRE(operation == CANVIEW_STM_FLASH_ERASE && address % 2048U == 0U && low == 0U && high == 0U);
        memset(bytes, UINT8_MAX, CANVIEW_STM_FLASH_PAGE_BYTES);
    }
    if (commands == corrupt_command) { bytes[0] ^= 1U; }
    return CANVIEW_OK;
}

static void reset_model(void)
{
    memset(flash, UINT8_MAX, sizeof(flash));
    reads = commands = progress = 0U;
    fail_read = fail_command = corrupt_command = UINT32_MAX;
    guard_result = 0;
}

int main(void)
{
    const uint32_t start = CANVIEW_STM_PRIMARY_ADDRESS;
    uint8_t input[512], output[514];
    for (size_t index = 0U; index < sizeof(input); ++index) { input[index] = (uint8_t)index; }
    for (uint32_t length = 1U; length <= sizeof(input); ++length)
    {
        reset_model();
        memcpy(flash + start - CANVIEW_STM_FLASH_BASE + 1U, input, sizeof(input));
        memset(output, 0x5a, sizeof(output));
        CHECK(canview_boot_flash_read(start + 1U, output + 1U, length) == 0);
        CHECK(memcmp(output + 1U, input, length) == 0);
        CHECK(output[0] == 0x5aU && output[length + 1U] == 0x5aU);
        CHECK(reads == (length + 255U) / 256U && commands == 0U && progress == 0U);
    }
    const uint32_t invalid[] = {0U, start - 1U, CANVIEW_STM_POLICY_A_ADDRESS, UINT32_MAX};
    for (size_t index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
    {
        reset_model();
        CHECK(canview_boot_flash_read(invalid[index], output, 1U) < 0);
        CHECK(canview_boot_flash_write(invalid[index], input, 8U) < 0);
        CHECK(canview_boot_flash_erase(invalid[index], 2048U) < 0);
        CHECK(reads == 0U && commands == 0U && progress == 0U);
    }
    reset_model();
    CHECK(canview_boot_flash_read(start, NULL, 1U) < 0);
    CHECK(canview_boot_flash_read(start, output, 0U) < 0);
    CHECK(canview_boot_flash_read(start, output, UINT32_MAX) < 0);
    CHECK(canview_boot_flash_read(CANVIEW_STM_SECONDARY_ADDRESS - 1U, output, 2U) < 0);
    CHECK(canview_boot_flash_write(start, NULL, 8U) < 0);
    CHECK(canview_boot_flash_write(start, input, 0U) < 0);
    CHECK(canview_boot_flash_write(start, input, UINT32_MAX) < 0);
    CHECK(canview_boot_flash_write(start + 1U, input, 8U) < 0);
    CHECK(canview_boot_flash_write(start, input, 7U) < 0);
    CHECK(canview_boot_flash_erase(start, 0U) < 0);
    CHECK(canview_boot_flash_erase(start, UINT32_MAX) < 0);
    CHECK(canview_boot_flash_erase(start + 1U, 2048U) < 0);
    CHECK(canview_boot_flash_erase(start, 2047U) < 0);
    CHECK(reads == 0U && commands == 0U);
    guard_result = -1;
    CHECK(canview_boot_flash_write(start, input, 8U) < 0);
    CHECK(canview_boot_flash_erase(start, 2048U) < 0);
    CHECK(reads == 0U && commands == 0U && progress == 0U);

    reset_model();
    fail_read = 2U;
    memset(output, 0x5a, sizeof(output));
    CHECK(canview_boot_flash_read(start, output, 512U) < 0);
    CHECK(output[0] == UINT8_MAX && output[256] == 0x5aU && reads == 2U);
    /* 256B 초과 read의 실패 출력은 사용 불가. 이전 chunk를 atomic rollback하지 않는다. */
    for (uint32_t fault = 1U; fault <= 128U; ++fault)
    {
        reset_model();
        fail_read = fault;
        CHECK(canview_boot_flash_write(start, input, sizeof(input)) < 0);
        CHECK(reads == fault && commands == fault / 2U && progress == (fault - 1U) / 2U);
    }
    for (uint32_t fault = 1U; fault <= 64U; ++fault)
    {
        reset_model();
        fail_command = fault;
        CHECK(canview_boot_flash_write(start, input, sizeof(input)) < 0);
        CHECK(commands == fault && progress == fault - 1U);
        reset_model();
        corrupt_command = fault;
        CHECK(canview_boot_flash_write(start, input, sizeof(input)) < 0);
        CHECK(commands == fault && progress == fault - 1U);
    }
    reset_model();
    CHECK(canview_boot_flash_write(start, input, sizeof(input)) == 0);
    CHECK(memcmp(flash + start - CANVIEW_STM_FLASH_BASE, input, sizeof(input)) == 0);
    CHECK(commands == 64U && progress == 64U);
    CHECK(canview_boot_flash_write(start, input, 8U) < 0 && commands == 64U);
    reset_model();
    memset(input, UINT8_MAX, sizeof(input));
    CHECK(canview_boot_flash_write(start, input, sizeof(input)) == 0);
    CHECK(commands == 0U && progress == 0U && reads == 64U);
    flash[start - CANVIEW_STM_FLASH_BASE] = 0U;
    CHECK(canview_boot_flash_write(start, input, 8U) < 0 && commands == 0U);

    for (uint32_t fault = 1U; fault <= 16U; ++fault)
    {
        reset_model();
        fail_read = fault;
        CHECK(canview_boot_flash_erase(start, 4096U) < 0);
        CHECK(reads == fault && commands == (fault + 7U) / 8U && progress == (fault - 1U) / 8U);
    }
    for (uint32_t fault = 1U; fault <= 2U; ++fault)
    {
        reset_model();
        fail_command = fault;
        CHECK(canview_boot_flash_erase(start, 4096U) < 0 && commands == fault && progress == fault - 1U);
        reset_model();
        corrupt_command = fault;
        CHECK(canview_boot_flash_erase(start, 4096U) < 0 && commands == fault && progress == fault - 1U);
    }
    reset_model();
    memset(flash, 0x5a, sizeof(flash));
    CHECK(canview_boot_flash_erase(CANVIEW_STM_SECONDARY_ADDRESS, CANVIEW_STM_SECONDARY_BYTES) == 0);
    CHECK(commands == 97U && progress == 97U && reads == 776U);
    for (uint32_t index = 0U; index < sizeof(flash); ++index)
    {
        const uint32_t at = index + CANVIEW_STM_FLASH_BASE;
        CHECK(flash[index] == (at >= CANVIEW_STM_SECONDARY_ADDRESS && at < CANVIEW_STM_POLICY_A_ADDRESS ? UINT8_MAX : 0x5aU));
    }
    CHECK(canview_boot_flash_read(CANVIEW_STM_POLICY_A_ADDRESS - 1U, output, 1U) == 0);
    puts("PASS: bounded IO chunking, program/erase read-back, FF skip, duplicate/fault no retry and protected ranges");
    return 0;
}
