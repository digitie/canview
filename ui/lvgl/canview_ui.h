#ifndef CANVIEW_UI_H
#define CANVIEW_UI_H

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CANVIEW_UI_SCREEN_DRIVE = 0,
    CANVIEW_UI_SCREEN_AUDIO,
    CANVIEW_UI_SCREEN_FFT,
    CANVIEW_UI_SCREEN_AUTOMATION,
    CANVIEW_UI_SCREEN_SETTINGS,
    CANVIEW_UI_SCREEN_COUNT,
} canview_ui_screen_t;

#define CANVIEW_UI_FFT_BIN_COUNT (23U)
#define CANVIEW_UI_WHEEL_COUNT (4U)
#define CANVIEW_UI_RTC_YEAR_MIN (2000U)
#define CANVIEW_UI_RTC_YEAR_MAX (2099U)

typedef enum {
    CANVIEW_UI_QUALITY_UNAVAILABLE = 0,
    CANVIEW_UI_QUALITY_CANDIDATE,
    CANVIEW_UI_QUALITY_ESTIMATED,
    CANVIEW_UI_QUALITY_VERIFIED,
    CANVIEW_UI_QUALITY_STALE,
} canview_ui_quality_t;

typedef enum {
    CANVIEW_UI_TIME_UNKNOWN = 0,
    CANVIEW_UI_TIME_RTC,
    CANVIEW_UI_TIME_GNSS,
    CANVIEW_UI_TIME_MANUAL,
} canview_ui_time_source_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
} canview_ui_datetime_t;

typedef enum {
    CANVIEW_UI_DRIVE_UNKNOWN = 0,
    CANVIEW_UI_DRIVE_NORMAL,
    CANVIEW_UI_DRIVE_ECO,
    CANVIEW_UI_DRIVE_COMFORT,
    CANVIEW_UI_DRIVE_SMART,
    CANVIEW_UI_DRIVE_SPORT,
} canview_ui_drive_mode_t;

typedef enum {
    CANVIEW_UI_NOISE_BAND_ROAD = 0,
    CANVIEW_UI_NOISE_BAND_BALANCED,
    CANVIEW_UI_NOISE_BAND_WIND,
    CANVIEW_UI_NOISE_BAND_COUNT,
} canview_ui_noise_band_t;

typedef enum {
    CANVIEW_UI_SENSITIVITY_LOW = 0,
    CANVIEW_UI_SENSITIVITY_NORMAL,
    CANVIEW_UI_SENSITIVITY_HIGH,
    CANVIEW_UI_SENSITIVITY_COUNT,
} canview_ui_sensitivity_t;

typedef enum {
    CANVIEW_UI_RESPONSE_GENTLE = 0,
    CANVIEW_UI_RESPONSE_NORMAL,
    CANVIEW_UI_RESPONSE_FAST,
    CANVIEW_UI_RESPONSE_COUNT,
} canview_ui_response_t;

typedef enum {
    CANVIEW_UI_WHEEL_FRONT_LEFT = 0,
    CANVIEW_UI_WHEEL_FRONT_RIGHT,
    CANVIEW_UI_WHEEL_REAR_LEFT,
    CANVIEW_UI_WHEEL_REAR_RIGHT,
} canview_ui_wheel_t;

typedef enum {
    CANVIEW_UI_CMD_SET_QUIET = 1,
    CANVIEW_UI_CMD_SET_REAR_BOOST,
    CANVIEW_UI_CMD_SET_ADAPTIVE_VOLUME,
    CANVIEW_UI_CMD_SET_SPORT_MONITOR,
    CANVIEW_UI_CMD_SET_BRIGHTNESS,
    CANVIEW_UI_CMD_SET_AUTO_BRIGHTNESS,
    CANVIEW_UI_CMD_SET_ADAPTIVE_NOISE_BAND,
    CANVIEW_UI_CMD_SET_ADAPTIVE_SENSITIVITY,
    CANVIEW_UI_CMD_SET_ADAPTIVE_RESPONSE,
    CANVIEW_UI_CMD_SET_ADAPTIVE_MAX_OFFSET,
    CANVIEW_UI_CMD_SET_SPORT_ENTRY_SPEED,
    CANVIEW_UI_CMD_SET_SPORT_ACCELERATION,
    CANVIEW_UI_CMD_SET_IDLE_TIMEOUT,
    CANVIEW_UI_CMD_SET_RTC_TIME,
    CANVIEW_UI_CMD_USER_ACTIVITY,
    CANVIEW_UI_CMD_SET_RTC_DATETIME,
    CANVIEW_UI_CMD_COUNT,
} canview_ui_command_id_t;

typedef struct {
    canview_ui_command_id_t id;
    union {
        bool enabled;
        int8_t step_delta;
        uint8_t percent;
        uint16_t option;
        canview_ui_datetime_t datetime;
    } value;
} canview_ui_command_t;

typedef void (*canview_ui_command_cb_t)(const canview_ui_command_t *command, void *user_data);

typedef struct {
    const lv_font_t *font;
    const lv_font_t *metric_font;
    canview_ui_command_cb_t command_cb;
    void *command_user_data;
} canview_ui_config_t;

typedef struct {
    uint16_t speed_tenth_kph;
    canview_ui_quality_t speed_quality;
    uint16_t engine_rpm;
    canview_ui_quality_t engine_rpm_quality;
    uint16_t battery_voltage_tenth_v;
    canview_ui_quality_t battery_voltage_quality;
    bool transmission_clutch_locked;
    canview_ui_quality_t transmission_clutch_lock_quality;
    int16_t engine_temperature_c;
    canview_ui_quality_t engine_temperature_quality;
    /* 과거 소스 호환용 미사용 필드. 현재 UI는 평균연비를 표시하지 않는다. */
    uint16_t average_fuel_economy_tenth_kmpl;
    uint16_t instant_fuel_economy_tenth_kmpl;
    canview_ui_quality_t fuel_economy_quality;

    uint8_t rear_coupling_percent;
    uint16_t clutch_torque_nm;
    canview_ui_quality_t four_wd_quality;
    uint8_t wheel_drive_percent[CANVIEW_UI_WHEEL_COUNT];
    uint16_t tire_pressure_tenth_psi[CANVIEW_UI_WHEEL_COUNT];
    canview_ui_quality_t tire_pressure_quality;
    uint8_t tire_pressure_warning_mask;
    /* 전체 quality와 별개인 바퀴별 invalid/stale bit. FL/FR/RL/RR 순서. */
    uint8_t wheel_drive_unavailable_mask;
    uint8_t tire_pressure_unavailable_mask;

    bool dpf_lamp_on;
    canview_ui_quality_t dpf_lamp_quality;
    uint8_t dpf_load_percent;
    canview_ui_quality_t dpf_load_quality;

    canview_ui_drive_mode_t drive_mode;
    canview_ui_drive_mode_t sport_previous_mode;
    canview_ui_quality_t drive_mode_quality;
    canview_ui_quality_t sport_previous_mode_quality;
    bool sport_monitor_enabled;
    uint8_t sport_entry_speed_kph;
    bool sport_acceleration_enabled;

    bool quiet_mode_enabled;
    bool rear_boost_enabled;
    bool adaptive_volume_enabled;
    uint8_t audio_volume_level;
    int8_t volume_offset_step;
    canview_ui_quality_t audio_volume_quality;
    canview_ui_quality_t audio_profile_quality;
    canview_ui_quality_t volume_offset_quality;
    canview_ui_noise_band_t adaptive_noise_band;
    canview_ui_sensitivity_t adaptive_sensitivity;
    canview_ui_response_t adaptive_response;
    uint8_t adaptive_max_offset_steps;

    uint8_t fft_bins[CANVIEW_UI_FFT_BIN_COUNT];
    uint16_t fft_peak_hz;
    /* signed digital dBFS의 0.1 단위(-1600..0). 음압 dBA가 아니다.
     * T-303의 full-scale/FFT window/peak 정규화 gate 전에는 quality를 VERIFIED로 올리지 않는다. */
    int16_t fft_peak_tenth_db;
    canview_ui_quality_t fft_quality;

    uint8_t display_brightness_percent;
    bool auto_brightness_enabled;
    uint16_t idle_timeout_seconds;
    bool night_mode_active;
    bool idle_dimmed;
    uint8_t rtc_hour;
    uint8_t rtc_minute;
    uint16_t rtc_year;
    uint8_t rtc_month;
    uint8_t rtc_day;
    canview_ui_time_source_t rtc_source;
    canview_ui_quality_t rtc_quality;
    bool headlamp_warning_active;

    bool speed_limit_active;
    uint8_t speed_limit_kph;
    canview_ui_quality_t speed_limit_quality;
    bool speed_limit_warning_active;
    bool speed_limit_warning_visible;

    uint8_t active_bus_count;
    int8_t esp_now_rssi_dbm;
    /* config A/B owner가 확정한 설정 snapshot. 차량 설정은 matching
     * CONFIG_RESULT/APPLIED, 새 state_revision과 owner readback 확인 뒤에만 주입한다.
     * transport ACK나 비권위 NVS cache만으로 true로 올리지 않는다. */
    bool settings_valid;
    /* 상위 정책 허용 AND verified speed == 0일 때만 설정 편집 가능. */
    bool settings_edit_allowed;
    /* 승인 profile/lease 등 상위 정책의 요청 허용. 최종 TX 권한이 아니다. */
    bool audio_control_allowed;
} canview_ui_model_t;

/** @brief 단일 320x480 UI. 재호출은 기존 root를 반환한다. LVGL 스레드 전용. */
lv_obj_t *canview_ui_create(lv_obj_t *parent, const canview_ui_config_t *config);

/** @brief 객체와 animation을 해제한다. parent의 외부 삭제도 자동 감지한다. */
void canview_ui_destroy(void);

/** @brief LVGL 스레드에서 검증·정규화된 snapshot만 전달한다. ISR에서 호출 금지. */
void canview_ui_update(const canview_ui_model_t *model);

/** @brief 유효한 화면으로 이동한다. telemetry/차량 mode를 변경하지 않는다. */
void canview_ui_show_screen(canview_ui_screen_t screen);
/** @brief 현재 화면. 생성 전/삭제 후 기본값은 DRIVE다. */
canview_ui_screen_t canview_ui_current_screen(void);

/** @brief 요청 종료/거부/timeout 후 중복 억제를 해제한다. 성공 상태를 만들지 않는다.
 * 상위 orchestration이 matching terminal 결과와 필요한 feedback/owner readback을
 * 확인하고 호출한다. transport ACK 수신만으로 호출하지 않는다.
 * callback은 command를 복사하며, 같은 LVGL 스레드에서 update/완료/삭제를 재진입할 수 있다.
 * 표시값은 canview_ui_update()의 수신 모델에서만 바뀐다.
 */
void canview_ui_command_complete(canview_ui_command_id_t id);

#ifdef __cplusplus
}
#endif

#endif
