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

#define CANVIEW_UI_FFT_BIN_COUNT 23

typedef enum {
    CANVIEW_UI_QUALITY_UNAVAILABLE = 0,
    CANVIEW_UI_QUALITY_CANDIDATE,
    CANVIEW_UI_QUALITY_ESTIMATED,
    CANVIEW_UI_QUALITY_VERIFIED,
} canview_ui_quality_t;

typedef enum {
    CANVIEW_UI_DRIVE_UNKNOWN = 0,
    CANVIEW_UI_DRIVE_NORMAL,
    CANVIEW_UI_DRIVE_SPORT,
} canview_ui_drive_mode_t;

typedef enum {
    CANVIEW_UI_CMD_SET_QUIET = 1,
    CANVIEW_UI_CMD_SET_REAR_BOOST,
    CANVIEW_UI_CMD_STEP_VOLUME,
    CANVIEW_UI_CMD_SET_ADAPTIVE_VOLUME,
    CANVIEW_UI_CMD_SET_SPORT_MONITOR,
    CANVIEW_UI_CMD_SET_BRIGHTNESS,
    CANVIEW_UI_CMD_SET_AUTO_BRIGHTNESS,
    CANVIEW_UI_CMD_SET_METRIC_UNITS,
} canview_ui_command_id_t;

typedef struct {
    canview_ui_command_id_t id;
    union {
        bool enabled;
        int8_t step_delta;
        uint8_t percent;
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
    uint16_t engine_rpm;

    uint8_t rear_coupling_percent;
    uint16_t clutch_torque_nm;
    canview_ui_quality_t four_wd_quality;

    bool dpf_lamp_on;

    canview_ui_drive_mode_t drive_mode;
    bool sport_monitor_enabled;

    bool quiet_mode_enabled;
    bool rear_boost_enabled;
    bool adaptive_volume_enabled;
    uint8_t audio_volume_level;
    int8_t volume_offset_step;

    uint8_t fft_bins[CANVIEW_UI_FFT_BIN_COUNT];
    uint16_t fft_peak_hz;
    uint16_t fft_peak_tenth_db;

    uint8_t display_brightness_percent;
    bool auto_brightness_enabled;
    bool metric_units;

    uint8_t active_bus_count;
    int8_t esp_now_rssi_dbm;
} canview_ui_model_t;

/* 단일 320x480 화면 인스턴스를 생성한다. parent가 NULL이면 lv_scr_act()를 사용한다. */
lv_obj_t *canview_ui_create(lv_obj_t *parent, const canview_ui_config_t *config);

/* LVGL 스레드에서만 호출한다. 값은 표시 모델이며 CAN 프레임을 직접 뜻하지 않는다. */
void canview_ui_update(const canview_ui_model_t *model);

void canview_ui_show_screen(canview_ui_screen_t screen);
canview_ui_screen_t canview_ui_current_screen(void);

#ifdef __cplusplus
}
#endif

#endif
