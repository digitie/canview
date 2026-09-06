#include "canview_ui.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "canview_theme.h"

typedef enum {
    ACTION_NAV_DRIVE = 1,
    ACTION_NAV_AUDIO,
    ACTION_NAV_FFT,
    ACTION_NAV_AUTOMATION,
    ACTION_NAV_SETTINGS,
    ACTION_QUIET,
    ACTION_REAR_BOOST,
    ACTION_ADAPTIVE_VOLUME,
    ACTION_SPORT_MONITOR,
    ACTION_AUTO_BRIGHTNESS,
    ACTION_SETTINGS_ADAPTIVE,
    ACTION_SPORT_ACCELERATION,
} action_t;

#define CANVIEW_UI_TORQUE_SEGMENTS (8U)
#define CANVIEW_UI_SPEED_MAX_TENTH_KPH (2400U)
#define CANVIEW_UI_RPM_MAX (6500U)
#define CANVIEW_UI_SETTING_DROPDOWNS (5U)
#define CANVIEW_UI_PRESSURE_MAX_TENTH_PSI (999U)
#define CANVIEW_UI_FFT_MIN_HZ (50U)
#define CANVIEW_UI_FFT_MAX_HZ (8000U)
#define CANVIEW_UI_DBFS_MIN_TENTH (-1600)
#define CANVIEW_UI_DBFS_MAX_TENTH (0)

typedef struct {
    lv_obj_t *root;
    lv_obj_t *screens[CANVIEW_UI_SCREEN_COUNT];
    lv_obj_t *nav_buttons[CANVIEW_UI_SCREEN_COUNT];
    canview_ui_screen_t screen;
    canview_ui_config_t config;
    canview_ui_model_t model;
    bool pending[CANVIEW_UI_CMD_COUNT];
    bool rtc_draft;
    lv_obj_t *settings_lock_label;
    lv_obj_t *setting_dropdowns[CANVIEW_UI_SETTING_DROPDOWNS];
    lv_obj_t *rtc_year_dropdown;
    lv_obj_t *rtc_month_dropdown;
    lv_obj_t *rtc_day_dropdown;
    lv_obj_t *rtc_apply_button;
    lv_obj_t *fft_rpm_label;
    lv_obj_t *fft_speed_label;
    lv_obj_t *fft_status_label;

    lv_obj_t *bus_label;
    lv_obj_t *link_label;
    lv_obj_t *header_speed_label;
    lv_obj_t *speed_limit_overlay;
    lv_obj_t *speed_limit_caption;
    lv_obj_t *speed_limit_label;
    lv_obj_t *headlamp_warning_overlay;
    lv_obj_t *headlamp_warning_label;
    lv_obj_t *speed_arc;
    lv_obj_t *speed_label;
    lv_obj_t *rpm_arc;
    lv_obj_t *rpm_label;
    lv_obj_t *instant_economy_label;
    lv_obj_t *dpf_label;
    lv_obj_t *dpf_load_label;
    lv_obj_t *dpf_load_bar;
    lv_obj_t *drive_mode_label;
    lv_obj_t *drive_aux_label;
    lv_obj_t *automation_aux_label;
    lv_obj_t *drive_mode_field;
    lv_obj_t *four_wd_label;
    lv_obj_t *wheel_drive_segments[CANVIEW_UI_WHEEL_COUNT][CANVIEW_UI_TORQUE_SEGMENTS];
    lv_obj_t *wheel_drive_labels[CANVIEW_UI_WHEEL_COUNT];
    lv_obj_t *wheel_pressure_labels[CANVIEW_UI_WHEEL_COUNT];

    lv_obj_t *quiet_button;
    lv_obj_t *rear_button;
    lv_obj_t *volume_label;
    lv_obj_t *volume_offset_label;
    lv_obj_t *audio_fft_chart;
    lv_chart_series_t *audio_fft_series;
    lv_obj_t *audio_peak_frequency_label;
    lv_obj_t *audio_peak_level_label;

    lv_obj_t *fft_chart;
    lv_chart_series_t *fft_series;
    lv_obj_t *fft_peak_frequency_label;
    lv_obj_t *fft_peak_level_label;

    lv_obj_t *sport_button;
    lv_obj_t *sport_button_label;
    lv_obj_t *sport_mode_field;
    lv_obj_t *sport_mode_label;
    lv_obj_t *sport_return_label;

    lv_obj_t *brightness_slider;
    lv_obj_t *brightness_label;
    lv_obj_t *auto_brightness_button;
    lv_obj_t *auto_brightness_button_label;
    lv_obj_t *settings_adaptive_button;
    lv_obj_t *settings_adaptive_button_label;
    lv_obj_t *settings_sport_button;
    lv_obj_t *settings_sport_button_label;
    lv_obj_t *sport_acceleration_button;
    lv_obj_t *sport_acceleration_button_label;
    lv_obj_t *idle_timeout_dropdown;
    lv_obj_t *rtc_hour_dropdown;
    lv_obj_t *rtc_minute_dropdown;
    lv_obj_t *rtc_time_label;
} ui_state_t;

static ui_state_t ui;

static lv_style_t style_root;
static lv_style_t style_card;
static lv_style_t style_card_soft;
static lv_style_t style_button;
static lv_style_t style_button_pressed;
static lv_style_t style_label_muted;
static lv_style_t style_label_micro;
static lv_style_t style_nav;
static bool styles_ready;

static void init_styles(const lv_font_t *font)
{
    lv_style_init(&style_root);
    lv_style_set_bg_color(&style_root, CANVIEW_COLOR_PAPER);
    lv_style_set_bg_opa(&style_root, LV_OPA_COVER);
    lv_style_set_text_color(&style_root, CANVIEW_COLOR_INK);
    lv_style_set_text_font(&style_root, font != NULL ? font : LV_FONT_DEFAULT);
    lv_style_set_pad_all(&style_root, 0);
    lv_style_set_border_width(&style_root, 0);

    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, CANVIEW_COLOR_PAPER_2);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_border_color(&style_card, CANVIEW_COLOR_RULE);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_radius(&style_card, CANVIEW_RADIUS_LG);
    lv_style_set_pad_all(&style_card, 12);

    lv_style_init(&style_card_soft);
    lv_style_set_bg_color(&style_card_soft, CANVIEW_COLOR_PAPER_2);
    lv_style_set_bg_opa(&style_card_soft, LV_OPA_COVER);
    lv_style_set_border_color(&style_card_soft, CANVIEW_COLOR_RULE_2);
    lv_style_set_border_width(&style_card_soft, 1);
    lv_style_set_radius(&style_card_soft, CANVIEW_RADIUS_MD);
    lv_style_set_pad_all(&style_card_soft, 12);

    lv_style_init(&style_button);
    lv_style_set_bg_color(&style_button, CANVIEW_COLOR_PAPER_2);
    lv_style_set_bg_opa(&style_button, LV_OPA_COVER);
    lv_style_set_border_color(&style_button, CANVIEW_COLOR_RULE);
    lv_style_set_border_width(&style_button, 1);
    lv_style_set_radius(&style_button, CANVIEW_RADIUS_SM);
    lv_style_set_text_color(&style_button, CANVIEW_COLOR_INK_2);
    lv_style_set_shadow_width(&style_button, 0);

    lv_style_init(&style_button_pressed);
    lv_style_set_bg_color(&style_button_pressed, CANVIEW_COLOR_ACCENT_WASH);
    lv_style_set_bg_opa(&style_button_pressed, LV_OPA_COVER);
    lv_style_set_border_color(&style_button_pressed, CANVIEW_COLOR_ACCENT_LINE);
    lv_style_set_text_color(&style_button_pressed, CANVIEW_COLOR_INK);

    lv_style_init(&style_label_muted);
    lv_style_set_text_color(&style_label_muted, CANVIEW_COLOR_MUTED);

    lv_style_init(&style_label_micro);
    lv_style_set_text_color(&style_label_micro, CANVIEW_COLOR_MUTED);
    lv_style_set_text_letter_space(&style_label_micro, 1);

    lv_style_init(&style_nav);
    lv_style_set_bg_color(&style_nav, CANVIEW_COLOR_PAPER_2);
    lv_style_set_bg_opa(&style_nav, LV_OPA_COVER);
    lv_style_set_border_color(&style_nav, CANVIEW_COLOR_RULE_2);
    lv_style_set_border_width(&style_nav, 1);
    lv_style_set_border_side(&style_nav, LV_BORDER_SIDE_TOP);
    lv_style_set_pad_all(&style_nav, 0);
    lv_style_set_pad_column(&style_nav, 0);

    styles_ready = true;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    return label;
}

static lv_obj_t *make_card(lv_obj_t *parent, lv_coord_t width, lv_coord_t height, bool soft)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_add_style(card, soft ? &style_card_soft : &style_card, LV_PART_MAIN);
    lv_obj_set_size(card, width, height);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

static lv_obj_t *make_button(lv_obj_t *parent, lv_coord_t width, lv_coord_t height,
                             const char *text, action_t action)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_remove_style_all(button);
    lv_obj_add_style(button, &style_button, (lv_style_selector_t)LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(button, &style_button_pressed, (lv_style_selector_t)LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(button, &style_button_pressed, (lv_style_selector_t)LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_opa(button, LV_OPA_50, LV_STATE_DISABLED);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_user_data(button, (void *)(uintptr_t)action);
    lv_obj_t *label = make_label(button, text);
    lv_obj_center(label);
    return button;
}

static bool quality_verified(canview_ui_quality_t quality)
{
    return quality == CANVIEW_UI_QUALITY_VERIFIED;
}

static bool settings_editable(void)
{
    return ui.model.settings_valid && ui.model.settings_edit_allowed &&
           quality_verified(ui.model.speed_quality) && ui.model.speed_tenth_kph == 0U;
}

static void render_settings(void);

static void emit_command(const canview_ui_command_t *command)
{
    if (ui.root == NULL || command == NULL || ui.config.command_cb == NULL ||
        (uint32_t)command->id >= CANVIEW_UI_CMD_COUNT || command->id == 0) {
        return;
    }
    const bool audio = command->id == CANVIEW_UI_CMD_SET_QUIET ||
                       command->id == CANVIEW_UI_CMD_SET_REAR_BOOST;
    if (ui.pending[command->id] ||
        (audio ? !ui.model.audio_control_allowed ||
                     !quality_verified(ui.model.audio_profile_quality)
               : !settings_editable())) {
        render_settings();
        return;
    }
    ui.pending[command->id] = true;
    render_settings();
    /* callback의 update/complete/destroy 재진입 후에는 ui에 접근하지 않는다. */
    ui.config.command_cb(command, ui.config.command_user_data);
}

static void emit_enabled(canview_ui_command_id_t id, bool enabled)
{
    canview_ui_command_t command = {.id = id};
    command.value.enabled = enabled;
    emit_command(&command);
}

static void emit_option(canview_ui_command_id_t id, uint16_t option)
{
    canview_ui_command_t command = {.id = id};
    command.value.option = option;
    emit_command(&command);
}

static void action_event(lv_event_t *event)
{
    lv_obj_t *target = lv_event_get_target(event);
    if (ui.root == NULL || lv_obj_has_state(target, LV_STATE_DISABLED)) {
        return;
    }
    const action_t action = (action_t)(uintptr_t)lv_obj_get_user_data(target);
    switch (action) {
    case ACTION_NAV_DRIVE:
    case ACTION_NAV_AUDIO:
    case ACTION_NAV_FFT:
    case ACTION_NAV_AUTOMATION:
    case ACTION_NAV_SETTINGS:
        canview_ui_show_screen((canview_ui_screen_t)(action - ACTION_NAV_DRIVE));
        break;
    case ACTION_QUIET:
        emit_enabled(CANVIEW_UI_CMD_SET_QUIET, !ui.model.quiet_mode_enabled);
        break;
    case ACTION_REAR_BOOST:
        emit_enabled(CANVIEW_UI_CMD_SET_REAR_BOOST, !ui.model.rear_boost_enabled);
        break;
    case ACTION_SETTINGS_ADAPTIVE:
    case ACTION_ADAPTIVE_VOLUME:
        emit_enabled(CANVIEW_UI_CMD_SET_ADAPTIVE_VOLUME, !ui.model.adaptive_volume_enabled);
        break;
    case ACTION_SPORT_MONITOR:
        emit_enabled(CANVIEW_UI_CMD_SET_SPORT_MONITOR, !ui.model.sport_monitor_enabled);
        break;
    case ACTION_AUTO_BRIGHTNESS:
        emit_enabled(CANVIEW_UI_CMD_SET_AUTO_BRIGHTNESS, !ui.model.auto_brightness_enabled);
        break;
    case ACTION_SPORT_ACCELERATION:
        emit_enabled(CANVIEW_UI_CMD_SET_SPORT_ACCELERATION, !ui.model.sport_acceleration_enabled);
        break;
    default:
        break;
    }
}

static void brightness_event(lv_event_t *event)
{
    if (ui.root == NULL) {
        return;
    }
    const int32_t value = lv_slider_get_value(lv_event_get_target(event));
    if (value < 10 || value > 100 || value == ui.model.display_brightness_percent) {
        render_settings();
        return;
    }
    canview_ui_command_t command = {.id = CANVIEW_UI_CMD_SET_BRIGHTNESS};
    command.value.percent = (uint8_t)value;
    emit_command(&command);
}

static void activity_event(lv_event_t *event);

static void style_dropdown(lv_obj_t *dropdown, lv_coord_t width)
{
    lv_obj_set_size(dropdown, width, 44);
    lv_obj_set_style_bg_color(dropdown, CANVIEW_COLOR_PAPER_3, 0);
    lv_obj_set_style_text_color(dropdown, CANVIEW_COLOR_INK_2, 0);
    lv_obj_set_style_text_font(dropdown, ui.config.font != NULL ? ui.config.font : LV_FONT_DEFAULT, 0);
    lv_obj_set_style_opa(dropdown, LV_OPA_50, LV_STATE_DISABLED);
    lv_obj_set_style_border_color(dropdown, CANVIEW_COLOR_RULE, 0);
    lv_obj_set_style_border_width(dropdown, 1, 0);
    lv_obj_set_style_radius(dropdown, CANVIEW_RADIUS_SM, 0);
    lv_obj_set_style_pad_all(dropdown, 8, 0);
    lv_obj_t *list = lv_dropdown_get_list(dropdown);
    lv_obj_set_style_bg_color(list, CANVIEW_COLOR_PAPER_3, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(list, CANVIEW_COLOR_INK, 0);
    lv_obj_set_style_text_font(list, ui.config.font != NULL ? ui.config.font : LV_FONT_DEFAULT, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_style_text_line_space(list, 10, 0);
    /* LVGL popup list는 screen 직속이므로 root의 press bubble에 포함되지 않는다. */
    lv_obj_add_event_cb(list, activity_event, LV_EVENT_PRESSED, NULL);
}

static const uint16_t idle_timeouts[] = {15U, 30U, 60U, 120U};
static const canview_ui_command_id_t setting_commands[CANVIEW_UI_SETTING_DROPDOWNS] = {
    CANVIEW_UI_CMD_SET_ADAPTIVE_NOISE_BAND, CANVIEW_UI_CMD_SET_ADAPTIVE_SENSITIVITY,
    CANVIEW_UI_CMD_SET_ADAPTIVE_RESPONSE, CANVIEW_UI_CMD_SET_ADAPTIVE_MAX_OFFSET,
    CANVIEW_UI_CMD_SET_SPORT_ENTRY_SPEED
};

static uint32_t setting_model_value(uint32_t index)
{
    switch (index) {
    case 0U: return (uint32_t)ui.model.adaptive_noise_band;
    case 1U: return (uint32_t)ui.model.adaptive_sensitivity;
    case 2U: return (uint32_t)ui.model.adaptive_response;
    case 3U: return ui.model.adaptive_max_offset_steps;
    case 4U: return ui.model.sport_entry_speed_kph;
    default: return UINT32_MAX;
    }
}

static void setting_dropdown_event(lv_event_t *event)
{
    lv_obj_t *target = lv_event_get_target(event);
    const uint32_t index = (uint32_t)(uintptr_t)lv_obj_get_user_data(target);
    const uint16_t selected = lv_dropdown_get_selected(target);
    if (ui.root == NULL || index >= CANVIEW_UI_SETTING_DROPDOWNS) {
        return;
    }
    const uint16_t value = index == 3U ? (uint16_t)(selected + 2U) :
                           index == 4U ? (uint16_t)(60U + selected * 10U) : selected;
    lv_dropdown_close(target);
    if (selected >= 3U || value == setting_model_value(index)) {
        render_settings();
        return;
    }
    emit_option(setting_commands[index], value);
}

static void idle_timeout_event(lv_event_t *event)
{
    const uint16_t selected = lv_dropdown_get_selected(lv_event_get_target(event));
    if (ui.root == NULL) {
        return;
    }
    lv_dropdown_close(lv_event_get_target(event));
    if (selected >= 4U || idle_timeouts[selected] == ui.model.idle_timeout_seconds) {
        render_settings();
        return;
    }
    emit_option(CANVIEW_UI_CMD_SET_IDLE_TIMEOUT, idle_timeouts[selected]);
}

static bool datetime_valid(const canview_ui_datetime_t *time)
{
    static const uint8_t month_days[] = {31U, 28U, 31U, 30U, 31U, 30U,
                                         31U, 31U, 30U, 31U, 30U, 31U};
    if (time == NULL || time->year < CANVIEW_UI_RTC_YEAR_MIN ||
        time->year > CANVIEW_UI_RTC_YEAR_MAX || time->month < 1U ||
        time->month > 12U || time->hour >= 24U || time->minute >= 60U) {
        return false;
    }
    const bool leap = time->year % 4U == 0U &&
                      (time->year % 100U != 0U || time->year % 400U == 0U);
    const uint8_t days = (uint8_t)(month_days[time->month - 1U] +
                                  (time->month == 2U && leap ? 1U : 0U));
    return time->day > 0U && time->day <= days;
}

static canview_ui_datetime_t model_datetime(void)
{
    const canview_ui_datetime_t time = {
        .year = ui.model.rtc_year, .month = ui.model.rtc_month, .day = ui.model.rtc_day,
        .hour = ui.model.rtc_hour, .minute = ui.model.rtc_minute
    };
    return time;
}

static bool rtc_valid(void)
{
    const canview_ui_datetime_t time = model_datetime();
    return quality_verified(ui.model.rtc_quality) && datetime_valid(&time) &&
           ui.model.rtc_source >= CANVIEW_UI_TIME_RTC &&
           ui.model.rtc_source <= CANVIEW_UI_TIME_MANUAL;
}

static void rtc_draft_event(lv_event_t *event)
{
    (void)event;
    if (ui.root != NULL && settings_editable() &&
        !ui.pending[CANVIEW_UI_CMD_SET_RTC_DATETIME]) {
        ui.rtc_draft = true;
    }
}

static void rtc_apply_event(lv_event_t *event)
{
    (void)event;
    if (ui.root == NULL || !settings_editable()) {
        return;
    }
    canview_ui_command_t command = {.id = CANVIEW_UI_CMD_SET_RTC_DATETIME};
    command.value.datetime.year = (uint16_t)(CANVIEW_UI_RTC_YEAR_MIN +
        lv_dropdown_get_selected(ui.rtc_year_dropdown));
    command.value.datetime.month = (uint8_t)(1U + lv_dropdown_get_selected(ui.rtc_month_dropdown));
    command.value.datetime.day = (uint8_t)(1U + lv_dropdown_get_selected(ui.rtc_day_dropdown));
    command.value.datetime.hour = (uint8_t)lv_dropdown_get_selected(ui.rtc_hour_dropdown);
    command.value.datetime.minute = (uint8_t)lv_dropdown_get_selected(ui.rtc_minute_dropdown);
    if (!datetime_valid(&command.value.datetime)) {
        lv_label_set_text(lv_obj_get_child(ui.rtc_apply_button, 0), "날짜 확인");
        return;
    }
    const canview_ui_datetime_t current = model_datetime();
    if (rtc_valid() && current.year == command.value.datetime.year &&
        current.month == command.value.datetime.month && current.day == command.value.datetime.day &&
        current.hour == command.value.datetime.hour && current.minute == command.value.datetime.minute) {
        ui.rtc_draft = false;
        render_settings();
        return;
    }
    emit_command(&command);
}

static void activity_event(lv_event_t *event)
{
    (void)event;
    if (ui.root != NULL && ui.config.command_cb != NULL) {
        const canview_ui_command_t command = {.id = CANVIEW_UI_CMD_USER_ACTIVITY};
        ui.config.command_cb(&command, ui.config.command_user_data);
    }
}

static void bubble_activity(lv_obj_t *object)
{
    lv_obj_add_flag(object, LV_OBJ_FLAG_EVENT_BUBBLE);
    const uint32_t count = lv_obj_get_child_cnt(object);
    for (uint32_t i = 0U; i < count; ++i) {
        bubble_activity(lv_obj_get_child(object, (int32_t)i));
    }
}

static void bind_button(lv_obj_t *button)
{
    lv_obj_add_event_cb(button, action_event, LV_EVENT_CLICKED, NULL);
}

static lv_obj_t *make_screen(void)
{
    lv_obj_t *screen = lv_obj_create(ui.root);
    lv_obj_remove_style_all(screen);
    lv_obj_set_pos(screen, 0, 36);
    lv_obj_set_size(screen, 320, 368);
    lv_obj_set_style_pad_left(screen, 12, 0);
    lv_obj_set_style_pad_right(screen, 12, 0);
    lv_obj_set_style_pad_top(screen, 8, 0);
    lv_obj_set_style_pad_bottom(screen, 8, 0);
    lv_obj_set_style_pad_row(screen, 8, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    return screen;
}

static void configure_arc(lv_obj_t *arc, int32_t maximum, lv_coord_t width)
{
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, (int16_t)maximum);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc, CANVIEW_COLOR_RULE, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, CANVIEW_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
}

static void arc_animation_exec(void *object, int32_t value)
{
    lv_arc_set_value((lv_obj_t *)object, (int16_t)value);
}

static void opacity_animation_exec(void *object, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)object, (lv_opa_t)value, 0);
}

static void bar_animation_exec(void *object, int32_t value)
{
    lv_bar_set_value((lv_obj_t *)object, value, LV_ANIM_OFF);
}

static void animate_bar_to(lv_obj_t *bar, int32_t value)
{
    lv_anim_t *running = lv_anim_get(bar, bar_animation_exec);
    if ((running != NULL && running->end_value == value) ||
        (running == NULL && lv_bar_get_value(bar) == value)) {
        return;
    }
    lv_anim_del(bar, bar_animation_exec);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, bar);
    lv_anim_set_values(&animation, lv_bar_get_value(bar), value);
    lv_anim_set_time(&animation, 300U);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&animation, bar_animation_exec);
    lv_anim_start(&animation);
}

static void animate_arc_to(lv_obj_t *arc, int32_t value)
{
    lv_anim_t *running = lv_anim_get(arc, arc_animation_exec);
    if ((running != NULL && running->end_value == value) ||
        (running == NULL && lv_arc_get_value(arc) == value)) {
        return;
    }
    lv_anim_del(arc, arc_animation_exec);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, arc);
    lv_anim_set_values(&animation, lv_arc_get_value(arc), value);
    lv_anim_set_time(&animation, 180U);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&animation, arc_animation_exec);
    lv_anim_start(&animation);
}

static void fade_screen_in(lv_obj_t *screen)
{
    lv_anim_del(screen, opacity_animation_exec);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, screen);
    lv_anim_set_values(&animation, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&animation, 160U);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&animation, opacity_animation_exec);
    lv_anim_start(&animation);
}

static void make_gauge(lv_obj_t *parent, const char *unit, int32_t maximum,
                       lv_obj_t **arc_out, lv_obj_t **value_out)
{
    lv_obj_t *group = lv_obj_create(parent);
    lv_obj_remove_style_all(group);
    lv_obj_set_size(group, 64, 56);
    lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    *arc_out = lv_arc_create(group);
    lv_obj_set_size(*arc_out, 56, 56);
    lv_obj_center(*arc_out);
    configure_arc(*arc_out, maximum, 3);
    *value_out = make_label(group, "—");
    lv_obj_align(*value_out, LV_ALIGN_CENTER, 0, -6);
    lv_obj_t *unit_label = make_label(group, unit);
    lv_obj_align(unit_label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(unit_label, &style_label_muted, 0);
}

static void create_header(void)
{
    lv_obj_t *header = lv_obj_create(ui.root);
    lv_obj_remove_style_all(header);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, 320, 36);
    lv_obj_set_style_border_color(header, CANVIEW_COLOR_RULE_2, 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_left(header, 12, 0);
    lv_obj_set_style_pad_right(header, 12, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *brand = make_label(header, "• CV");
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_color(brand, CANVIEW_COLOR_INK_2, 0);
    lv_obj_set_style_text_letter_space(brand, 1, 0);

    ui.header_speed_label = make_label(header, "—");
    lv_obj_set_size(ui.header_speed_label, 60, 32);
    lv_obj_set_pos(ui.header_speed_label, 72, 1);
    lv_obj_set_style_text_align(ui.header_speed_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(ui.header_speed_label, CANVIEW_COLOR_INK, 0);
    lv_obj_set_style_text_font(ui.header_speed_label,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : LV_FONT_DEFAULT,
                               0);
    lv_obj_t *speed_unit = make_label(header, "km/h");
    lv_obj_set_pos(speed_unit, 138, 15);
    lv_obj_add_style(speed_unit, &style_label_micro, 0);

    lv_obj_t *right = lv_obj_create(header);
    lv_obj_remove_style_all(right);
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_height(right, LV_SIZE_CONTENT);
    lv_obj_set_width(right, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(right, 8, 0);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    ui.bus_label = make_label(right, "●●●");
    lv_obj_add_style(ui.bus_label, &style_label_micro, 0);
    lv_obj_set_style_text_color(ui.bus_label, CANVIEW_COLOR_ACCENT, 0);
    ui.link_label = make_label(right, "—");
    lv_obj_set_style_text_color(ui.link_label, CANVIEW_COLOR_INK_2, 0);

    ui.speed_limit_overlay = lv_obj_create(ui.root);
    lv_obj_remove_style_all(ui.speed_limit_overlay);
    lv_obj_set_pos(ui.speed_limit_overlay, 257, 50);
    lv_obj_set_size(ui.speed_limit_overlay, 51, 51);
    lv_obj_set_style_radius(ui.speed_limit_overlay, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ui.speed_limit_overlay, CANVIEW_COLOR_INK, 0);
    lv_obj_set_style_bg_opa(ui.speed_limit_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ui.speed_limit_overlay, CANVIEW_COLOR_ERROR, 0);
    lv_obj_set_style_border_width(ui.speed_limit_overlay, 4, 0);
    lv_obj_clear_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_SCROLLABLE);
    ui.speed_limit_caption = make_label(ui.speed_limit_overlay, "LIMIT");
    lv_obj_align(ui.speed_limit_caption, LV_ALIGN_CENTER, 0, -17);
    lv_obj_add_style(ui.speed_limit_caption, &style_label_micro, 0);
    lv_obj_set_style_text_color(ui.speed_limit_caption, CANVIEW_COLOR_ERROR, 0);
    ui.speed_limit_label = make_label(ui.speed_limit_overlay, "—");
    lv_obj_align(ui.speed_limit_label, LV_ALIGN_CENTER, 0, 4);
    lv_obj_set_style_text_color(ui.speed_limit_label, CANVIEW_COLOR_PAPER, 0);
    lv_obj_add_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_HIDDEN);

    ui.headlamp_warning_overlay = lv_obj_create(ui.root);
    lv_obj_remove_style_all(ui.headlamp_warning_overlay);
    lv_obj_set_pos(ui.headlamp_warning_overlay, 96, 156);
    lv_obj_set_size(ui.headlamp_warning_overlay, 128, 128);
    lv_obj_set_style_radius(ui.headlamp_warning_overlay, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ui.headlamp_warning_overlay, CANVIEW_COLOR_INK, 0);
    lv_obj_set_style_bg_opa(ui.headlamp_warning_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ui.headlamp_warning_overlay, CANVIEW_COLOR_WARNING, 0);
    lv_obj_set_style_border_width(ui.headlamp_warning_overlay, 6, 0);
    lv_obj_clear_flag(ui.headlamp_warning_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui.headlamp_warning_overlay, LV_OBJ_FLAG_SCROLLABLE);
    ui.headlamp_warning_label = make_label(ui.headlamp_warning_overlay, "LIGHTS");
    lv_obj_center(ui.headlamp_warning_label);
    lv_obj_set_style_text_color(ui.headlamp_warning_label, CANVIEW_COLOR_WARNING, 0);
    lv_obj_set_style_text_font(ui.headlamp_warning_label,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : LV_FONT_DEFAULT,
                               0);
    lv_obj_add_flag(ui.headlamp_warning_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void create_drive_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_DRIVE] = screen;

    lv_obj_t *four_wd = make_card(screen, LV_PCT(100), 250, false);
    lv_obj_set_style_pad_all(four_wd, 8, 0);
    lv_obj_set_style_pad_top(four_wd, 0, 0);
    lv_obj_set_style_pad_bottom(four_wd, 0, 0);
    lv_obj_t *four_wd_title = make_label(four_wd, "4WD 지수");
    lv_obj_add_style(four_wd_title, &style_label_micro, 0);
    ui.drive_mode_field = make_card(four_wd, 78, 20, true);
    lv_obj_set_style_pad_all(ui.drive_mode_field, 0, 0);
    lv_obj_align(ui.drive_mode_field, LV_ALIGN_TOP_RIGHT, 0, 0);
    ui.drive_mode_label = make_label(ui.drive_mode_field, "—");
    lv_obj_center(ui.drive_mode_label);

    static const lv_coord_t wheel_x[CANVIEW_UI_WHEEL_COUNT] = {10, 208, 10, 208};
    static const lv_coord_t wheel_y[CANVIEW_UI_WHEEL_COUNT] = {30, 30, 168, 168};
    for (uint8_t i = 0; i < CANVIEW_UI_WHEEL_COUNT; ++i) {
        lv_obj_t *wheel = lv_obj_create(four_wd);
        lv_obj_remove_style_all(wheel);
        lv_obj_set_pos(wheel, wheel_x[i], wheel_y[i]);
        lv_obj_set_size(wheel, 62, 63);
        lv_obj_clear_flag(wheel, LV_OBJ_FLAG_SCROLLABLE);

        ui.wheel_drive_labels[i] = make_label(wheel, "—%");
        lv_obj_align(ui.wheel_drive_labels[i], LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_text_color(ui.wheel_drive_labels[i], CANVIEW_COLOR_ACCENT, 0);

        for (uint8_t segment = 0; segment < CANVIEW_UI_TORQUE_SEGMENTS; ++segment) {
            lv_obj_t *bar_segment = lv_obj_create(wheel);
            lv_obj_remove_style_all(bar_segment);
            lv_obj_set_pos(bar_segment, 7, (lv_coord_t)(20 + segment * 3));
            lv_obj_set_size(bar_segment, 48, 2);
            lv_obj_set_style_bg_color(bar_segment, CANVIEW_COLOR_RULE, 0);
            lv_obj_set_style_bg_opa(bar_segment, LV_OPA_COVER, 0);
            lv_obj_clear_flag(bar_segment, LV_OBJ_FLAG_SCROLLABLE);
            ui.wheel_drive_segments[i][segment] = bar_segment;
        }

        ui.wheel_pressure_labels[i] = make_label(wheel, "—");
        lv_obj_align(ui.wheel_pressure_labels[i], LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_text_color(ui.wheel_pressure_labels[i], CANVIEW_COLOR_INK_2, 0);
    }

    static const lv_coord_t connector_x[CANVIEW_UI_WHEEL_COUNT] = {72, 188, 72, 188};
    static const lv_coord_t connector_y[CANVIEW_UI_WHEEL_COUNT] = {57, 57, 199, 199};
    for (uint8_t i = 0; i < CANVIEW_UI_WHEEL_COUNT; ++i) {
        lv_obj_t *connector = lv_obj_create(four_wd);
        lv_obj_remove_style_all(connector);
        lv_obj_set_pos(connector, connector_x[i], connector_y[i]);
        lv_obj_set_size(connector, 20, 1);
        lv_obj_set_style_bg_color(connector, CANVIEW_COLOR_RULE, 0);
        lv_obj_set_style_bg_opa(connector, LV_OPA_COVER, 0);
    }

    lv_obj_t *vehicle = lv_obj_create(four_wd);
    lv_obj_remove_style_all(vehicle);
    lv_obj_set_pos(vehicle, 84, 1);
    lv_obj_set_size(vehicle, 112, 246);
    lv_obj_clear_flag(vehicle, LV_OBJ_FLAG_SCROLLABLE);

    static lv_point_t vehicle_outline[] = {
        {27, 3},   {85, 3},   {96, 10},  {101, 23}, {105, 48},
        {105, 211}, {104, 221}, {99, 231}, {94, 238}, {83, 244},
        {29, 244}, {18, 238}, {13, 231}, {8, 221},  {7, 211},
        {7, 48},   {11, 23},  {16, 10},  {27, 3}};
    lv_obj_t *outline = lv_line_create(vehicle);
    lv_line_set_points(outline, vehicle_outline,
                       sizeof(vehicle_outline) / sizeof(vehicle_outline[0]));
    lv_obj_set_style_line_color(outline, CANVIEW_COLOR_INK_2, 0);
    lv_obj_set_style_line_width(outline, 2, 0);
    lv_obj_set_style_line_rounded(outline, false, 0);

    static const lv_coord_t car_wheel_x[CANVIEW_UI_WHEEL_COUNT] = {6, 92, 6, 92};
    static const lv_coord_t car_wheel_y[CANVIEW_UI_WHEEL_COUNT] = {36, 36, 180, 180};
    for (uint8_t i = 0; i < CANVIEW_UI_WHEEL_COUNT; ++i) {
        lv_obj_t *car_wheel = lv_obj_create(vehicle);
        lv_obj_remove_style_all(car_wheel);
        lv_obj_set_pos(car_wheel, car_wheel_x[i], car_wheel_y[i]);
        lv_obj_set_size(car_wheel, 14, 32);
        lv_obj_set_style_bg_color(car_wheel, CANVIEW_COLOR_PAPER_3, 0);
        lv_obj_set_style_bg_opa(car_wheel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(car_wheel, CANVIEW_COLOR_INK_2, 0);
        lv_obj_set_style_border_width(car_wheel, 2, 0);
        lv_obj_set_style_radius(car_wheel, 6, 0);
    }

    static const lv_coord_t axle_y[] = {52, 196};
    for (uint8_t i = 0; i < 2U; ++i) {
        lv_obj_t *axle = lv_obj_create(vehicle);
        lv_obj_remove_style_all(axle);
        lv_obj_set_pos(axle, 13, axle_y[i]);
        lv_obj_set_size(axle, 86, 3);
        lv_obj_set_style_bg_color(axle, CANVIEW_COLOR_ACCENT_LINE, 0);
        lv_obj_set_style_bg_opa(axle, LV_OPA_COVER, 0);

        lv_obj_t *differential = lv_obj_create(vehicle);
        lv_obj_remove_style_all(differential);
        lv_obj_set_pos(differential, 43, (lv_coord_t)(axle_y[i] - 10));
        lv_obj_set_size(differential, 26, 20);
        lv_obj_set_style_bg_color(differential, CANVIEW_COLOR_PAPER_3, 0);
        lv_obj_set_style_bg_opa(differential, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(differential, CANVIEW_COLOR_ACCENT_LINE, 0);
        lv_obj_set_style_border_width(differential, 2, 0);
        lv_obj_set_style_radius(differential, 5, 0);
    }

    lv_obj_t *shaft = lv_obj_create(vehicle);
    lv_obj_remove_style_all(shaft);
    lv_obj_set_pos(shaft, 55, 60);
    lv_obj_set_size(shaft, 3, 126);
    lv_obj_set_style_bg_color(shaft, CANVIEW_COLOR_ACCENT_LINE, 0);
    lv_obj_set_style_bg_opa(shaft, LV_OPA_COVER, 0);

    lv_obj_t *coupling = lv_obj_create(vehicle);
    lv_obj_remove_style_all(coupling);
    lv_obj_set_pos(coupling, 42, 99);
    lv_obj_set_size(coupling, 28, 48);
    lv_obj_set_style_bg_color(coupling, CANVIEW_COLOR_ACCENT_WASH, 0);
    lv_obj_set_style_bg_opa(coupling, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(coupling, CANVIEW_COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(coupling, 2, 0);
    lv_obj_set_style_radius(coupling, 4, 0);

    static lv_point_t front_window_points[] = {
        {28, 23}, {42, 13}, {70, 13}, {84, 23}, {90, 56}, {22, 56}, {28, 23}};
    lv_obj_t *front_window = lv_line_create(vehicle);
    lv_line_set_points(front_window, front_window_points,
                       sizeof(front_window_points) / sizeof(front_window_points[0]));
    lv_obj_set_style_line_color(front_window, CANVIEW_COLOR_RULE, 0);
    lv_obj_set_style_line_width(front_window, 1, 0);
    static lv_point_t rear_window_points[] = {
        {22, 172}, {90, 172}, {84, 214}, {28, 214}, {22, 172}};
    lv_obj_t *rear_window = lv_line_create(vehicle);
    lv_line_set_points(rear_window, rear_window_points,
                       sizeof(rear_window_points) / sizeof(rear_window_points[0]));
    lv_obj_set_style_line_color(rear_window, CANVIEW_COLOR_RULE, 0);
    lv_obj_set_style_line_width(rear_window, 1, 0);

    static lv_point_t front_bumper_points[] = {{27, 5}, {85, 5}};
    lv_obj_t *front_bumper = lv_line_create(vehicle);
    lv_line_set_points(front_bumper, front_bumper_points,
                       sizeof(front_bumper_points) / sizeof(front_bumper_points[0]));
    lv_obj_set_style_line_color(front_bumper, CANVIEW_COLOR_INK_2, 0);
    lv_obj_set_style_line_width(front_bumper, 1, 0);

    lv_obj_t *instant = lv_obj_create(vehicle);
    lv_obj_remove_style_all(instant);
    lv_obj_set_pos(instant, 20, 92);
    lv_obj_set_size(instant, 72, 64);
    lv_obj_set_style_bg_color(instant, CANVIEW_COLOR_PAPER, 0);
    lv_obj_set_style_bg_opa(instant, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(instant, CANVIEW_COLOR_ACCENT_LINE, 0);
    lv_obj_set_style_border_width(instant, 1, 0);
    lv_obj_set_style_radius(instant, 4, 0);
    lv_obj_t *instant_title = make_label(instant, "순간");
    lv_obj_align(instant_title, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_add_style(instant_title, &style_label_micro, 0);
    ui.instant_economy_label = make_label(instant, "—");
    lv_obj_align(ui.instant_economy_label, LV_ALIGN_TOP_MID, 0, 23);
    lv_obj_t *economy_unit = make_label(instant, "km/L");
    lv_obj_align(economy_unit, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_add_style(economy_unit, &style_label_muted, 0);
    lv_obj_set_style_text_color(ui.instant_economy_label, CANVIEW_COLOR_INK, 0);

    ui.four_wd_label = make_label(vehicle, "R —%");
    lv_obj_align(ui.four_wd_label, LV_ALIGN_BOTTOM_MID, 0, -3);
    lv_obj_set_style_text_color(ui.four_wd_label, CANVIEW_COLOR_ACCENT, 0);

    lv_obj_t *dpf = make_card(screen, LV_PCT(100), 30, true);
    lv_obj_set_style_pad_all(dpf, 8, 0);
    lv_obj_set_style_pad_top(dpf, 4, 0);
    lv_obj_set_style_pad_bottom(dpf, 4, 0);
    lv_obj_t *dpf_title = make_label(dpf, "DPF");
    lv_obj_align(dpf_title, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_style(dpf_title, &style_label_micro, 0);
    ui.dpf_label = make_label(dpf, "—");
    lv_obj_align(ui.dpf_label, LV_ALIGN_LEFT_MID, 35, 0);
    lv_obj_set_style_text_color(ui.dpf_label, CANVIEW_COLOR_ACCENT, 0);
    ui.dpf_load_bar = lv_bar_create(dpf);
    lv_obj_set_size(ui.dpf_load_bar, 118, 4);
    lv_obj_align(ui.dpf_load_bar, LV_ALIGN_LEFT_MID, 78, 0);
    lv_bar_set_range(ui.dpf_load_bar, 0, 100);
    lv_obj_set_style_bg_color(ui.dpf_load_bar, CANVIEW_COLOR_RULE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui.dpf_load_bar, CANVIEW_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_anim_time(ui.dpf_load_bar, 300U, 0);
    ui.dpf_load_label = make_label(dpf, "—%");
    lv_obj_align(ui.dpf_load_label, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *gauges = lv_obj_create(screen);
    lv_obj_remove_style_all(gauges);
    lv_obj_set_size(gauges, LV_PCT(100), 56);
    lv_obj_set_style_pad_column(gauges, 8, 0);
    lv_obj_set_flex_flow(gauges, LV_FLEX_FLOW_ROW);
    make_gauge(gauges, "km/h", CANVIEW_UI_SPEED_MAX_TENTH_KPH,
               &ui.speed_arc, &ui.speed_label);
    make_gauge(gauges, "RPM", CANVIEW_UI_RPM_MAX, &ui.rpm_arc, &ui.rpm_label);
    lv_obj_t *aux = make_card(gauges, 152, 56, true);
    lv_obj_set_style_pad_all(aux, 3, 0);
    ui.drive_aux_label = make_label(aux, "BATT —\nLOCK —\nTEMP —");
    lv_obj_set_width(ui.drive_aux_label, LV_PCT(100));
    lv_obj_set_style_text_color(ui.drive_aux_label, CANVIEW_COLOR_INK_2, 0);
}

static lv_obj_t *make_profile_button(lv_obj_t *parent, const char *title, action_t action)
{
    lv_obj_t *button = make_button(parent, 144, 76, title, action);
    bind_button(button);
    return button;
}

static lv_obj_t *make_fft_chart(lv_obj_t *card, lv_coord_t height,
                                lv_chart_series_t **series_out)
{
    lv_obj_t *chart = lv_chart_create(card);
    lv_obj_set_size(chart, LV_PCT(100), height);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_chart_set_type(chart, LV_CHART_TYPE_BAR);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(chart, CANVIEW_UI_FFT_BIN_COUNT);
    lv_chart_set_div_line_count(chart, 4, 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_line_color(chart, CANVIEW_COLOR_RULE_2, LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, 0, 0);
    *series_out = lv_chart_add_series(chart, CANVIEW_COLOR_ACCENT_LINE,
                                      LV_CHART_AXIS_PRIMARY_Y);
    return chart;
}

static void make_fft_metric(lv_obj_t *card, const char *caption, lv_align_t align,
                            lv_coord_t x, lv_obj_t **value_out)
{
    lv_obj_t *caption_label = make_label(card, caption);
    lv_obj_align(caption_label, align, x, 0);
    lv_obj_add_style(caption_label, &style_label_micro, 0);
    lv_obj_t *value = make_label(card, "—");
    lv_obj_align(value, align, x, 20);
    lv_obj_set_style_text_color(value, CANVIEW_COLOR_INK, 0);
    *value_out = value;
}

static void create_audio_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_AUDIO] = screen;

    lv_obj_t *quick = lv_obj_create(screen);
    lv_obj_remove_style_all(quick);
    lv_obj_set_size(quick, LV_PCT(100), 76);
    lv_obj_set_style_pad_column(quick, 8, 0);
    lv_obj_set_flex_flow(quick, LV_FLEX_FLOW_ROW);
    ui.quiet_button = make_profile_button(quick, "취침", ACTION_QUIET);
    ui.rear_button = make_profile_button(quick, "뒷좌석 +", ACTION_REAR_BOOST);

    lv_obj_t *volume = make_card(screen, LV_PCT(100), 80, false);
    lv_obj_t *volume_title = make_label(volume, "CURRENT VOLUME");
    lv_obj_add_style(volume_title, &style_label_micro, 0);
    ui.volume_label = make_label(volume, "—");
    lv_obj_align(ui.volume_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_font(ui.volume_label,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : (ui.config.font != NULL ? ui.config.font
                                                                                       : LV_FONT_DEFAULT),
                               0);
    lv_obj_t *adaptive_title = make_label(volume, "소음 보정");
    lv_obj_align(adaptive_title, LV_ALIGN_RIGHT_MID, -36, 0);
    lv_obj_add_style(adaptive_title, &style_label_micro, 0);
    ui.volume_offset_label = make_label(volume, "—");
    lv_obj_align(ui.volume_offset_label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_color(ui.volume_offset_label, CANVIEW_COLOR_ACCENT, 0);

    lv_obj_t *spectrum = make_card(screen, LV_PCT(100), 180, true);
    make_fft_metric(spectrum, "PEAK", LV_ALIGN_TOP_LEFT, 0,
                    &ui.audio_peak_frequency_label);
    make_fft_metric(spectrum, "LEVEL", LV_ALIGN_TOP_RIGHT, 0,
                    &ui.audio_peak_level_label);
    ui.audio_fft_chart = make_fft_chart(spectrum, 96, &ui.audio_fft_series);
}

static void create_fft_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_FFT] = screen;

    lv_obj_t *spectrum = make_card(screen, LV_PCT(100), 352, false);
    make_fft_metric(spectrum, "PEAK", LV_ALIGN_TOP_LEFT, 0,
                    &ui.fft_peak_frequency_label);
    make_fft_metric(spectrum, "LEVEL", LV_ALIGN_TOP_RIGHT, 0,
                    &ui.fft_peak_level_label);
    lv_obj_t *live = make_label(spectrum, "CABIN FFT · —");
    ui.fft_status_label = live;
    lv_obj_align(live, LV_ALIGN_TOP_LEFT, 0, 45);
    lv_obj_add_style(live, &style_label_micro, 0);
    lv_obj_set_style_text_color(live, CANVIEW_COLOR_ACCENT, 0);
    ui.fft_speed_label = make_label(spectrum, "— km/h");
    lv_obj_align(ui.fft_speed_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    ui.fft_rpm_label = make_label(spectrum, "— RPM");
    lv_obj_align(ui.fft_rpm_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    ui.fft_chart = make_fft_chart(spectrum, 212, &ui.fft_series);
    lv_obj_align(ui.fft_chart, LV_ALIGN_BOTTOM_MID, 0, -46);
    lv_obj_t *axis = make_label(spectrum, "50 Hz                         8 kHz");
    lv_obj_align(axis, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_add_style(axis, &style_label_muted, 0);
}

static void create_automation_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_AUTOMATION] = screen;

    lv_obj_t *sport = make_card(screen, LV_PCT(100), 352, false);
    lv_obj_t *eyebrow = make_label(sport, "SPORT AUTOMATION");
    lv_obj_add_style(eyebrow, &style_label_micro, 0);
    ui.sport_mode_field = lv_obj_create(sport);
    lv_obj_remove_style_all(ui.sport_mode_field);
    lv_obj_set_pos(ui.sport_mode_field, 0, 44);
    lv_obj_set_size(ui.sport_mode_field, LV_PCT(100), 190);
    lv_obj_set_style_border_color(ui.sport_mode_field, CANVIEW_COLOR_MODE_SPORT, 0);
    lv_obj_set_style_border_width(ui.sport_mode_field, 4, 0);
    lv_obj_set_style_border_side(ui.sport_mode_field, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_clear_flag(ui.sport_mode_field, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *current_mode = make_label(sport, "현재 모드");
    lv_obj_set_pos(current_mode, 0, 70);
    lv_obj_add_style(current_mode, &style_label_micro, 0);
    ui.sport_mode_label = make_label(sport, "UNKNOWN");
    lv_obj_set_pos(ui.sport_mode_label, 0, 102);
    lv_obj_set_style_text_font(ui.sport_mode_label,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : (ui.config.font != NULL ? ui.config.font
                                                                                       : LV_FONT_DEFAULT),
                               0);
    lv_obj_set_style_text_color(ui.sport_mode_label, CANVIEW_COLOR_MODE_SPORT, 0);
    ui.automation_aux_label = make_label(sport, "BATT —\nLOCK —\nTEMP —");
    lv_obj_set_pos(ui.automation_aux_label, 0, 154);
    lv_obj_set_width(ui.automation_aux_label, LV_PCT(100));
    lv_obj_add_style(ui.automation_aux_label, &style_label_muted, 0);
    lv_obj_t *return_title = make_label(sport, "해제 후");
    lv_obj_set_pos(return_title, 0, 252);
    lv_obj_add_style(return_title, &style_label_micro, 0);
    ui.sport_return_label = make_label(sport, "—");
    lv_obj_align(ui.sport_return_label, LV_ALIGN_TOP_RIGHT, 0, 247);

    ui.sport_button = make_button(sport, LV_PCT(100), 48, "끔", ACTION_SPORT_MONITOR);
    lv_obj_align(ui.sport_button, LV_ALIGN_BOTTOM_MID, 0, 0);
    ui.sport_button_label = lv_obj_get_child(ui.sport_button, 0);
    bind_button(ui.sport_button);
}

static lv_obj_t *make_settings_group(lv_obj_t *parent, lv_coord_t height,
                                     const char *title)
{
    lv_obj_t *group = make_card(parent, LV_PCT(100), height, true);
    lv_obj_set_style_pad_all(group, 8, 0);
    lv_obj_set_style_pad_row(group, 2, 0);
    lv_obj_set_flex_flow(group, LV_FLEX_FLOW_COLUMN);
    lv_obj_t *heading = make_label(group, title);
    lv_obj_add_style(heading, &style_label_micro, 0);
    return group;
}

static lv_obj_t *make_setting_button_row(lv_obj_t *parent, const char *title,
                                         const char *button_text, action_t action,
                                         lv_obj_t **button_out, lv_obj_t **label_out)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), 52);
    lv_obj_set_style_border_color(row, CANVIEW_COLOR_RULE_2, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_TOP, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_label = make_label(row, title);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 2, 0);

    lv_obj_t *button = make_button(row, 104, 44, button_text, action);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_align(button, LV_ALIGN_RIGHT_MID, 0, 0);
    bind_button(button);
    if (button_out != NULL) {
        *button_out = button;
    }
    if (label_out != NULL) {
        *label_out = lv_obj_get_child(button, 0);
    }
    return row;
}

static lv_obj_t *make_setting_dropdown(lv_obj_t *parent, const char *title,
                                        const char *options, uint32_t index)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), 52);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *label = make_label(row, title);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *dropdown = lv_dropdown_create(row);
    style_dropdown(dropdown, 140);
    lv_dropdown_set_options(dropdown, options);
    lv_obj_align(dropdown, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_user_data(dropdown, (void *)(uintptr_t)index);
    lv_obj_add_event_cb(dropdown, setting_dropdown_event, LV_EVENT_VALUE_CHANGED, NULL);
    return dropdown;
}

static lv_obj_t *make_number_dropdown(lv_obj_t *parent, uint16_t first, uint16_t last,
                                      lv_coord_t x, lv_coord_t y, lv_coord_t width)
{
    /* 최대 100개 연도 + 미확정 항목. 옵션 문자열은 LVGL이 복사한다. */
    char options[520];
    size_t used = 0U;
    for (uint16_t value = first; value <= last; ++value) {
        const int32_t count = snprintf(options + used, sizeof(options) - used,
                                       "%02u\n", value);
        if (count < 0 || (size_t)count >= sizeof(options) - used) {
            return NULL;
        }
        used += (size_t)count;
    }
    const int32_t count = snprintf(options + used, sizeof(options) - used, "—");
    if (count < 0 || (size_t)count >= sizeof(options) - used) {
        return NULL;
    }
    lv_obj_t *dropdown = lv_dropdown_create(parent);
    style_dropdown(dropdown, width);
    lv_dropdown_set_options(dropdown, options);
    lv_obj_set_pos(dropdown, x, y);
    lv_obj_add_event_cb(dropdown, rtc_draft_event, LV_EVENT_VALUE_CHANGED, NULL);
    return dropdown;
}

static void create_settings_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_SETTINGS] = screen;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(screen, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_AUTO);
    ui.settings_lock_label = make_label(screen, "정차 확인 후 설정");
    lv_obj_set_width(ui.settings_lock_label, LV_PCT(100));
    lv_obj_add_style(ui.settings_lock_label, &style_label_muted, 0);

    lv_obj_t *time = make_card(screen, LV_PCT(100), 250, true);
    lv_obj_t *title = make_label(time, "TIME · UTC+09:00");
    lv_obj_add_style(title, &style_label_micro, 0);
    ui.rtc_time_label = make_label(time, "시간 원천 — · invalid");
    lv_obj_set_pos(ui.rtc_time_label, 0, 25);
    lv_obj_set_width(ui.rtc_time_label, LV_PCT(100));
    ui.rtc_year_dropdown = make_number_dropdown(time, 2000U, 2099U, 0, 78, 96);
    ui.rtc_month_dropdown = make_number_dropdown(time, 1U, 12U, 100, 78, 80);
    ui.rtc_day_dropdown = make_number_dropdown(time, 1U, 31U, 184, 78, 84);
    ui.rtc_hour_dropdown = make_number_dropdown(time, 0U, 23U, 0, 130, 80);
    ui.rtc_minute_dropdown = make_number_dropdown(time, 0U, 59U, 88, 130, 80);
    ui.rtc_apply_button = make_button(time, 92, 44, "적용 요청", ACTION_AUTO_BRIGHTNESS);
    lv_obj_set_pos(ui.rtc_apply_button, 176, 130);
    lv_obj_add_event_cb(ui.rtc_apply_button, rtc_apply_event, LV_EVENT_CLICKED, NULL);
    lv_obj_t *hint = make_label(time, "년 / 월 / 일 · 시 / 분\n수신 확인 뒤 현재 시각 갱신");
    lv_obj_set_pos(hint, 0, 183);
    lv_obj_add_style(hint, &style_label_muted, 0);

    lv_obj_t *brightness = make_card(screen, LV_PCT(100), 106, false);
    lv_obj_t *display = make_label(brightness, "화면 밝기");
    lv_obj_add_style(display, &style_label_muted, 0);
    ui.brightness_label = make_label(brightness, "—%");
    lv_obj_align(ui.brightness_label, LV_ALIGN_TOP_RIGHT, 0, 0);
    ui.brightness_slider = lv_slider_create(brightness);
    lv_obj_set_size(ui.brightness_slider, 236, 12);
    lv_obj_align(ui.brightness_slider, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_obj_set_ext_click_area(ui.brightness_slider, 16);
    lv_obj_set_style_opa(ui.brightness_slider, LV_OPA_50, LV_STATE_DISABLED);
    lv_slider_set_range(ui.brightness_slider, 10, 100);
    lv_obj_set_style_bg_color(ui.brightness_slider, CANVIEW_COLOR_RULE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui.brightness_slider, CANVIEW_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_add_event_cb(ui.brightness_slider, brightness_event, LV_EVENT_RELEASED, NULL);

    lv_obj_t *automatic = make_settings_group(screen, 90, "DISPLAY");
    make_setting_button_row(automatic, "CAN 자동 밝기", "—", ACTION_AUTO_BRIGHTNESS,
                            &ui.auto_brightness_button, &ui.auto_brightness_button_label);
    lv_obj_t *idle = make_card(screen, LV_PCT(100), 76, true);
    lv_obj_t *idle_title = make_label(idle, "대기 후 복귀");
    lv_obj_align(idle_title, LV_ALIGN_LEFT_MID, 0, 0);
    ui.idle_timeout_dropdown = lv_dropdown_create(idle);
    lv_dropdown_set_options(ui.idle_timeout_dropdown, "15초\n30초\n60초\n120초\n—");
    style_dropdown(ui.idle_timeout_dropdown, 104);
    lv_obj_align(ui.idle_timeout_dropdown, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(ui.idle_timeout_dropdown, idle_timeout_event, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *noise = make_settings_group(screen, 306, "ROAD NOISE");
    make_setting_button_row(noise, "소음 보정", "—", ACTION_SETTINGS_ADAPTIVE,
                            &ui.settings_adaptive_button, &ui.settings_adaptive_button_label);
    ui.setting_dropdowns[0] = make_setting_dropdown(noise, "대역",
        "노면 125–500\n표준 160–1250\n풍절 500–2000\n—", 0U);
    ui.setting_dropdowns[1] = make_setting_dropdown(noise, "민감도", "낮음\n보통\n높음\n—", 1U);
    ui.setting_dropdowns[2] = make_setting_dropdown(noise, "반응", "느리게\n자연스럽게\n빠르게\n—", 2U);
    ui.setting_dropdowns[3] = make_setting_dropdown(noise, "최대 보정", "+2\n+3\n+4\n—", 3U);

    lv_obj_t *sport = make_settings_group(screen, 198, "SPORT AUTO");
    make_setting_button_row(sport, "자동 전환", "—", ACTION_SPORT_MONITOR,
                            &ui.settings_sport_button, &ui.settings_sport_button_label);
    ui.setting_dropdowns[4] = make_setting_dropdown(sport, "진입 속도",
        "60 km/h\n70 km/h\n80 km/h\n—", 4U);
    make_setting_button_row(sport, "급가속 감지", "—", ACTION_SPORT_ACCELERATION,
                            &ui.sport_acceleration_button, &ui.sport_acceleration_button_label);
}

static void create_nav(void)
{
    static const char *labels[CANVIEW_UI_SCREEN_COUNT] = {
        "주행", "소리", "FFT", "자동", "설정"};
    static const action_t actions[CANVIEW_UI_SCREEN_COUNT] = {
        ACTION_NAV_DRIVE, ACTION_NAV_AUDIO, ACTION_NAV_FFT,
        ACTION_NAV_AUTOMATION, ACTION_NAV_SETTINGS};

    lv_obj_t *nav = lv_obj_create(ui.root);
    lv_obj_remove_style_all(nav);
    lv_obj_add_style(nav, &style_nav, LV_PART_MAIN);
    lv_obj_set_pos(nav, 0, 404);
    lv_obj_set_size(nav, 320, CANVIEW_TOUCH_PRIMARY);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);

    for (uint32_t i = 0U; i < CANVIEW_UI_SCREEN_COUNT; ++i) {
        lv_obj_t *button = make_button(nav, 64, CANVIEW_TOUCH_PRIMARY, labels[i], actions[i]);
        lv_obj_set_style_radius(button, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
        lv_obj_clear_flag(button, LV_OBJ_FLAG_CHECKABLE);
        bind_button(button);
        ui.nav_buttons[i] = button;
    }
}

static void set_checked(lv_obj_t *object, bool checked)
{
    if (checked) {
        lv_obj_add_state(object, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(object, LV_STATE_CHECKED);
    }
}

static const char *drive_mode_text(canview_ui_drive_mode_t mode)
{
    switch (mode) {
    case CANVIEW_UI_DRIVE_NORMAL:
        return "NORMAL";
    case CANVIEW_UI_DRIVE_ECO:
        return "ECO";
    case CANVIEW_UI_DRIVE_COMFORT:
        return "COMFORT";
    case CANVIEW_UI_DRIVE_SMART:
        return "SMART";
    case CANVIEW_UI_DRIVE_SPORT:
        return "SPORT";
    case CANVIEW_UI_DRIVE_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

static lv_color_t drive_mode_color(canview_ui_drive_mode_t mode)
{
    switch (mode) {
    case CANVIEW_UI_DRIVE_SPORT:
        return CANVIEW_COLOR_MODE_SPORT;
    case CANVIEW_UI_DRIVE_ECO:
        return CANVIEW_COLOR_MODE_ECO;
    case CANVIEW_UI_DRIVE_NORMAL:
        return CANVIEW_COLOR_ACCENT;
    default:
        return CANVIEW_COLOR_INK_2;
    }
}

static bool speed_valid(void)
{
    return quality_verified(ui.model.speed_quality) &&
           ui.model.speed_tenth_kph <= CANVIEW_UI_SPEED_MAX_TENTH_KPH;
}

static void render_overlays(void)
{
    const bool drive = ui.screen == CANVIEW_UI_SCREEN_DRIVE;
    const bool limit = ui.model.speed_limit_active &&
                       quality_verified(ui.model.speed_limit_quality) &&
                       ui.model.speed_limit_kph > 0U && ui.model.speed_limit_kph <= 240U;
    const bool warning = limit && speed_valid() && ui.model.speed_limit_warning_active;
    const bool lights = ui.model.headlamp_warning_active && !warning;
    const bool prominent = drive && warning;
    lv_obj_set_pos(ui.speed_limit_overlay, prominent ? 96 : 252, prominent ? 156 : 50);
    lv_obj_set_size(ui.speed_limit_overlay, prominent ? 128 : 56, prominent ? 128 : 56);
    lv_obj_set_style_border_width(ui.speed_limit_overlay, prominent ? 6 : 4, 0);
    /* 작은 표지에는 숫자만 두어 LIMIT/3자리 수치가 겹치지 않는다. */
    if (prominent) {
        lv_obj_clear_flag(ui.speed_limit_caption, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui.speed_limit_caption, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_align(ui.speed_limit_caption, LV_ALIGN_CENTER, 0, -40);
    lv_obj_align(ui.speed_limit_label, LV_ALIGN_CENTER, 0, prominent ? 8 : 0);
    lv_obj_set_style_text_font(ui.speed_limit_label,
        prominent && ui.config.metric_font != NULL ? ui.config.metric_font :
        ui.config.font != NULL ? ui.config.font : LV_FONT_DEFAULT, 0);
    lv_label_set_text_fmt(ui.speed_limit_label, "%u", ui.model.speed_limit_kph);
    if (limit) {
        lv_obj_clear_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    const lv_opa_t opacity = drive ? LV_OPA_COVER : LV_OPA_60;
    lv_obj_set_style_opa(ui.speed_limit_overlay,
        warning && !ui.model.speed_limit_warning_visible ? LV_OPA_30 : opacity, 0);
    lv_obj_set_pos(ui.headlamp_warning_overlay, drive ? 96 : 212, drive ? 156 : 112);
    lv_obj_set_size(ui.headlamp_warning_overlay, drive ? 128 : 96, drive ? 128 : 48);
    lv_obj_set_style_border_width(ui.headlamp_warning_overlay, drive ? 6 : 2, 0);
    lv_obj_set_style_text_font(ui.headlamp_warning_label,
        drive && ui.config.metric_font != NULL ? ui.config.metric_font :
        ui.config.font != NULL ? ui.config.font : LV_FONT_DEFAULT, 0);
    lv_obj_set_style_opa(ui.headlamp_warning_overlay, opacity, 0);
    if (lights) {
        lv_obj_clear_flag(ui.headlamp_warning_overlay, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui.headlamp_warning_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_text_color(ui.header_speed_label,
        warning || lights ? CANVIEW_COLOR_WARNING : CANVIEW_COLOR_INK, 0);
    lv_obj_move_foreground(ui.speed_limit_overlay);
    lv_obj_move_foreground(ui.headlamp_warning_overlay);
}

static void set_disabled(lv_obj_t *object, bool disabled)
{
    if (disabled) {
        if (lv_obj_check_type(object, &lv_dropdown_class)) {
            lv_dropdown_close(object);
        }
        lv_obj_add_state(object, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(object, LV_STATE_DISABLED);
    }
}

static void render_toggle(lv_obj_t *button, lv_obj_t *label, bool value,
                          canview_ui_command_id_t id)
{
    set_checked(button, ui.model.settings_valid && value);
    lv_label_set_text(label, ui.pending[id] ? "요청 중" :
        !ui.model.settings_valid ? "—" : value ? "사용" : "끔");
    set_disabled(button, !settings_editable() || ui.pending[id] || ui.config.command_cb == NULL);
}

static void render_settings(void)
{
    const bool editable = settings_editable() && ui.config.command_cb != NULL;
    lv_label_set_text(ui.settings_lock_label, editable ? "설정 · 확인된 값" : "정차 확인 후 설정");
    render_toggle(ui.auto_brightness_button, ui.auto_brightness_button_label,
                  ui.model.auto_brightness_enabled, CANVIEW_UI_CMD_SET_AUTO_BRIGHTNESS);
    render_toggle(ui.settings_adaptive_button, ui.settings_adaptive_button_label,
                  ui.model.adaptive_volume_enabled, CANVIEW_UI_CMD_SET_ADAPTIVE_VOLUME);
    render_toggle(ui.settings_sport_button, ui.settings_sport_button_label,
                  ui.model.sport_monitor_enabled, CANVIEW_UI_CMD_SET_SPORT_MONITOR);
    render_toggle(ui.sport_button, ui.sport_button_label,
                  ui.model.sport_monitor_enabled, CANVIEW_UI_CMD_SET_SPORT_MONITOR);
    render_toggle(ui.sport_acceleration_button, ui.sport_acceleration_button_label,
                  ui.model.sport_acceleration_enabled, CANVIEW_UI_CMD_SET_SPORT_ACCELERATION);
    const bool brightness_valid = ui.model.settings_valid &&
        ui.model.display_brightness_percent >= 10U && ui.model.display_brightness_percent <= 100U;
    if (brightness_valid) {
        lv_label_set_text_fmt(ui.brightness_label, "%u%%", ui.model.display_brightness_percent);
    } else {
        lv_label_set_text(ui.brightness_label, "—%");
    }
    if (!lv_obj_has_state(ui.brightness_slider, LV_STATE_PRESSED) || !editable ||
        ui.pending[CANVIEW_UI_CMD_SET_BRIGHTNESS]) {
        lv_slider_set_value(ui.brightness_slider,
            brightness_valid ? ui.model.display_brightness_percent : 10, LV_ANIM_OFF);
    }
    set_disabled(ui.brightness_slider, !editable || ui.pending[CANVIEW_UI_CMD_SET_BRIGHTNESS]);
    for (uint32_t i = 0U; i < CANVIEW_UI_SETTING_DROPDOWNS; ++i) {
        uint16_t selected = 3U;
        const uint32_t value = setting_model_value(i);
        if (ui.model.settings_valid) {
            if (i < 3U && value < 3U) {
                selected = (uint16_t)value;
            } else if (i == 3U && value >= 2U && value <= 4U) {
                selected = (uint16_t)(value - 2U);
            } else if (i == 4U && (value == 60U || value == 70U || value == 80U)) {
                selected = (uint16_t)((value - 60U) / 10U);
            }
        }
        if (!lv_dropdown_is_open(ui.setting_dropdowns[i]) || !editable) {
            lv_dropdown_set_selected(ui.setting_dropdowns[i], selected);
        }
        set_disabled(ui.setting_dropdowns[i], !editable || ui.pending[setting_commands[i]]);
    }
    uint16_t idle_selected = 4U;
    for (uint16_t i = 0U; i < 4U; ++i) {
        if (ui.model.settings_valid && ui.model.idle_timeout_seconds == idle_timeouts[i]) {
            idle_selected = i;
        }
    }
    if (!lv_dropdown_is_open(ui.idle_timeout_dropdown) || !editable) {
        lv_dropdown_set_selected(ui.idle_timeout_dropdown, idle_selected);
    }
    set_disabled(ui.idle_timeout_dropdown, !editable || ui.pending[CANVIEW_UI_CMD_SET_IDLE_TIMEOUT]);

    if (rtc_valid()) {
        static const char *sources[] = {"—", "RTC", "GNSS", "수동"};
        lv_label_set_text_fmt(ui.rtc_time_label, "%04u-%02u-%02u %02u:%02u\n시간 원천 %s",
            ui.model.rtc_year, ui.model.rtc_month, ui.model.rtc_day,
            ui.model.rtc_hour, ui.model.rtc_minute, sources[ui.model.rtc_source]);
    } else {
        lv_label_set_text(ui.rtc_time_label, "---- -- -- --:--\n시간 원천 — · invalid");
    }
    lv_obj_t *dates[] = {ui.rtc_year_dropdown, ui.rtc_month_dropdown, ui.rtc_day_dropdown,
                         ui.rtc_hour_dropdown, ui.rtc_minute_dropdown};
    const uint16_t values[] = {
        (uint16_t)(ui.model.rtc_year - CANVIEW_UI_RTC_YEAR_MIN),
        (uint16_t)(ui.model.rtc_month - 1U), (uint16_t)(ui.model.rtc_day - 1U),
        ui.model.rtc_hour, ui.model.rtc_minute
    };
    const uint16_t unknown[] = {100U, 12U, 31U, 24U, 60U};
    if (!editable && !ui.pending[CANVIEW_UI_CMD_SET_RTC_DATETIME]) {
        ui.rtc_draft = false;
    }
    for (uint32_t i = 0U; i < 5U; ++i) {
        if (!ui.rtc_draft) {
            lv_dropdown_set_selected(dates[i], rtc_valid() ? values[i] : unknown[i]);
        }
        set_disabled(dates[i], !editable || ui.pending[CANVIEW_UI_CMD_SET_RTC_DATETIME]);
    }
    set_disabled(ui.rtc_apply_button, !editable || ui.pending[CANVIEW_UI_CMD_SET_RTC_DATETIME]);
    lv_label_set_text(lv_obj_get_child(ui.rtc_apply_button, 0),
        ui.pending[CANVIEW_UI_CMD_SET_RTC_DATETIME] ? "요청 중" : "적용 요청");
    const bool audio_valid = quality_verified(ui.model.audio_profile_quality);
    set_checked(ui.quiet_button, audio_valid && ui.model.quiet_mode_enabled);
    set_checked(ui.rear_button, audio_valid && ui.model.rear_boost_enabled);
    set_disabled(ui.quiet_button, !audio_valid || !ui.model.audio_control_allowed ||
        ui.pending[CANVIEW_UI_CMD_SET_QUIET] || ui.config.command_cb == NULL);
    set_disabled(ui.rear_button, !audio_valid || !ui.model.audio_control_allowed ||
        ui.pending[CANVIEW_UI_CMD_SET_REAR_BOOST] || ui.config.command_cb == NULL);
    lv_label_set_text(lv_obj_get_child(ui.quiet_button, 0),
        ui.pending[CANVIEW_UI_CMD_SET_QUIET] ? "취침 · 요청 중" : "취침");
    lv_label_set_text(lv_obj_get_child(ui.rear_button, 0),
        ui.pending[CANVIEW_UI_CMD_SET_REAR_BOOST] ? "뒷좌석 · 요청 중" : "뒷좌석 +");
}

static void root_delete_event(lv_event_t *event)
{
    if (lv_event_get_target(event) == ui.root) {
        memset(&ui, 0, sizeof(ui));
    }
}

void canview_ui_destroy(void)
{
    if (ui.root != NULL) {
        lv_obj_del(ui.root);
    }
}

void canview_ui_command_complete(canview_ui_command_id_t id)
{
    if (ui.root == NULL || (uint32_t)id >= CANVIEW_UI_CMD_COUNT || id == 0) {
        return;
    }
    ui.pending[id] = false;
    if (id == CANVIEW_UI_CMD_SET_RTC_DATETIME) {
        ui.rtc_draft = false;
    }
    render_settings();
}

lv_obj_t *canview_ui_create(lv_obj_t *parent, const canview_ui_config_t *config)
{
    if (ui.root != NULL) {
        return ui.root;
    }
    if (parent == NULL && lv_disp_get_default() == NULL) {
        return NULL;
    }
    memset(&ui, 0, sizeof(ui));
    if (config != NULL) {
        ui.config = *config;
    }
    ui.model.display_brightness_percent = 42;
    ui.model.adaptive_noise_band = CANVIEW_UI_NOISE_BAND_BALANCED;
    ui.model.adaptive_sensitivity = CANVIEW_UI_SENSITIVITY_NORMAL;
    ui.model.adaptive_response = CANVIEW_UI_RESPONSE_NORMAL;
    ui.model.adaptive_max_offset_steps = 4U;
    ui.model.sport_entry_speed_kph = 70U;
    ui.model.sport_acceleration_enabled = true;
    ui.model.idle_timeout_seconds = 30U;
    ui.model.rtc_hour = 12U;
    ui.model.rtc_minute = 0U;
    ui.model.rtc_quality = CANVIEW_UI_QUALITY_UNAVAILABLE;
    ui.screen = CANVIEW_UI_SCREEN_COUNT;
    if (!styles_ready) {
        init_styles(ui.config.font);
    }
    lv_style_set_text_font(&style_root, ui.config.font != NULL ? ui.config.font : LV_FONT_DEFAULT);

    ui.root = lv_obj_create(parent != NULL ? parent : lv_scr_act());
    lv_obj_remove_style_all(ui.root);
    lv_obj_add_style(ui.root, &style_root, LV_PART_MAIN);
    lv_obj_set_style_text_font(ui.root, ui.config.font != NULL ? ui.config.font : LV_FONT_DEFAULT, 0);
    lv_obj_add_event_cb(ui.root, root_delete_event, LV_EVENT_DELETE, NULL);
    lv_obj_add_event_cb(ui.root, activity_event, LV_EVENT_PRESSED, NULL);
    lv_obj_set_size(ui.root, 320, 480);
    lv_obj_set_pos(ui.root, 0, 0);
    lv_obj_clear_flag(ui.root, LV_OBJ_FLAG_SCROLLABLE);

    create_header();
    create_drive_screen();
    create_audio_screen();
    create_fft_screen();
    create_automation_screen();
    create_settings_screen();
    create_nav();
    bubble_activity(ui.root);
    lv_obj_move_foreground(ui.speed_limit_overlay);
    lv_obj_move_foreground(ui.headlamp_warning_overlay);
    canview_ui_show_screen(CANVIEW_UI_SCREEN_DRIVE);
    canview_ui_update(&ui.model);
    return ui.root;
}

void canview_ui_show_screen(canview_ui_screen_t screen)
{
    if (ui.root == NULL || (uint32_t)screen >= CANVIEW_UI_SCREEN_COUNT) {
        return;
    }
    if (ui.screen == CANVIEW_UI_SCREEN_SETTINGS && screen != ui.screen) {
        for (uint32_t i = 0U; i < CANVIEW_UI_SETTING_DROPDOWNS; ++i) {
            lv_dropdown_close(ui.setting_dropdowns[i]);
        }
        lv_obj_t *dates[] = {ui.rtc_year_dropdown, ui.rtc_month_dropdown, ui.rtc_day_dropdown,
                             ui.rtc_hour_dropdown, ui.rtc_minute_dropdown, ui.idle_timeout_dropdown};
        for (uint32_t i = 0U; i < sizeof(dates) / sizeof(dates[0]); ++i) {
            lv_dropdown_close(dates[i]);
        }
        if (!ui.pending[CANVIEW_UI_CMD_SET_RTC_DATETIME]) {
            ui.rtc_draft = false;
        }
        render_settings();
    }
    ui.screen = screen;
    for (uint32_t i = 0U; i < CANVIEW_UI_SCREEN_COUNT; ++i) {
        if (i == (uint32_t)screen) {
            const bool hidden = lv_obj_has_flag(ui.screens[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui.screens[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui.nav_buttons[i], LV_STATE_CHECKED);
            if (hidden) {
                fade_screen_in(ui.screens[i]);
            }
        } else {
            lv_anim_del(ui.screens[i], opacity_animation_exec);
            lv_obj_set_style_opa(ui.screens[i], LV_OPA_COVER, 0);
            lv_obj_add_flag(ui.screens[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui.nav_buttons[i], LV_STATE_CHECKED);
        }
    }
    render_overlays();
}

canview_ui_screen_t canview_ui_current_screen(void)
{
    return ui.root == NULL ? CANVIEW_UI_SCREEN_DRIVE : ui.screen;
}

static void update_arc(lv_obj_t *arc, int32_t value, bool valid)
{
    if (valid) {
        animate_arc_to(arc, value);
    } else {
        lv_anim_del(arc, arc_animation_exec);
        lv_arc_set_value(arc, 0);
    }
    lv_obj_set_style_arc_opa(arc, valid ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_INDICATOR);
}

static void render_drive(void)
{
    const canview_ui_model_t *model = &ui.model;
    const bool speed = speed_valid();
    const bool rpm = quality_verified(model->engine_rpm_quality) &&
                     model->engine_rpm <= CANVIEW_UI_RPM_MAX;
    if (speed) {
        lv_label_set_text_fmt(ui.header_speed_label, "%u", model->speed_tenth_kph / 10U);
        lv_label_set_text_fmt(ui.speed_label, "%u", model->speed_tenth_kph / 10U);
        lv_label_set_text_fmt(ui.fft_speed_label, "%u km/h", model->speed_tenth_kph / 10U);
    } else {
        lv_label_set_text(ui.header_speed_label, "—");
        lv_label_set_text(ui.speed_label, "—");
        lv_label_set_text(ui.fft_speed_label, "— km/h");
    }
    if (rpm) {
        lv_label_set_text_fmt(ui.rpm_label, "%u", model->engine_rpm);
        lv_label_set_text_fmt(ui.fft_rpm_label, "%u RPM", model->engine_rpm);
    } else {
        lv_label_set_text(ui.rpm_label, "—");
        lv_label_set_text(ui.fft_rpm_label, "— RPM");
    }
    update_arc(ui.speed_arc, model->speed_tenth_kph, speed);
    update_arc(ui.rpm_arc, model->engine_rpm, rpm);

    char battery[24] = "BATT —";
    char lock[24] = "LOCK —";
    char temperature[24] = "TEMP —";
    if (quality_verified(model->battery_voltage_quality) && model->battery_voltage_tenth_v <= 320U) {
        (void)snprintf(battery, sizeof(battery), "BATT %u.%u V",
            model->battery_voltage_tenth_v / 10U, model->battery_voltage_tenth_v % 10U);
    }
    if (quality_verified(model->transmission_clutch_lock_quality)) {
        (void)snprintf(lock, sizeof(lock), "LOCK %s", model->transmission_clutch_locked ? "ON" : "OFF");
    }
    if (quality_verified(model->engine_temperature_quality) &&
        model->engine_temperature_c >= -40 && model->engine_temperature_c <= 215) {
        (void)snprintf(temperature, sizeof(temperature), "TEMP %d °C", model->engine_temperature_c);
    }
    lv_label_set_text_fmt(ui.drive_aux_label, "%s\n%s\n%s", battery, lock, temperature);
    lv_label_set_text(ui.automation_aux_label, lv_label_get_text(ui.drive_aux_label));
    if (quality_verified(model->fuel_economy_quality) && model->instant_fuel_economy_tenth_kmpl <= 999U) {
        lv_label_set_text_fmt(ui.instant_economy_label, "%u.%u",
            model->instant_fuel_economy_tenth_kmpl / 10U, model->instant_fuel_economy_tenth_kmpl % 10U);
    } else {
        lv_label_set_text(ui.instant_economy_label, "—");
    }
    if (quality_verified(model->four_wd_quality) && model->rear_coupling_percent <= 100U) {
        lv_label_set_text_fmt(ui.four_wd_label, "R %u%%", model->rear_coupling_percent);
    } else {
        lv_label_set_text(ui.four_wd_label, "R —%");
    }
    for (uint32_t i = 0U; i < CANVIEW_UI_WHEEL_COUNT; ++i) {
        const uint8_t bit = (uint8_t)(1U << i);
        const bool drive = quality_verified(model->four_wd_quality) &&
            (model->wheel_drive_unavailable_mask & bit) == 0U && model->wheel_drive_percent[i] <= 100U;
        const uint8_t count = drive ?
            (uint8_t)(((uint16_t)model->wheel_drive_percent[i] * CANVIEW_UI_TORQUE_SEGMENTS + 99U) / 100U) : 0U;
        for (uint32_t segment = 0U; segment < CANVIEW_UI_TORQUE_SEGMENTS; ++segment) {
            lv_obj_set_style_bg_color(ui.wheel_drive_segments[i][segment],
                segment >= CANVIEW_UI_TORQUE_SEGMENTS - count ? CANVIEW_COLOR_ACCENT : CANVIEW_COLOR_RULE, 0);
        }
        if (drive) {
            lv_label_set_text_fmt(ui.wheel_drive_labels[i], "%u%%", model->wheel_drive_percent[i]);
        } else {
            lv_label_set_text(ui.wheel_drive_labels[i], "—%");
        }
        const bool pressure = quality_verified(model->tire_pressure_quality) &&
            (model->tire_pressure_unavailable_mask & bit) == 0U &&
            model->tire_pressure_tenth_psi[i] <= CANVIEW_UI_PRESSURE_MAX_TENTH_PSI;
        if (pressure) {
            lv_label_set_text_fmt(ui.wheel_pressure_labels[i], "%u.%u psi",
                model->tire_pressure_tenth_psi[i] / 10U, model->tire_pressure_tenth_psi[i] % 10U);
        } else {
            lv_label_set_text(ui.wheel_pressure_labels[i], "—");
        }
        const bool warning = pressure && (model->tire_pressure_warning_mask & bit) != 0U;
        lv_obj_set_style_text_color(ui.wheel_pressure_labels[i],
            warning ? CANVIEW_COLOR_WARNING : CANVIEW_COLOR_INK_2, 0);
        lv_obj_t *wheel = lv_obj_get_parent(ui.wheel_pressure_labels[i]);
        lv_obj_set_style_outline_width(wheel, warning ? 1 : 0, 0);
        lv_obj_set_style_outline_color(wheel, CANVIEW_COLOR_WARNING, 0);
    }
    const bool lamp = quality_verified(model->dpf_lamp_quality);
    lv_label_set_text(ui.dpf_label, !lamp ? "—" : model->dpf_lamp_on ? "확인" : "");
    lv_obj_set_style_text_color(ui.dpf_label, !lamp ? CANVIEW_COLOR_MUTED :
        model->dpf_lamp_on ? CANVIEW_COLOR_WARNING : CANVIEW_COLOR_ACCENT, 0);
    const bool load = quality_verified(model->dpf_load_quality) && model->dpf_load_percent <= 100U;
    if (load) {
        lv_label_set_text_fmt(ui.dpf_load_label, "%u%%", model->dpf_load_percent);
        animate_bar_to(ui.dpf_load_bar, model->dpf_load_percent);
    } else {
        lv_label_set_text(ui.dpf_load_label, "—%");
        lv_anim_del(ui.dpf_load_bar, bar_animation_exec);
        lv_bar_set_value(ui.dpf_load_bar, 0, LV_ANIM_OFF);
    }
    lv_obj_set_style_bg_opa(ui.dpf_load_bar, load ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui.dpf_load_bar,
        lamp && model->dpf_lamp_on ? CANVIEW_COLOR_WARNING : CANVIEW_COLOR_ACCENT, LV_PART_INDICATOR);

    const canview_ui_drive_mode_t mode = quality_verified(model->drive_mode_quality) &&
        model->drive_mode >= CANVIEW_UI_DRIVE_NORMAL && model->drive_mode <= CANVIEW_UI_DRIVE_SPORT ?
        model->drive_mode : CANVIEW_UI_DRIVE_UNKNOWN;
    const canview_ui_drive_mode_t previous = quality_verified(model->sport_previous_mode_quality) &&
        model->sport_previous_mode >= CANVIEW_UI_DRIVE_NORMAL &&
        model->sport_previous_mode < CANVIEW_UI_DRIVE_SPORT ?
        model->sport_previous_mode : CANVIEW_UI_DRIVE_UNKNOWN;
    lv_label_set_text(ui.drive_mode_label, mode == CANVIEW_UI_DRIVE_UNKNOWN ? "—" : drive_mode_text(mode));
    lv_label_set_text(ui.sport_mode_label, mode == CANVIEW_UI_DRIVE_UNKNOWN ? "—" : drive_mode_text(mode));
    lv_obj_set_style_text_color(ui.drive_mode_label, drive_mode_color(mode), 0);
    lv_obj_set_style_border_color(ui.drive_mode_field, drive_mode_color(mode), 0);
    lv_obj_set_style_text_color(ui.sport_mode_label, drive_mode_color(mode), 0);
    lv_obj_set_style_border_color(ui.sport_mode_field, drive_mode_color(mode), 0);
    lv_label_set_text(ui.sport_return_label,
        previous == CANVIEW_UI_DRIVE_UNKNOWN ? "—" : drive_mode_text(previous));
    lv_obj_set_style_text_color(ui.sport_return_label, drive_mode_color(previous), 0);
}

static void render_audio(void)
{
    const canview_ui_model_t *model = &ui.model;
    if (quality_verified(model->audio_volume_quality) && model->audio_volume_level <= 100U) {
        lv_label_set_text_fmt(ui.volume_label, "%u", model->audio_volume_level);
    } else {
        lv_label_set_text(ui.volume_label, "—");
    }
    if (quality_verified(model->volume_offset_quality) &&
        model->volume_offset_step >= 0 && model->volume_offset_step <= 4) {
        lv_label_set_text_fmt(ui.volume_offset_label, "%+d", model->volume_offset_step);
    } else {
        lv_label_set_text(ui.volume_offset_label, "—");
    }
    bool fft = quality_verified(model->fft_quality);
    for (uint32_t i = 0U; i < CANVIEW_UI_FFT_BIN_COUNT; ++i) {
        fft = fft && model->fft_bins[i] <= 100U;
    }
    for (uint16_t i = 0U; i < CANVIEW_UI_FFT_BIN_COUNT; ++i) {
        const lv_coord_t value = fft ? model->fft_bins[i] : LV_CHART_POINT_NONE;
        lv_chart_set_value_by_id(ui.audio_fft_chart, ui.audio_fft_series, i, value);
        lv_chart_set_value_by_id(ui.fft_chart, ui.fft_series, i, value);
    }
    lv_chart_refresh(ui.audio_fft_chart);
    lv_chart_refresh(ui.fft_chart);
    lv_label_set_text(ui.fft_status_label, fft ? "CABIN FFT" : "CABIN FFT · —");
    lv_obj_set_style_text_color(ui.fft_status_label, fft ? CANVIEW_COLOR_ACCENT : CANVIEW_COLOR_MUTED, 0);
    if (fft && model->fft_peak_hz >= CANVIEW_UI_FFT_MIN_HZ &&
        model->fft_peak_hz < CANVIEW_UI_FFT_MAX_HZ) {
        lv_label_set_text_fmt(ui.audio_peak_frequency_label, "%u Hz", model->fft_peak_hz);
        lv_label_set_text_fmt(ui.fft_peak_frequency_label, "%u Hz", model->fft_peak_hz);
    } else {
        lv_label_set_text(ui.audio_peak_frequency_label, "— Hz");
        lv_label_set_text(ui.fft_peak_frequency_label, "— Hz");
    }
    if (fft && model->fft_peak_tenth_db >= CANVIEW_UI_DBFS_MIN_TENTH &&
        model->fft_peak_tenth_db <= CANVIEW_UI_DBFS_MAX_TENTH) {
        const int32_t value = model->fft_peak_tenth_db;
        const uint32_t magnitude = (uint32_t)(value < 0 ? -value : value);
        lv_label_set_text_fmt(ui.audio_peak_level_label, "%s%u.%u dBFS", value < 0 ? "-" : "",
            magnitude / 10U, magnitude % 10U);
        lv_label_set_text(ui.fft_peak_level_label, lv_label_get_text(ui.audio_peak_level_label));
    } else {
        lv_label_set_text(ui.audio_peak_level_label, "— dBFS");
        lv_label_set_text(ui.fft_peak_level_label, "— dBFS");
    }
}

void canview_ui_update(const canview_ui_model_t *model)
{
    if (ui.root == NULL || model == NULL) {
        return;
    }
    const bool idle_started = model->idle_dimmed && !ui.model.idle_dimmed;
    ui.model = *model;
    if (idle_started) {
        canview_ui_show_screen(CANVIEW_UI_SCREEN_DRIVE);
    }
    static const char *bus_states[] = {"○○○", "●○○", "●●○", "●●●"};
    lv_label_set_text(ui.bus_label, ui.model.active_bus_count <= 3U ?
        bus_states[ui.model.active_bus_count] : "—");
    if (ui.model.esp_now_rssi_dbm < 0) {
        lv_label_set_text_fmt(ui.link_label, "%d", ui.model.esp_now_rssi_dbm);
    } else {
        lv_label_set_text(ui.link_label, "—");
    }
    render_drive();
    render_audio();
    render_settings();
    render_overlays();
}
