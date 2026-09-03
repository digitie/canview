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
    ACTION_VOLUME_DOWN,
    ACTION_VOLUME_UP,
    ACTION_ADAPTIVE_VOLUME,
    ACTION_SPORT_MONITOR,
    ACTION_AUTO_BRIGHTNESS,
    ACTION_SETTINGS_ADAPTIVE,
    ACTION_NOISE_BAND,
    ACTION_NOISE_SENSITIVITY,
    ACTION_NOISE_RESPONSE,
    ACTION_NOISE_MAX_OFFSET,
    ACTION_SPORT_ENTRY_SPEED,
    ACTION_SPORT_ACCELERATION,
    ACTION_UNITS_METRIC,
    ACTION_UNITS_IMPERIAL,
} action_t;

typedef struct {
    lv_obj_t *root;
    lv_obj_t *screens[CANVIEW_UI_SCREEN_COUNT];
    lv_obj_t *nav_buttons[CANVIEW_UI_SCREEN_COUNT];
    canview_ui_screen_t screen;
    canview_ui_config_t config;
    canview_ui_model_t model;

    lv_obj_t *bus_label;
    lv_obj_t *link_label;
    lv_obj_t *speed_arc;
    lv_obj_t *speed_label;
    lv_obj_t *speed_unit_label;
    lv_obj_t *rpm_arc;
    lv_obj_t *rpm_label;
    lv_obj_t *dpf_label;
    lv_obj_t *drive_mode_label;
    lv_obj_t *four_wd_label;
    lv_obj_t *wheel_drive_bars[CANVIEW_UI_WHEEL_COUNT];
    lv_obj_t *wheel_pressure_labels[CANVIEW_UI_WHEEL_COUNT];
    lv_obj_t *wheel_objects[CANVIEW_UI_WHEEL_COUNT];

    lv_obj_t *quiet_button;
    lv_obj_t *rear_button;
    lv_obj_t *volume_label;
    lv_obj_t *adaptive_button;
    lv_obj_t *adaptive_button_label;
    lv_obj_t *volume_offset_label;

    lv_obj_t *fft_speed_arc;
    lv_obj_t *fft_speed_label;
    lv_obj_t *fft_speed_unit_label;
    lv_obj_t *fft_rpm_arc;
    lv_obj_t *fft_rpm_label;
    lv_obj_t *fft_chart;
    lv_chart_series_t *fft_series;
    lv_obj_t *fft_peak_frequency_label;
    lv_obj_t *fft_peak_level_label;

    lv_obj_t *sport_button;
    lv_obj_t *sport_button_label;
    lv_obj_t *sport_dial;
    lv_obj_t *sport_mode_label;
    lv_obj_t *sport_return_label;

    lv_obj_t *brightness_slider;
    lv_obj_t *brightness_label;
    lv_obj_t *auto_brightness_button;
    lv_obj_t *auto_brightness_button_label;
    lv_obj_t *settings_adaptive_button;
    lv_obj_t *settings_adaptive_button_label;
    lv_obj_t *noise_band_button_label;
    lv_obj_t *noise_sensitivity_button_label;
    lv_obj_t *noise_response_button_label;
    lv_obj_t *noise_max_offset_button_label;
    lv_obj_t *settings_sport_button;
    lv_obj_t *settings_sport_button_label;
    lv_obj_t *sport_entry_speed_button_label;
    lv_obj_t *sport_acceleration_button;
    lv_obj_t *sport_acceleration_button_label;
    lv_obj_t *metric_button;
    lv_obj_t *imperial_button;
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
    lv_obj_add_style(button, &style_button, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(button, &style_button_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(button, &style_button_pressed, LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_size(button, width, height);
    lv_obj_add_flag(button, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_user_data(button, (void *)(uintptr_t)action);
    lv_obj_t *label = make_label(button, text);
    lv_obj_center(label);
    return button;
}

static void emit_enabled(canview_ui_command_id_t id, bool enabled)
{
    if (ui.config.command_cb == NULL) {
        return;
    }
    canview_ui_command_t command = {.id = id};
    command.value.enabled = enabled;
    ui.config.command_cb(&command, ui.config.command_user_data);
}

static void emit_step(canview_ui_command_id_t id, int8_t step_delta)
{
    if (ui.config.command_cb == NULL) {
        return;
    }
    canview_ui_command_t command = {.id = id};
    command.value.step_delta = step_delta;
    ui.config.command_cb(&command, ui.config.command_user_data);
}

static void emit_percent(canview_ui_command_id_t id, uint8_t percent)
{
    if (ui.config.command_cb == NULL) {
        return;
    }
    canview_ui_command_t command = {.id = id};
    command.value.percent = percent;
    ui.config.command_cb(&command, ui.config.command_user_data);
}

static void emit_option(canview_ui_command_id_t id, uint16_t option)
{
    if (ui.config.command_cb == NULL) {
        return;
    }
    canview_ui_command_t command = {.id = id};
    command.value.option = option;
    ui.config.command_cb(&command, ui.config.command_user_data);
}

static void action_event(lv_event_t *event)
{
    lv_obj_t *target = lv_event_get_target(event);
    action_t action = (action_t)(uintptr_t)lv_obj_get_user_data(target);

    switch (action) {
    case ACTION_NAV_DRIVE:
        canview_ui_show_screen(CANVIEW_UI_SCREEN_DRIVE);
        break;
    case ACTION_NAV_AUDIO:
        canview_ui_show_screen(CANVIEW_UI_SCREEN_AUDIO);
        break;
    case ACTION_NAV_FFT:
        canview_ui_show_screen(CANVIEW_UI_SCREEN_FFT);
        break;
    case ACTION_NAV_AUTOMATION:
        canview_ui_show_screen(CANVIEW_UI_SCREEN_AUTOMATION);
        break;
    case ACTION_NAV_SETTINGS:
        canview_ui_show_screen(CANVIEW_UI_SCREEN_SETTINGS);
        break;
    case ACTION_QUIET:
        emit_enabled(CANVIEW_UI_CMD_SET_QUIET, !ui.model.quiet_mode_enabled);
        break;
    case ACTION_REAR_BOOST:
        emit_enabled(CANVIEW_UI_CMD_SET_REAR_BOOST, !ui.model.rear_boost_enabled);
        break;
    case ACTION_VOLUME_DOWN:
        emit_step(CANVIEW_UI_CMD_STEP_VOLUME, -1);
        break;
    case ACTION_VOLUME_UP:
        emit_step(CANVIEW_UI_CMD_STEP_VOLUME, 1);
        break;
    case ACTION_ADAPTIVE_VOLUME:
        emit_enabled(CANVIEW_UI_CMD_SET_ADAPTIVE_VOLUME, !ui.model.adaptive_volume_enabled);
        break;
    case ACTION_SPORT_MONITOR:
        emit_enabled(CANVIEW_UI_CMD_SET_SPORT_MONITOR, !ui.model.sport_monitor_enabled);
        break;
    case ACTION_AUTO_BRIGHTNESS:
        emit_enabled(CANVIEW_UI_CMD_SET_AUTO_BRIGHTNESS, !ui.model.auto_brightness_enabled);
        break;
    case ACTION_SETTINGS_ADAPTIVE:
        emit_enabled(CANVIEW_UI_CMD_SET_ADAPTIVE_VOLUME, !ui.model.adaptive_volume_enabled);
        break;
    case ACTION_NOISE_BAND:
        emit_option(CANVIEW_UI_CMD_SET_ADAPTIVE_NOISE_BAND,
                    (uint16_t)((ui.model.adaptive_noise_band + 1U) %
                               CANVIEW_UI_NOISE_BAND_COUNT));
        break;
    case ACTION_NOISE_SENSITIVITY:
        emit_option(CANVIEW_UI_CMD_SET_ADAPTIVE_SENSITIVITY,
                    (uint16_t)((ui.model.adaptive_sensitivity + 1U) %
                               CANVIEW_UI_SENSITIVITY_COUNT));
        break;
    case ACTION_NOISE_RESPONSE:
        emit_option(CANVIEW_UI_CMD_SET_ADAPTIVE_RESPONSE,
                    (uint16_t)((ui.model.adaptive_response + 1U) %
                               CANVIEW_UI_RESPONSE_COUNT));
        break;
    case ACTION_NOISE_MAX_OFFSET:
        emit_option(CANVIEW_UI_CMD_SET_ADAPTIVE_MAX_OFFSET,
                    ui.model.adaptive_max_offset_steps < 2U
                        ? 2U
                        : ui.model.adaptive_max_offset_steps >= 4U
                        ? 2U
                        : (uint16_t)(ui.model.adaptive_max_offset_steps + 1U));
        break;
    case ACTION_SPORT_ENTRY_SPEED:
        emit_option(CANVIEW_UI_CMD_SET_SPORT_ENTRY_SPEED,
                    ui.model.sport_entry_speed_kph < 60U
                        ? 60U
                        : ui.model.sport_entry_speed_kph >= 80U
                        ? 60U
                        : (uint16_t)(ui.model.sport_entry_speed_kph + 10U));
        break;
    case ACTION_SPORT_ACCELERATION:
        emit_enabled(CANVIEW_UI_CMD_SET_SPORT_ACCELERATION,
                     !ui.model.sport_acceleration_enabled);
        break;
    case ACTION_UNITS_METRIC:
        emit_enabled(CANVIEW_UI_CMD_SET_METRIC_UNITS, true);
        break;
    case ACTION_UNITS_IMPERIAL:
        emit_enabled(CANVIEW_UI_CMD_SET_METRIC_UNITS, false);
        break;
    default:
        break;
    }
}

static void brightness_event(lv_event_t *event)
{
    lv_obj_t *slider = lv_event_get_target(event);
    emit_percent(CANVIEW_UI_CMD_SET_BRIGHTNESS, (uint8_t)lv_slider_get_value(slider));
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
    return screen;
}

static void configure_arc(lv_obj_t *arc, int32_t maximum, lv_coord_t width)
{
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, maximum);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc, CANVIEW_COLOR_RULE, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, CANVIEW_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
}

static void make_gauge(lv_obj_t *parent, lv_coord_t width, lv_coord_t height,
                       lv_coord_t diameter, const char *caption, const char *unit,
                       int32_t maximum, lv_obj_t **arc_out, lv_obj_t **value_out,
                       lv_obj_t **unit_out)
{
    lv_obj_t *group = lv_obj_create(parent);
    lv_obj_remove_style_all(group);
    lv_obj_set_size(group, width, height);
    lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *arc = lv_arc_create(group);
    lv_obj_set_size(arc, diameter, diameter);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 4);
    configure_arc(arc, maximum, diameter >= 120 ? 5 : 4);

    if (caption != NULL && caption[0] != '\0') {
        lv_obj_t *caption_label = make_label(group, caption);
        lv_obj_align(caption_label, LV_ALIGN_CENTER, 0, -25);
        lv_obj_add_style(caption_label, &style_label_micro, 0);
    }

    lv_obj_t *value = make_label(group, "—");
    lv_obj_align(value, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(value, CANVIEW_COLOR_INK, 0);
    lv_obj_set_style_text_font(value,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : (ui.config.font != NULL ? ui.config.font
                                                                                       : LV_FONT_DEFAULT),
                               0);
    lv_obj_t *unit_label = make_label(group, unit);
    lv_obj_align(unit_label, LV_ALIGN_CENTER, 0, 24);
    lv_obj_add_style(unit_label, &style_label_micro, 0);

    *arc_out = arc;
    *value_out = value;
    if (unit_out != NULL) {
        *unit_out = unit_label;
    }
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
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *brand = make_label(header, "• CANVIEW");
    lv_obj_set_style_text_color(brand, CANVIEW_COLOR_INK_2, 0);
    lv_obj_set_style_text_letter_space(brand, 1, 0);

    lv_obj_t *right = lv_obj_create(header);
    lv_obj_remove_style_all(right);
    lv_obj_set_height(right, LV_SIZE_CONTENT);
    lv_obj_set_width(right, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_column(right, 8, 0);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    ui.bus_label = make_label(right, "BUS 0/3");
    lv_obj_add_style(ui.bus_label, &style_label_micro, 0);
    ui.link_label = make_label(right, "연결 대기");
    lv_obj_set_style_text_color(ui.link_label, CANVIEW_COLOR_INK_2, 0);
}

static void create_drive_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_DRIVE] = screen;

    lv_obj_t *gauges = make_card(screen, LV_PCT(100), 150, false);
    lv_obj_set_style_pad_all(gauges, 0, 0);
    lv_obj_set_flex_flow(gauges, LV_FLEX_FLOW_ROW);
    make_gauge(gauges, 147, 150, 116, "SPEED", "km/h", 2400,
               &ui.speed_arc, &ui.speed_label, &ui.speed_unit_label);
    make_gauge(gauges, 147, 150, 116, "RPM", "×1000", 6500,
               &ui.rpm_arc, &ui.rpm_label, NULL);

    lv_obj_t *status = lv_obj_create(screen);
    lv_obj_remove_style_all(status);
    lv_obj_set_size(status, LV_PCT(100), 48);
    lv_obj_set_style_pad_column(status, 8, 0);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);

    lv_obj_t *dpf = make_card(status, 142, 48, true);
    lv_obj_set_style_pad_all(dpf, 8, 0);
    lv_obj_t *dpf_title = make_label(dpf, "DPF");
    lv_obj_align(dpf_title, LV_ALIGN_LEFT_MID, 0, 0);
    ui.dpf_label = make_label(dpf, "정보 없음");
    lv_obj_align(ui.dpf_label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_color(ui.dpf_label, CANVIEW_COLOR_ACCENT, 0);

    lv_obj_t *mode = make_card(status, 142, 48, true);
    lv_obj_set_style_pad_all(mode, 8, 0);
    lv_obj_t *mode_title = make_label(mode, "DRIVE MODE");
    lv_obj_add_style(mode_title, &style_label_micro, 0);
    ui.drive_mode_label = make_label(mode, "UNKNOWN");
    lv_obj_align(ui.drive_mode_label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_color(ui.drive_mode_label, CANVIEW_COLOR_INK, 0);

    lv_obj_t *four_wd = make_card(screen, LV_PCT(100), 138, false);
    lv_obj_set_style_pad_all(four_wd, 8, 0);
    lv_obj_t *four_wd_title = make_label(four_wd, "4WD · PSI");
    lv_obj_add_style(four_wd_title, &style_label_micro, 0);

    static const lv_coord_t wheel_x[CANVIEW_UI_WHEEL_COUNT] = {0, 202, 0, 202};
    static const lv_coord_t wheel_y[CANVIEW_UI_WHEEL_COUNT] = {26, 26, 76, 76};
    for (uint8_t i = 0; i < CANVIEW_UI_WHEEL_COUNT; ++i) {
        lv_obj_t *wheel = lv_obj_create(four_wd);
        lv_obj_remove_style_all(wheel);
        lv_obj_set_pos(wheel, wheel_x[i], wheel_y[i]);
        lv_obj_set_size(wheel, 70, 40);
        lv_obj_set_style_bg_color(wheel, CANVIEW_COLOR_PAPER_3, 0);
        lv_obj_set_style_bg_opa(wheel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(wheel, CANVIEW_COLOR_RULE, 0);
        lv_obj_set_style_border_width(wheel, 1, 0);
        lv_obj_set_style_radius(wheel, CANVIEW_RADIUS_SM, 0);
        lv_obj_clear_flag(wheel, LV_OBJ_FLAG_SCROLLABLE);
        ui.wheel_objects[i] = wheel;

        lv_obj_t *bar = lv_bar_create(wheel);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, 5, 28);
        lv_obj_align(bar, i == CANVIEW_UI_WHEEL_FRONT_RIGHT ||
                              i == CANVIEW_UI_WHEEL_REAR_RIGHT
                          ? LV_ALIGN_RIGHT_MID
                          : LV_ALIGN_LEFT_MID,
                     i == CANVIEW_UI_WHEEL_FRONT_RIGHT ||
                              i == CANVIEW_UI_WHEEL_REAR_RIGHT
                          ? -4
                          : 4,
                     0);
        lv_bar_set_range(bar, 0, 100);
        lv_obj_set_style_bg_color(bar, CANVIEW_COLOR_RULE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, CANVIEW_COLOR_ACCENT, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
        ui.wheel_drive_bars[i] = bar;

        ui.wheel_pressure_labels[i] = make_label(wheel, "—");
        lv_obj_center(ui.wheel_pressure_labels[i]);
        lv_obj_set_style_text_font(ui.wheel_pressure_labels[i],
                                   ui.config.metric_font != NULL ? ui.config.metric_font
                                                                 : LV_FONT_DEFAULT,
                                   0);
    }

    lv_obj_t *vehicle = lv_obj_create(four_wd);
    lv_obj_remove_style_all(vehicle);
    lv_obj_set_pos(vehicle, 107, 25);
    lv_obj_set_size(vehicle, 58, 92);
    lv_obj_set_style_bg_color(vehicle, CANVIEW_COLOR_PAPER_3, 0);
    lv_obj_set_style_bg_opa(vehicle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(vehicle, CANVIEW_COLOR_RULE, 0);
    lv_obj_set_style_border_width(vehicle, 1, 0);
    lv_obj_set_style_radius(vehicle, 20, 0);
    lv_obj_clear_flag(vehicle, LV_OBJ_FLAG_SCROLLABLE);
    ui.four_wd_label = make_label(vehicle, "—");
    lv_obj_center(ui.four_wd_label);
    lv_obj_set_style_text_align(ui.four_wd_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui.four_wd_label, CANVIEW_COLOR_ACCENT, 0);
}

static lv_obj_t *make_profile_button(lv_obj_t *parent, const char *title, action_t action)
{
    lv_obj_t *button = make_button(parent, 142, 82, title, action);
    bind_button(button);
    return button;
}

static void create_audio_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_AUDIO] = screen;

    lv_obj_t *quick = lv_obj_create(screen);
    lv_obj_remove_style_all(quick);
    lv_obj_set_size(quick, LV_PCT(100), 82);
    lv_obj_set_style_pad_column(quick, 8, 0);
    lv_obj_set_flex_flow(quick, LV_FLEX_FLOW_ROW);
    ui.quiet_button = make_profile_button(quick, "취침", ACTION_QUIET);
    ui.rear_button = make_profile_button(quick, "뒷좌석 +", ACTION_REAR_BOOST);

    lv_obj_t *volume = make_card(screen, LV_PCT(100), 178, false);
    lv_obj_t *volume_title = make_label(volume, "VOLUME");
    lv_obj_add_style(volume_title, &style_label_micro, 0);
    ui.volume_label = make_label(volume, "—");
    lv_obj_align(ui.volume_label, LV_ALIGN_LEFT_MID, 0, 8);
    lv_obj_set_style_text_font(ui.volume_label,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : (ui.config.font != NULL ? ui.config.font
                                                                                       : LV_FONT_DEFAULT),
                               0);
    lv_obj_t *down = make_button(volume, 60, 60, LV_SYMBOL_MINUS, ACTION_VOLUME_DOWN);
    lv_obj_align(down, LV_ALIGN_RIGHT_MID, -68, 8);
    lv_obj_clear_flag(down, LV_OBJ_FLAG_CHECKABLE);
    bind_button(down);
    lv_obj_t *up = make_button(volume, 60, 60, LV_SYMBOL_PLUS, ACTION_VOLUME_UP);
    lv_obj_align(up, LV_ALIGN_RIGHT_MID, 0, 8);
    lv_obj_clear_flag(up, LV_OBJ_FLAG_CHECKABLE);
    bind_button(up);

    lv_obj_t *adaptive = make_card(screen, LV_PCT(100), 76, true);
    lv_obj_t *adaptive_title = make_label(adaptive, "주행 소음 보정");
    lv_obj_align(adaptive_title, LV_ALIGN_LEFT_MID, 0, 0);
    ui.volume_offset_label = make_label(adaptive, "—");
    lv_obj_align(ui.volume_offset_label, LV_ALIGN_LEFT_MID, 112, 0);
    lv_obj_set_style_text_color(ui.volume_offset_label, CANVIEW_COLOR_ACCENT, 0);
    ui.adaptive_button = make_button(adaptive, 76, 48, "끔", ACTION_ADAPTIVE_VOLUME);
    lv_obj_align(ui.adaptive_button, LV_ALIGN_RIGHT_MID, 0, 0);
    ui.adaptive_button_label = lv_obj_get_child(ui.adaptive_button, 0);
    bind_button(ui.adaptive_button);
}

static void create_fft_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_FFT] = screen;

    lv_obj_t *gauges = make_card(screen, LV_PCT(100), 132, false);
    lv_obj_set_style_pad_all(gauges, 0, 0);
    lv_obj_set_flex_flow(gauges, LV_FLEX_FLOW_ROW);
    make_gauge(gauges, 147, 132, 104, "", "km/h", 2400,
               &ui.fft_speed_arc, &ui.fft_speed_label, &ui.fft_speed_unit_label);
    make_gauge(gauges, 147, 132, 104, "", "×1000 rpm", 6500,
               &ui.fft_rpm_arc, &ui.fft_rpm_label, NULL);

    lv_obj_t *spectrum = make_card(screen, LV_PCT(100), 148, true);
    lv_obj_t *spectrum_title = make_label(spectrum, "CABIN FFT");
    lv_obj_add_style(spectrum_title, &style_label_micro, 0);
    lv_obj_t *live = make_label(spectrum, "LIVE");
    lv_obj_align(live, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_text_color(live, CANVIEW_COLOR_ACCENT, 0);

    ui.fft_chart = lv_chart_create(spectrum);
    lv_obj_set_size(ui.fft_chart, LV_PCT(100), 94);
    lv_obj_align(ui.fft_chart, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_chart_set_type(ui.fft_chart, LV_CHART_TYPE_BAR);
    lv_chart_set_range(ui.fft_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(ui.fft_chart, CANVIEW_UI_FFT_BIN_COUNT);
    lv_chart_set_div_line_count(ui.fft_chart, 3, 0);
    lv_obj_set_style_bg_opa(ui.fft_chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui.fft_chart, 0, 0);
    lv_obj_set_style_line_color(ui.fft_chart, CANVIEW_COLOR_RULE_2, LV_PART_MAIN);
    lv_obj_set_style_line_width(ui.fft_chart, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui.fft_chart, 0, 0);
    ui.fft_series = lv_chart_add_series(ui.fft_chart, CANVIEW_COLOR_ACCENT_LINE,
                                        LV_CHART_AXIS_PRIMARY_Y);

    lv_obj_t *peak = make_card(screen, LV_PCT(100), 56, true);
    lv_obj_set_style_pad_top(peak, 8, 0);
    lv_obj_set_style_pad_bottom(peak, 8, 0);
    lv_obj_t *peak_title = make_label(peak, "PEAK");
    lv_obj_add_style(peak_title, &style_label_micro, 0);
    ui.fft_peak_frequency_label = make_label(peak, "—");
    lv_obj_align(ui.fft_peak_frequency_label, LV_ALIGN_TOP_LEFT, 72, 0);
    lv_obj_t *level_title = make_label(peak, "LEVEL");
    lv_obj_align(level_title, LV_ALIGN_TOP_LEFT, 146, 0);
    lv_obj_add_style(level_title, &style_label_micro, 0);
    ui.fft_peak_level_label = make_label(peak, "—");
    lv_obj_align(ui.fft_peak_level_label, LV_ALIGN_TOP_RIGHT, 0, 0);
}

static void create_automation_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_AUTOMATION] = screen;

    lv_obj_t *sport = make_card(screen, LV_PCT(100), 352, false);
    lv_obj_t *eyebrow = make_label(sport, "SPORT AUTOMATION");
    lv_obj_add_style(eyebrow, &style_label_micro, 0);
    lv_obj_t *title = make_label(sport, "SPORT 자동");
    lv_obj_set_pos(title, 0, 24);
    lv_obj_set_style_text_color(title, CANVIEW_COLOR_INK, 0);

    ui.sport_dial = lv_arc_create(sport);
    lv_obj_set_size(ui.sport_dial, 190, 190);
    lv_obj_align(ui.sport_dial, LV_ALIGN_CENTER, 0, 8);
    configure_arc(ui.sport_dial, 2, 7);
    lv_obj_t *current_mode = make_label(sport, "현재 모드");
    lv_obj_align(current_mode, LV_ALIGN_CENTER, 0, -4);
    lv_obj_add_style(current_mode, &style_label_micro, 0);
    ui.sport_mode_label = make_label(sport, "UNKNOWN");
    lv_obj_align(ui.sport_mode_label, LV_ALIGN_CENTER, 0, 21);
    lv_obj_set_style_text_font(ui.sport_mode_label,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : (ui.config.font != NULL ? ui.config.font
                                                                                       : LV_FONT_DEFAULT),
                               0);
    ui.sport_return_label = make_label(sport, "복귀 —");
    lv_obj_align(ui.sport_return_label, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_style(ui.sport_return_label, &style_label_micro, 0);

    ui.sport_button = make_button(sport, 120, 48, "끔", ACTION_SPORT_MONITOR);
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

static void create_settings_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_SETTINGS] = screen;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(screen, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t *brightness = make_card(screen, LV_PCT(100), 132, false);
    lv_obj_t *display = make_label(brightness, "DISPLAY");
    lv_obj_add_style(display, &style_label_micro, 0);
    lv_obj_t *brightness_title = make_label(brightness, "화면 밝기");
    lv_obj_set_pos(brightness_title, 0, 22);
    ui.brightness_label = make_label(brightness, "—%");
    lv_obj_align(ui.brightness_label, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_text_color(ui.brightness_label, CANVIEW_COLOR_ACCENT, 0);

    ui.brightness_slider = lv_slider_create(brightness);
    lv_obj_set_size(ui.brightness_slider, LV_PCT(100), 48);
    lv_obj_align(ui.brightness_slider, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_slider_set_range(ui.brightness_slider, 10, 100);
    lv_obj_set_style_bg_color(ui.brightness_slider, CANVIEW_COLOR_RULE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui.brightness_slider, CANVIEW_COLOR_ACCENT,
                              LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui.brightness_slider, CANVIEW_COLOR_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(ui.brightness_slider, brightness_event, LV_EVENT_RELEASED, NULL);

    lv_obj_t *automatic = make_card(screen, LV_PCT(100), 96, true);
    lv_obj_t *sensor = make_label(automatic, "CAN");
    lv_obj_add_style(sensor, &style_label_micro, 0);
    lv_obj_t *automatic_title = make_label(automatic, "자동 밝기");
    lv_obj_align(automatic_title, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    ui.auto_brightness_button = make_button(automatic, 76, 48, "끔",
                                             ACTION_AUTO_BRIGHTNESS);
    lv_obj_align(ui.auto_brightness_button, LV_ALIGN_RIGHT_MID, 0, 0);
    ui.auto_brightness_button_label = lv_obj_get_child(ui.auto_brightness_button, 0);
    bind_button(ui.auto_brightness_button);

    lv_obj_t *noise = make_settings_group(screen, 302, "ROAD NOISE");
    make_setting_button_row(noise, "소음 보정", "끔", ACTION_SETTINGS_ADAPTIVE,
                            &ui.settings_adaptive_button,
                            &ui.settings_adaptive_button_label);
    make_setting_button_row(noise, "주파수 대역", "표준", ACTION_NOISE_BAND,
                            NULL, &ui.noise_band_button_label);
    make_setting_button_row(noise, "민감도", "보통", ACTION_NOISE_SENSITIVITY,
                            NULL, &ui.noise_sensitivity_button_label);
    make_setting_button_row(noise, "반응", "부드럽게", ACTION_NOISE_RESPONSE,
                            NULL, &ui.noise_response_button_label);
    make_setting_button_row(noise, "최대 보정", "+4", ACTION_NOISE_MAX_OFFSET,
                            NULL, &ui.noise_max_offset_button_label);

    lv_obj_t *sport = make_settings_group(screen, 194, "SPORT AUTO");
    make_setting_button_row(sport, "자동 전환", "끔", ACTION_SPORT_MONITOR,
                            &ui.settings_sport_button,
                            &ui.settings_sport_button_label);
    make_setting_button_row(sport, "진입 속도", "70 km/h", ACTION_SPORT_ENTRY_SPEED,
                            NULL, &ui.sport_entry_speed_button_label);
    make_setting_button_row(sport, "급가속 감지", "사용", ACTION_SPORT_ACCELERATION,
                            &ui.sport_acceleration_button,
                            &ui.sport_acceleration_button_label);

    lv_obj_t *units = make_card(screen, LV_PCT(100), 76, true);
    lv_obj_t *units_title = make_label(units, "UNITS");
    lv_obj_add_style(units_title, &style_label_micro, 0);
    lv_obj_t *speed_units = make_label(units, "속도 단위");
    lv_obj_align(speed_units, LV_ALIGN_LEFT_MID, 0, 10);
    ui.metric_button = make_button(units, 66, 44, "km/h", ACTION_UNITS_METRIC);
    lv_obj_align(ui.metric_button, LV_ALIGN_RIGHT_MID, -70, 0);
    bind_button(ui.metric_button);
    ui.imperial_button = make_button(units, 66, 44, "mph", ACTION_UNITS_IMPERIAL);
    lv_obj_align(ui.imperial_button, LV_ALIGN_RIGHT_MID, 0, 0);
    bind_button(ui.imperial_button);
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

    for (int i = 0; i < CANVIEW_UI_SCREEN_COUNT; ++i) {
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

static const char *noise_band_text(canview_ui_noise_band_t band)
{
    static const char *labels[CANVIEW_UI_NOISE_BAND_COUNT] = {
        "노면", "표준", "풍절"};
    return band < CANVIEW_UI_NOISE_BAND_COUNT ? labels[band] : labels[0];
}

static const char *sensitivity_text(canview_ui_sensitivity_t sensitivity)
{
    static const char *labels[CANVIEW_UI_SENSITIVITY_COUNT] = {
        "낮음", "보통", "높음"};
    return sensitivity < CANVIEW_UI_SENSITIVITY_COUNT ? labels[sensitivity] : labels[0];
}

static const char *response_text(canview_ui_response_t response)
{
    static const char *labels[CANVIEW_UI_RESPONSE_COUNT] = {
        "느리게", "자연스럽게", "빠르게"};
    return response < CANVIEW_UI_RESPONSE_COUNT ? labels[response] : labels[0];
}

lv_obj_t *canview_ui_create(lv_obj_t *parent, const canview_ui_config_t *config)
{
    memset(&ui, 0, sizeof(ui));
    if (config != NULL) {
        ui.config = *config;
    }
    ui.model.metric_units = true;
    ui.model.audio_volume_level = 18;
    ui.model.display_brightness_percent = 42;
    ui.model.adaptive_noise_band = CANVIEW_UI_NOISE_BAND_BALANCED;
    ui.model.adaptive_sensitivity = CANVIEW_UI_SENSITIVITY_NORMAL;
    ui.model.adaptive_response = CANVIEW_UI_RESPONSE_NORMAL;
    ui.model.adaptive_max_offset_steps = 4U;
    ui.model.sport_entry_speed_kph = 70U;
    ui.model.sport_acceleration_enabled = true;
    if (!styles_ready) {
        init_styles(ui.config.font);
    }

    ui.root = lv_obj_create(parent != NULL ? parent : lv_scr_act());
    lv_obj_remove_style_all(ui.root);
    lv_obj_add_style(ui.root, &style_root, LV_PART_MAIN);
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
    canview_ui_show_screen(CANVIEW_UI_SCREEN_DRIVE);
    canview_ui_update(&ui.model);
    return ui.root;
}

void canview_ui_show_screen(canview_ui_screen_t screen)
{
    if (ui.root == NULL || screen >= CANVIEW_UI_SCREEN_COUNT) {
        return;
    }
    ui.screen = screen;
    for (int i = 0; i < CANVIEW_UI_SCREEN_COUNT; ++i) {
        if (i == (int)screen) {
            lv_obj_clear_flag(ui.screens[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui.nav_buttons[i], LV_STATE_CHECKED);
        } else {
            lv_obj_add_flag(ui.screens[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui.nav_buttons[i], LV_STATE_CHECKED);
        }
    }
}

canview_ui_screen_t canview_ui_current_screen(void)
{
    return ui.screen;
}

void canview_ui_update(const canview_ui_model_t *model)
{
    if (ui.root == NULL || model == NULL) {
        return;
    }
    ui.model = *model;

    lv_label_set_text_fmt(ui.bus_label, "BUS %u/3", model->active_bus_count);
    if (model->esp_now_rssi_dbm == 0) {
        lv_label_set_text(ui.link_label, "연결 대기");
    } else {
        lv_label_set_text_fmt(ui.link_label, "연결 · %d dBm", model->esp_now_rssi_dbm);
    }

    uint16_t speed_value = model->speed_tenth_kph / 10U;
    if (!model->metric_units) {
        speed_value = (uint16_t)(((uint32_t)model->speed_tenth_kph * 621U + 5000U) / 10000U);
    }
    lv_label_set_text_fmt(ui.speed_label, "%u", speed_value);
    lv_label_set_text_fmt(ui.fft_speed_label, "%u", speed_value);
    lv_label_set_text(ui.speed_unit_label, model->metric_units ? "km/h" : "mph");
    lv_label_set_text(ui.fft_speed_unit_label, model->metric_units ? "km/h" : "mph");
    lv_arc_set_value(ui.speed_arc, model->speed_tenth_kph);
    lv_arc_set_value(ui.fft_speed_arc, model->speed_tenth_kph);

    lv_label_set_text_fmt(ui.rpm_label, "%u.%u", model->engine_rpm / 1000U,
                          (model->engine_rpm % 1000U) / 100U);
    lv_label_set_text_fmt(ui.fft_rpm_label, "%u.%u", model->engine_rpm / 1000U,
                          (model->engine_rpm % 1000U) / 100U);
    lv_arc_set_value(ui.rpm_arc, model->engine_rpm);
    lv_arc_set_value(ui.fft_rpm_arc, model->engine_rpm);

    lv_label_set_text(ui.drive_mode_label, drive_mode_text(model->drive_mode));
    if (model->four_wd_quality == CANVIEW_UI_QUALITY_UNAVAILABLE) {
        lv_label_set_text(ui.four_wd_label, "4WD\n—");
    } else {
        lv_label_set_text_fmt(ui.four_wd_label, "4WD\n%u%%",
                              model->rear_coupling_percent);
    }
    for (uint8_t i = 0; i < CANVIEW_UI_WHEEL_COUNT; ++i) {
        const uint8_t drive_value = model->wheel_drive_percent[i] > 100U
                                        ? 100U
                                        : model->wheel_drive_percent[i];
        lv_bar_set_value(ui.wheel_drive_bars[i],
                         model->four_wd_quality == CANVIEW_UI_QUALITY_UNAVAILABLE
                             ? 0
                             : drive_value,
                         LV_ANIM_OFF);
        const bool pressure_valid =
            model->tire_pressure_quality != CANVIEW_UI_QUALITY_UNAVAILABLE &&
            model->tire_pressure_tenth_psi[i] > 0U;
        if (pressure_valid) {
            lv_label_set_text_fmt(ui.wheel_pressure_labels[i], "%u.%u",
                                  model->tire_pressure_tenth_psi[i] / 10U,
                                  model->tire_pressure_tenth_psi[i] % 10U);
        } else {
            lv_label_set_text(ui.wheel_pressure_labels[i], "—");
        }
        const bool pressure_warning =
            (model->tire_pressure_warning_mask & (uint8_t)(1U << i)) != 0U;
        lv_obj_set_style_border_color(ui.wheel_objects[i],
                                      pressure_warning ? CANVIEW_COLOR_WARNING
                                                       : CANVIEW_COLOR_RULE,
                                      0);
        lv_obj_set_style_text_color(ui.wheel_pressure_labels[i],
                                    pressure_warning ? CANVIEW_COLOR_WARNING
                                                     : CANVIEW_COLOR_INK_2,
                                    0);
    }
    if (model->dpf_lamp_on) {
        lv_label_set_text(ui.dpf_label, "확인");
        lv_obj_set_style_text_color(ui.dpf_label, CANVIEW_COLOR_WARNING, 0);
    } else {
        lv_label_set_text(ui.dpf_label, "정상");
        lv_obj_set_style_text_color(ui.dpf_label, CANVIEW_COLOR_ACCENT, 0);
    }

    set_checked(ui.quiet_button, model->quiet_mode_enabled);
    set_checked(ui.rear_button, model->rear_boost_enabled);
    lv_label_set_text_fmt(ui.volume_label, "%u", model->audio_volume_level);
    set_checked(ui.adaptive_button, model->adaptive_volume_enabled);
    lv_label_set_text(ui.adaptive_button_label, model->adaptive_volume_enabled ? "사용" : "끔");
    lv_label_set_text_fmt(ui.volume_offset_label, "%+d", model->volume_offset_step);
    set_checked(ui.settings_adaptive_button, model->adaptive_volume_enabled);
    lv_label_set_text(ui.settings_adaptive_button_label,
                      model->adaptive_volume_enabled ? "사용" : "끔");
    lv_label_set_text(ui.noise_band_button_label,
                      noise_band_text(model->adaptive_noise_band));
    lv_label_set_text(ui.noise_sensitivity_button_label,
                      sensitivity_text(model->adaptive_sensitivity));
    lv_label_set_text(ui.noise_response_button_label,
                      response_text(model->adaptive_response));
    lv_label_set_text_fmt(ui.noise_max_offset_button_label, "+%u",
                          model->adaptive_max_offset_steps);

    for (uint16_t i = 0; i < CANVIEW_UI_FFT_BIN_COUNT; ++i) {
        lv_chart_set_value_by_id(ui.fft_chart, ui.fft_series, i, model->fft_bins[i]);
    }
    lv_chart_refresh(ui.fft_chart);
    if (model->fft_peak_hz >= 1000U) {
        lv_label_set_text_fmt(ui.fft_peak_frequency_label, "%u.%02u kHz",
                              model->fft_peak_hz / 1000U,
                              (model->fft_peak_hz % 1000U) / 10U);
    } else {
        lv_label_set_text_fmt(ui.fft_peak_frequency_label, "%u Hz", model->fft_peak_hz);
    }
    lv_label_set_text_fmt(ui.fft_peak_level_label, "%u.%u dB",
                          model->fft_peak_tenth_db / 10U,
                          model->fft_peak_tenth_db % 10U);

    set_checked(ui.sport_button, model->sport_monitor_enabled);
    lv_label_set_text(ui.sport_button_label, model->sport_monitor_enabled ? "사용" : "끔");
    set_checked(ui.settings_sport_button, model->sport_monitor_enabled);
    lv_label_set_text(ui.settings_sport_button_label,
                      model->sport_monitor_enabled ? "사용" : "끔");
    lv_label_set_text_fmt(ui.sport_entry_speed_button_label, "%u km/h",
                          model->sport_entry_speed_kph);
    set_checked(ui.sport_acceleration_button, model->sport_acceleration_enabled);
    lv_label_set_text(ui.sport_acceleration_button_label,
                      model->sport_acceleration_enabled ? "사용" : "끔");
    lv_arc_set_value(ui.sport_dial, model->drive_mode == CANVIEW_UI_DRIVE_SPORT ? 2 : 1);
    lv_label_set_text(ui.sport_mode_label, drive_mode_text(model->drive_mode));
    if (model->sport_previous_mode == CANVIEW_UI_DRIVE_UNKNOWN) {
        lv_label_set_text(ui.sport_return_label, "복귀 —");
    } else {
        lv_label_set_text_fmt(ui.sport_return_label, "복귀 %s",
                              drive_mode_text(model->sport_previous_mode));
    }

    uint8_t brightness = model->display_brightness_percent;
    if (brightness < 10U) {
        brightness = 10U;
    } else if (brightness > 100U) {
        brightness = 100U;
    }
    lv_slider_set_value(ui.brightness_slider, brightness, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui.brightness_label, "%u%%", brightness);
    set_checked(ui.auto_brightness_button, model->auto_brightness_enabled);
    lv_label_set_text(ui.auto_brightness_button_label,
                      model->auto_brightness_enabled ? "사용" : "끔");
    if (model->auto_brightness_enabled) {
        lv_obj_add_state(ui.brightness_slider, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(ui.brightness_slider, LV_STATE_DISABLED);
    }
    set_checked(ui.metric_button, model->metric_units);
    set_checked(ui.imperial_button, !model->metric_units);
}
