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
        .idle_timeout_ms = 30000U,
        .idle_dim_percent = 35U,
        .speed_warning_enter_percent = 110U,
        .speed_warning_exit_percent = 105U,
        .speed_warning_on_confirm_ms = 500U,
        .speed_warning_off_confirm_ms = 1000U,
        .speed_warning_flash_interval_ms = 400U,
        .speed_warning_brightness_percent = 90U,
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

static bool speed_is_at_least_percent(uint16_t speed_tenth_kph,
                                      uint8_t limit_kph,
                                      uint8_t percent)
{
    if (limit_kph == 0U) {
        return false;
    }
    const uint32_t threshold_tenth_kph =
        ((uint32_t)limit_kph * 10U * percent + 99U) / 100U;
    return speed_tenth_kph >= threshold_tenth_kph;
}

static void update_speed_warning(canview_auto_brightness_state_t *state,
                                 const canview_auto_brightness_config_t *config,
                                 const canview_auto_brightness_input_t *input,
                                 uint32_t elapsed_ms)
{
    const bool limit_available = input->speed_limit_valid && input->speed_limit_active &&
                                 input->speed_limit_kph > 0U;
    const bool enter_condition = limit_available &&
        speed_is_at_least_percent(input->speed_tenth_kph, input->speed_limit_kph,
                                  config->speed_warning_enter_percent);
    const bool keep_condition = limit_available &&
        speed_is_at_least_percent(input->speed_tenth_kph, input->speed_limit_kph,
                                  config->speed_warning_exit_percent);

    if (!state->speed_warning_active) {
        state->speed_warning_off_ms = 0U;
        if (enter_condition) {
            state->speed_warning_on_ms = add_saturated(
                state->speed_warning_on_ms, elapsed_ms,
                config->speed_warning_on_confirm_ms);
            if (state->speed_warning_on_ms >= config->speed_warning_on_confirm_ms) {
                state->speed_warning_active = true;
                state->speed_warning_on_ms = 0U;
                state->speed_warning_flash_ms = 0U;
            }
        } else {
            state->speed_warning_on_ms = 0U;
        }
        return;
    }

    state->speed_warning_on_ms = 0U;
    if (keep_condition) {
        state->speed_warning_off_ms = 0U;
    } else {
        state->speed_warning_off_ms = add_saturated(
            state->speed_warning_off_ms, elapsed_ms,
            config->speed_warning_off_confirm_ms);
        if (state->speed_warning_off_ms >= config->speed_warning_off_confirm_ms) {
            state->speed_warning_active = false;
            state->speed_warning_off_ms = 0U;
            state->speed_warning_flash_ms = 0U;
        }
    }
}

static bool warning_flash_visible(canview_auto_brightness_state_t *state,
                                  const canview_auto_brightness_config_t *config,
                                  uint32_t elapsed_ms)
{
    if (!state->speed_warning_active) {
        return false;
    }
    if (config->speed_warning_flash_interval_ms == 0U) {
        return true;
    }
    const uint32_t cycle_ms = (uint32_t)config->speed_warning_flash_interval_ms * 2U;
    state->speed_warning_flash_ms =
        (state->speed_warning_flash_ms + (elapsed_ms % cycle_ms)) % cycle_ms;
    return state->speed_warning_flash_ms < config->speed_warning_flash_interval_ms;
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

    output.return_to_default_screen = false;
    if (input->user_interaction) {
        state->idle_elapsed_ms = 0U;
        state->idle_dimmed = false;
    } else if (config->idle_timeout_ms > 0U) {
        state->idle_elapsed_ms = add_saturated(state->idle_elapsed_ms, elapsed_ms,
                                               config->idle_timeout_ms);
        if (!state->idle_dimmed && state->idle_elapsed_ms >= config->idle_timeout_ms) {
            state->idle_dimmed = true;
            output.return_to_default_screen = true;
        }
    } else {
        state->idle_elapsed_ms = 0U;
        state->idle_dimmed = false;
    }

    update_speed_warning(state, config, input, elapsed_ms);

    uint8_t target = clamp_percent(input->manual_percent);
    const bool signal_valid = input->lighting_valid &&
                              input->lighting_age_ms <= config->stale_timeout_ms;

    if (signal_valid) {
        const bool requested_night = input->tail_lamps_on;
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
    } else {
        state->pending_ms = 0U;
    }

    if (!input->enabled) {
        output.status = CANVIEW_BRIGHTNESS_MANUAL;
    } else if (!signal_valid) {
        target = (uint8_t)((state->current_tenth_percent + 5U) / 10U);
        output.status = CANVIEW_BRIGHTNESS_CAN_STALE;
    } else {
        if (state->night_active) {
            target = night_target(config, input);
            output.status = CANVIEW_BRIGHTNESS_CAN_NIGHT;
        } else {
            output.status = CANVIEW_BRIGHTNESS_CAN_DAY;
        }
    }

    if (state->idle_dimmed) {
        uint8_t dim_target = (uint8_t)(((uint16_t)target *
                                        clamp_percent(config->idle_dim_percent) + 50U) /
                                       100U);
        if (dim_target < 8U) {
            dim_target = 8U;
        }
        target = dim_target;
        output.status = CANVIEW_BRIGHTNESS_IDLE_DIM;
    }
    if (state->speed_warning_active) {
        const uint8_t warning_target = clamp_percent(
            config->speed_warning_brightness_percent);
        if (target < warning_target) {
            target = warning_target;
        }
        output.status = CANVIEW_BRIGHTNESS_SPEED_WARNING;
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
    output.night_mode_active = state->night_active;
    output.idle_dimmed = state->idle_dimmed;
    output.speed_warning_active = state->speed_warning_active;
    output.speed_warning_visible = warning_flash_visible(state, config, elapsed_ms);
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

canview_headlamp_warning_config_t canview_headlamp_warning_default_config(void)
{
    const canview_headlamp_warning_config_t config = {
        .after_sunset_grace_ms = 60000U,
        .warning_on_confirm_ms = 2000U,
        .warning_off_confirm_ms = 1000U,
        .stale_timeout_ms = 2000U,
    };
    return config;
}

void canview_headlamp_warning_reset(canview_headlamp_warning_state_t *state)
{
    if (state != NULL) {
        *state = (canview_headlamp_warning_state_t){.initialized = true};
    }
}

static bool minutes_are_valid(uint16_t minutes)
{
    return minutes < 1440U;
}

static bool after_sunset_with_grace(const canview_headlamp_warning_config_t *config,
                                    const canview_headlamp_warning_input_t *input)
{
    if (input->sunrise_minutes >= input->sunset_minutes ||
        !minutes_are_valid(input->local_minutes) ||
        !minutes_are_valid(input->sunrise_minutes) ||
        !minutes_are_valid(input->sunset_minutes)) {
        return false;
    }

    bool night = false;
    uint32_t minutes_since_sunset = 0U;
    if (input->local_minutes >= input->sunset_minutes) {
        night = true;
        minutes_since_sunset = input->local_minutes - input->sunset_minutes;
    } else if (input->local_minutes < input->sunrise_minutes) {
        night = true;
        minutes_since_sunset = (1440U - input->sunset_minutes) +
                               input->local_minutes;
    }
    if (!night) {
        return false;
    }
    return minutes_since_sunset * 60000U >= config->after_sunset_grace_ms;
}

canview_headlamp_warning_output_t canview_headlamp_warning_update(
    canview_headlamp_warning_state_t *state,
    const canview_headlamp_warning_config_t *config,
    const canview_headlamp_warning_input_t *input,
    uint32_t elapsed_ms)
{
    canview_headlamp_warning_output_t output = {
        .status = CANVIEW_HEADLAMP_WARNING_INVALID,
    };
    if (state == NULL || config == NULL || input == NULL) {
        return output;
    }
    if (!state->initialized) {
        canview_headlamp_warning_reset(state);
    }

    const bool source_fresh = input->rtc_valid && input->solar_valid &&
                              input->headlamp_valid &&
                              input->rtc_age_ms <= config->stale_timeout_ms &&
                              input->solar_age_ms <= config->stale_timeout_ms &&
                              input->headlamp_age_ms <= config->stale_timeout_ms;
    const bool night = source_fresh && input->enabled && input->vehicle_awake &&
                       after_sunset_with_grace(config, input);
    output.night_active = night;
    if (!source_fresh || !input->enabled || !input->vehicle_awake) {
        state->warning_active = false;
        state->warning_on_evidence_ms = 0U;
        state->warning_off_evidence_ms = 0U;
        output.status = CANVIEW_HEADLAMP_WARNING_INVALID;
        return output;
    }
    if (!night) {
        state->warning_active = false;
        state->warning_on_evidence_ms = 0U;
        state->warning_off_evidence_ms = 0U;
        output.status = CANVIEW_HEADLAMP_WARNING_DAY;
        return output;
    }

    if (state->warning_active) {
        state->warning_on_evidence_ms = 0U;
        if (input->headlamps_on) {
            state->warning_off_evidence_ms = add_saturated(
                state->warning_off_evidence_ms, elapsed_ms,
                config->warning_off_confirm_ms);
            if (state->warning_off_evidence_ms >= config->warning_off_confirm_ms) {
                state->warning_active = false;
                state->warning_off_evidence_ms = 0U;
            }
        } else {
            state->warning_off_evidence_ms = 0U;
        }
    } else if (input->headlamps_on) {
        state->warning_on_evidence_ms = 0U;
        state->warning_off_evidence_ms = 0U;
    } else {
        state->warning_off_evidence_ms = 0U;
        state->warning_on_evidence_ms = add_saturated(
            state->warning_on_evidence_ms, elapsed_ms,
            config->warning_on_confirm_ms);
        if (state->warning_on_evidence_ms >= config->warning_on_confirm_ms) {
            state->warning_active = true;
            state->warning_on_evidence_ms = 0U;
        }
    }

    output.warning_active = state->warning_active;
    output.status = state->warning_active ? CANVIEW_HEADLAMP_WARNING_ACTIVE
                                          : CANVIEW_HEADLAMP_WARNING_PENDING;
    if (!state->warning_active && input->headlamps_on) {
        output.status = CANVIEW_HEADLAMP_WARNING_NIGHT_OK;
    }
    return output;
}
