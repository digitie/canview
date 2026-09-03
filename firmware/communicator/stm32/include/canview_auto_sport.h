#ifndef CANVIEW_AUTO_SPORT_H
#define CANVIEW_AUTO_SPORT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CANVIEW_DRIVE_MODE_UNKNOWN = 0,
    CANVIEW_DRIVE_MODE_NORMAL,
    CANVIEW_DRIVE_MODE_ECO,
    CANVIEW_DRIVE_MODE_COMFORT,
    CANVIEW_DRIVE_MODE_SMART,
    CANVIEW_DRIVE_MODE_SPORT,
} canview_drive_mode_t;

typedef enum {
    CANVIEW_SPORT_DISABLED = 0,
    CANVIEW_SPORT_ARMED,
    CANVIEW_SPORT_ENTER_PENDING,
    CANVIEW_SPORT_ACTIVE,
    CANVIEW_SPORT_EXIT_PENDING,
    CANVIEW_SPORT_MANUAL_HOLD,
    CANVIEW_SPORT_INHIBITED,
} canview_auto_sport_status_t;

typedef enum {
    CANVIEW_SPORT_ACTION_NONE = 0,
    CANVIEW_SPORT_ACTION_ENTER,
    CANVIEW_SPORT_ACTION_RESTORE_PREVIOUS,
} canview_auto_sport_action_t;

typedef struct {
    bool acceleration_detection_enabled;
    uint16_t enter_speed_tenth_kph;
    uint16_t exit_speed_tenth_kph;
    uint16_t acceleration_min_speed_tenth_kph;
    int16_t acceleration_enter_milli_mps2;
    int16_t acceleration_release_milli_mps2;
    uint16_t speed_confirm_ms;
    uint16_t acceleration_confirm_ms;
    uint16_t exit_confirm_ms;
    uint16_t minimum_sport_hold_ms;
    uint16_t command_timeout_ms;
} canview_auto_sport_config_t;

typedef struct {
    bool enabled;
    bool signals_fresh;
    bool forward_gear;
    bool brake_active;
    bool abs_tcs_esc_active;
    bool drive_mode_fault;
    bool control_link_ready;
    bool physical_mode_change;
    bool rearm_requested;
    uint16_t speed_tenth_kph;
    int16_t longitudinal_acceleration_milli_mps2;
    canview_drive_mode_t current_mode;
} canview_auto_sport_input_t;

typedef struct {
    canview_auto_sport_status_t status;
    canview_drive_mode_t previous_mode;
    bool owns_sport_mode;
    uint32_t speed_evidence_ms;
    uint32_t acceleration_evidence_ms;
    uint32_t exit_evidence_ms;
    uint32_t sport_active_ms;
    uint32_t pending_ms;
} canview_auto_sport_state_t;

typedef struct {
    canview_auto_sport_action_t action;
    canview_auto_sport_status_t status;
    canview_drive_mode_t restore_mode;
} canview_auto_sport_output_t;

#define CANVIEW_ACCEL_MEDIAN_WINDOW 5

typedef struct {
    int16_t samples[CANVIEW_ACCEL_MEDIAN_WINDOW];
    int16_t filtered_milli_mps2;
    uint8_t sample_count;
    uint8_t write_index;
    bool initialized;
} canview_longitudinal_accel_filter_t;

canview_auto_sport_config_t canview_auto_sport_default_config(void);
void canview_auto_sport_set_entry_speed(canview_auto_sport_config_t *config,
                                        uint8_t speed_kph);
void canview_auto_sport_reset(canview_auto_sport_state_t *state);
void canview_longitudinal_accel_filter_reset(
    canview_longitudinal_accel_filter_t *filter);
int16_t canview_longitudinal_accel_filter_update(
    canview_longitudinal_accel_filter_t *filter,
    int16_t raw_milli_mps2,
    bool sample_valid);
canview_auto_sport_output_t canview_auto_sport_update(
    canview_auto_sport_state_t *state,
    const canview_auto_sport_config_t *config,
    const canview_auto_sport_input_t *input,
    uint32_t elapsed_ms);

#ifdef __cplusplus
}
#endif

#endif
