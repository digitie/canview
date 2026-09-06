#ifndef CANVIEW_CONTROLLER_AUTOMATION_H
#define CANVIEW_CONTROLLER_AUTOMATION_H

#include <stdbool.h>
#include <stdint.h>

#define CANVIEW_AUTOMATION_MAX_EVIDENCE_MS (100U)
#define CANVIEW_AUTOMATION_MAX_TICK_GAP_MS (250U)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CANVIEW_BRIGHTNESS_MANUAL = 0,
    CANVIEW_BRIGHTNESS_CAN_DAY,
    CANVIEW_BRIGHTNESS_CAN_NIGHT,
    CANVIEW_BRIGHTNESS_CAN_STALE,
    CANVIEW_BRIGHTNESS_IDLE_DIM,
    CANVIEW_BRIGHTNESS_SPEED_WARNING,
} canview_brightness_status_t;

typedef struct {
    uint8_t night_min_percent;
    uint8_t night_max_percent;
    uint8_t night_default_percent;
    uint16_t lamp_on_confirm_ms;
    uint16_t lamp_off_confirm_ms;
    uint16_t transition_ms;
    uint16_t stale_timeout_ms;
    uint32_t idle_timeout_ms;
    uint8_t idle_dim_percent;
    uint8_t speed_warning_enter_percent;
    uint8_t speed_warning_exit_percent;
    uint16_t speed_warning_on_confirm_ms;
    uint16_t speed_warning_off_confirm_ms;
    uint16_t speed_warning_flash_interval_ms;
    uint8_t speed_warning_brightness_percent;
    uint16_t speed_stale_timeout_ms; /**< 표시용 차속 freshness 상한, 기본 150 ms. */
} canview_auto_brightness_config_t;

typedef struct {
    bool enabled;
    bool lighting_valid;
    bool tail_lamps_on;
    bool dimmer_valid;
    uint8_t dimmer_percent;
    uint8_t manual_percent;
    uint16_t lighting_age_ms;
    bool user_interaction;
    bool speed_limit_valid;
    bool speed_limit_active;
    uint16_t speed_tenth_kph;
    uint8_t speed_limit_kph;
    bool speed_valid; /**< 검증된 차속 source만 true. 초기값 false는 경고 차단. */
    uint16_t speed_age_ms; /**< 마지막 차속 sample 이후 경과 시간, 포화 처리. */
} canview_auto_brightness_input_t;

typedef struct {
    bool initialized;
    bool night_active;
    bool pending_night;
    uint32_t pending_ms;
    uint16_t current_tenth_percent;
    uint32_t idle_elapsed_ms;
    bool idle_dimmed;
    bool speed_warning_active;
    uint32_t speed_warning_on_ms;
    uint32_t speed_warning_off_ms;
    uint32_t speed_warning_flash_ms;
    uint8_t last_safe_base_percent; /**< idle 감광/경고 boost 전 마지막 유효 목표. */
} canview_auto_brightness_state_t;

typedef struct {
    uint8_t brightness_percent;
    canview_brightness_status_t status;
    bool night_mode_active;
    bool idle_dimmed;
    bool return_to_default_screen;
    bool speed_warning_active;
    bool speed_warning_visible;
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
    bool command_pending; /**< reconcile 전에는 다음 offset 요청을 만들지 않는다. */
    bool reset_offset_requested; /**< pending 중 수동 변경의 0 offset 의도를 보존한다. */
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

/** @brief 출력과 last-safe base를 초기화한다. state는 호출자가 직렬화한다. */
void canview_auto_brightness_reset(canview_auto_brightness_state_t *state,
                                   uint8_t initial_percent);
/** @brief 유효 base에 idle/경고를 적용한다. stale 차속은 경고를 즉시 해제한다. */
canview_auto_brightness_output_t canview_auto_brightness_update(
    canview_auto_brightness_state_t *state,
    const canview_auto_brightness_config_t *config,
    const canview_auto_brightness_input_t *input,
    uint32_t elapsed_ms);

/** @brief 확인된 적용 offset으로 초기화한다. 진행 중 요청을 지우는 용도로 쓰지 않는다. */
void canview_adaptive_volume_reset(canview_adaptive_volume_state_t *state,
                                   int8_t applied_offset_steps);
/**
 * @brief terminal 결과와 최신 feedback을 확인한 호출자가 pending을 해제한다.
 * @param applied_offset_steps 성공/거부/취소 후 실제 확인된 offset. ACK는 근거가 아니다.
 * @note update와 같은 실행 문맥에서 호출한다. timeout만으로 적용값을 추정하지 않는다.
 */
void canview_adaptive_volume_reconcile(canview_adaptive_volume_state_t *state,
                                       int8_t applied_offset_steps);
/** @brief offset 의도를 하나만 생성한다. 반환 target은 적용 성공을 의미하지 않는다. */
canview_adaptive_volume_output_t canview_adaptive_volume_update(
    canview_adaptive_volume_state_t *state,
    const canview_adaptive_volume_config_t *config,
    const canview_adaptive_volume_input_t *input,
    uint32_t elapsed_ms);

typedef enum {
    CANVIEW_HEADLAMP_WARNING_INVALID = 0,
    CANVIEW_HEADLAMP_WARNING_DAY,
    CANVIEW_HEADLAMP_WARNING_NIGHT_OK,
    CANVIEW_HEADLAMP_WARNING_PENDING,
    CANVIEW_HEADLAMP_WARNING_ACTIVE,
} canview_headlamp_warning_status_t;

typedef struct {
    uint32_t after_sunset_grace_ms;
    uint16_t warning_on_confirm_ms;
    uint16_t warning_off_confirm_ms;
    uint16_t stale_timeout_ms;
} canview_headlamp_warning_config_t;

typedef struct {
    bool enabled;
    bool vehicle_awake;
    bool rtc_valid;
    uint16_t local_minutes;
    uint16_t rtc_age_ms;
    bool solar_valid;
    uint16_t sunrise_minutes;
    uint16_t sunset_minutes;
    uint16_t solar_age_ms;
    bool headlamp_valid;
    bool headlamps_on;
    uint16_t headlamp_age_ms;
} canview_headlamp_warning_input_t;

typedef struct {
    bool initialized;
    bool warning_active;
    uint32_t warning_on_evidence_ms;
    uint32_t warning_off_evidence_ms;
} canview_headlamp_warning_state_t;

typedef struct {
    canview_headlamp_warning_status_t status;
    bool night_active;
    bool warning_active;
} canview_headlamp_warning_output_t;

canview_headlamp_warning_config_t canview_headlamp_warning_default_config(void);
void canview_headlamp_warning_reset(canview_headlamp_warning_state_t *state);
canview_headlamp_warning_output_t canview_headlamp_warning_update(
    canview_headlamp_warning_state_t *state,
    const canview_headlamp_warning_config_t *config,
    const canview_headlamp_warning_input_t *input,
    uint32_t elapsed_ms);

#ifdef __cplusplus
}
#endif

#endif
