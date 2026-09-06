# 계획·UI 독립 리뷰 추가 probe 보존

이 파일은 review evidence이며 제품 코드나 규범이 아니다. 임시 review worktree 정리 후에도 추가 재현 소스를 확인하도록 보존한다. 원본 파일의 byte SHA-256을 함께 기록했고 본문은 LF로 정규화했다. 현재 runtime 반영 여부는 [통합 리뷰](../2026-09-06-plan-ui.md)를 따른다. 파일을 복사해 실행할 때 작업 경로·include·library 경로는 해당 immutable checkout에 맞춘다.

## review_lvgl.c

- 원본 경로: `F:/dev/canview-wt/review-plan-ui-a/.tools/review-a-extra/review_lvgl.c`
- 원본 SHA-256: `7ead2199dbee847b03f1fdb461d8331b592b11294c830045b782ff4f9fca0b7c`

```text
/* 독립 reviewer 임시 시험. tracked source는 수정하지 않는다. */
#define main candidate_suite_main
#include "../../tests/lvgl/test_lvgl.c"
#undef main

static uint32_t reentrant_count;

static void recreate_sink(const canview_ui_command_t *command, void *context)
{
    (void)context;
    assert(command != NULL);
    const canview_ui_command_t copy = *command;
    ++reentrant_count;
    canview_ui_destroy();
    const canview_ui_config_t config = {
        .font = &lv_font_montserrat_14,
        .metric_font = &lv_font_montserrat_28,
        .command_cb = recreate_sink,
    };
    assert(canview_ui_create(NULL, &config) != NULL);
    canview_ui_model_t model = fixture();
    canview_ui_update(&model);
    /* 이전 instance의 ACK/완료를 전달하지 않고 새 instance 상태만 확인한다. */
    assert(copy.id == CANVIEW_UI_CMD_USER_ACTIVITY || copy.id == CANVIEW_UI_CMD_SET_QUIET);
    for (uint32_t i = 0; i < CANVIEW_UI_CMD_COUNT; ++i) assert(!ui.pending[i]);
}

int main(void)
{
    (void)_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    (void)_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    (void)_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    (void)SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    lv_init();
    lv_disp_draw_buf_init(&draw_buffer, draw_pixels, NULL, 320U * 20U);
    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = 320;
    display_driver.ver_res = 480;
    display_driver.draw_buf = &draw_buffer;
    display_driver.flush_cb = flush_display;
    assert(lv_disp_drv_register(&display_driver) != NULL);
    const canview_ui_config_t config = {
        .font = &lv_font_montserrat_14,
        .metric_font = &lv_font_montserrat_28,
        .command_cb = recreate_sink,
    };
    assert(canview_ui_create(NULL, &config) != NULL);
    canview_ui_destroy();
    advance(400U);
    lv_mem_monitor_t baseline;
    lv_mem_monitor(&baseline);
    for (uint32_t i = 0; i < 36U; ++i) {
        assert(canview_ui_create(NULL, &config) != NULL);
        canview_ui_model_t model = fixture();
        canview_ui_update(&model);
        lv_obj_t *target;
        lv_event_code_t code = LV_EVENT_PRESSED;
        if (i % 3U == 0U) {
            target = ui.nav_buttons[CANVIEW_UI_SCREEN_SETTINGS];
        } else if (i % 3U == 1U) {
            canview_ui_show_screen(CANVIEW_UI_SCREEN_SETTINGS);
            lv_dropdown_open(ui.rtc_minute_dropdown);
            target = lv_dropdown_get_list(ui.rtc_minute_dropdown);
        } else {
            canview_ui_show_screen(CANVIEW_UI_SCREEN_AUDIO);
            target = ui.quiet_button;
            code = LV_EVENT_CLICKED;
        }
        const uint32_t before = reentrant_count;
        assert(lv_event_send(target, code, NULL) == LV_RES_INV);
        assert(reentrant_count == before + 1U);
        assert(ui.root != NULL);
        advance(50U);
        canview_ui_destroy();
        advance(400U);
        assert(lv_anim_count_running() == 0U);
        assert(lv_mem_test() == LV_RES_OK);
        lv_mem_monitor_t current;
        lv_mem_monitor(&current);
        assert(current.free_size == baseline.free_size);
    }
    puts("PASS: reviewer-A 36 reentrant destroy/recreate cases (nav activity, popup activity, command), no heap growth");
    return 0;
}
```

## CMakeLists.txt

- 원본 경로: `F:/dev/canview-wt/review-plan-ui-a/.tools/review-a-extra/CMakeLists.txt`
- 원본 SHA-256: `1eb3e25a31d8104c57a78822f335f0daafd0cab3d9122962a9136fcfff8b3c6f`

```text
cmake_minimum_required(VERSION 3.20)
project(review_a_extra LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_executable(review_a_extra review_lvgl.c)
target_include_directories(review_a_extra SYSTEM PRIVATE "F:/dev/canview/.tools/lvgl-8.4.0")
target_include_directories(review_a_extra PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../tests/lvgl")
target_compile_definitions(review_a_extra PRIVATE LV_CONF_INCLUDE_SIMPLE _CRT_SECURE_NO_WARNINGS)
target_compile_options(review_a_extra PRIVATE /utf-8 /W4 /WX /UNDEBUG)
target_link_libraries(review_a_extra PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../lvgl-host-build/lvgl_real.lib")
enable_testing()
add_test(NAME review_a_activity_recreate COMMAND review_a_extra)
set_tests_properties(review_a_activity_recreate PROPERTIES TIMEOUT 60)
```

## review-b-sport-gap.c

- 원본 경로: `F:/dev/canview-wt/review-plan-ui-b/.tools/review-b-sport-gap.c`
- 원본 SHA-256: `e04df1c68a0aa717663e45a63979fd39c6ff96984b79822b9c19cf78ad2e6a64`

```text
#include "canview_auto_sport.h"
#include <assert.h>
#include <stdio.h>

static void probe(uint32_t gap_ms)
{
    canview_auto_sport_config_t config = canview_auto_sport_default_config();
    canview_auto_sport_state_t state = {
        .status = CANVIEW_SPORT_ACTIVE,
        .previous_mode = CANVIEW_DRIVE_MODE_ECO,
        .owns_sport_mode = true,
    };
    canview_auto_sport_input_t input = {
        .enabled = true, .signals_fresh = true, .forward_gear = true,
        .control_link_ready = true, .current_mode = CANVIEW_DRIVE_MODE_NORMAL,
    };
    canview_auto_sport_output_t out =
        canview_auto_sport_update(&state, &config, &input, gap_ms);
    printf("gap=%u observed NORMAL: status=%d owns=%d previous=%d action=%d\n",
           gap_ms, out.status, state.owns_sport_mode, state.previous_mode, out.action);
    input.current_mode = CANVIEW_DRIVE_MODE_SPORT;
    input.enabled = false;
    out = canview_auto_sport_update(&state, &config, &input, 100U);
    printf("then observed SPORT and disable: status=%d owns=%d restore=%d action=%d\n",
           out.status, state.owns_sport_mode, out.restore_mode, out.action);
    if (gap_ms == 100U) assert(out.action == CANVIEW_SPORT_ACTION_NONE);
    if (gap_ms == 251U) assert(out.action == CANVIEW_SPORT_ACTION_RESTORE_PREVIOUS);
}

int main(void)
{
    probe(100U);
    probe(251U);
    return 0;
}
```

## review-b-browser.cjs

- 원본 경로: `F:/dev/canview-wt/review-plan-ui-b/.tools/review-b-browser.cjs`
- 원본 SHA-256: `e9ecb2187f0ba8532fcb8de8a381e97db882a68591edb083587209c6bc5f436c`

```text
const {chromium} = require('playwright');
const {pathToFileURL} = require('node:url');
(async () => {
  const browser = await chromium.launch({channel: 'msedge', headless: true});
  try {
    const page = await browser.newPage({viewport: {width: 320, height: 480}, reducedMotion: 'reduce'});
    await page.route(/^https?:/, route => route.abort());
    await page.clock.install({time: new Date('2026-09-06T03:00:00Z')});
    await page.goto(pathToFileURL('F:/dev/canview-wt/review-plan-ui-b/ui/prototype/index.html').href + '?screen=audio&warning=1&headlamp=1');
    await page.locator('[data-profile="quiet"]').click();
    await page.locator('[data-profile="rear"]').click();
    await page.locator('[data-target="drive"]').click();
    async function snapshot(label) {
      console.log(label, await page.evaluate(() => {
        const panel = document.querySelector('#screen-drive');
        return {active: panel.classList.contains('is-active'), hidden: panel.hidden,
          opacity: getComputedStyle(panel).opacity,
          animations: panel.getAnimations().map(a => ({state: a.playState, time: a.currentTime}))};
      }));
    }
    await snapshot('immediate');
    await page.screenshot({path: 'F:/dev/canview-wt/review-plan-ui-b/.tools/review-b-warning-immediate.png'});
    await page.clock.runFor(200);
    await snapshot('after 200ms');
    await page.screenshot({path: 'F:/dev/canview-wt/review-plan-ui-b/.tools/review-b-warning-settled.png'});
  } finally { await browser.close(); }
})().catch(error => {console.error(error); process.exitCode = 1;});
```
