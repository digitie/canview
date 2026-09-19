/* SPDX-License-Identifier: GPL-3.0-only */
/* 실제 handoff C의 경계/cleanup 시험. runtime/read는 오류 주입 대역이며 HIL이 아니다. */
#include "canview_boot_handoff.h"
#include "canview_boot_runtime.h"
#include "canview_stm_flash_read.h"
#include "flash_layout.h"
#include "register_model.h"
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

model_scb_t model_scb;
model_systick_t model_systick;
model_nvic_t model_nvic;
model_mpu_t model_mpu;
model_fpu_t model_fpu;
static jmp_buf branch_return;
static uint32_t vector_stack, vector_entry, branch_stack, branch_entry;
static unsigned ready_calls, read_calls, disabled, branches, fail_ready_at;
static bool late_mpu, late_fpu;
static canview_status_t read_status;

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); return 1; } } while (0)
#define REQUIRE(test) do { if (!(test)) { \
    fprintf(stderr, "FAIL hook %s:%d: %s\n", __FILE__, __LINE__, #test); abort(); } } while (0)
#define PAYLOAD (1024U)
#define FIRST_PC (CANVIEW_STM_PRIMARY_VECTOR + CANVIEW_STM_PRIMARY_VECTOR_BYTES)

bool canview_boot_runtime_ready(void)
{
    REQUIRE(disabled == 0U && branches == 0U);
    ++ready_calls;
    return ready_calls != fail_ready_at;
}

static void put_u32(uint8_t *bytes, uint32_t value)
{
    for (unsigned index = 0U; index < 4U; ++index)
    {
        bytes[index] = (uint8_t)(value >> (8U * index));
    }
}

canview_status_t canview_stm_flash_read(uint32_t address, void *destination, uint32_t length)
{
    REQUIRE(ready_calls == 1U && disabled == 0U && read_calls == 0U);
    REQUIRE(address == CANVIEW_STM_PRIMARY_VECTOR && destination != NULL && length == 8U);
    ++read_calls;
    if (read_status != CANVIEW_OK) { return read_status; }
    put_u32(destination, vector_stack);
    put_u32((uint8_t *)destination + 4U, vector_entry);
    if (late_mpu) { model_mpu.CTRL = 1U; }
    if (late_fpu) { model_fpu.FPCCR = FPU_FPCCR_LSPACT_Msk; }
    return CANVIEW_OK;
}

void model_disable_irq(void)
{
    REQUIRE(ready_calls == 2U && read_calls == 1U && disabled == 0U);
    REQUIRE(model_scb.VTOR == CANVIEW_STM_FLASH_BASE && model_scb.ICSR == 17U);
    REQUIRE(model_systick.CTRL == 7U && model_systick.LOAD == 123U && model_systick.VAL == 45U);
    for (size_t index = 0U; index < 8U; ++index)
    {
        REQUIRE(model_nvic.ICER[index] == 0U && model_nvic.ICPR[index] == 0U);
    }
    ++disabled;
}

void canview_stm_handoff_test_branch(uint32_t stack, uint32_t entry)
{
    REQUIRE(disabled == 1U && ready_calls == 2U && read_calls == 1U);
    REQUIRE(model_scb.VTOR == CANVIEW_STM_PRIMARY_VECTOR);
    REQUIRE(model_scb.ICSR == (SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk));
    REQUIRE(model_systick.CTRL == 0U && model_systick.LOAD == 0U && model_systick.VAL == 0U);
    for (size_t index = 0U; index < 8U; ++index)
    {
        REQUIRE(model_nvic.ICER[index] == UINT32_MAX && model_nvic.ICPR[index] == UINT32_MAX);
    }
    ++branches;
    branch_stack = stack;
    branch_entry = entry;
    longjmp(branch_return, 1);
}

static void reset_model(void)
{
    model_scb.VTOR = CANVIEW_STM_FLASH_BASE;
    model_scb.ICSR = 17U;
    model_systick.CTRL = 7U; model_systick.LOAD = 123U; model_systick.VAL = 45U;
    (void)memset(&model_nvic, 0, sizeof(model_nvic));
    model_mpu.CTRL = 0U; model_fpu.FPCCR = 0U;
    vector_stack = CANVIEW_STM_APP_STACK_TOP; vector_entry = FIRST_PC | 1U;
    branch_stack = 0U; branch_entry = 0U;
    ready_calls = 0U; read_calls = 0U; disabled = 0U; branches = 0U; fail_ready_at = 0U;
    late_mpu = false; late_fpu = false; read_status = CANVIEW_OK;
}

static int unchanged(void)
{
    CHECK(disabled == 0U && branches == 0U);
    CHECK(model_scb.VTOR == CANVIEW_STM_FLASH_BASE && model_scb.ICSR == 17U);
    CHECK(model_systick.CTRL == 7U && model_systick.LOAD == 123U && model_systick.VAL == 45U);
    for (size_t index = 0U; index < 8U; ++index)
    {
        CHECK(model_nvic.ICER[index] == 0U && model_nvic.ICPR[index] == 0U);
    }
    return 0;
}

static int test_bounds(void)
{
    const uint32_t lengths[] = {0U, 1U, CANVIEW_STM_PRIMARY_VECTOR_BYTES,
        CANVIEW_STM_PRIMARY_VECTOR_BYTES + 1U, CANVIEW_STM_PRIMARY_VECTOR_BYTES + 2U,
        CANVIEW_STM_PRIMARY_VECTOR_BYTES + 3U, CANVIEW_STM_APP_PAYLOAD_MAX_BYTES + 1U, UINT32_MAX};
    for (size_t index = 0U; index < sizeof(lengths) / sizeof(lengths[0]); ++index)
    {
        reset_model();
        CHECK(canview_boot_handoff(lengths[index]) == CANVIEW_INVALID_ARGUMENT);
        CHECK(ready_calls == 0U && read_calls == 0U && unchanged() == 0);
    }
    for (unsigned bit = 0U; bit < 32U; ++bit)
    {
        reset_model(); vector_stack ^= UINT32_C(1) << bit;
        CHECK(canview_boot_handoff(PAYLOAD) == CANVIEW_AUTH_FAILED);
        CHECK(ready_calls == 1U && read_calls == 1U && unchanged() == 0);
    }
    const uint32_t entries[] = {0U, UINT32_MAX, FIRST_PC, FIRST_PC - 1U,
        CANVIEW_STM_PRIMARY_VECTOR | 1U, (CANVIEW_STM_PRIMARY_VECTOR + PAYLOAD) | 1U,
        (CANVIEW_STM_PRIMARY_VECTOR + PAYLOAD - 2U) | 1U,
        CANVIEW_STM_SECONDARY_IMAGE | 1U, CANVIEW_STM_APP_STACK_TOP | 1U};
    for (size_t index = 0U; index < sizeof(entries) / sizeof(entries[0]); ++index)
    {
        reset_model(); vector_entry = entries[index];
        CHECK(canview_boot_handoff(PAYLOAD) == CANVIEW_AUTH_FAILED);
        CHECK(ready_calls == 1U && read_calls == 1U && unchanged() == 0);
    }
    return 0;
}

static int test_faults(void)
{
    for (unsigned fault = 0U; fault < 6U; ++fault)
    {
        reset_model();
        if (fault < 2U) { fail_ready_at = fault + 1U; }
        if (fault == 2U) { model_mpu.CTRL = 1U; }
        if (fault == 3U) { model_fpu.FPCCR = FPU_FPCCR_LSPACT_Msk; }
        late_mpu = fault == 4U; late_fpu = fault == 5U;
        CHECK(canview_boot_handoff(PAYLOAD) == CANVIEW_RESOURCE_BUSY);
        CHECK(read_calls == ((fault == 1U || fault >= 4U) ? 1U : 0U));
        CHECK(unchanged() == 0);
    }
    const canview_status_t errors[] = {CANVIEW_INVALID_ARGUMENT, CANVIEW_AUTH_FAILED,
        CANVIEW_RESOURCE_BUSY, CANVIEW_TIMEOUT};
    for (size_t index = 0U; index < sizeof(errors) / sizeof(errors[0]); ++index)
    {
        reset_model(); read_status = errors[index];
        CHECK(canview_boot_handoff(PAYLOAD) == errors[index]);
        CHECK(ready_calls == 1U && read_calls == 1U && unchanged() == 0);
    }
    return 0;
}

static int successful_branch(uint32_t length, uint32_t entry)
{
    reset_model(); vector_entry = entry;
    model_fpu.FPCCR = UINT32_C(0xc0000000); /* idle ASPEN/LSPEN은 active context가 아님 */
    if (setjmp(branch_return) == 0)
    {
        (void)canview_boot_handoff(length);
        CHECK(false);
    }
    CHECK(branches == 1U && branch_stack == CANVIEW_STM_APP_STACK_TOP && branch_entry == entry);
    return 0;
}

int main(void)
{
    CHECK(test_bounds() == 0);
    CHECK(test_faults() == 0);
    CHECK(successful_branch(CANVIEW_STM_PRIMARY_VECTOR_BYTES + 4U, FIRST_PC | 1U) == 0);
    CHECK(successful_branch(PAYLOAD, (CANVIEW_STM_PRIMARY_VECTOR + PAYLOAD - 4U) | 1U) == 0);
    CHECK(successful_branch(CANVIEW_STM_APP_PAYLOAD_MAX_BYTES,
        (CANVIEW_STM_PRIMARY_VECTOR + CANVIEW_STM_APP_PAYLOAD_MAX_BYTES - 4U) | 1U) == 0);
    puts("PASS: handoff bounds/read/context/cleanup/branch; signature policy and HIL not simulated");
    return 0;
}
