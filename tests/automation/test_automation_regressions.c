#include "canview_controller_automation.h"
#include "canview_command_tracker.h"

#include <assert.h>
#include <stdio.h>
#include "automation_test_ticks.h"
#include <string.h>

static canview_auto_brightness_input_t brightness_input(void)
{
    return (canview_auto_brightness_input_t){
        .enabled = true,
        .lighting_valid = true,
        .manual_percent = 80U,
        .speed_valid = true,
        .speed_limit_valid = true,
        .speed_limit_active = true,
        .speed_limit_kph = 70U,
    };
}

static void test_stale_base(void)
{
    canview_auto_brightness_config_t config = canview_auto_brightness_default_config();
    config.transition_ms = 0U;
    config.idle_timeout_ms = 1000U;
    canview_auto_brightness_state_t state = {0};
    canview_auto_brightness_input_t input = brightness_input();
    canview_auto_brightness_output_t output =
        brightness_update_for(&state, &config, &input, 1000U);
    assert(output.brightness_percent == 28U);
    assert(output.return_to_default_screen);

    input.lighting_age_ms = config.stale_timeout_ms + 1U;
    for (uint16_t i = 0U; i < 100U; ++i) {
        output = brightness_update_for(&state, &config, &input, 100U);
        assert(output.brightness_percent == 28U);
        assert(!output.return_to_default_screen);
        assert(state.last_safe_base_percent == 80U);
    }
    input.user_interaction = true;
    output = brightness_update_for(&state, &config, &input, 100U);
    assert(output.brightness_percent == 80U);
    assert(!output.idle_dimmed);
    assert(output.status == CANVIEW_BRIGHTNESS_CAN_STALE);

    input.user_interaction = false;
    input.speed_tenth_kph = 770U;
    output = brightness_update_for(&state, &config, &input, 500U);
    assert(output.speed_warning_active);
    assert(output.brightness_percent == 90U);
    input.speed_valid = false;
    output = brightness_update_for(&state, &config, &input, 100U);
    assert(!output.speed_warning_active);
    assert(output.brightness_percent == 80U);
    output = brightness_update_for(&state, &config, &input, 400U);
    assert(output.brightness_percent == 28U);
    assert(state.last_safe_base_percent == 80U);

    /* stale 도중 명시적 수동 밝기는 새 base가 된다. */
    input.enabled = false;
    input.manual_percent = 60U;
    output = brightness_update_for(&state, &config, &input, 100U);
    assert(output.brightness_percent == 21U);
    input.enabled = true;
    input.manual_percent = 100U;
    input.user_interaction = true;
    output = brightness_update_for(&state, &config, &input, 100U);
    assert(output.brightness_percent == 60U);

    /* 초기 stale에는 초기 수동 base, 유효 재수신 후에는 새 야간 base를 쓴다. */
    canview_auto_brightness_reset(&state, 40U);
    output = brightness_update_for(&state, &config, &input, 100U);
    assert(output.brightness_percent == 40U);
    input.lighting_age_ms = 0U;
    input.tail_lamps_on = true;
    input.dimmer_valid = true;
    input.dimmer_percent = 50U;
    output = brightness_update_for(&state, &config, &input, 500U);
    assert(output.brightness_percent == 31U);
    input.lighting_valid = false;
    input.user_interaction = false;
    output = brightness_update_for(&state, &config, &input, 1000U);
    assert(output.brightness_percent == 11U);
    for (uint16_t i = 0U; i < 100U; ++i) {
        output = brightness_update_for(&state, &config, &input, 100U);
        assert(output.brightness_percent == 11U);
        assert(output.night_mode_active);
    }
}

static void test_stale_warning_restore_ramp(void)
{
    canview_auto_brightness_config_t config = canview_auto_brightness_default_config();
    config.idle_timeout_ms = 1000U;
    canview_auto_brightness_state_t state = {0};
    canview_auto_brightness_input_t input = brightness_input();
    input.tail_lamps_on = true;
    canview_auto_brightness_output_t output = {0};
    for (uint16_t i = 0U; i < 30U; ++i) {
        output = brightness_update_for(&state, &config, &input, 100U);
    }
    assert(output.brightness_percent == 11U);
    input.speed_tenth_kph = 770U;
    for (uint16_t i = 0U; i < 20U; ++i) {
        output = brightness_update_for(&state, &config, &input, 100U);
    }
    assert(output.brightness_percent == 90U);

    input.lighting_valid = false;
    input.speed_tenth_kph = 700U;
    for (uint16_t i = 0U; i < 9U; ++i) {
        output = brightness_update_for(&state, &config, &input, 100U);
        assert(output.speed_warning_active);
    }
    for (uint16_t i = 0U; i < 30U; ++i) {
        const uint8_t previous = output.brightness_percent;
        output = brightness_update_for(&state, &config, &input, 100U);
        assert(!output.speed_warning_active);
        assert(output.brightness_percent <= previous);
        assert(previous - output.brightness_percent <= 9U);
    }
    assert(output.brightness_percent == 11U);
    assert(state.last_safe_base_percent == 30U);
    input.user_interaction = true;
    for (uint16_t i = 0U; i < 10U; ++i) {
        output = brightness_update_for(&state, &config, &input, 100U);
    }
    assert(output.brightness_percent == 30U);
}

static void test_speed_freshness(void)
{
    canview_auto_brightness_config_t config = canview_auto_brightness_default_config();
    config.transition_ms = 0U;
    config.idle_timeout_ms = 0U;
    assert(config.speed_stale_timeout_ms == 150U);
    canview_auto_brightness_state_t state = {0};
    canview_auto_brightness_input_t input = brightness_input();
    input.speed_tenth_kph = 770U;
    input.speed_valid = false;
    canview_auto_brightness_output_t output =
        brightness_update_for(&state, &config, &input, 500U);
    assert(!output.speed_warning_active);
    input.speed_valid = true;
    input.speed_age_ms = 151U;
    output = brightness_update_for(&state, &config, &input, 500U);
    assert(!output.speed_warning_active);
    assert(state.speed_warning_on_ms == 0U);

    input.speed_age_ms = 150U;
    output = brightness_update_for(&state, &config, &input, 400U);
    assert(!output.speed_warning_active);
    input.speed_age_ms = UINT16_MAX;
    output = brightness_update_for(&state, &config, &input, 0U);
    assert(state.speed_warning_on_ms == 0U);
    input.speed_age_ms = 150U;
    output = brightness_update_for(&state, &config, &input, 100U);
    assert(!output.speed_warning_active);
    output = brightness_update_for(&state, &config, &input, 400U);
    assert(output.speed_warning_active);

    input.speed_age_ms = 151U;
    output = brightness_update_for(&state, &config, &input, 0U);
    assert(!output.speed_warning_active);
    assert(!output.speed_warning_visible);
    assert(output.brightness_percent == 80U);
    assert(state.speed_warning_on_ms == 0U);
    assert(state.speed_warning_off_ms == 0U);
    assert(state.speed_warning_flash_ms == 0U);

    /* freshness 회복은 dwell을 처음부터 재평가하고 각 limit 상실은 즉시 해제한다. */
    input.speed_age_ms = 0U;
    for (uint8_t scenario = 0U; scenario < 4U; ++scenario) {
        input.speed_valid = true;
        input.speed_limit_valid = true;
        input.speed_limit_active = true;
        input.speed_limit_kph = 70U;
        output = brightness_update_for(&state, &config, &input, 500U);
        assert(output.speed_warning_active);
        if (scenario == 0U) {
            input.speed_valid = false;
        } else if (scenario == 1U) {
            input.speed_limit_valid = false;
        } else if (scenario == 2U) {
            input.speed_limit_active = false;
        } else {
            input.speed_limit_kph = 0U;
        }
        output = brightness_update_for(&state, &config, &input, 1U);
        assert(!output.speed_warning_active);
        assert(!output.speed_warning_visible);
    }
}

static canview_adaptive_volume_input_t volume_input(void)
{
    return (canview_adaptive_volume_input_t){
        .enabled = true,
        .fft_valid = true,
        .speed_tenth_kph = 700U,
        .dominant_frequency_hz = 500U,
        .band_excess_tenth_db = 60,
        .confidence_percent = 90U,
    };
}

static void test_volume_pending(void)
{
    const canview_adaptive_volume_config_t config = canview_adaptive_volume_default_config();
    canview_adaptive_volume_state_t state = {0};
    canview_adaptive_volume_input_t input = volume_input();
    canview_adaptive_volume_output_t output =
        volume_update_for(&state, &config, &input, config.attack_ms);
    assert(output.action == CANVIEW_VOLUME_ACTION_SET_OFFSET);
    assert(output.target_offset_steps == 1);
    for (uint16_t i = 0U; i < 100U; ++i) {
        output = volume_update_for(&state, &config, &input, 1000U);
        assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
        assert(output.target_offset_steps == 1);
    }
    assert(state.command_pending);
    assert(state.attack_evidence_ms == 0U);
    /* input 값의 일치나 transport ACK를 적용 성공으로 사용할 수 없다. */
    input.applied_offset_steps = 1;
    output = volume_update_for(&state, &config, &input, config.attack_ms);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
    assert(state.command_pending);
    canview_adaptive_volume_reconcile(&state, 1);
    output = volume_update_for(&state, &config, &input, config.attack_ms - 1U);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
    output = volume_update_for(&state, &config, &input, 1U);
    assert(output.action == CANVIEW_VOLUME_ACTION_SET_OFFSET);
    assert(output.target_offset_steps == 2);

    input.manual_volume_changed = true;
    output = volume_update_for(&state, &config, &input, 100U);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
    assert(state.reset_offset_requested);
    input.manual_volume_changed = false;
    canview_adaptive_volume_reconcile(&state, 2);
    output = volume_update_for(&state, &config, &input, 100U);
    assert(output.action == CANVIEW_VOLUME_ACTION_SET_OFFSET);
    assert(output.target_offset_steps == 0);
    assert(output.status == CANVIEW_VOLUME_PAUSED);
    output = volume_update_for(&state, &config, &input, 100U);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
    /* 거부 뒤 실제 2를 확인해도 수동 hold 중 0 요청을 무한 반복하지 않는다. */
    canview_adaptive_volume_reconcile(&state, 2);
    output = volume_update_for(&state, &config, &input, 100U);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
    assert(output.target_offset_steps == 2);

    canview_adaptive_volume_reset(&state, 0);
    output = volume_update_for(&state, &config, &input, config.attack_ms);
    assert(output.action == CANVIEW_VOLUME_ACTION_SET_OFFSET);
    input.enabled = false;
    output = volume_update_for(&state, &config, &input, config.attack_ms);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
    canview_adaptive_volume_reconcile(&state, 1);
    output = volume_update_for(&state, &config, &input, 100U);
    assert(output.action == CANVIEW_VOLUME_ACTION_SET_OFFSET);
    assert(output.target_offset_steps == 0);
    canview_adaptive_volume_reconcile(&state, 0);
    output = volume_update_for(&state, &config, &input, 100U);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
    assert(output.status == CANVIEW_VOLUME_IDLE);
}

static void test_volume_invalid_resets_evidence(void)
{
    const canview_adaptive_volume_config_t config = canview_adaptive_volume_default_config();
    canview_adaptive_volume_input_t input = volume_input();
    canview_adaptive_volume_state_t state = {0};
    canview_adaptive_volume_output_t output =
        volume_update_for(&state, &config, &input, config.attack_ms - 1U);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
    input.fft_valid = false;
    output = volume_update_for(&state, &config, &input, 0U);
    assert(output.status == CANVIEW_VOLUME_INVALID);
    assert(state.attack_evidence_ms == 0U);
    input.fft_valid = true;
    output = volume_update_for(&state, &config, &input, 1U);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);

    canview_adaptive_volume_reset(&state, 2);
    input.band_excess_tenth_db = 0;
    output = volume_update_for(&state, &config, &input, config.release_ms - 1U);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
    input.confidence_percent = config.min_confidence_percent - 1U;
    output = volume_update_for(&state, &config, &input, 0U);
    assert(output.status == CANVIEW_VOLUME_INVALID);
    assert(state.release_evidence_ms == 0U);
    input.confidence_percent = config.min_confidence_percent;
    output = volume_update_for(&state, &config, &input, 1U);
    assert(output.action == CANVIEW_VOLUME_ACTION_NONE);
}

static void test_headlamp_freshness(void)
{
    const canview_headlamp_warning_config_t config = canview_headlamp_warning_default_config();
    canview_headlamp_warning_input_t input = {
        .enabled = true,
        .vehicle_awake = true,
        .rtc_valid = true,
        .local_minutes = 60U,
        .solar_valid = true,
        .sunrise_minutes = 360U,
        .sunset_minutes = 1080U,
        .headlamp_valid = true,
    };
    /* 일출 전 OR 분기도 모든 validity/age/awake/enable guard 아래 있다. */
    for (uint8_t scenario = 0U; scenario < 8U; ++scenario) {
        canview_headlamp_warning_state_t state = {0};
        canview_headlamp_warning_input_t invalid = input;
        canview_headlamp_warning_output_t output =
            headlamp_update_for(&state, &config, &input, 2000U);
        assert(output.warning_active);
        switch (scenario) {
        case 0U: invalid.rtc_valid = false; break;
        case 1U: invalid.solar_valid = false; break;
        case 2U: invalid.headlamp_valid = false; break;
        case 3U: invalid.rtc_age_ms = config.stale_timeout_ms + 1U; break;
        case 4U: invalid.solar_age_ms = config.stale_timeout_ms + 1U; break;
        case 5U: invalid.headlamp_age_ms = config.stale_timeout_ms + 1U; break;
        case 6U: invalid.vehicle_awake = false; break;
        default: invalid.enabled = false; break;
        }
        output = headlamp_update_for(&state, &config, &invalid, 0U);
        assert(output.status == CANVIEW_HEADLAMP_WARNING_INVALID);
        assert(!output.warning_active);
        assert(!output.night_active);
        output = headlamp_update_for(&state, &config, &input, 1999U);
        assert(!output.warning_active);
        output = headlamp_update_for(&state, &config, &input, 1U);
        assert(output.warning_active);
    }
    canview_auto_brightness_config_t brightness_config = canview_auto_brightness_default_config();
    brightness_config.transition_ms = 0U;
    canview_auto_brightness_state_t brightness_state = {0};
    canview_auto_brightness_input_t lighting = brightness_input();
    lighting.tail_lamps_on = true;
    const canview_auto_brightness_output_t brightness =
        brightness_update_for(&brightness_state, &brightness_config, &lighting, 500U);
    assert(brightness.night_mode_active);
    canview_headlamp_warning_state_t state = {0};
    canview_headlamp_warning_output_t output =
        headlamp_update_for(&state, &config, &input, 2000U);
    assert(output.warning_active); /* 미등 ON으로 low beam OFF를 덮지 않는다. */
    input.headlamps_on = true;
    output = headlamp_update_for(&state, &config, &input, 999U);
    assert(output.warning_active);
    output = headlamp_update_for(&state, &config, &input, 1U);
    assert(!output.warning_active);
}

static canview_auto_sport_input_t sport_input(void)
{
    return (canview_auto_sport_input_t){
        .enabled = true,
        .signals_fresh = true,
        .forward_gear = true,
        .control_link_ready = true,
        .current_mode = CANVIEW_DRIVE_MODE_ECO,
        .speed_tenth_kph = 700U,
    };
}

static void test_sport_feedback(void)
{
    const canview_auto_sport_config_t config = canview_auto_sport_default_config();
    for (uint8_t exit_pending = 0U; exit_pending < 2U; ++exit_pending) {
        canview_auto_sport_state_t state = {
            .status = exit_pending ? CANVIEW_SPORT_EXIT_PENDING : CANVIEW_SPORT_ENTER_PENDING,
            .previous_mode = CANVIEW_DRIVE_MODE_ECO,
            .owns_sport_mode = exit_pending != 0U,
        };
        canview_auto_sport_input_t input = sport_input();
        input.current_mode = exit_pending ? CANVIEW_DRIVE_MODE_ECO : CANVIEW_DRIVE_MODE_SPORT;
        input.signals_fresh = false;
        canview_auto_sport_output_t output =
            canview_auto_sport_update(&state, &config, &input, 100U);
        assert(output.status == (exit_pending ? CANVIEW_SPORT_EXIT_PENDING : CANVIEW_SPORT_ENTER_PENDING));
        assert(state.owns_sport_mode == (exit_pending != 0U));
        input.signals_fresh = true;
        input.control_link_ready = false;
        output = canview_auto_sport_update(&state, &config, &input, 100U);
        assert(output.status == (exit_pending ? CANVIEW_SPORT_EXIT_PENDING : CANVIEW_SPORT_ENTER_PENDING));
        input.control_link_ready = true;
        output = canview_auto_sport_update(&state, &config, &input, 100U);
        assert(output.status == (exit_pending ? CANVIEW_SPORT_ARMED : CANVIEW_SPORT_ACTIVE));

        state = (canview_auto_sport_state_t){
            .status = exit_pending ? CANVIEW_SPORT_EXIT_PENDING : CANVIEW_SPORT_ENTER_PENDING,
            .previous_mode = CANVIEW_DRIVE_MODE_ECO,
            .owns_sport_mode = exit_pending != 0U,
            .pending_ms = config.command_timeout_ms - 1U,
        };
        output = canview_auto_sport_update(&state, &config, &input, 1U);
        assert(output.status == CANVIEW_SPORT_MANUAL_HOLD);
        assert(output.action == CANVIEW_SPORT_ACTION_NONE);
        assert(!state.owns_sport_mode);
        assert(state.previous_mode == CANVIEW_DRIVE_MODE_UNKNOWN);
        output = sport_update_for(&state, &config, &input, 5000U);
        assert(output.status == CANVIEW_SPORT_MANUAL_HOLD);
        assert(output.action == CANVIEW_SPORT_ACTION_NONE);
    }
}

static void test_sport_ownership(void)
{
    const canview_auto_sport_config_t config = canview_auto_sport_default_config();
    for (uint8_t disabled = 0U; disabled < 2U; ++disabled) {
        canview_auto_sport_state_t state = {
            .status = CANVIEW_SPORT_ACTIVE,
            .previous_mode = CANVIEW_DRIVE_MODE_ECO,
            .owns_sport_mode = true,
            .sport_active_ms = config.minimum_sport_hold_ms,
            .exit_evidence_ms = config.exit_confirm_ms - 100U,
        };
        canview_auto_sport_input_t input = sport_input();
        input.enabled = disabled == 0U;
        input.speed_tenth_kph = 0U;
        input.current_mode = CANVIEW_DRIVE_MODE_NORMAL;
        canview_auto_sport_output_t output =
            canview_auto_sport_update(&state, &config, &input, 100U);
        assert(output.status == CANVIEW_SPORT_MANUAL_HOLD);
        assert(output.action == CANVIEW_SPORT_ACTION_NONE);
        assert(!state.owns_sport_mode);
        assert(state.previous_mode == CANVIEW_DRIVE_MODE_UNKNOWN);
    }
    canview_auto_sport_state_t state = {0};
    canview_auto_sport_input_t input = sport_input();
    input.current_mode = (canview_drive_mode_t)255;
    const canview_auto_sport_output_t output = sport_update_for(&state, &config, &input, 2500U);
    assert(output.status == CANVIEW_SPORT_INHIBITED);
    assert(output.action == CANVIEW_SPORT_ACTION_NONE);
}

static void test_time_gaps(void)
{
    const uint32_t gaps[] = {251U, 5000U, UINT32_MAX};
    const canview_auto_brightness_config_t brightness_config = canview_auto_brightness_default_config();
    const canview_adaptive_volume_config_t volume_config = canview_adaptive_volume_default_config();
    const canview_auto_sport_config_t sport_config = canview_auto_sport_default_config();
    for (size_t i = 0U; i < sizeof(gaps) / sizeof(gaps[0]); ++i) {
        canview_auto_brightness_state_t lighting = {0};
        canview_auto_brightness_input_t lights = brightness_input();
        lights.tail_lamps_on = true;
        lights.speed_tenth_kph = 770U;
        (void)brightness_update_for(&lighting, &brightness_config, &lights, 400U);
        const canview_auto_brightness_output_t brightness =
            canview_auto_brightness_update(&lighting, &brightness_config, &lights, gaps[i]);
        assert(!brightness.night_mode_active);
        assert(!brightness.speed_warning_active);
        assert(lighting.pending_ms == 0U);
        assert(lighting.speed_warning_on_ms == 0U);

        canview_adaptive_volume_state_t volume = {0};
        canview_adaptive_volume_input_t noise = volume_input();
        (void)volume_update_for(&volume, &volume_config, &noise, volume_config.attack_ms - 1U);
        const canview_adaptive_volume_output_t offset =
            canview_adaptive_volume_update(&volume, &volume_config, &noise, gaps[i]);
        assert(offset.action == CANVIEW_VOLUME_ACTION_NONE);
        assert(volume.attack_evidence_ms == 0U);
        assert(volume.release_evidence_ms == 0U);

        canview_auto_sport_state_t sport = {0};
        canview_auto_sport_input_t drive = sport_input();
        (void)sport_update_for(&sport, &sport_config, &drive, sport_config.speed_confirm_ms - 1U);
        canview_auto_sport_output_t mode =
            canview_auto_sport_update(&sport, &sport_config, &drive, gaps[i]);
        assert(mode.action == CANVIEW_SPORT_ACTION_NONE);
        assert(sport.speed_evidence_ms == 0U);
        assert(sport.acceleration_evidence_ms == 0U);
        mode = canview_auto_sport_update(&sport, &sport_config, &drive, 100U);
        assert(mode.action == CANVIEW_SPORT_ACTION_NONE);

        sport.status = CANVIEW_SPORT_ENTER_PENDING;
        sport.previous_mode = CANVIEW_DRIVE_MODE_ECO;
        drive.current_mode = CANVIEW_DRIVE_MODE_SPORT;
        mode = canview_auto_sport_update(&sport, &sport_config, &drive, gaps[i]);
        assert(mode.status != CANVIEW_SPORT_ACTIVE);
        assert(!sport.owns_sport_mode);
        assert(mode.action == CANVIEW_SPORT_ACTION_NONE);
    }
    canview_auto_sport_state_t sport = {0};
    canview_auto_sport_input_t drive = sport_input();
    (void)canview_auto_sport_update(&sport, &sport_config, &drive, 250U);
    assert(sport.speed_evidence_ms == 100U);
    canview_adaptive_volume_state_t volume = {0};
    canview_adaptive_volume_input_t noise = volume_input();
    (void)canview_adaptive_volume_update(&volume, &volume_config, &noise, 250U);
    assert(volume.attack_evidence_ms == 100U);
}

static void test_command_double_ack(void)
{
    canview_command_tracker_t tracker;
    canview_command_tracker_init(&tracker);
    assert(canview_command_tracker_begin(&tracker, 123U, 0x0201U, 1000U, UINT32_MAX - 100U));
    for (uint8_t i = 0U; i < 2U; ++i) {
        assert(canview_command_tracker_on_ack(&tracker, 123U, CANVIEW_ACK_ACCEPTED));
        assert(canview_command_tracker_on_ack(&tracker, 123U, CANVIEW_ACK_DUPLICATE));
        assert(canview_command_tracker_status(&tracker, 123U) == CANVIEW_COMMAND_TRACK_WAITING_RESULT);
    }
    canview_command_result_t result = {
        .request_token_le = 123U,
        .command_id_le = 0x0201U,
        .stage = CANVIEW_COMMAND_COMPLETED,
    };
    assert(canview_command_tracker_on_result(&tracker, &result));
    assert(canview_command_tracker_on_result(&tracker, &result));
    assert(canview_command_tracker_on_ack(&tracker, 123U, CANVIEW_ACK_DUPLICATE));
    assert(canview_command_tracker_status(&tracker, 123U) == CANVIEW_COMMAND_TRACK_COMPLETED);
    assert(canview_command_tracker_begin(&tracker, 124U, 0x0201U, 1000U, UINT32_MAX - 100U));
    canview_command_tracker_expire(&tracker, 898U);
    assert(canview_command_tracker_status(&tracker, 124U) == CANVIEW_COMMAND_TRACK_WAITING_ACK);
    canview_command_tracker_expire(&tracker, 899U);
    assert(canview_command_tracker_status(&tracker, 124U) == CANVIEW_COMMAND_TRACK_EXPIRED);
    assert(canview_command_tracker_on_ack(&tracker, 124U, CANVIEW_ACK_DUPLICATE));
    assert(canview_command_tracker_status(&tracker, 124U) == CANVIEW_COMMAND_TRACK_EXPIRED);
    result.request_token_le = 124U;
    assert(!canview_command_tracker_on_result(&tracker, &result));
}

typedef struct {
    const char *name;
    void (*run)(void);
} regression_case_t;

int main(int argc, char **argv)
{
    static const regression_case_t cases[] = {
        {"brightness-stale", test_stale_base},
        {"brightness-ramp", test_stale_warning_restore_ramp},
        {"brightness-speed", test_speed_freshness},
        {"volume-pending", test_volume_pending},
        {"volume-invalid", test_volume_invalid_resets_evidence},
        {"headlamp-freshness", test_headlamp_freshness},
        {"sport-feedback", test_sport_feedback},
        {"sport-ownership", test_sport_ownership},
        {"automation-time-gap", test_time_gaps},
        {"command-double-ack", test_command_double_ack},
    };
    bool ran = false;
    for (size_t i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (argc == 1 || (argc == 2 && strcmp(argv[1], cases[i].name) == 0)) {
            cases[i].run();
            puts(cases[i].name);
            ran = true;
        }
    }
    return ran ? 0 : 1;
}
