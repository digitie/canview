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
    ACTION_NOISE_BAND,
    ACTION_NOISE_SENSITIVITY,
    ACTION_NOISE_RESPONSE,
    ACTION_NOISE_MAX_OFFSET,
    ACTION_SPORT_ENTRY_SPEED,
    ACTION_SPORT_ACCELERATION,
} action_t;

#define CANVIEW_UI_TORQUE_SEGMENTS 8U

typedef struct {
    lv_obj_t *root;
    lv_obj_t *screens[CANVIEW_UI_SCREEN_COUNT];
    lv_obj_t *nav_buttons[CANVIEW_UI_SCREEN_COUNT];
    canview_ui_screen_t screen;
    canview_ui_config_t config;
    canview_ui_model_t model;

    lv_obj_t *bus_label;
    lv_obj_t *link_label;
    lv_obj_t *header_speed_label;
    lv_obj_t *speed_limit_overlay;
    lv_obj_t *speed_limit_label;
    lv_obj_t *speed_arc;
    lv_obj_t *speed_label;
    lv_obj_t *rpm_arc;
    lv_obj_t *rpm_label;
    lv_obj_t *instant_economy_label;
    lv_obj_t *dpf_label;
    lv_obj_t *dpf_load_label;
    lv_obj_t *dpf_load_bar;
    lv_obj_t *drive_mode_label;
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
    lv_obj_t *noise_band_button_label;
    lv_obj_t *noise_sensitivity_button_label;
    lv_obj_t *noise_response_button_label;
    lv_obj_t *noise_max_offset_button_label;
    lv_obj_t *settings_sport_button;
    lv_obj_t *settings_sport_button_label;
    lv_obj_t *sport_entry_speed_button_label;
    lv_obj_t *sport_acceleration_button;
    lv_obj_t *sport_acceleration_button_label;
    lv_obj_t *idle_timeout_dropdown;
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

static void emit_percent(canview_ui_command_id_t id, uint8_t percent)
{
    if (ui.config.command_cb == NULL) {
        return;
    }
    canview_ui_command_t command = {.id = id};
    command.value.percent = percent;
    ui.config.command_cb(&command, ui.config.command_user_data);
}

static void emit_activity(void)
{
    if (ui.config.command_cb == NULL) {
        return;
    }
    const canview_ui_command_t command = {.id = CANVIEW_UI_CMD_USER_ACTIVITY};
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
    emit_activity();

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
    default:
        break;
    }
}

static void brightness_event(lv_event_t *event)
{
    lv_obj_t *slider = lv_event_get_target(event);
    emit_activity();
    emit_percent(CANVIEW_UI_CMD_SET_BRIGHTNESS, (uint8_t)lv_slider_get_value(slider));
}

static void idle_timeout_event(lv_event_t *event)
{
    static const uint16_t timeouts[] = {15U, 30U, 60U, 120U};
    lv_obj_t *dropdown = lv_event_get_target(event);
    const uint16_t selected = lv_dropdown_get_selected(dropdown);
    emit_activity();
    if (selected < sizeof(timeouts) / sizeof(timeouts[0])) {
        emit_option(CANVIEW_UI_CMD_SET_IDLE_TIMEOUT, timeouts[selected]);
    }
}

static void activity_event(lv_event_t *event)
{
    (void)event;
    emit_activity();
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
    lv_obj_add_event_cb(screen, activity_event, LV_EVENT_PRESSED, NULL);
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

static void arc_animation_exec(void *object, int32_t value)
{
    lv_arc_set_value((lv_obj_t *)object, value);
}

static void opacity_animation_exec(void *object, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)object, (lv_opa_t)value, 0);
}

static void animate_arc_to(lv_obj_t *arc, int32_t value)
{
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
    const lv_font_t *value_font = diameter < 50
                                      ? (ui.config.font != NULL ? ui.config.font
                                                                : LV_FONT_DEFAULT)
                                      : (ui.config.metric_font != NULL
                                             ? ui.config.metric_font
                                             : (ui.config.font != NULL ? ui.config.font
                                                                       : LV_FONT_DEFAULT));
    lv_obj_set_style_text_font(value, value_font, 0);
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
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *brand = make_label(header, "• CV");
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_color(brand, CANVIEW_COLOR_INK_2, 0);
    lv_obj_set_style_text_letter_space(brand, 1, 0);

    ui.header_speed_label = make_label(header, "0");
    lv_obj_align(ui.header_speed_label, LV_ALIGN_CENTER, -8, 0);
    lv_obj_set_style_text_color(ui.header_speed_label, CANVIEW_COLOR_INK, 0);
    lv_obj_set_style_text_font(ui.header_speed_label,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : LV_FONT_DEFAULT,
                               0);
    lv_obj_t *speed_unit = make_label(header, "km/h");
    lv_obj_align(speed_unit, LV_ALIGN_CENTER, 23, 7);
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
    ui.speed_limit_label = make_label(ui.speed_limit_overlay, "—");
    lv_obj_center(ui.speed_limit_label);
    lv_obj_set_style_text_color(ui.speed_limit_label, CANVIEW_COLOR_PAPER, 0);
    lv_obj_set_style_text_font(ui.speed_limit_label,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : LV_FONT_DEFAULT,
                               0);
    lv_obj_add_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void create_drive_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_DRIVE] = screen;

    lv_obj_t *four_wd = make_card(screen, LV_PCT(100), 264, false);
    lv_obj_set_style_pad_all(four_wd, 8, 0);
    lv_obj_t *four_wd_title = make_label(four_wd, "4WD TORQUE · PSI");
    lv_obj_add_style(four_wd_title, &style_label_micro, 0);
    lv_obj_t *awd_label = make_label(four_wd, "AWD");
    lv_obj_align(awd_label, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_style(awd_label, &style_label_micro, 0);
    lv_obj_set_style_text_color(awd_label, CANVIEW_COLOR_ACCENT, 0);

    static const lv_coord_t wheel_x[CANVIEW_UI_WHEEL_COUNT] = {10, 208, 10, 208};
    static const lv_coord_t wheel_y[CANVIEW_UI_WHEEL_COUNT] = {38, 38, 132, 132};
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
            lv_obj_set_pos(bar_segment, 7, (lv_coord_t)(14 + segment * 4));
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
    static const lv_coord_t connector_y[CANVIEW_UI_WHEEL_COUNT] = {60, 60, 154, 154};
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
    lv_obj_set_pos(vehicle, 84, 22);
    lv_obj_set_size(vehicle, 112, 230);
    lv_obj_clear_flag(vehicle, LV_OBJ_FLAG_SCROLLABLE);

    static lv_point_t vehicle_outline[] = {
        {37, 4}, {27, 7}, {22, 20}, {20, 37}, {16, 143}, {17, 164},
        {30, 181}, {56, 186}, {82, 181}, {95, 164}, {96, 143}, {92, 37},
        {90, 20}, {85, 7}, {75, 4}, {37, 4}};
    lv_obj_t *outline = lv_line_create(vehicle);
    lv_line_set_points(outline, vehicle_outline,
                       sizeof(vehicle_outline) / sizeof(vehicle_outline[0]));
    lv_obj_set_style_line_color(outline, CANVIEW_COLOR_INK_2, 0);
    lv_obj_set_style_line_width(outline, 2, 0);
    lv_obj_set_style_line_rounded(outline, true, 0);

    static const lv_coord_t car_wheel_x[CANVIEW_UI_WHEEL_COUNT] = {5, 93, 5, 93};
    static const lv_coord_t car_wheel_y[CANVIEW_UI_WHEEL_COUNT] = {36, 36, 116, 116};
    for (uint8_t i = 0; i < CANVIEW_UI_WHEEL_COUNT; ++i) {
        lv_obj_t *car_wheel = lv_obj_create(vehicle);
        lv_obj_remove_style_all(car_wheel);
        lv_obj_set_pos(car_wheel, car_wheel_x[i], car_wheel_y[i]);
        lv_obj_set_size(car_wheel, 14, 38);
        lv_obj_set_style_bg_color(car_wheel, CANVIEW_COLOR_PAPER_3, 0);
        lv_obj_set_style_bg_opa(car_wheel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(car_wheel, CANVIEW_COLOR_INK_2, 0);
        lv_obj_set_style_border_width(car_wheel, 2, 0);
        lv_obj_set_style_radius(car_wheel, 6, 0);
    }

    static const lv_coord_t axle_y[] = {55, 135};
    for (uint8_t i = 0; i < 2U; ++i) {
        lv_obj_t *axle = lv_obj_create(vehicle);
        lv_obj_remove_style_all(axle);
        lv_obj_set_pos(axle, 19, axle_y[i]);
        lv_obj_set_size(axle, 74, 3);
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
    lv_obj_set_pos(shaft, 55, 67);
    lv_obj_set_size(shaft, 3, 56);
    lv_obj_set_style_bg_color(shaft, CANVIEW_COLOR_ACCENT_LINE, 0);
    lv_obj_set_style_bg_opa(shaft, LV_OPA_COVER, 0);

    lv_obj_t *instant = lv_obj_create(vehicle);
    lv_obj_remove_style_all(instant);
    lv_obj_set_pos(instant, 31, 78);
    lv_obj_set_size(instant, 50, 48);
    lv_obj_set_style_bg_color(instant, CANVIEW_COLOR_PAPER, 0);
    lv_obj_set_style_bg_opa(instant, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(instant, CANVIEW_COLOR_ACCENT_LINE, 0);
    lv_obj_set_style_border_width(instant, 1, 0);
    lv_obj_set_style_radius(instant, 4, 0);
    lv_obj_t *instant_title = make_label(instant, "순간");
    lv_obj_align(instant_title, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_add_style(instant_title, &style_label_micro, 0);
    ui.instant_economy_label = make_label(instant, "—");
    lv_obj_align(ui.instant_economy_label, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_text_color(ui.instant_economy_label, CANVIEW_COLOR_INK, 0);

    ui.four_wd_label = make_label(vehicle, "R —%");
    lv_obj_align(ui.four_wd_label, LV_ALIGN_BOTTOM_MID, 0, -3);
    lv_obj_set_style_text_color(ui.four_wd_label, CANVIEW_COLOR_ACCENT, 0);

    lv_obj_t *dpf = make_card(screen, LV_PCT(100), 34, true);
    lv_obj_set_style_pad_all(dpf, 8, 0);
    lv_obj_t *dpf_title = make_label(dpf, "DPF");
    lv_obj_align(dpf_title, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_style(dpf_title, &style_label_micro, 0);
    ui.dpf_label = make_label(dpf, "—");
    lv_obj_align(ui.dpf_label, LV_ALIGN_LEFT_MID, 35, 0);
    lv_obj_set_style_text_color(ui.dpf_label, CANVIEW_COLOR_ACCENT, 0);
    ui.dpf_load_bar = lv_bar_create(dpf);
    lv_obj_set_size(ui.dpf_load_bar, 145, 4);
    lv_obj_align(ui.dpf_load_bar, LV_ALIGN_LEFT_MID, 78, 0);
    lv_bar_set_range(ui.dpf_load_bar, 0, 100);
    lv_obj_set_style_bg_color(ui.dpf_load_bar, CANVIEW_COLOR_RULE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui.dpf_load_bar, CANVIEW_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_anim_time(ui.dpf_load_bar, 300U, 0);
    ui.dpf_load_label = make_label(dpf, "—%");
    lv_obj_align(ui.dpf_load_label, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *gauges = lv_obj_create(screen);
    lv_obj_remove_style_all(gauges);
    lv_obj_set_size(gauges, LV_PCT(100), 38);
    lv_obj_set_style_pad_column(gauges, 8, 0);
    lv_obj_set_flex_flow(gauges, LV_FLEX_FLOW_ROW);
    make_gauge(gauges, 38, 38, 36, "", "", 2400,
               &ui.speed_arc, &ui.speed_label, NULL);
    make_gauge(gauges, 38, 38, 36, "", "", 6500,
               &ui.rpm_arc, &ui.rpm_label, NULL);
    lv_obj_t *mode = make_card(gauges, 204, 38, true);
    lv_obj_set_style_pad_all(mode, 8, 0);
    lv_obj_set_style_border_color(mode, CANVIEW_COLOR_ACCENT_LINE, 0);
    lv_obj_set_style_border_width(mode, 2, 0);
    lv_obj_set_style_border_side(mode, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_t *mode_title = make_label(mode, "DRIVE MODE");
    lv_obj_add_style(mode_title, &style_label_micro, 0);
    ui.drive_mode_label = make_label(mode, "UNKNOWN");
    lv_obj_align(ui.drive_mode_label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_color(ui.drive_mode_label, CANVIEW_COLOR_ACCENT, 0);
}

static lv_obj_t *make_profile_button(lv_obj_t *parent, const char *title, action_t action)
{
    lv_obj_t *button = make_button(parent, 142, 76, title, action);
    bind_button(button);
    return button;
}

static lv_obj_t *make_fft_chart(lv_obj_t *card, lv_coord_t height,
                                lv_chart_series_t **series_out)
{
    lv_obj_t *chart = lv_chart_create(card);
    lv_obj_set_size(chart, LV_PCT(100), height);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -10);
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
    lv_obj_align(value, align, x, 16);
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

    lv_obj_t *volume = make_card(screen, LV_PCT(100), 70, false);
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

    lv_obj_t *spectrum = make_card(screen, LV_PCT(100), 190, true);
    make_fft_metric(spectrum, "PEAK", LV_ALIGN_TOP_LEFT, 0,
                    &ui.audio_peak_frequency_label);
    make_fft_metric(spectrum, "LEVEL", LV_ALIGN_TOP_RIGHT, -52,
                    &ui.audio_peak_level_label);
    ui.audio_fft_chart = make_fft_chart(spectrum, 130, &ui.audio_fft_series);
}

static void create_fft_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_FFT] = screen;

    lv_obj_t *spectrum = make_card(screen, LV_PCT(100), 352, false);
    make_fft_metric(spectrum, "PEAK", LV_ALIGN_TOP_LEFT, 0,
                    &ui.fft_peak_frequency_label);
    make_fft_metric(spectrum, "LEVEL", LV_ALIGN_TOP_RIGHT, -52,
                    &ui.fft_peak_level_label);
    lv_obj_t *live = make_label(spectrum, "● CABIN FFT");
    lv_obj_align(live, LV_ALIGN_TOP_LEFT, 0, 34);
    lv_obj_add_style(live, &style_label_micro, 0);
    lv_obj_set_style_text_color(live, CANVIEW_COLOR_ACCENT, 0);
    ui.fft_chart = make_fft_chart(spectrum, 274, &ui.fft_series);
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
    lv_obj_set_style_anim_time(ui.brightness_slider, 240U, 0);
    lv_obj_add_event_cb(ui.brightness_slider, brightness_event, LV_EVENT_RELEASED, NULL);

    lv_obj_t *automatic = make_card(screen, LV_PCT(100), 76, true);
    lv_obj_t *sensor = make_label(automatic, "CAN");
    lv_obj_add_style(sensor, &style_label_micro, 0);
    lv_obj_t *automatic_title = make_label(automatic, "자동 밝기");
    lv_obj_align(automatic_title, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    ui.auto_brightness_button = make_button(automatic, 76, 48, "끔",
                                             ACTION_AUTO_BRIGHTNESS);
    lv_obj_align(ui.auto_brightness_button, LV_ALIGN_RIGHT_MID, 0, 0);
    ui.auto_brightness_button_label = lv_obj_get_child(ui.auto_brightness_button, 0);
    bind_button(ui.auto_brightness_button);

    lv_obj_t *idle = make_card(screen, LV_PCT(100), 76, true);
    lv_obj_t *idle_title = make_label(idle, "대기 후 복귀");
    lv_obj_align(idle_title, LV_ALIGN_LEFT_MID, 0, 0);
    ui.idle_timeout_dropdown = lv_dropdown_create(idle);
    lv_dropdown_set_options(ui.idle_timeout_dropdown, "15초\n30초\n60초\n120초");
    lv_obj_set_size(ui.idle_timeout_dropdown, 104, 44);
    lv_obj_align(ui.idle_timeout_dropdown, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(ui.idle_timeout_dropdown, CANVIEW_COLOR_PAPER_3, 0);
    lv_obj_set_style_text_color(ui.idle_timeout_dropdown, CANVIEW_COLOR_INK_2, 0);
    lv_obj_set_style_border_color(ui.idle_timeout_dropdown, CANVIEW_COLOR_RULE, 0);
    lv_obj_set_style_border_width(ui.idle_timeout_dropdown, 1, 0);
    lv_obj_set_style_radius(ui.idle_timeout_dropdown, CANVIEW_RADIUS_SM, 0);
    lv_obj_add_event_cb(ui.idle_timeout_dropdown, idle_timeout_event,
                        LV_EVENT_VALUE_CHANGED, NULL);

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
    ui.model.audio_volume_level = 18;
    ui.model.display_brightness_percent = 42;
    ui.model.adaptive_noise_band = CANVIEW_UI_NOISE_BAND_BALANCED;
    ui.model.adaptive_sensitivity = CANVIEW_UI_SENSITIVITY_NORMAL;
    ui.model.adaptive_response = CANVIEW_UI_RESPONSE_NORMAL;
    ui.model.adaptive_max_offset_steps = 4U;
    ui.model.sport_entry_speed_kph = 70U;
    ui.model.sport_acceleration_enabled = true;
    ui.model.idle_timeout_seconds = 30U;
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
    lv_obj_move_foreground(ui.speed_limit_overlay);
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
            const bool was_hidden = lv_obj_has_flag(ui.screens[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui.screens[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui.nav_buttons[i], LV_STATE_CHECKED);
            if (was_hidden) {
                fade_screen_in(ui.screens[i]);
            }
        } else {
            lv_obj_add_flag(ui.screens[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui.nav_buttons[i], LV_STATE_CHECKED);
        }
    }
    const bool touch_screen = screen == CANVIEW_UI_SCREEN_AUDIO ||
                              screen == CANVIEW_UI_SCREEN_AUTOMATION ||
                              screen == CANVIEW_UI_SCREEN_SETTINGS;
    lv_obj_set_style_opa(ui.speed_limit_overlay,
                         touch_screen ? LV_OPA_60 : LV_OPA_COVER, 0);
    lv_obj_move_foreground(ui.speed_limit_overlay);
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
    const bool idle_started = model->idle_dimmed && !ui.model.idle_dimmed;
    ui.model = *model;
    if (idle_started) {
        canview_ui_show_screen(CANVIEW_UI_SCREEN_DRIVE);
    }

    static const char *bus_states[] = {"○○○", "●○○", "●●○", "●●●"};
    const uint8_t bus_count = model->active_bus_count > 3U ? 3U : model->active_bus_count;
    lv_label_set_text(ui.bus_label, bus_states[bus_count]);
    if (model->esp_now_rssi_dbm == 0) {
        lv_label_set_text(ui.link_label, "—");
    } else {
        lv_label_set_text_fmt(ui.link_label, "%d", model->esp_now_rssi_dbm);
    }

    const uint16_t speed_value = model->speed_tenth_kph / 10U;
    lv_label_set_text_fmt(ui.header_speed_label, "%u", speed_value);
    lv_label_set_text_fmt(ui.speed_label, "%u", speed_value);
    animate_arc_to(ui.speed_arc, model->speed_tenth_kph);

    lv_label_set_text_fmt(ui.rpm_label, "%u.%u", model->engine_rpm / 1000U,
                          (model->engine_rpm % 1000U) / 100U);
    animate_arc_to(ui.rpm_arc, model->engine_rpm);

    if (model->fuel_economy_quality == CANVIEW_UI_QUALITY_UNAVAILABLE) {
        lv_label_set_text(ui.instant_economy_label, "—");
    } else {
        lv_label_set_text_fmt(ui.instant_economy_label, "%u.%u",
                              model->instant_fuel_economy_tenth_kmpl / 10U,
                              model->instant_fuel_economy_tenth_kmpl % 10U);
    }

    lv_label_set_text(ui.drive_mode_label, drive_mode_text(model->drive_mode));
    lv_obj_set_style_text_color(ui.drive_mode_label, drive_mode_color(model->drive_mode), 0);
    if (model->four_wd_quality == CANVIEW_UI_QUALITY_UNAVAILABLE) {
        lv_label_set_text(ui.four_wd_label, "R —%");
    } else {
        lv_label_set_text_fmt(ui.four_wd_label, "R %u%%",
                              model->rear_coupling_percent);
    }
    for (uint8_t i = 0; i < CANVIEW_UI_WHEEL_COUNT; ++i) {
        const uint8_t drive_value = model->wheel_drive_percent[i] > 100U
                                        ? 100U
                                        : model->wheel_drive_percent[i];
        const uint8_t active_segments =
            model->four_wd_quality == CANVIEW_UI_QUALITY_UNAVAILABLE
                ? 0U
                : (uint8_t)(((uint16_t)drive_value * CANVIEW_UI_TORQUE_SEGMENTS + 99U) /
                            100U);
        for (uint8_t segment = 0; segment < CANVIEW_UI_TORQUE_SEGMENTS; ++segment) {
            const bool active = segment >= CANVIEW_UI_TORQUE_SEGMENTS - active_segments;
            lv_obj_set_style_bg_color(ui.wheel_drive_segments[i][segment],
                                      active ? CANVIEW_COLOR_ACCENT : CANVIEW_COLOR_RULE, 0);
        }
        if (model->four_wd_quality == CANVIEW_UI_QUALITY_UNAVAILABLE) {
            lv_label_set_text(ui.wheel_drive_labels[i], "—%");
        } else {
            lv_label_set_text_fmt(ui.wheel_drive_labels[i], "%u%%", drive_value);
        }
        const bool pressure_valid =
            model->tire_pressure_quality != CANVIEW_UI_QUALITY_UNAVAILABLE &&
            model->tire_pressure_tenth_psi[i] > 0U;
        if (pressure_valid) {
            lv_label_set_text_fmt(ui.wheel_pressure_labels[i], "%u.%u psi",
                                  model->tire_pressure_tenth_psi[i] / 10U,
                                  model->tire_pressure_tenth_psi[i] % 10U);
        } else {
            lv_label_set_text(ui.wheel_pressure_labels[i], "—");
        }
        const bool pressure_warning =
            (model->tire_pressure_warning_mask & (uint8_t)(1U << i)) != 0U;
        lv_obj_set_style_text_color(ui.wheel_pressure_labels[i],
                                    pressure_warning ? CANVIEW_COLOR_WARNING
                                                     : CANVIEW_COLOR_INK_2,
                                    0);
    }
    const uint8_t dpf_load = model->dpf_load_percent > 100U
                                 ? 100U
                                 : model->dpf_load_percent;
    if (model->dpf_lamp_quality == CANVIEW_UI_QUALITY_UNAVAILABLE) {
        lv_label_set_text(ui.dpf_label, "—");
        lv_obj_set_style_text_color(ui.dpf_label, CANVIEW_COLOR_MUTED, 0);
    } else if (model->dpf_lamp_on) {
        lv_label_set_text(ui.dpf_label, "확인");
        lv_obj_set_style_text_color(ui.dpf_label, CANVIEW_COLOR_WARNING, 0);
    } else {
        lv_label_set_text(ui.dpf_label, "정상");
        lv_obj_set_style_text_color(ui.dpf_label, CANVIEW_COLOR_ACCENT, 0);
    }
    if (model->dpf_load_quality == CANVIEW_UI_QUALITY_UNAVAILABLE) {
        lv_label_set_text(ui.dpf_load_label, "—%");
        lv_bar_set_value(ui.dpf_load_bar, 0, LV_ANIM_ON);
        lv_obj_set_style_bg_color(ui.dpf_load_bar, CANVIEW_COLOR_ACCENT,
                                  LV_PART_INDICATOR);
    } else {
        lv_label_set_text_fmt(ui.dpf_load_label, "%u%%", dpf_load);
        lv_bar_set_value(ui.dpf_load_bar, dpf_load, LV_ANIM_ON);
        lv_obj_set_style_bg_color(ui.dpf_load_bar,
                                  model->dpf_lamp_quality != CANVIEW_UI_QUALITY_UNAVAILABLE &&
                                          model->dpf_lamp_on
                                      ? CANVIEW_COLOR_WARNING
                                      : CANVIEW_COLOR_ACCENT,
                                  LV_PART_INDICATOR);
    }

    set_checked(ui.quiet_button, model->quiet_mode_enabled);
    set_checked(ui.rear_button, model->rear_boost_enabled);
    lv_label_set_text_fmt(ui.volume_label, "%u", model->audio_volume_level);
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
        lv_chart_set_value_by_id(ui.audio_fft_chart, ui.audio_fft_series, i,
                                 model->fft_bins[i]);
        lv_chart_set_value_by_id(ui.fft_chart, ui.fft_series, i, model->fft_bins[i]);
    }
    lv_chart_refresh(ui.audio_fft_chart);
    lv_chart_refresh(ui.fft_chart);
    if (model->fft_peak_hz >= 1000U) {
        lv_label_set_text_fmt(ui.audio_peak_frequency_label, "%u.%02u kHz",
                              model->fft_peak_hz / 1000U,
                              (model->fft_peak_hz % 1000U) / 10U);
        lv_label_set_text_fmt(ui.fft_peak_frequency_label, "%u.%02u kHz",
                              model->fft_peak_hz / 1000U,
                              (model->fft_peak_hz % 1000U) / 10U);
    } else {
        lv_label_set_text_fmt(ui.audio_peak_frequency_label, "%u Hz",
                              model->fft_peak_hz);
        lv_label_set_text_fmt(ui.fft_peak_frequency_label, "%u Hz", model->fft_peak_hz);
    }
    lv_label_set_text_fmt(ui.audio_peak_level_label, "%u.%u dB",
                          model->fft_peak_tenth_db / 10U,
                          model->fft_peak_tenth_db % 10U);
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
    lv_label_set_text(ui.sport_mode_label, drive_mode_text(model->drive_mode));
    const lv_color_t mode_color = drive_mode_color(model->drive_mode);
    lv_obj_set_style_text_color(ui.sport_mode_label, mode_color, 0);
    lv_obj_set_style_border_color(ui.sport_mode_field, mode_color, 0);
    if (model->sport_previous_mode == CANVIEW_UI_DRIVE_UNKNOWN) {
        lv_label_set_text(ui.sport_return_label, "—");
    } else {
        lv_label_set_text_fmt(ui.sport_return_label, "%s",
                              drive_mode_text(model->sport_previous_mode));
        lv_obj_set_style_text_color(ui.sport_return_label,
                                    drive_mode_color(model->sport_previous_mode), 0);
    }

    uint8_t brightness = model->display_brightness_percent;
    if (brightness < 10U) {
        brightness = 10U;
    } else if (brightness > 100U) {
        brightness = 100U;
    }
    lv_slider_set_value(ui.brightness_slider, brightness, LV_ANIM_ON);
    lv_label_set_text_fmt(ui.brightness_label, "%u%%", brightness);
    set_checked(ui.auto_brightness_button, model->auto_brightness_enabled);
    lv_label_set_text(ui.auto_brightness_button_label,
                      model->auto_brightness_enabled ? "사용" : "끔");
    uint16_t idle_selected = 1U;
    if (model->idle_timeout_seconds <= 15U) {
        idle_selected = 0U;
    } else if (model->idle_timeout_seconds <= 30U) {
        idle_selected = 1U;
    } else if (model->idle_timeout_seconds <= 60U) {
        idle_selected = 2U;
    } else {
        idle_selected = 3U;
    }
    lv_dropdown_set_selected(ui.idle_timeout_dropdown, idle_selected);

    if (model->speed_limit_active && model->speed_limit_kph > 0U) {
        lv_obj_clear_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(ui.speed_limit_label, "%u", model->speed_limit_kph);
        const bool touch_screen = ui.screen == CANVIEW_UI_SCREEN_AUDIO ||
                                  ui.screen == CANVIEW_UI_SCREEN_AUTOMATION ||
                                  ui.screen == CANVIEW_UI_SCREEN_SETTINGS;
        lv_opa_t overlay_opacity = touch_screen ? LV_OPA_60 : LV_OPA_COVER;
        if (model->speed_limit_warning_active) {
            overlay_opacity = model->speed_limit_warning_visible
                                  ? (touch_screen ? LV_OPA_80 : LV_OPA_COVER)
                                  : LV_OPA_30;
        }
        lv_obj_set_style_opa(ui.speed_limit_overlay, overlay_opacity, 0);
    } else {
        lv_obj_add_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_text_color(ui.header_speed_label,
                                model->speed_limit_warning_active
                                    ? CANVIEW_COLOR_WARNING
                                    : CANVIEW_COLOR_INK,
                                0);
    lv_obj_move_foreground(ui.speed_limit_overlay);
}
