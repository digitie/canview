#include "canview_command_tracker.h"
#include "canview_controller_can.h"
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

static canview_controller_can_filter_config_t test_filter(uint32_t filter_id,
                                                          uint32_t can_id)
{
    return (canview_controller_can_filter_config_t){
        .filter_id = filter_id,
        .can_id = can_id,
        .can_id_mask = 0x7FFU,
        .period_ms = 100U,
        .bus_id = 0U,
        .min_dlc = 0U,
        .max_dlc = 8U,
        .max_records_per_period = 2U,
        .enabled = true,
    };
}

static canview_can_record_t test_record(uint32_t can_id)
{
    canview_can_record_t record = {
        .bus_id = 0U,
        .flags_dlc = 8U,
    };
    record.can_id_le = can_id;
    return record;
}

static void test_controller_filter_default_deny_and_updates(void)
{
    canview_controller_can_filter_store_t store;
    canview_controller_can_filter_store_init(&store);
    canview_controller_can_filter_config_t config = test_filter(1U, 0x386U);
    assert(!canview_controller_can_accept_record(&store, &(canview_can_record_t){0}, 0U));
    canview_can_record_t invalid_record = test_record(0x800U);
    assert(!canview_controller_can_accept_record(&store, &invalid_record, 0U));
    invalid_record = test_record(0x386U);
    invalid_record.bus_id = 3U;
    assert(!canview_controller_can_accept_record(&store, &invalid_record, 0U));
    assert(canview_controller_can_filter_apply(&store, CANVIEW_CAN_FILTER_ADD, &config) ==
           CANVIEW_CONTROLLER_CAN_FILTER_OK);

    canview_can_record_t allowed = test_record(0x386U);
    canview_can_record_t denied = test_record(0x329U);
    assert(canview_controller_can_accept_record(&store, &allowed, 0U));
    assert(!canview_controller_can_accept_record(&store, &denied, 0U));
    assert(canview_controller_can_accept_record(&store, &allowed, 0U));
    assert(!canview_controller_can_accept_record(&store, &allowed, 0U));
    assert(canview_controller_can_accept_record(&store, &allowed, 100U));

    config.can_id = 0x329U;
    assert(canview_controller_can_filter_apply(&store, CANVIEW_CAN_FILTER_REPLACE, &config) ==
           CANVIEW_CONTROLLER_CAN_FILTER_OK);
    assert(!canview_controller_can_accept_record(&store, &allowed, 200U));
    assert(canview_controller_can_accept_record(&store, &denied, 200U));
    assert(canview_controller_can_filter_apply(&store, CANVIEW_CAN_FILTER_DELETE, &config) ==
           CANVIEW_CONTROLLER_CAN_FILTER_OK);
    assert(!canview_controller_can_accept_record(&store, &denied, 300U));
    assert(canview_controller_can_filter_apply(&store, CANVIEW_CAN_FILTER_CLEAR, NULL) ==
           CANVIEW_CONTROLLER_CAN_FILTER_OK);

    canview_controller_can_filter_store_t budget_store;
    canview_controller_can_filter_store_init(&budget_store);
    config = test_filter(2U, 0x386U);
    assert(canview_controller_can_filter_apply(&budget_store, CANVIEW_CAN_FILTER_ADD, &config) ==
           CANVIEW_CONTROLLER_CAN_FILTER_OK);
    const canview_controller_can_stream_config_t budget = {
        .period_ms = 100U,
        .max_records_per_period = 32U,
        .max_bytes_per_second = 16U,
        .burst_bytes = 16U,
        .enabled = true,
    };
    assert(canview_controller_can_filter_set_stream(&budget_store, &budget));
    assert(canview_controller_can_accept_record(&budget_store, &allowed, 0U));
    assert(!canview_controller_can_accept_record(&budget_store, &allowed, 0U));
}

static void test_controller_signal_decode_is_catalog_driven(void)
{
    canview_can_record_t record = test_record(0x316U);
    record.data[2] = 0xD2U;
    record.data[3] = 0x04U;
    const canview_controller_signal_descriptor_t descriptor = {
        .signal_id = 10U,
        .bus_id = 0U,
        .can_id = 0x316U,
        .can_id_mask = 0x7FFU,
        .start_bit = 16U,
        .bit_length = 16U,
        .byte_order = CANVIEW_CONTROLLER_SIGNAL_LITTLE_ENDIAN,
        .value_type = CANVIEW_VALUE_F32,
        .quality = CANVIEW_QUALITY_UNVERIFIED,
        .factor = 0.25F,
        .offset = 0.0F,
    };
    canview_controller_decoded_signal_t decoded;
    assert(canview_controller_decode_signal(&record, &descriptor, &decoded));
    assert(decoded.quality == CANVIEW_QUALITY_UNVERIFIED);
    assert(decoded.physical_value > 308.4F && decoded.physical_value < 308.6F);

    canview_can_record_t signed_record = test_record(0x220U);
    signed_record.data[0] = 0xF0U;
    const canview_controller_signal_descriptor_t signed_descriptor = {
        .signal_id = 11U,
        .bus_id = 0U,
        .can_id = 0x220U,
        .can_id_mask = 0x7FFU,
        .start_bit = 7U,
        .bit_length = 4U,
        .byte_order = CANVIEW_CONTROLLER_SIGNAL_BIG_ENDIAN,
        .value_type = CANVIEW_VALUE_I32,
        .quality = CANVIEW_QUALITY_VALID,
        .is_signed = true,
        .factor = 1.0F,
        .offset = 0.0F,
    };
    assert(canview_controller_decode_signal(&signed_record, &signed_descriptor, &decoded));
    assert(decoded.physical_value == -1.0F);
}

static void test_command_completion_requires_result(void)
{
    canview_command_tracker_t tracker;
    canview_command_tracker_init(&tracker);
    assert(canview_command_tracker_begin(&tracker, 1U, 0x0201U, 1000U, 0U));
    assert(canview_command_tracker_status(&tracker, 1U) ==
           CANVIEW_COMMAND_TRACK_WAITING_ACK);
    assert(!canview_command_tracker_should_retry(&tracker, 1U, 79U));
    assert(canview_command_tracker_should_retry(&tracker, 1U, 80U));
    assert(canview_command_tracker_mark_retry(&tracker, 1U, 80U));
    assert(!canview_command_tracker_should_retry(&tracker, 1U, 239U));
    assert(canview_command_tracker_should_retry(&tracker, 1U, 240U));
    assert(canview_command_tracker_mark_retry(&tracker, 1U, 240U));
    assert(canview_command_tracker_on_ack(&tracker, 1U, CANVIEW_ACK_DUPLICATE));
    assert(canview_command_tracker_status(&tracker, 1U) ==
           CANVIEW_COMMAND_TRACK_WAITING_RESULT);

    canview_command_result_t result = {
        .command_id_le = 0x0201U,
        .stage = CANVIEW_COMMAND_COMPLETED,
    };
    result.request_token_le = 1U;
    assert(canview_command_tracker_on_result(&tracker, &result));
    assert(canview_command_tracker_status(&tracker, 1U) ==
           CANVIEW_COMMAND_TRACK_COMPLETED);

    canview_command_tracker_t reordered;
    canview_command_tracker_init(&reordered);
    assert(canview_command_tracker_begin(&reordered, 2U, 0x0201U, 1000U, 0U));
    canview_command_result_t early_result = {
        .command_id_le = 0x0201U,
        .stage = CANVIEW_COMMAND_COMPLETED,
    };
    early_result.request_token_le = 2U;
    assert(canview_command_tracker_on_result(&reordered, &early_result));
    assert(canview_command_tracker_status(&reordered, 2U) ==
           CANVIEW_COMMAND_TRACK_COMPLETED);
    assert(canview_command_tracker_on_ack(&reordered, 2U, CANVIEW_ACK_ACCEPTED));
    assert(canview_command_tracker_status(&reordered, 2U) ==
           CANVIEW_COMMAND_TRACK_COMPLETED);

    canview_command_tracker_t expired;
    canview_command_tracker_init(&expired);
    assert(canview_command_tracker_begin(&expired, 3U, 0x0201U, 100U, 0U));
    canview_command_tracker_expire(&expired, 100U);
    canview_command_result_t late_result = early_result;
    late_result.request_token_le = 3U;
    assert(!canview_command_tracker_on_result(&expired, &late_result));
    assert(canview_command_tracker_status(&expired, 3U) ==
           CANVIEW_COMMAND_TRACK_EXPIRED);

    canview_command_tracker_t reusable;
    canview_command_tracker_init(&reusable);
    for (uint64_t token = 1U; token <= CANVIEW_COMMAND_TRACKER_MAX_PENDING; ++token) {
        assert(canview_command_tracker_begin(&reusable, token, 0x0201U, 1000U, 0U));
        canview_command_result_t completed = {
            .request_token_le = token,
            .command_id_le = 0x0201U,
            .stage = CANVIEW_COMMAND_COMPLETED,
        };
        assert(canview_command_tracker_on_result(&reusable, &completed));
    }
    assert(canview_command_tracker_begin(&reusable, 9U, 0x0201U, 1000U, 0U));
}

static void test_headlamp_warning_uses_rtc_sunset_and_hysteresis(void)
{
    const canview_headlamp_warning_config_t config =
        canview_headlamp_warning_default_config();
    canview_headlamp_warning_state_t state = {0};
    canview_headlamp_warning_input_t input = {
        .enabled = true,
        .vehicle_awake = true,
        .rtc_valid = true,
        .local_minutes = 1081U,
        .solar_valid = true,
        .sunrise_minutes = 360U,
        .sunset_minutes = 1080U,
        .headlamp_valid = true,
        .headlamps_on = false,
    };
    canview_headlamp_warning_output_t output =
        canview_headlamp_warning_update(&state, &config, &input, 1000U);
    assert(output.night_active);
    assert(!output.warning_active);
    output = canview_headlamp_warning_update(&state, &config, &input, 1000U);
    assert(output.warning_active);
    input.headlamps_on = true;
    output = canview_headlamp_warning_update(&state, &config, &input, 1000U);
    assert(!output.warning_active);
    input.local_minutes = 720U;
    input.headlamps_on = false;
    output = canview_headlamp_warning_update(&state, &config, &input, 1000U);
    assert(!output.night_active);
    assert(!output.warning_active);
}

int main(void)
{
    assert(CANVIEW_PROTOCOL_MINOR == 2U);
    assert(sizeof(canview_config_record_t) == 8U);
    test_can_brightness_debounce_and_stale_hold();
    test_idle_return_touch_restore_and_warning_priority();
    test_night_mode_tracks_tail_lamps_with_auto_brightness_off();
    test_adaptive_volume_attack_release_and_manual_hold();
    test_auto_sport_speed_hysteresis_and_restore();
    test_auto_sport_mid_speed_acceleration_and_manual_priority();
    test_acceleration_filter_rejects_single_spike();
    test_controller_filter_default_deny_and_updates();
    test_controller_signal_decode_is_catalog_driven();
    test_command_completion_requires_result();
    test_headlamp_warning_uses_rtc_sunset_and_hysteresis();
    puts("automation tests passed");
    return 0;
}
