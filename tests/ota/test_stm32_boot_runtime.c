/* SPDX-License-Identifier: GPL-3.0-only */
/* 실제 boot runtime C의 register/시간 오류 주입. 전기적 IWDG/HIL 모형은 아니다. */
#include "canview_boot_runtime.h"
#include "canview_boot_flash.h"
#include "register_model.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

model_rcc_t model_rcc;
model_iwdg_t model_iwdg;
model_debug_t model_debug;
model_dwt_t model_dwt;
model_scb_t model_scb;
uint32_t model_ipsr, model_control, model_basepri, model_faultmask;
static uint32_t primask, feeds, polls, keys[3], key_count;
static bool clock_stopped, lsi_stopped, update_stuck;
static volatile uint32_t *late_fault;
static uint32_t late_value;

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #test); return 1; } } while (0)
#define INTERVAL (UINT32_C(1600000))
#define DEADLINE (UINT32_C(480000000))

uint32_t model_primask(void) { return primask; }

void canview_stm_boot_test_key(uint32_t value)
{
    model_iwdg.KR = value;
    if (value == UINT32_C(0xaaaa)) { ++feeds; }
    if (key_count < 3U) { keys[key_count] = value; }
    ++key_count;
}

void canview_stm_test_poll(void)
{
    ++polls;
    if (!clock_stopped) { ++model_dwt.CYCCNT; }
    if (!lsi_stopped && (model_rcc.CSR & RCC_CSR_LSION) != 0U)
    {
        model_rcc.CSR |= RCC_CSR_LSIRDY;
    }
    if (key_count >= 2U)
    {
        if (!update_stuck) { model_iwdg.SR = 0U; }
        if (late_fault != NULL) { *late_fault = late_value; }
    }
}

static void reset_model(void)
{
    (void)memset(&model_rcc, 0, sizeof(model_rcc));
    (void)memset(&model_iwdg, 0, sizeof(model_iwdg));
    (void)memset(&model_debug, 0, sizeof(model_debug));
    (void)memset(&model_dwt, 0, sizeof(model_dwt));
    model_scb.VTOR = UINT32_C(0x08000000);
    model_rcc.CFGR = RCC_CFGR_SW_HSI | RCC_CFGR_SWS_HSI;
    model_rcc.CR = RCC_CR_HSION | RCC_CR_HSIRDY;
    model_iwdg.WINR = UINT32_C(4095);
    model_iwdg.RLR = UINT32_C(4095);
    model_iwdg.SR = IWDG_SR_PVU | IWDG_SR_RVU;
    model_ipsr = 0U; model_control = 0U; model_basepri = 0U; model_faultmask = 0U;
    primask = 0U; feeds = 0U; polls = 0U; key_count = 0U;
    clock_stopped = false; lsi_stopped = false; update_stuck = false;
    late_fault = NULL; late_value = 0U;
    canview_stm_boot_test_reset();
}

static int test_lifecycle(void)
{
    reset_model();
    CHECK(!canview_boot_runtime_ready());
    canview_boot_progress();
    CHECK(feeds == 0U && key_count == 0U && polls == 0U);
    CHECK(canview_boot_runtime_start() == CANVIEW_OK);
    CHECK(feeds == 1U && key_count == 3U);
    CHECK(keys[0] == UINT32_C(0xcccc) && keys[1] == UINT32_C(0x5555) && keys[2] == UINT32_C(0xaaaa));
    CHECK(model_iwdg.PR == 6U && model_iwdg.RLR == 4095U && model_iwdg.WINR == 4095U);
    CHECK(canview_boot_runtime_ready() && feeds == 1U);
    CHECK(canview_boot_runtime_start() == CANVIEW_RESOURCE_BUSY && feeds == 1U);
    const uint32_t last = model_dwt.CYCCNT - 1U; /* ready의 ticking probe 한 cycle */
    model_dwt.CYCCNT = last + INTERVAL - 2U;
    canview_boot_progress();
    CHECK(feeds == 1U);
    canview_boot_progress();
    CHECK(feeds == 2U); /* exactly interval */
    canview_boot_progress();
    CHECK(feeds == 2U); /* duplicate progress는 feed 아님 */
    for (uint32_t count = 0U; count < 1000U; ++count) { CHECK(canview_boot_runtime_ready()); }
    CHECK(feeds == 2U); /* readiness polling은 feed 아님 */
    return 0;
}

static int test_deadline_and_wrap(void)
{
    const uint32_t starts[] = {0U, UINT32_MAX - 3U, UINT32_MAX - INTERVAL, UINT32_MAX - DEADLINE};
    for (size_t index = 0U; index < sizeof(starts) / sizeof(starts[0]); ++index)
    {
        reset_model();
        model_dwt.CYCCNT = starts[index];
        CHECK(canview_boot_runtime_start() == CANVIEW_OK);
        const uint32_t origin = model_dwt.CYCCNT - 1U; /* start 내부 ready 직전 */
        model_dwt.CYCCNT += INTERVAL - 1U;
        canview_boot_progress();
        CHECK(feeds == 2U);
        model_dwt.CYCCNT = origin + DEADLINE - 2U;
        CHECK(canview_boot_runtime_ready());
        canview_boot_progress();
        CHECK(!canview_boot_runtime_ready() && feeds == 2U);
        model_dwt.CYCCNT = origin; /* deadline 위반 뒤 시간 복구로 latch 해제 안 됨 */
        canview_boot_progress();
        CHECK(!canview_boot_runtime_ready() && feeds == 2U);
        CHECK(canview_boot_runtime_start() == CANVIEW_RESOURCE_BUSY);
    }
    reset_model();
    CHECK(canview_boot_runtime_start() == CANVIEW_OK);
    --model_dwt.CYCCNT;
    model_dwt.CYCCNT -= 100U; /* backward clock => unsigned elapsed 범위 밖 */
    canview_boot_progress();
    CHECK(!canview_boot_runtime_ready() && feeds == 1U);
    reset_model();
    CHECK(canview_boot_runtime_start() == CANVIEW_OK);
    clock_stopped = true;
    canview_boot_progress();
    CHECK(!canview_boot_runtime_ready() && feeds == 1U);
    clock_stopped = false;
    CHECK(!canview_boot_runtime_ready());
    reset_model();
    CHECK(canview_boot_runtime_start() == CANVIEW_OK);
    model_dwt.CYCCNT += INTERVAL / 2U;
    CHECK(canview_boot_runtime_ready());
    model_dwt.CYCCNT -= 10U; /* start보다 뒤지만 직전 관측보다 역행 */
    canview_boot_progress();
    CHECK(!canview_boot_runtime_ready() && feeds == 1U);
    return 0;
}

static int test_rejections(void)
{
    struct mutation { volatile uint32_t *reg; uint32_t value; bool before_start; };
    const struct mutation cases[] = {
        {&model_ipsr, 1U, true}, {&model_control, 1U, true}, {&model_control, 2U, true},
        {&model_basepri, 1U, true}, {&model_faultmask, 1U, true}, {&primask, 1U, true},
        {&model_rcc.CFGR, 0U, true}, {&model_rcc.CFGR, RCC_CFGR_SW_PLL | RCC_CFGR_SWS_PLL, true},
        {&model_rcc.CFGR, RCC_CFGR_SW_HSI | RCC_CFGR_SWS_HSI | RCC_CFGR_HPRE_DIV2, true},
        {&model_rcc.CFGR, RCC_CFGR_SW_HSI | RCC_CFGR_SWS_HSI | RCC_CFGR_PPRE1_DIV2, true},
        {&model_rcc.CFGR, RCC_CFGR_SW_HSI | RCC_CFGR_SWS_HSI | RCC_CFGR_PPRE2_DIV2, true},
        {&model_rcc.CR, RCC_CR_HSIRDY, true}, {&model_rcc.CR, RCC_CR_HSION, true},
        {&model_rcc.CR, RCC_CR_HSION | RCC_CR_HSIRDY | RCC_CR_HSEON, true},
        {&model_rcc.CR, RCC_CR_HSION | RCC_CR_HSIRDY | RCC_CR_PLLON, true},
        {&model_rcc.CR, RCC_CR_HSION | RCC_CR_HSIRDY | RCC_CR_CSSON, true},
        {&model_iwdg.WINR, 4094U, true},
        {&model_rcc.CSR, RCC_CSR_LSION, false}, {&model_rcc.CSR, RCC_CSR_LSIRDY, false},
        {&model_debug.DEMCR, 0U, false}, {&model_dwt.CTRL, 0U, false},
        {&model_iwdg.PR, 5U, false}, {&model_iwdg.RLR, 4094U, false},
        {&model_iwdg.SR, IWDG_SR_PVU, false}, {&model_iwdg.SR, IWDG_SR_RVU, false},
        {&model_iwdg.SR, IWDG_SR_WVU, false}
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index)
    {
        reset_model();
        if (cases[index].before_start)
        {
            *cases[index].reg = cases[index].value;
            CHECK(canview_boot_runtime_start() == CANVIEW_RESOURCE_BUSY);
            CHECK(feeds == 0U && key_count == 0U);
        }
        reset_model();
        CHECK(canview_boot_runtime_start() == CANVIEW_OK);
        const uint32_t saved = *cases[index].reg;
        *cases[index].reg = cases[index].value;
        model_dwt.CYCCNT += INTERVAL;
        canview_boot_progress();
        CHECK(!canview_boot_runtime_ready() && feeds == 1U);
        *cases[index].reg = saved;
        CHECK(!canview_boot_runtime_ready());
        CHECK(canview_boot_runtime_start() == CANVIEW_RESOURCE_BUSY);
    }
    reset_model();
    model_scb.VTOR = UINT32_C(0x08010200);
    CHECK(canview_boot_runtime_start() == CANVIEW_RESOURCE_BUSY && feeds == 0U);
    reset_model();
    CHECK(canview_boot_runtime_start() == CANVIEW_OK);
    model_scb.VTOR = 0U;
    canview_boot_progress();
    CHECK(!canview_boot_runtime_ready() && feeds == 1U);
    return 0;
}

static int test_partial_initialization(void)
{
    for (uint32_t failure = 0U; failure < 3U; ++failure)
    {
        reset_model();
        clock_stopped = failure == 0U;
        lsi_stopped = failure == 1U;
        update_stuck = failure == 2U;
        CHECK(canview_boot_runtime_start() == CANVIEW_TIMEOUT);
        CHECK(!canview_boot_runtime_ready() && feeds == 0U);
        CHECK(polls <= UINT32_C(1000003));
        CHECK((key_count == 2U) == (failure == 2U));
        clock_stopped = false; lsi_stopped = false; update_stuck = false;
        CHECK(canview_boot_runtime_start() == CANVIEW_RESOURCE_BUSY);
        canview_boot_progress();
        CHECK(feeds == 0U);
    }
    /* IWDG를 이미 켠 뒤 context/clock/config가 손상돼도 최초 feed를 하면 안 된다. */
    volatile uint32_t *const regs[] = {&model_ipsr, &model_debug.DEMCR, &model_dwt.CTRL,
        &model_iwdg.PR, &model_iwdg.RLR, &model_iwdg.WINR, &model_rcc.CR, &model_rcc.CSR};
    for (size_t index = 0U; index < sizeof(regs) / sizeof(regs[0]); ++index)
    {
        reset_model();
        late_fault = regs[index];
        late_value = index == 0U ? 1U : 0U;
        CHECK(canview_boot_runtime_start() == CANVIEW_TIMEOUT);
        CHECK(feeds == 0U && !canview_boot_runtime_ready());
        late_fault = NULL;
        CHECK(canview_boot_runtime_start() == CANVIEW_RESOURCE_BUSY);
    }
    return 0;
}

int main(void)
{
    CHECK(test_lifecycle() == 0);
    CHECK(test_deadline_and_wrap() == 0);
    CHECK(test_rejections() == 0);
    CHECK(test_partial_initialization() == 0);
    puts("PASS: boot runtime lifecycle/deadline/wrap/stall/context/config/partial init; physical IWDG NOT_RUN");
    return 0;
}
