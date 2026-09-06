#ifndef CANVIEW_AUTOMATION_TEST_TICKS_H
#define CANVIEW_AUTOMATION_TEST_TICKS_H

#include <assert.h>

#include "canview_controller_automation.h"
#include "canview_auto_sport.h"

/* 지속시간 시험은 100 ms sample을 반복한다. gap 시험은 원 API를 직접 호출한다. */
static inline canview_auto_brightness_output_t brightness_update_for(
    canview_auto_brightness_state_t *state,
    const canview_auto_brightness_config_t *config,
    const canview_auto_brightness_input_t *input, uint32_t duration_ms)
{
    canview_auto_brightness_output_t output;
    bool return_to_default = false;
    do {
        const uint32_t tick = duration_ms > 100U ? 100U : duration_ms;
        output = canview_auto_brightness_update(state, config, input, tick);
        return_to_default = return_to_default || output.return_to_default_screen;
        duration_ms -= tick;
    } while (duration_ms > 0U);
    output.return_to_default_screen = return_to_default;
    return output;
}

static inline canview_adaptive_volume_output_t volume_update_for(
    canview_adaptive_volume_state_t *state,
    const canview_adaptive_volume_config_t *config,
    const canview_adaptive_volume_input_t *input, uint32_t duration_ms)
{
    canview_adaptive_volume_output_t output;
    canview_adaptive_volume_action_t action = CANVIEW_VOLUME_ACTION_NONE;
    do {
        const uint32_t tick = duration_ms > 100U ? 100U : duration_ms;
        output = canview_adaptive_volume_update(state, config, input, tick);
        if (output.action != CANVIEW_VOLUME_ACTION_NONE) {
            assert(action == CANVIEW_VOLUME_ACTION_NONE);
            action = output.action;
        }
        duration_ms -= tick;
    } while (duration_ms > 0U);
    output.action = action;
    return output;
}

static inline canview_auto_sport_output_t sport_update_for(
    canview_auto_sport_state_t *state,
    const canview_auto_sport_config_t *config,
    const canview_auto_sport_input_t *input, uint32_t duration_ms)
{
    canview_auto_sport_output_t output;
    canview_auto_sport_action_t action = CANVIEW_SPORT_ACTION_NONE;
    do {
        const uint32_t tick = duration_ms > 100U ? 100U : duration_ms;
        output = canview_auto_sport_update(state, config, input, tick);
        if (output.action != CANVIEW_SPORT_ACTION_NONE) {
            assert(action == CANVIEW_SPORT_ACTION_NONE);
            action = output.action;
        }
        duration_ms -= tick;
    } while (duration_ms > 0U);
    output.action = action;
    return output;
}

static inline canview_headlamp_warning_output_t headlamp_update_for(
    canview_headlamp_warning_state_t *state,
    const canview_headlamp_warning_config_t *config,
    const canview_headlamp_warning_input_t *input, uint32_t duration_ms)
{
    canview_headlamp_warning_output_t output;
    do {
        const uint32_t tick = duration_ms > 100U ? 100U : duration_ms;
        output = canview_headlamp_warning_update(state, config, input, tick);
        duration_ms -= tick;
    } while (duration_ms > 0U);
    return output;
}

#endif
