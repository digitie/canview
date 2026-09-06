# 계획·UI post-fix 독립 probe 보존

재검토의 추가 probe 사본이며 제품 코드나 규범이 아니다. SHA-256은 원본 bytes, 본문은 LF 정규화다. 경로는 재현할 immutable checkout에 맞춘다. A의 `baseline_sport.c`는 `git show d078437722635a2ef4c067ca3c08bc6d801d270c:firmware/communicator/stm32/src/canview_auto_sport.c` 원문이다. 중복 복제하지 않는다. A의 baseline PASS는 결함 재현 성공이며 정상 동작 승인과 반대다. 결과는 [통합 리뷰](../2026-09-06-plan-ui.md)에 있다.

## sport_probe.c

- 원본: `F:/dev/canview-wt/review-plan-ui-a/.tools/review-a-postfix/sport_probe.c`
- SHA-256: `e94931aea5fa5ab9fa04f4bac79c2b1b303591b57fc5b4a6b19f9bdfc7a77943`

```text
/* 독립 A 교차 probe: 정상 진입부터 소유권을 얻어 이전/수정 소스의 차이를 확인한다. */
#include "canview_auto_sport.h"
#include <stdio.h>
#include <stdint.h>

static uint32_t run_case(uint32_t elapsed, canview_drive_mode_t external, bool enabled, bool fresh)
{
    canview_auto_sport_state_t state = {0};
    const canview_auto_sport_config_t config = canview_auto_sport_default_config();
    canview_auto_sport_input_t input = {
        .enabled = true, .signals_fresh = true, .forward_gear = true,
        .control_link_ready = true, .speed_tenth_kph = 800U,
        .current_mode = CANVIEW_DRIVE_MODE_ECO,
    };
    canview_auto_sport_output_t output = {0};
    for (uint32_t i = 0U; i < 25U; ++i) {
        output = canview_auto_sport_update(&state, &config, &input, 100U);
    }
    if (output.action != CANVIEW_SPORT_ACTION_ENTER) return 1U;
    input.current_mode = CANVIEW_DRIVE_MODE_SPORT;
    output = canview_auto_sport_update(&state, &config, &input, 100U);
    if (!state.owns_sport_mode || output.status != CANVIEW_SPORT_ACTIVE) return 2U;
    input.current_mode = external;
    input.enabled = enabled;
    input.signals_fresh = fresh;
    output = canview_auto_sport_update(&state, &config, &input, elapsed);
    if (output.action != CANVIEW_SPORT_ACTION_NONE) return 3U;
    bool should_revoke = fresh;
#ifdef EXPECT_BASELINE_BUG
    should_revoke = fresh && elapsed <= CANVIEW_SPORT_MAX_TICK_GAP_MS;
#endif
    if (state.owns_sport_mode == should_revoke) return 4U;
    if (should_revoke && (state.previous_mode != CANVIEW_DRIVE_MODE_UNKNOWN ||
                         output.status != CANVIEW_SPORT_MANUAL_HOLD)) return 5U;
    input.current_mode = CANVIEW_DRIVE_MODE_SPORT;
    input.enabled = false;
    input.signals_fresh = true;
    output = canview_auto_sport_update(&state, &config, &input, 100U);
    if (should_revoke && output.action != CANVIEW_SPORT_ACTION_NONE) return 6U;
    if (!should_revoke && output.action != CANVIEW_SPORT_ACTION_RESTORE_PREVIOUS) return 7U;
    if (!should_revoke && output.restore_mode != CANVIEW_DRIVE_MODE_ECO) return 8U;
    return 0U;
}

int main(void)
{
    const uint32_t elapsed[] = {100U, 250U, 251U, 5000U, UINT32_MAX};
    uint32_t checked = 0U;
    for (uint32_t i = 0U; i < 5U; ++i) {
        for (uint32_t mode = CANVIEW_DRIVE_MODE_NORMAL; mode < CANVIEW_DRIVE_MODE_SPORT; ++mode) {
            for (uint32_t enabled = 0U; enabled < 2U; ++enabled) {
                for (uint32_t fresh = 0U; fresh < 2U; ++fresh) {
                    const uint32_t failure = run_case(elapsed[i], (canview_drive_mode_t)mode,
                                                       enabled != 0U, fresh != 0U);
                    if (failure != 0U) {
                        printf("FAIL elapsed=%u mode=%u enabled=%u fresh=%u step=%u\n",
                               elapsed[i], mode, enabled, fresh, failure);
                        return 1;
                    }
                    ++checked;
                }
            }
        }
    }
#ifdef EXPECT_BASELINE_BUG
    printf("BASELINE: %u cases match old behavior; fresh external mode + gap retains ECO restore (B-01 reproduced)\n", checked);
#else
    printf("FIXED: %u cases; fresh external mode revokes with/without gap; stale observation does not falsely revoke\n", checked);
#endif
    return 0;
}
```

## CMakeLists.txt

- 원본: `F:/dev/canview-wt/review-plan-ui-a/.tools/review-a-postfix/CMakeLists.txt`
- SHA-256: `420d5dce14b21597e6e075b3d605a84d228e5158e6be6aa28fc17b556f4b69b2`

```text
cmake_minimum_required(VERSION 3.20)
project(review_a_postfix LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
foreach(variant baseline fixed)
  if(variant STREQUAL "baseline")
    set(sport_source baseline_sport.c)
  else()
    set(sport_source ../../firmware/communicator/stm32/src/canview_auto_sport.c)
  endif()
  add_executable(probe_${variant} sport_probe.c ${sport_source})
  target_include_directories(probe_${variant} PRIVATE ../../firmware/communicator/stm32/include)
  target_compile_options(probe_${variant} PRIVATE /utf-8 /W4 /WX /UNDEBUG)
  if(variant STREQUAL "baseline")
    target_compile_definitions(probe_${variant} PRIVATE EXPECT_BASELINE_BUG)
  endif()
endforeach()
enable_testing()
add_test(NAME reproduce_baseline_bug COMMAND probe_baseline)
add_test(NAME confirm_fixed_revocation COMMAND probe_fixed)
set_tests_properties(reproduce_baseline_bug confirm_fixed_revocation PROPERTIES TIMEOUT 30)
```

## review-b-post-sport.c

- 원본: `F:/dev/canview-wt/review-plan-ui-b/.tools/review-b-post-sport.c`
- SHA-256: `be34d3fb79fe80d9309cdfe39a2f521657a39f264aabfcec3b415c98a077b315`

```text
#include "canview_auto_sport.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
#ifdef _MSC_VER
    (void)_set_error_mode(_OUT_TO_STDERR);
    (void)_set_abort_behavior(0U, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    const uint32_t intervals[] = {100U, 250U, 251U, 5000U, UINT32_MAX};
    const canview_auto_sport_config_t config = canview_auto_sport_default_config();
    uint32_t cases = 0U;
    for (uint32_t mode = CANVIEW_DRIVE_MODE_NORMAL; mode < CANVIEW_DRIVE_MODE_SPORT; ++mode) {
        for (uint32_t t = 0U; t < 5U; ++t) {
            for (uint32_t stale_first = 0U; stale_first < 2U; ++stale_first) {
                canview_auto_sport_state_t state = {0};
                canview_auto_sport_input_t input = {
                    .enabled = true, .signals_fresh = true, .forward_gear = true,
                    .control_link_ready = true, .speed_tenth_kph = 800U,
                    .current_mode = CANVIEW_DRIVE_MODE_ECO,
                };
                canview_auto_sport_output_t output = {0};
                for (uint32_t sample = 0U; sample < 25U; ++sample) {
                    output = canview_auto_sport_update(&state, &config, &input, 100U);
                }
                assert(output.action == CANVIEW_SPORT_ACTION_ENTER);
                input.current_mode = CANVIEW_DRIVE_MODE_SPORT;
                output = canview_auto_sport_update(&state, &config, &input, 100U);
                assert(state.owns_sport_mode && output.status == CANVIEW_SPORT_ACTIVE);
                input.current_mode = (canview_drive_mode_t)mode;
                if (stale_first != 0U) {
                    input.signals_fresh = false;
                    output = canview_auto_sport_update(&state, &config, &input, intervals[t]);
                    assert(output.action == CANVIEW_SPORT_ACTION_NONE);
                    assert(state.owns_sport_mode);
                    input.signals_fresh = true;
                }
                output = canview_auto_sport_update(&state, &config, &input, intervals[t]);
                assert(output.status == CANVIEW_SPORT_MANUAL_HOLD);
                assert(output.action == CANVIEW_SPORT_ACTION_NONE);
                assert(!state.owns_sport_mode);
                assert(state.previous_mode == CANVIEW_DRIVE_MODE_UNKNOWN);
                input.current_mode = CANVIEW_DRIVE_MODE_SPORT;
                for (uint32_t next = 0U; next < 300U; ++next) {
                    input.enabled = next != 1U;
                    output = canview_auto_sport_update(&state, &config, &input, 100U);
                    assert(output.action == CANVIEW_SPORT_ACTION_NONE);
                    assert(!state.owns_sport_mode);
                }
                ++cases;
            }
        }
    }
    printf("PASS: %u independent SPORT revocation cases; normal entry, stale recovery, gap, user SPORT, disable/re-enable, 30-second tail\n", cases);
    return 0;
}
```
