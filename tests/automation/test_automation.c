#include "canview_auto_sport.h"
#include "canview_controller_automation.h"
#include "canview_protocol.h"

#include <assert.h>
#include <stdio.h>

static void test_can_brightness_debounce_and_stale_hold(void)
{
    canview_auto_brightness_config_t config = canview_auto_brightness_default_config();
    config.transition_ms = 0U;
    canview_auto_brightness_state_t state = {0};
    canview_auto_brightness_input_t input = {
        .enabled = true,
        .lighting_valid = true,
        .dimmer_valid = true,
        .dimmer_percent = 50U,
        .manual_percent = 80U,
    };

    canview_auto_brightness_output_t output =
        canview_auto_brightness_update(&state, &config, &input, 100U);
    assert(output.status == CANVIEW_BRIGHTNESS_CAN_DAY);
    assert(output.brightness_percent == 80U);

    input.tail_lamps_on = true;
    for (int i = 0; i < 4; ++i) {
        output = canview_auto_brightness_update(&state, &config, &input, 100U);
        assert(output.status == CANVIEW_BRIGHTNESS_CAN_DAY);
    }
    output = canview_auto_brightness_update(&state, &config, &input, 100U);
    assert(output.status == CANVIEW_BRIGHTNESS_CAN_NIGHT);
    assert(output.brightness_percent == 31U);

    input.lighting_age_ms = 600U;
    input.tail_lamps_on = false;
    output = canview_auto_brightness_update(&state, &config, &input, 1000U);
    assert(output.status == CANVIEW_BRIGHTNESS_CAN_STALE);
    assert(output.brightness_percent == 31U);
}

static void test_idle_return_touch_restore_and_warning_priority(void)
{
    canview_auto_brightness_config_t config = canview_auto_brightness_default_config();
    config.transition_ms = 0U;
    config.idle_timeout_ms = 30000U;
    config.speed_warning_on_confirm_ms = 500U;
    config.speed_warning_off_confirm_ms = 1000U;
    canview_auto_brightness_state_t state = {0};
    canview_auto_brightness_input_t input = {
        .enabled = true,
        .lighting_valid = true,
        .manual_percent = 80U,
        .speed_limit_valid = true,
        .speed_limit_active = true,
        .speed_limit_kph = 70U,
        .speed_tenth_kph = 700U,
    };

    canview_auto_brightness_output_t output =
        canview_auto_brightness_update(&state, &config, &input, 29900U);
    assert(!output.idle_dimmed);
    output = canview_auto_brightness_update(&state, &config, &input, 100U);
    assert(output.idle_dimmed);
    assert(output.return_to_default_screen);
    assert(output.brightness_percent == 28U);

    output = canview_auto_brightness_update(&state, &config, &input, 100U);
    assert(output.idle_dimmed);
    assert(!output.return_to_default_screen);

    input.user_interaction = true;
    output = canview_auto_brightness_update(&state, &config, &input, 100U);
    assert(!output.idle_dimmed);
    assert(output.brightness_percent == 80U);
    input.user_interaction = false;

    input.speed_tenth_kph = 770U;
    for (int i = 0; i < 4; ++i) {
        output = canview_auto_brightness_update(&state, &config, &input, 100U);
        assert(!output.speed_warning_active);
    }
    output = canview_auto_brightness_update(&state, &config, &input, 100U);
    assert(output.speed_warning_active);
    assert(output.brightness_percent == 90U);

    input.speed_tenth_kph = 730U;
    for (int i = 0; i < 10; ++i) {
        output = canview_auto_brightness_update(&state, &config, &input, 100U);
    }
    assert(!output.speed_warning_active);
    assert(output.brightness_percent == 80U);

    input.speed_tenth_kph = 700U;
    output = canview_auto_brightness_update(&state, &config, &input, 30000U);
    assert(output.idle_dimmed);
    assert(output.brightness_percent == 28U);

    input.speed_tenth_kph = 770U;
    output = canview_auto_brightness_update(&state, &config, &input, 500U);
    assert(output.idle_dimmed);
    assert(output.speed_warning_active);
    assert(output.brightness_percent == 90U);

    input.speed_tenth_kph = 730U;
    output = canview_auto_brightness_update(&state, &config, &input, 1000U);
    assert(output.idle_dimmed);
    assert(!output.speed_warning_active);
    assert(output.brightness_percent == 28U);
}

static void test_night_mode_tracks_tail_lamps_with_auto_brightness_off(void)
{
    canview_auto_brightness_config_t config = canview_auto_brightness_default_config();
    config.transition_ms = 0U;
    canview_auto_brightness_state_t state = {0};
    canview_auto_brightness_input_t input = {
        .enabled = false,
        .lighting_valid = true,
        .tail_lamps_on = true,
        .manual_percent = 60U,
    };

    canview_auto_brightness_output_t output = {0};
    for (int i = 0; i < 5; ++i) {
        output = canview_auto_brightness_update(&state, &config, &input, 100U);
    }
    assert(output.status == CANVIEW_BRIGHTNESS_MANUAL);
    assert(output.night_mode_active);
    assert(output.brightness_percent == 60U);
}

static void test_adaptive_volume_attack_release_and_manual_hold(void)
{
    canview_adaptive_volume_config_t config = canview_adaptive_volume_default_config();
    canview_adaptive_volume_apply_settings(&config, CANVIEW_NOISE_BAND_BALANCED,
                                           CANVIEW_NOISE_SENSITIVITY_NORMAL,
                                           CANVIEW_NOISE_RESPONSE_NORMAL, 4U);
    assert(config.focus_low_hz == 160U);
    assert(config.focus_high_hz == 1250U);
    assert(config.attack_ms == 5000U);
    assert(config.release_ms == 12000U);
    canview_adaptive_volume_apply_settings(&config, CANVIEW_NOISE_BAND_BALANCED,
                                           CANVIEW_NOISE_SENSITIVITY_NORMAL,
                                           CANVIEW_NOISE_RESPONSE_NORMAL, 1U);
    assert(config.max_offset_steps == 2U);
    config.max_offset_steps = 4U;
    canview_adaptive_volume_state_t state = {0};
    canview_adaptive_volume_input_t input = {
        .enabled = true,
        .fft_valid = true,
        .speed_tenth_kph = 700U,
        .dominant_frequency_hz = 500U,
        .band_excess_tenth_db = 60,
        .confidence_percent = 90U,
    };

    canview_adaptive_volume_output_t output = {0};
    for (int i = 0; i < 4; ++i) {
        output = canview_adaptive_volume_update(&state, &config, &input, 1000U);
        assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
    }
    output = canview_adaptive_volume_update(&state, &config, &input, 1000U);
    assert(output.action == CANVIEW_VOLUME_ACTION_SET_OFFSET);
    assert(output.target_offset_steps == 1);
    canview_adaptive_volume_reconcile(&state, 1);

    input.dominant_frequency_hz = 3000U;
    bool saw_release = false;
    for (int i = 0; i < 7; ++i) {
        output = canview_adaptive_volume_update(&state, &config, &input, 1000U);
        assert(output.target_offset_steps == 1);
    }
    for (int i = 0; i < 7; ++i) {
        output = canview_adaptive_volume_update(&state, &config, &input, 1000U);
        saw_release = saw_release || output.action == CANVIEW_VOLUME_ACTION_SET_OFFSET;
    }
    assert(saw_release);
    assert(output.target_offset_steps == 0);

    state.desired_offset_steps = 2;
    input.manual_volume_changed = true;
    output = canview_adaptive_volume_update(&state, &config, &input, 100U);
    assert(output.action == CANVIEW_VOLUME_ACTION_SET_OFFSET);
    assert(output.target_offset_steps == 0);
    assert(output.status == CANVIEW_VOLUME_PAUSED);
}

static canview_auto_sport_input_t safe_sport_input(void)
{
    const canview_auto_sport_input_t input = {
        .enabled = true,
        .signals_fresh = true,
        .forward_gear = true,
        .control_link_ready = true,
        .current_mode = CANVIEW_DRIVE_MODE_NORMAL,
    };
    return input;
}

static void test_auto_sport_speed_hysteresis_and_restore(void)
{
    canview_auto_sport_config_t config = canview_auto_sport_default_config();
    canview_auto_sport_set_entry_speed(&config, 73U);
    assert(config.enter_speed_tenth_kph == 700U);
    assert(config.exit_speed_tenth_kph == 550U);
    canview_auto_sport_state_t state;
    canview_auto_sport_reset(&state);
    canview_auto_sport_input_t input = safe_sport_input();
    input.current_mode = CANVIEW_DRIVE_MODE_ECO;
    input.speed_tenth_kph = 700U;

    canview_auto_sport_output_t output = {0};
    for (int i = 0; i < 4; ++i) {
        output = canview_auto_sport_update(&state, &config, &input, 500U);
        assert(output.action == CANVIEW_SPORT_ACTION_NONE);
    }
    output = canview_auto_sport_update(&state, &config, &input, 500U);
    assert(output.action == CANVIEW_SPORT_ACTION_ENTER);
    assert(output.restore_mode == CANVIEW_DRIVE_MODE_ECO);

    input.current_mode = CANVIEW_DRIVE_MODE_SPORT;
    output = canview_auto_sport_update(&state, &config, &input, 100U);
    assert(output.status == CANVIEW_SPORT_ACTIVE);

    input.speed_tenth_kph = 600U;
    for (int i = 0; i < 150; ++i) {
        output = canview_auto_sport_update(&state, &config, &input, 100U);
    }
    assert(output.action == CANVIEW_SPORT_ACTION_NONE);
    assert(output.status == CANVIEW_SPORT_ACTIVE);

    input.speed_tenth_kph = 550U;
    for (int i = 0; i < 79; ++i) {
        output = canview_auto_sport_update(&state, &config, &input, 100U);
        assert(output.action == CANVIEW_SPORT_ACTION_NONE);
    }
    output = canview_auto_sport_update(&state, &config, &input, 100U);
    assert(output.action == CANVIEW_SPORT_ACTION_RESTORE_PREVIOUS);
    assert(output.restore_mode == CANVIEW_DRIVE_MODE_ECO);

    input.current_mode = CANVIEW_DRIVE_MODE_ECO;
    output = canview_auto_sport_update(&state, &config, &input, 100U);
    assert(output.status == CANVIEW_SPORT_ARMED);
}

static void test_auto_sport_mid_speed_acceleration_and_manual_priority(void)
{
    const canview_auto_sport_config_t config = canview_auto_sport_default_config();
    canview_auto_sport_state_t state;
    canview_auto_sport_reset(&state);
    canview_auto_sport_input_t input = safe_sport_input();
    input.speed_tenth_kph = 450U;
    input.longitudinal_acceleration_milli_mps2 = 1500;

    canview_auto_sport_output_t output = {0};
    for (int i = 0; i < 7; ++i) {
        output = canview_auto_sport_update(&state, &config, &input, 100U);
        assert(output.action == CANVIEW_SPORT_ACTION_NONE);
    }
    output = canview_auto_sport_update(&state, &config, &input, 100U);
    assert(output.action == CANVIEW_SPORT_ACTION_ENTER);

    input.physical_mode_change = true;
    output = canview_auto_sport_update(&state, &config, &input, 100U);
    assert(output.status == CANVIEW_SPORT_MANUAL_HOLD);
    assert(output.action == CANVIEW_SPORT_ACTION_NONE);
}

static void test_acceleration_filter_rejects_single_spike(void)
{
    canview_longitudinal_accel_filter_t filter;
    canview_longitudinal_accel_filter_reset(&filter);
    for (int i = 0; i < 5; ++i) {
        assert(canview_longitudinal_accel_filter_update(&filter, 0, true) == 0);
    }
    assert(canview_longitudinal_accel_filter_update(&filter, 3000, true) == 0);
    assert(canview_longitudinal_accel_filter_update(&filter, 0, false) == 0);
}

int main(void)
{
    assert(CANVIEW_PROTOCOL_MINOR == 1U);
    assert(sizeof(canview_config_record_t) == 8U);
    test_can_brightness_debounce_and_stale_hold();
    test_idle_return_touch_restore_and_warning_priority();
    test_night_mode_tracks_tail_lamps_with_auto_brightness_off();
    test_adaptive_volume_attack_release_and_manual_hold();
    test_auto_sport_speed_hysteresis_and_restore();
    test_auto_sport_mid_speed_acceleration_and_manual_priority();
    test_acceleration_filter_rejects_single_spike();
    puts("automation tests passed");
    return 0;
}
