#ifndef CANVIEW_CONTROLLER_AUTOMATION_H
#define CANVIEW_CONTROLLER_AUTOMATION_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CANVIEW_BRIGHTNESS_MANUAL = 0,
    CANVIEW_BRIGHTNESS_CAN_DAY,
    CANVIEW_BRIGHTNESS_CAN_NIGHT,
    CANVIEW_BRIGHTNESS_CAN_STALE,
} canview_brightness_status_t;

typedef struct {
    uint8_t night_min_percent;
    uint8_t night_max_percent;
    uint8_t night_default_percent;
    uint16_t lamp_on_confirm_ms;
    uint16_t lamp_off_confirm_ms;
    uint16_t transition_ms;
    uint16_t stale_timeout_ms;
} canview_auto_brightness_config_t;

typedef struct {
    bool enabled;
    bool lighting_valid;
    bool exterior_lamps_on;
    bool dimmer_valid;
    uint8_t dimmer_percent;
    uint8_t manual_percent;
    uint16_t lighting_age_ms;
} canview_auto_brightness_input_t;

typedef struct {
    bool initialized;
    bool night_active;
    bool pending_night;
    uint32_t pending_ms;
    uint16_t current_tenth_percent;
} canview_auto_brightness_state_t;

typedef struct {
    uint8_t brightness_percent;
    canview_brightness_status_t status;
} canview_auto_brightness_output_t;

typedef enum {
    CANVIEW_VOLUME_IDLE = 0,
    CANVIEW_VOLUME_LISTENING,
    CANVIEW_VOLUME_ATTACK,
    CANVIEW_VOLUME_RELEASE,
    CANVIEW_VOLUME_PAUSED,
    CANVIEW_VOLUME_INVALID,
} canview_adaptive_volume_status_t;

typedef enum {
    CANVIEW_VOLUME_ACTION_NONE = 0,
    CANVIEW_VOLUME_ACTION_SET_OFFSET,
} canview_adaptive_volume_action_t;

typedef enum {
    CANVIEW_NOISE_BAND_ROAD = 0,
    CANVIEW_NOISE_BAND_BALANCED,
    CANVIEW_NOISE_BAND_WIND,
} canview_noise_band_profile_t;

typedef enum {
    CANVIEW_NOISE_SENSITIVITY_LOW = 0,
    CANVIEW_NOISE_SENSITIVITY_NORMAL,
    CANVIEW_NOISE_SENSITIVITY_HIGH,
} canview_noise_sensitivity_t;

typedef enum {
    CANVIEW_NOISE_RESPONSE_GENTLE = 0,
    CANVIEW_NOISE_RESPONSE_NORMAL,
    CANVIEW_NOISE_RESPONSE_FAST,
} canview_noise_response_t;

typedef struct {
    uint16_t focus_low_hz;
    uint16_t focus_high_hz;
    uint16_t min_speed_tenth_kph;
    int16_t raise_excess_tenth_db;
    int16_t lower_excess_tenth_db;
    uint16_t attack_ms;
    uint16_t release_ms;
    uint16_t step_interval_ms;
    uint16_t feedback_freeze_ms;
    uint32_t manual_hold_ms;
    uint8_t max_offset_steps;
    uint8_t min_confidence_percent;
} canview_adaptive_volume_config_t;

typedef struct {
    bool enabled;
    bool fft_valid;
    bool reverse_active;
    bool audio_priority_active;
    bool manual_volume_changed;
    uint16_t speed_tenth_kph;
    uint16_t dominant_frequency_hz;
    int16_t band_excess_tenth_db;
    uint8_t confidence_percent;
    int8_t applied_offset_steps;
} canview_adaptive_volume_input_t;

typedef struct {
    bool initialized;
    int8_t desired_offset_steps;
    uint32_t attack_evidence_ms;
    uint32_t release_evidence_ms;
    uint32_t step_cooldown_ms;
    uint32_t feedback_freeze_ms;
    uint32_t manual_hold_ms;
} canview_adaptive_volume_state_t;

typedef struct {
    canview_adaptive_volume_action_t action;
    canview_adaptive_volume_status_t status;
    int8_t target_offset_steps;
    uint16_t attack_progress_permil;
    uint16_t release_progress_permil;
} canview_adaptive_volume_output_t;

canview_auto_brightness_config_t canview_auto_brightness_default_config(void);
canview_adaptive_volume_config_t canview_adaptive_volume_default_config(void);
void canview_adaptive_volume_apply_settings(
    canview_adaptive_volume_config_t *config,
    canview_noise_band_profile_t band,
    canview_noise_sensitivity_t sensitivity,
    canview_noise_response_t response,
    uint8_t maximum_offset_steps);

void canview_auto_brightness_reset(canview_auto_brightness_state_t *state,
                                   uint8_t initial_percent);
canview_auto_brightness_output_t canview_auto_brightness_update(
    canview_auto_brightness_state_t *state,
    const canview_auto_brightness_config_t *config,
    const canview_auto_brightness_input_t *input,
    uint32_t elapsed_ms);

void canview_adaptive_volume_reset(canview_adaptive_volume_state_t *state,
                                   int8_t applied_offset_steps);
void canview_adaptive_volume_reconcile(canview_adaptive_volume_state_t *state,
                                       int8_t applied_offset_steps);
canview_adaptive_volume_output_t canview_adaptive_volume_update(
    canview_adaptive_volume_state_t *state,
    const canview_adaptive_volume_config_t *config,
    const canview_adaptive_volume_input_t *input,
    uint32_t elapsed_ms);

#ifdef __cplusplus
}
#endif

#endif
