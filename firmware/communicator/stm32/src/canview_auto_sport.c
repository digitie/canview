#include "canview_auto_sport.h"

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

static bool mode_can_be_restored(canview_drive_mode_t mode)
{
    return mode > CANVIEW_DRIVE_MODE_UNKNOWN && mode < CANVIEW_DRIVE_MODE_SPORT;
}

static bool safety_ready(const canview_auto_sport_input_t *input)
{
    return input->signals_fresh && input->forward_gear && !input->brake_active &&
           !input->abs_tcs_esc_active && !input->drive_mode_fault &&
           input->control_link_ready &&
           input->current_mode > CANVIEW_DRIVE_MODE_UNKNOWN &&
           input->current_mode <= CANVIEW_DRIVE_MODE_SPORT;
}

canview_auto_sport_config_t canview_auto_sport_default_config(void)
{
    const canview_auto_sport_config_t config = {
        .acceleration_detection_enabled = true,
        .enter_speed_tenth_kph = 700U,
        .exit_speed_tenth_kph = 550U,
        .acceleration_min_speed_tenth_kph = 350U,
        .acceleration_enter_milli_mps2 = 1400,
        .acceleration_release_milli_mps2 = 350,
        .speed_confirm_ms = 2500U,
        .acceleration_confirm_ms = 800U,
        .exit_confirm_ms = 8000U,
        .minimum_sport_hold_ms = 15000U,
        .command_timeout_ms = 1500U,
    };
    return config;
}

void canview_auto_sport_set_entry_speed(canview_auto_sport_config_t *config,
                                        uint8_t speed_kph)
{
    if (config == NULL) {
        return;
    }
    const uint8_t preset_kph =
        speed_kph <= 65U ? 60U : (speed_kph <= 75U ? 70U : 80U);
    config->enter_speed_tenth_kph = (uint16_t)preset_kph * 10U;
    config->exit_speed_tenth_kph = (uint16_t)(preset_kph - 15U) * 10U;
}

void canview_auto_sport_reset(canview_auto_sport_state_t *state)
{
    if (state == NULL) {
        return;
    }
    *state = (canview_auto_sport_state_t){.status = CANVIEW_SPORT_DISABLED};
}

void canview_longitudinal_accel_filter_reset(
    canview_longitudinal_accel_filter_t *filter)
{
    if (filter == NULL) {
        return;
    }
    *filter = (canview_longitudinal_accel_filter_t){0};
}

static int16_t median_sample(const canview_longitudinal_accel_filter_t *filter)
{
    int16_t sorted[CANVIEW_ACCEL_MEDIAN_WINDOW];
    for (uint8_t i = 0; i < filter->sample_count; ++i) {
        sorted[i] = filter->samples[i];
    }
    for (uint8_t i = 1; i < filter->sample_count; ++i) {
        const int16_t value = sorted[i];
        uint8_t position = i;
        while (position > 0U && sorted[position - 1U] > value) {
            sorted[position] = sorted[position - 1U];
            --position;
        }
        sorted[position] = value;
    }
    return sorted[filter->sample_count / 2U];
}

int16_t canview_longitudinal_accel_filter_update(
    canview_longitudinal_accel_filter_t *filter,
    int16_t raw_milli_mps2,
    bool sample_valid)
{
    if (filter == NULL || !sample_valid) {
        return filter != NULL ? filter->filtered_milli_mps2 : 0;
    }
    filter->samples[filter->write_index] = raw_milli_mps2;
    filter->write_index = (uint8_t)((filter->write_index + 1U) %
                                    CANVIEW_ACCEL_MEDIAN_WINDOW);
    if (filter->sample_count < CANVIEW_ACCEL_MEDIAN_WINDOW) {
        ++filter->sample_count;
    }

    const int16_t median = median_sample(filter);
    if (!filter->initialized) {
        filter->filtered_milli_mps2 = median;
        filter->initialized = true;
    } else {
        const int32_t error = (int32_t)median - filter->filtered_milli_mps2;
        filter->filtered_milli_mps2 = (int16_t)(filter->filtered_milli_mps2 + error / 4);
    }
    return filter->filtered_milli_mps2;
}

static canview_auto_sport_output_t make_output(const canview_auto_sport_state_t *state)
{
    const canview_auto_sport_output_t output = {
        .status = state->status,
        .restore_mode = state->previous_mode,
    };
    return output;
}

static void enter_manual_hold(canview_auto_sport_state_t *state)
{
    *state = (canview_auto_sport_state_t){.status = CANVIEW_SPORT_MANUAL_HOLD};
}

canview_auto_sport_output_t canview_auto_sport_update(
    canview_auto_sport_state_t *state,
    const canview_auto_sport_config_t *config,
    const canview_auto_sport_input_t *input,
    uint32_t elapsed_ms)
{
    if (state == NULL || config == NULL || input == NULL) {
        const canview_auto_sport_output_t invalid = {
            .status = CANVIEW_SPORT_INHIBITED,
        };
        return invalid;
    }

    canview_auto_sport_output_t output = make_output(state);
    const bool tick_gap = elapsed_ms > CANVIEW_SPORT_MAX_TICK_GAP_MS;
    const uint32_t evidence_ms = elapsed_ms > CANVIEW_SPORT_MAX_EVIDENCE_MS
                                     ? CANVIEW_SPORT_MAX_EVIDENCE_MS : elapsed_ms;
    if (tick_gap) {
        state->speed_evidence_ms = 0U;
        state->acceleration_evidence_ms = 0U;
        state->exit_evidence_ms = 0U;
        state->sport_active_ms = 0U;
    }

    if (input->physical_mode_change) {
        enter_manual_hold(state);
        return make_output(state);
    }

    if (state->status == CANVIEW_SPORT_MANUAL_HOLD) {
        if (!input->enabled) {
            state->status = CANVIEW_SPORT_DISABLED;
        } else if (input->rearm_requested && safety_ready(input) && !tick_gap) {
            state->status = CANVIEW_SPORT_ARMED;
        }
        return make_output(state);
    }

    if (state->status == CANVIEW_SPORT_ENTER_PENDING) {
        state->pending_ms = add_saturated(state->pending_ms, elapsed_ms,
                                          config->command_timeout_ms);
        if (state->pending_ms >= config->command_timeout_ms) {
            enter_manual_hold(state);
            return make_output(state);
        }
        if (!tick_gap && safety_ready(input) &&
            input->current_mode == CANVIEW_DRIVE_MODE_SPORT) {
            state->status = CANVIEW_SPORT_ACTIVE;
            state->owns_sport_mode = true;
            state->pending_ms = 0U;
            state->sport_active_ms = 0U;
            return make_output(state);
        }
        return make_output(state);
    }

    if (state->status == CANVIEW_SPORT_EXIT_PENDING) {
        state->pending_ms = add_saturated(state->pending_ms, elapsed_ms,
                                          config->command_timeout_ms);
        if (state->pending_ms >= config->command_timeout_ms) {
            enter_manual_hold(state);
            return make_output(state);
        }
        if (!tick_gap && safety_ready(input) &&
            input->current_mode == state->previous_mode) {
            state->status = input->enabled ? CANVIEW_SPORT_ARMED : CANVIEW_SPORT_DISABLED;
            state->owns_sport_mode = false;
            state->previous_mode = CANVIEW_DRIVE_MODE_UNKNOWN;
            state->pending_ms = 0U;
            return make_output(state);
        }
        return make_output(state);
    }

    /* fresh 외부 mode 관찰은 권한 철회다. gap의 새 명령/승격 금지와 독립이다. */
    if (input->signals_fresh && state->owns_sport_mode &&
        mode_can_be_restored(input->current_mode)) {
        enter_manual_hold(state);
        return make_output(state);
    }
    if (tick_gap) {
        state->status = input->enabled ? CANVIEW_SPORT_INHIBITED : CANVIEW_SPORT_DISABLED;
        return make_output(state);
    }

    const bool ready = safety_ready(input);
    if (!ready) {
        state->status = input->enabled ? CANVIEW_SPORT_INHIBITED : CANVIEW_SPORT_DISABLED;
        state->speed_evidence_ms = 0U;
        state->acceleration_evidence_ms = 0U;
        state->exit_evidence_ms = 0U;
        return make_output(state);
    }

    if (!input->enabled) {
        if (state->owns_sport_mode && input->current_mode == CANVIEW_DRIVE_MODE_SPORT &&
            mode_can_be_restored(state->previous_mode)) {
            state->status = CANVIEW_SPORT_EXIT_PENDING;
            state->pending_ms = 0U;
            output = make_output(state);
            output.action = CANVIEW_SPORT_ACTION_RESTORE_PREVIOUS;
            return output;
        }
        state->status = CANVIEW_SPORT_DISABLED;
        state->owns_sport_mode = false;
        state->previous_mode = CANVIEW_DRIVE_MODE_UNKNOWN;
        return make_output(state);
    }

    if (state->status == CANVIEW_SPORT_DISABLED || state->status == CANVIEW_SPORT_INHIBITED) {
        state->status = state->owns_sport_mode && input->current_mode == CANVIEW_DRIVE_MODE_SPORT
                            ? CANVIEW_SPORT_ACTIVE
                            : CANVIEW_SPORT_ARMED;
    }

    if (state->status == CANVIEW_SPORT_ACTIVE) {
        state->sport_active_ms = add_saturated(state->sport_active_ms, evidence_ms,
                                               config->minimum_sport_hold_ms);
        const bool exit_condition =
            input->speed_tenth_kph <= config->exit_speed_tenth_kph &&
            input->longitudinal_acceleration_milli_mps2 <=
                config->acceleration_release_milli_mps2;
        if (exit_condition) {
            state->exit_evidence_ms = add_saturated(state->exit_evidence_ms, evidence_ms,
                                                    config->exit_confirm_ms);
        } else {
            state->exit_evidence_ms = subtract_saturated(state->exit_evidence_ms,
                                                         doubled_saturated(evidence_ms));
        }
        if (state->sport_active_ms >= config->minimum_sport_hold_ms &&
            state->exit_evidence_ms >= config->exit_confirm_ms &&
            mode_can_be_restored(state->previous_mode)) {
            state->status = CANVIEW_SPORT_EXIT_PENDING;
            state->pending_ms = 0U;
            output = make_output(state);
            output.action = CANVIEW_SPORT_ACTION_RESTORE_PREVIOUS;
            return output;
        }
        return make_output(state);
    }

    if (input->current_mode == CANVIEW_DRIVE_MODE_SPORT) {
        state->speed_evidence_ms = 0U;
        state->acceleration_evidence_ms = 0U;
        return make_output(state);
    }

    const bool speed_condition = input->speed_tenth_kph >= config->enter_speed_tenth_kph;
    const bool acceleration_condition =
        config->acceleration_detection_enabled &&
        input->speed_tenth_kph >= config->acceleration_min_speed_tenth_kph &&
        input->longitudinal_acceleration_milli_mps2 >= config->acceleration_enter_milli_mps2;

    state->speed_evidence_ms = speed_condition
                                   ? add_saturated(state->speed_evidence_ms, evidence_ms,
                                                   config->speed_confirm_ms)
                                   : subtract_saturated(state->speed_evidence_ms,
                                                        doubled_saturated(evidence_ms));
    state->acceleration_evidence_ms =
        acceleration_condition
            ? add_saturated(state->acceleration_evidence_ms, evidence_ms,
                            config->acceleration_confirm_ms)
            : subtract_saturated(state->acceleration_evidence_ms,
                                 doubled_saturated(evidence_ms));

    if ((state->speed_evidence_ms >= config->speed_confirm_ms ||
         state->acceleration_evidence_ms >= config->acceleration_confirm_ms) &&
        mode_can_be_restored(input->current_mode)) {
        state->previous_mode = input->current_mode;
        state->status = CANVIEW_SPORT_ENTER_PENDING;
        state->pending_ms = 0U;
        state->speed_evidence_ms = 0U;
        state->acceleration_evidence_ms = 0U;
        output = make_output(state);
        output.action = CANVIEW_SPORT_ACTION_ENTER;
        return output;
    }

    return make_output(state);
}
