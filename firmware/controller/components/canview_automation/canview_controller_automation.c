#include "canview_controller_automation.h"

#include <limits.h>
#include <stddef.h>

static uint32_t add_saturated(uint32_t value, uint32_t increment, uint32_t limit)
{
    if (value >= limit || increment >= limit - value) {
        return limit;
    }
    return value + increment;
}

static uint32_t subtract_saturated(uint32_t value, uint32_t decrement)
{
    return decrement >= value ? 0U : value - decrement;
}

static uint32_t doubled_saturated(uint32_t value)
{
    return value > UINT32_MAX / 2U ? UINT32_MAX : value * 2U;
}

static uint8_t clamp_percent(uint8_t value)
{
    return value > 100U ? 100U : value;
}

static uint16_t progress_permil(uint32_t value, uint32_t limit)
{
    if (limit == 0U || value >= limit) {
        return 1000U;
    }
    return (uint16_t)((value * 1000U) / limit);
}

canview_auto_brightness_config_t canview_auto_brightness_default_config(void)
{
    const canview_auto_brightness_config_t config = {
        .night_min_percent = 14U,
        .night_max_percent = 48U,
        .night_default_percent = 30U,
        .lamp_on_confirm_ms = 500U,
        .lamp_off_confirm_ms = 1500U,
        .transition_ms = 1200U,
        .stale_timeout_ms = 500U,
    };
    return config;
}

canview_adaptive_volume_config_t canview_adaptive_volume_default_config(void)
{
    const canview_adaptive_volume_config_t config = {
        .focus_low_hz = 160U,
        .focus_high_hz = 1250U,
        .min_speed_tenth_kph = 300U,
        .raise_excess_tenth_db = 50,
        .lower_excess_tenth_db = 25,
        .attack_ms = 5000U,
        .release_ms = 12000U,
        .step_interval_ms = 3000U,
        .feedback_freeze_ms = 2000U,
        .manual_hold_ms = 60000U,
        .max_offset_steps = 4U,
        .min_confidence_percent = 65U,
    };
    return config;
}

void canview_adaptive_volume_apply_settings(
    canview_adaptive_volume_config_t *config,
    canview_noise_band_profile_t band,
    canview_noise_sensitivity_t sensitivity,
    canview_noise_response_t response,
    uint8_t maximum_offset_steps)
{
    if (config == NULL) {
        return;
    }
    switch (band) {
    case CANVIEW_NOISE_BAND_ROAD:
        config->focus_low_hz = 125U;
        config->focus_high_hz = 500U;
        break;
    case CANVIEW_NOISE_BAND_WIND:
        config->focus_low_hz = 500U;
        config->focus_high_hz = 2000U;
        break;
    case CANVIEW_NOISE_BAND_BALANCED:
    default:
        config->focus_low_hz = 160U;
        config->focus_high_hz = 1250U;
        break;
    }

    switch (sensitivity) {
    case CANVIEW_NOISE_SENSITIVITY_LOW:
        config->raise_excess_tenth_db = 70;
        config->lower_excess_tenth_db = 45;
        break;
    case CANVIEW_NOISE_SENSITIVITY_HIGH:
        config->raise_excess_tenth_db = 35;
        config->lower_excess_tenth_db = 15;
        break;
    case CANVIEW_NOISE_SENSITIVITY_NORMAL:
    default:
        config->raise_excess_tenth_db = 50;
        config->lower_excess_tenth_db = 25;
        break;
    }

    switch (response) {
    case CANVIEW_NOISE_RESPONSE_GENTLE:
        config->attack_ms = 8000U;
        config->release_ms = 18000U;
        break;
    case CANVIEW_NOISE_RESPONSE_FAST:
        config->attack_ms = 3000U;
        config->release_ms = 8000U;
        break;
    case CANVIEW_NOISE_RESPONSE_NORMAL:
    default:
        config->attack_ms = 5000U;
        config->release_ms = 12000U;
        break;
    }
    config->max_offset_steps = maximum_offset_steps < 2U
                                   ? 2U
                                   : (maximum_offset_steps > 4U ? 4U
                                                                 : maximum_offset_steps);
}

void canview_auto_brightness_reset(canview_auto_brightness_state_t *state,
                                   uint8_t initial_percent)
{
    if (state == NULL) {
        return;
    }
    *state = (canview_auto_brightness_state_t){
        .initialized = true,
        .current_tenth_percent = (uint16_t)clamp_percent(initial_percent) * 10U,
    };
}

static uint8_t night_target(const canview_auto_brightness_config_t *config,
                            const canview_auto_brightness_input_t *input)
{
    const uint8_t minimum = clamp_percent(config->night_min_percent);
    const uint8_t maximum = clamp_percent(config->night_max_percent);
    if (!input->dimmer_valid || maximum <= minimum) {
        return clamp_percent(config->night_default_percent);
    }
    const uint8_t dimmer = clamp_percent(input->dimmer_percent);
    return (uint8_t)(minimum + (((uint16_t)(maximum - minimum) * dimmer + 50U) / 100U));
}

canview_auto_brightness_output_t canview_auto_brightness_update(
    canview_auto_brightness_state_t *state,
    const canview_auto_brightness_config_t *config,
    const canview_auto_brightness_input_t *input,
    uint32_t elapsed_ms)
{
    canview_auto_brightness_output_t output = {0};
    if (state == NULL || config == NULL || input == NULL) {
        output.status = CANVIEW_BRIGHTNESS_CAN_STALE;
        return output;
    }
    if (!state->initialized) {
        canview_auto_brightness_reset(state, input->manual_percent);
    }

    uint8_t target = clamp_percent(input->manual_percent);
    const bool signal_valid = input->lighting_valid &&
                              input->lighting_age_ms <= config->stale_timeout_ms;

    if (!input->enabled) {
        state->pending_ms = 0U;
        output.status = CANVIEW_BRIGHTNESS_MANUAL;
    } else if (!signal_valid) {
        target = (uint8_t)((state->current_tenth_percent + 5U) / 10U);
        state->pending_ms = 0U;
        output.status = CANVIEW_BRIGHTNESS_CAN_STALE;
    } else {
        const bool requested_night = input->exterior_lamps_on;
        if (requested_night == state->night_active) {
            state->pending_ms = 0U;
            state->pending_night = requested_night;
        } else {
            if (state->pending_ms == 0U || state->pending_night != requested_night) {
                state->pending_night = requested_night;
                state->pending_ms = 0U;
            }
            const uint32_t confirm_ms = requested_night ? config->lamp_on_confirm_ms
                                                        : config->lamp_off_confirm_ms;
            state->pending_ms = add_saturated(state->pending_ms, elapsed_ms, confirm_ms);
            if (state->pending_ms >= confirm_ms) {
                state->night_active = requested_night;
                state->pending_ms = 0U;
            }
        }

        if (state->night_active) {
            target = night_target(config, input);
            output.status = CANVIEW_BRIGHTNESS_CAN_NIGHT;
        } else {
            output.status = CANVIEW_BRIGHTNESS_CAN_DAY;
        }
    }

    const uint16_t target_tenth = (uint16_t)target * 10U;
    if (config->transition_ms == 0U) {
        state->current_tenth_percent = target_tenth;
    } else {
        uint32_t maximum_delta =
            (uint32_t)(((uint64_t)1000U * elapsed_ms + config->transition_ms - 1U) /
                       config->transition_ms);
        if (maximum_delta > 1000U) {
            maximum_delta = 1000U;
        }
        if (maximum_delta == 0U && elapsed_ms > 0U) {
            maximum_delta = 1U;
        }
        if (state->current_tenth_percent < target_tenth) {
            const uint32_t distance = target_tenth - state->current_tenth_percent;
            state->current_tenth_percent += (uint16_t)(distance < maximum_delta ? distance
                                                                                : maximum_delta);
        } else if (state->current_tenth_percent > target_tenth) {
            const uint32_t distance = state->current_tenth_percent - target_tenth;
            state->current_tenth_percent -= (uint16_t)(distance < maximum_delta ? distance
                                                                                : maximum_delta);
        }
    }

    output.brightness_percent = (uint8_t)((state->current_tenth_percent + 5U) / 10U);
    return output;
}

void canview_adaptive_volume_reset(canview_adaptive_volume_state_t *state,
                                   int8_t applied_offset_steps)
{
    if (state == NULL) {
        return;
    }
    *state = (canview_adaptive_volume_state_t){
        .initialized = true,
        .desired_offset_steps = applied_offset_steps,
    };
}

void canview_adaptive_volume_reconcile(canview_adaptive_volume_state_t *state,
                                       int8_t applied_offset_steps)
{
    if (state == NULL) {
        return;
    }
    state->initialized = true;
    state->desired_offset_steps = applied_offset_steps;
    state->attack_evidence_ms = 0U;
    state->release_evidence_ms = 0U;
}

canview_adaptive_volume_output_t canview_adaptive_volume_update(
    canview_adaptive_volume_state_t *state,
    const canview_adaptive_volume_config_t *config,
    const canview_adaptive_volume_input_t *input,
    uint32_t elapsed_ms)
{
    canview_adaptive_volume_output_t output = {0};
    if (state == NULL || config == NULL || input == NULL) {
        output.status = CANVIEW_VOLUME_INVALID;
        return output;
    }
    if (!state->initialized) {
        canview_adaptive_volume_reset(state, input->applied_offset_steps);
    }

    state->step_cooldown_ms = subtract_saturated(state->step_cooldown_ms, elapsed_ms);
    state->feedback_freeze_ms = subtract_saturated(state->feedback_freeze_ms, elapsed_ms);
    state->manual_hold_ms = subtract_saturated(state->manual_hold_ms, elapsed_ms);

    if (input->manual_volume_changed) {
        state->manual_hold_ms = config->manual_hold_ms;
        state->attack_evidence_ms = 0U;
        state->release_evidence_ms = 0U;
        if (state->desired_offset_steps != 0) {
            state->desired_offset_steps = 0;
            output.action = CANVIEW_VOLUME_ACTION_SET_OFFSET;
        }
    }

    if (!input->enabled) {
        state->attack_evidence_ms = 0U;
        state->release_evidence_ms = 0U;
        if (state->desired_offset_steps != 0) {
            state->desired_offset_steps = 0;
            output.action = CANVIEW_VOLUME_ACTION_SET_OFFSET;
        }
        output.status = CANVIEW_VOLUME_IDLE;
        output.target_offset_steps = state->desired_offset_steps;
        return output;
    }

    if (state->manual_hold_ms > 0U || input->audio_priority_active) {
        output.status = CANVIEW_VOLUME_PAUSED;
        output.target_offset_steps = state->desired_offset_steps;
        return output;
    }

    if (!input->fft_valid || input->confidence_percent < config->min_confidence_percent) {
        state->attack_evidence_ms = subtract_saturated(state->attack_evidence_ms, elapsed_ms);
        state->release_evidence_ms = subtract_saturated(state->release_evidence_ms, elapsed_ms);
        output.status = CANVIEW_VOLUME_INVALID;
        output.target_offset_steps = state->desired_offset_steps;
        return output;
    }

    const bool in_focus_band = input->dominant_frequency_hz >= config->focus_low_hz &&
                               input->dominant_frequency_hz <= config->focus_high_hz;
    const bool speed_ready = input->speed_tenth_kph >= config->min_speed_tenth_kph;
    const bool raise_condition = speed_ready && !input->reverse_active && in_focus_band &&
                                 input->band_excess_tenth_db >= config->raise_excess_tenth_db;
    const bool lower_condition = input->reverse_active || !speed_ready || !in_focus_band ||
                                 input->band_excess_tenth_db <= config->lower_excess_tenth_db;

    if (state->feedback_freeze_ms > 0U) {
        output.status = CANVIEW_VOLUME_LISTENING;
    } else if (raise_condition) {
        state->attack_evidence_ms = add_saturated(state->attack_evidence_ms, elapsed_ms,
                                                  config->attack_ms);
        state->release_evidence_ms = subtract_saturated(state->release_evidence_ms,
                                                        doubled_saturated(elapsed_ms));
        output.status = CANVIEW_VOLUME_ATTACK;
    } else if (lower_condition) {
        state->release_evidence_ms = add_saturated(state->release_evidence_ms, elapsed_ms,
                                                   config->release_ms);
        state->attack_evidence_ms = subtract_saturated(state->attack_evidence_ms,
                                                       doubled_saturated(elapsed_ms));
        output.status = CANVIEW_VOLUME_RELEASE;
    } else {
        state->attack_evidence_ms = subtract_saturated(state->attack_evidence_ms, elapsed_ms);
        state->release_evidence_ms = subtract_saturated(state->release_evidence_ms, elapsed_ms);
        output.status = CANVIEW_VOLUME_LISTENING;
    }

    if (state->step_cooldown_ms == 0U && state->feedback_freeze_ms == 0U &&
        state->attack_evidence_ms >= config->attack_ms &&
        state->desired_offset_steps < (int8_t)config->max_offset_steps) {
        ++state->desired_offset_steps;
        state->attack_evidence_ms = 0U;
        state->step_cooldown_ms = config->step_interval_ms;
        state->feedback_freeze_ms = config->feedback_freeze_ms;
        output.action = CANVIEW_VOLUME_ACTION_SET_OFFSET;
    } else if (state->step_cooldown_ms == 0U && state->feedback_freeze_ms == 0U &&
               state->release_evidence_ms >= config->release_ms &&
               state->desired_offset_steps > 0) {
        --state->desired_offset_steps;
        state->release_evidence_ms = 0U;
        state->step_cooldown_ms = config->step_interval_ms;
        state->feedback_freeze_ms = config->feedback_freeze_ms;
        output.action = CANVIEW_VOLUME_ACTION_SET_OFFSET;
    }

    output.target_offset_steps = state->desired_offset_steps;
    output.attack_progress_permil = progress_permil(state->attack_evidence_ms, config->attack_ms);
    output.release_progress_permil = progress_permil(state->release_evidence_ms,
                                                     config->release_ms);
    return output;
}
