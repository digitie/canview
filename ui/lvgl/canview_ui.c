#include "canview_ui.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "canview_theme.h"

typedef enum {
    ACTION_NAV_DRIVE = 1,
    ACTION_NAV_AUDIO,
    ACTION_NAV_AUTOMATION,
    ACTION_QUIET,
    ACTION_REAR_BOOST,
    ACTION_ADAPTIVE_VOLUME,
    ACTION_SPORT_MONITOR,
    ACTION_SOUND_UP,
    ACTION_SOUND_DOWN,
    ACTION_SOUND_LEFT,
    ACTION_SOUND_RIGHT,
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
    lv_obj_t *speed_label;
    lv_obj_t *rpm_label;
    lv_obj_t *rpm_bar;
    lv_obj_t *coupling_arc;
    lv_obj_t *coupling_label;
    lv_obj_t *coupling_quality_label;
    lv_obj_t *dpf_label;
    lv_obj_t *dpf_detail_label;
    lv_obj_t *dpf_badge;
    lv_obj_t *drive_mode_label;
    lv_obj_t *clutch_torque_label;

    lv_obj_t *quiet_button;
    lv_obj_t *rear_button;
    lv_obj_t *adaptive_button;
    lv_obj_t *adaptive_button_label;
    lv_obj_t *noise_label;
    lv_obj_t *volume_offset_label;
    lv_obj_t *sound_focus;

    lv_obj_t *sport_button;
    lv_obj_t *sport_button_label;
    lv_obj_t *sport_dial;
    lv_obj_t *sport_mode_label;
    lv_obj_t *accel_label;
    lv_obj_t *diagnostic_label;
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
    lv_style_set_radius(&style_card, CANVIEW_RADIUS_MD);
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

static void emit_sound_position(int8_t balance_delta, int8_t fader_delta)
{
    if (ui.config.command_cb == NULL) {
        return;
    }
    canview_ui_command_t command = {.id = CANVIEW_UI_CMD_MOVE_SOUND_POSITION};
    command.value.sound_position.balance_delta = balance_delta;
    command.value.sound_position.fader_delta = fader_delta;
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
    case ACTION_NAV_AUTOMATION:
        canview_ui_show_screen(CANVIEW_UI_SCREEN_AUTOMATION);
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
    case ACTION_SOUND_UP:
        emit_sound_position(0, 1);
        break;
    case ACTION_SOUND_DOWN:
        emit_sound_position(0, -1);
        break;
    case ACTION_SOUND_LEFT:
        emit_sound_position(-1, 0);
        break;
    case ACTION_SOUND_RIGHT:
        emit_sound_position(1, 0);
        break;
    default:
        break;
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
    lv_obj_set_style_pad_top(screen, 10, 0);
    lv_obj_set_style_pad_bottom(screen, 8, 0);
    lv_obj_set_style_pad_row(screen, 8, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    return screen;
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

    lv_obj_t *overview = make_card(screen, LV_PCT(100), 182, false);
    lv_obj_set_style_pad_all(overview, 0, 0);
    lv_obj_set_flex_flow(overview, LV_FLEX_FLOW_ROW);

    lv_obj_t *speed = lv_obj_create(overview);
    lv_obj_remove_style_all(speed);
    lv_obj_set_size(speed, 124, LV_PCT(100));
    lv_obj_set_style_pad_left(speed, 12, 0);
    lv_obj_set_style_pad_top(speed, 12, 0);
    lv_obj_set_style_pad_bottom(speed, 12, 0);
    lv_obj_set_flex_flow(speed, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(speed, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_t *speed_title = make_label(speed, "VEHICLE SPEED");
    lv_obj_add_style(speed_title, &style_label_micro, 0);
    ui.speed_label = make_label(speed, "0 km/h");
    lv_obj_set_style_text_color(ui.speed_label, CANVIEW_COLOR_INK, 0);
    lv_obj_set_style_text_font(ui.speed_label,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : (ui.config.font != NULL ? ui.config.font
                                                                                       : LV_FONT_DEFAULT),
                               0);
    lv_obj_set_style_text_letter_space(ui.speed_label, -1, 0);

    lv_obj_t *rpm_group = lv_obj_create(speed);
    lv_obj_remove_style_all(rpm_group);
    lv_obj_set_size(rpm_group, LV_PCT(100), 28);
    ui.rpm_bar = lv_bar_create(rpm_group);
    lv_obj_set_size(ui.rpm_bar, LV_PCT(100), 3);
    lv_obj_align(ui.rpm_bar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_bar_set_range(ui.rpm_bar, 0, 6500);
    lv_obj_set_style_bg_color(ui.rpm_bar, CANVIEW_COLOR_RULE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui.rpm_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui.rpm_bar, CANVIEW_COLOR_ACCENT, LV_PART_INDICATOR);
    ui.rpm_label = make_label(rpm_group, "0 rpm");
    lv_obj_align(ui.rpm_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_color(ui.rpm_label, CANVIEW_COLOR_INK_2, 0);

    lv_obj_t *drive = lv_obj_create(overview);
    lv_obj_remove_style_all(drive);
    lv_obj_set_flex_grow(drive, 1);
    lv_obj_set_height(drive, LV_PCT(100));
    ui.coupling_arc = lv_arc_create(drive);
    lv_obj_set_size(ui.coupling_arc, 112, 112);
    lv_obj_align(ui.coupling_arc, LV_ALIGN_CENTER, 0, -5);
    lv_arc_set_rotation(ui.coupling_arc, 135);
    lv_arc_set_bg_angles(ui.coupling_arc, 0, 270);
    lv_arc_set_range(ui.coupling_arc, 0, 100);
    lv_obj_remove_style(ui.coupling_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(ui.coupling_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ui.coupling_arc, CANVIEW_COLOR_RULE, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ui.coupling_arc, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ui.coupling_arc, CANVIEW_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ui.coupling_arc, 6, LV_PART_INDICATOR);
    ui.coupling_label = make_label(drive, "0%");
    lv_obj_align(ui.coupling_label, LV_ALIGN_CENTER, 0, -7);
    lv_obj_set_style_text_font(ui.coupling_label,
                               ui.config.metric_font != NULL ? ui.config.metric_font
                                                             : (ui.config.font != NULL ? ui.config.font
                                                                                       : LV_FONT_DEFAULT),
                               0);
    ui.coupling_quality_label = make_label(drive, "후륜 결합 · 없음");
    lv_obj_align(ui.coupling_quality_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_style(ui.coupling_quality_label, &style_label_micro, 0);

    lv_obj_t *dpf = make_card(screen, LV_PCT(100), 72, false);
    lv_obj_set_style_pad_all(dpf, 12, 0);
    lv_obj_t *dpf_copy = lv_obj_create(dpf);
    lv_obj_remove_style_all(dpf_copy);
    lv_obj_set_size(dpf_copy, 206, LV_PCT(100));
    ui.dpf_label = make_label(dpf_copy, "DPF 정보 없음");
    lv_obj_align(ui.dpf_label, LV_ALIGN_TOP_LEFT, 0, 4);
    ui.dpf_detail_label = make_label(dpf_copy, "신호를 기다리는 중");
    lv_obj_align(ui.dpf_detail_label, LV_ALIGN_BOTTOM_LEFT, 0, -4);
    lv_obj_add_style(ui.dpf_detail_label, &style_label_muted, 0);
    ui.dpf_badge = make_label(dpf, "—");
    lv_obj_align(ui.dpf_badge, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_color(ui.dpf_badge, CANVIEW_COLOR_ACCENT, 0);

    lv_obj_t *mini = lv_obj_create(screen);
    lv_obj_remove_style_all(mini);
    lv_obj_set_size(mini, LV_PCT(100), 80);
    lv_obj_set_style_pad_column(mini, 8, 0);
    lv_obj_set_flex_flow(mini, LV_FLEX_FLOW_ROW);
    lv_obj_t *mode = make_card(mini, 166, 80, true);
    make_label(mode, "DRIVE MODE");
    ui.drive_mode_label = make_label(mode, "UNKNOWN");
    lv_obj_align(ui.drive_mode_label, LV_ALIGN_LEFT_MID, 0, 4);
    lv_obj_set_style_text_color(ui.drive_mode_label, CANVIEW_COLOR_INK, 0);
    lv_obj_t *monitor = make_label(mode, "자동 SPORT · 감시 전용");
    lv_obj_align(monitor, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_style(monitor, &style_label_micro, 0);
    lv_obj_t *torque = make_card(mini, 118, 80, true);
    make_label(torque, "4WD CLUTCH");
    ui.clutch_torque_label = make_label(torque, "— Nm");
    lv_obj_align(ui.clutch_torque_label, LV_ALIGN_LEFT_MID, 0, 4);
    lv_obj_t *candidate = make_label(torque, "DBC 후보값");
    lv_obj_align(candidate, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_style(candidate, &style_label_micro, 0);
}

static lv_obj_t *make_profile_button(lv_obj_t *parent, const char *title,
                                     const char *detail, action_t action)
{
    lv_obj_t *button = make_button(parent, 142, 88, "", action);
    lv_obj_set_style_pad_all(button, 12, 0);
    lv_obj_t *title_label = make_label(button, title);
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 0, 4);
    lv_obj_t *detail_label = make_label(button, detail);
    lv_obj_align(detail_label, LV_ALIGN_BOTTOM_LEFT, 0, -4);
    lv_obj_add_style(detail_label, &style_label_micro, 0);
    bind_button(button);
    return button;
}

static lv_obj_t *make_pad_button(lv_obj_t *parent, const char *symbol, action_t action)
{
    lv_obj_t *button = make_button(parent, 48, 48, symbol, action);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CHECKABLE);
    bind_button(button);
    return button;
}

static void create_audio_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_AUDIO] = screen;

    lv_obj_t *quick = lv_obj_create(screen);
    lv_obj_remove_style_all(quick);
    lv_obj_set_size(quick, LV_PCT(100), 88);
    lv_obj_set_style_pad_column(quick, 8, 0);
    lv_obj_set_flex_flow(quick, LV_FLEX_FLOW_ROW);
    ui.quiet_button = make_profile_button(quick, "취침", "뒤 스피커 끄기", ACTION_QUIET);
    ui.rear_button = make_profile_button(quick, "뒷좌석 +", "페이더 뒤 2단", ACTION_REAR_BOOST);

    lv_obj_t *position = make_card(screen, LV_PCT(100), 150, false);
    lv_obj_set_style_pad_all(position, 8, 0);
    lv_obj_t *cabin = lv_obj_create(position);
    lv_obj_remove_style_all(cabin);
    lv_obj_set_pos(cabin, 10, 5);
    lv_obj_set_size(cabin, 112, 124);
    lv_obj_set_style_border_color(cabin, CANVIEW_COLOR_RULE, 0);
    lv_obj_set_style_border_width(cabin, 1, 0);
    lv_obj_set_style_radius(cabin, 18, 0);
    lv_obj_t *front = make_label(cabin, "FRONT");
    lv_obj_align(front, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_add_style(front, &style_label_micro, 0);
    lv_obj_t *rear = make_label(cabin, "REAR");
    lv_obj_align(rear, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_add_style(rear, &style_label_micro, 0);
    for (lv_coord_t row = 0; row < 2; ++row) {
        for (lv_coord_t col = 0; col < 2; ++col) {
            lv_obj_t *seat = lv_obj_create(cabin);
            lv_obj_remove_style_all(seat);
            lv_obj_set_size(seat, 24, 28);
            lv_obj_set_pos(seat, (lv_coord_t)(25 + col * 38),
                           (lv_coord_t)(27 + row * 47));
            lv_obj_set_style_bg_color(seat, row == 1 ? CANVIEW_COLOR_ACCENT_WASH : CANVIEW_COLOR_PAPER_3, 0);
            lv_obj_set_style_bg_opa(seat, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(seat, row == 1 ? CANVIEW_COLOR_ACCENT_LINE : CANVIEW_COLOR_RULE, 0);
            lv_obj_set_style_border_width(seat, 1, 0);
            lv_obj_set_style_radius(seat, 7, 0);
        }
    }
    ui.sound_focus = lv_obj_create(cabin);
    lv_obj_remove_style_all(ui.sound_focus);
    lv_obj_set_size(ui.sound_focus, 16, 16);
    lv_obj_set_style_bg_color(ui.sound_focus, CANVIEW_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(ui.sound_focus, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ui.sound_focus, CANVIEW_COLOR_INK, 0);
    lv_obj_set_style_border_width(ui.sound_focus, 2, 0);
    lv_obj_set_style_radius(ui.sound_focus, LV_RADIUS_CIRCLE, 0);

    lv_obj_t *controls = lv_obj_create(position);
    lv_obj_remove_style_all(controls);
    lv_obj_set_pos(controls, 145, 0);
    lv_obj_set_size(controls, 130, 134);
    lv_obj_t *caption = make_label(controls, "SOUND POSITION");
    lv_obj_align(caption, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(caption, &style_label_micro, 0);
    lv_obj_t *pad = lv_obj_create(controls);
    lv_obj_remove_style_all(pad);
    lv_obj_set_size(pad, 96, 96);
    lv_obj_align(pad, LV_ALIGN_CENTER, 0, 5);
    lv_obj_set_flex_flow(pad, LV_FLEX_FLOW_ROW_WRAP);
    make_pad_button(pad, LV_SYMBOL_UP, ACTION_SOUND_UP);
    make_pad_button(pad, LV_SYMBOL_DOWN, ACTION_SOUND_DOWN);
    make_pad_button(pad, LV_SYMBOL_LEFT, ACTION_SOUND_LEFT);
    make_pad_button(pad, LV_SYMBOL_RIGHT, ACTION_SOUND_RIGHT);
    lv_obj_t *parked = make_label(controls, "정차 시 조절");
    lv_obj_align(parked, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(parked, &style_label_micro, 0);

    lv_obj_t *adaptive = make_card(screen, LV_PCT(100), 80, true);
    lv_obj_t *adaptive_title = make_label(adaptive, "주행 소음 보정");
    lv_obj_align(adaptive_title, LV_ALIGN_TOP_LEFT, 0, 3);
    ui.noise_label = make_label(adaptive, "상대 — dB");
    lv_obj_align(ui.noise_label, LV_ALIGN_BOTTOM_LEFT, 0, -3);
    lv_obj_add_style(ui.noise_label, &style_label_muted, 0);
    ui.volume_offset_label = make_label(adaptive, "음량 —");
    lv_obj_align(ui.volume_offset_label, LV_ALIGN_BOTTOM_LEFT, 82, -3);
    lv_obj_set_style_text_color(ui.volume_offset_label, CANVIEW_COLOR_ACCENT, 0);
    ui.adaptive_button = make_button(adaptive, 76, 48, "꺼짐", ACTION_ADAPTIVE_VOLUME);
    lv_obj_align(ui.adaptive_button, LV_ALIGN_RIGHT_MID, 0, 0);
    ui.adaptive_button_label = lv_obj_get_child(ui.adaptive_button, 0);
    bind_button(ui.adaptive_button);
}

static void create_automation_screen(void)
{
    lv_obj_t *screen = make_screen();
    ui.screens[CANVIEW_UI_SCREEN_AUTOMATION] = screen;

    lv_obj_t *sport = make_card(screen, LV_PCT(100), 146, false);
    lv_obj_t *eyebrow = make_label(sport, "SPORT AUTOMATION");
    lv_obj_add_style(eyebrow, &style_label_micro, 0);
    lv_obj_t *title = make_label(sport, "필요할 때만 SPORT");
    lv_obj_set_pos(title, 0, 25);
    lv_obj_set_style_text_color(title, CANVIEW_COLOR_INK, 0);
    lv_obj_t *body = make_label(sport, "가속 요구를 감지해 제안합니다.\n현재 차량 송신 없이 감시합니다.");
    lv_obj_set_pos(body, 0, 50);
    lv_obj_set_style_text_color(body, CANVIEW_COLOR_MUTED, 0);
    ui.sport_button = make_button(sport, 94, 48, "사용 안 함", ACTION_SPORT_MONITOR);
    lv_obj_align(ui.sport_button, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    ui.sport_button_label = lv_obj_get_child(ui.sport_button, 0);
    bind_button(ui.sport_button);

    ui.sport_dial = lv_arc_create(sport);
    lv_obj_set_size(ui.sport_dial, 96, 96);
    lv_obj_align(ui.sport_dial, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_arc_set_rotation(ui.sport_dial, 135);
    lv_arc_set_bg_angles(ui.sport_dial, 0, 270);
    lv_arc_set_range(ui.sport_dial, 0, 2);
    lv_obj_remove_style(ui.sport_dial, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(ui.sport_dial, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ui.sport_dial, CANVIEW_COLOR_RULE, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ui.sport_dial, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ui.sport_dial, CANVIEW_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ui.sport_dial, 7, LV_PART_INDICATOR);
    ui.sport_mode_label = make_label(sport, "UNKNOWN");
    lv_obj_align(ui.sport_mode_label, LV_ALIGN_RIGHT_MID, -9, 0);

    lv_obj_t *metrics = lv_obj_create(screen);
    lv_obj_remove_style_all(metrics);
    lv_obj_set_size(metrics, LV_PCT(100), 98);
    lv_obj_set_style_pad_column(metrics, 8, 0);
    lv_obj_set_flex_flow(metrics, LV_FLEX_FLOW_ROW);
    lv_obj_t *accel = make_card(metrics, 126, 98, true);
    make_label(accel, "LONG ACCEL");
    ui.accel_label = make_label(accel, "— m/s²");
    lv_obj_align(ui.accel_label, LV_ALIGN_LEFT_MID, 0, 4);
    lv_obj_t *threshold = make_label(accel, "진입 1.50 이상");
    lv_obj_align(threshold, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_style(threshold, &style_label_micro, 0);
    lv_obj_t *decision = make_card(metrics, 158, 98, true);
    make_label(decision, "DECISION PATH");
    lv_obj_t *path = make_label(decision, "● ─ ○ ─ ○");
    lv_obj_align(path, LV_ALIGN_LEFT_MID, 0, 4);
    lv_obj_set_style_text_color(path, CANVIEW_COLOR_ACCENT, 0);
    lv_obj_t *path_caption = make_label(decision, "감시 → 조건 유지 → 제안");
    lv_obj_align(path_caption, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_style(path_caption, &style_label_micro, 0);

    lv_obj_t *diagnostics = make_card(screen, LV_PCT(100), 74, true);
    lv_obj_t *lock = make_label(diagnostics, "제어 잠금 정상");
    lv_obj_align(lock, LV_ALIGN_TOP_LEFT, 0, 4);
    ui.diagnostic_label = make_label(diagnostics, "암호화 — · TX OFF · RTT — ms");
    lv_obj_align(ui.diagnostic_label, LV_ALIGN_BOTTOM_LEFT, 0, -4);
    lv_obj_add_style(ui.diagnostic_label, &style_label_muted, 0);
}

static void create_nav(void)
{
    static const char *labels[CANVIEW_UI_SCREEN_COUNT] = {"주행", "오디오", "자동"};
    static const action_t actions[CANVIEW_UI_SCREEN_COUNT] = {
        ACTION_NAV_DRIVE, ACTION_NAV_AUDIO, ACTION_NAV_AUTOMATION};

    lv_obj_t *nav = lv_obj_create(ui.root);
    lv_obj_remove_style_all(nav);
    lv_obj_add_style(nav, &style_nav, LV_PART_MAIN);
    lv_obj_set_pos(nav, 0, 404);
    lv_obj_set_size(nav, 320, CANVIEW_TOUCH_PRIMARY);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);

    for (int i = 0; i < CANVIEW_UI_SCREEN_COUNT; ++i) {
        lv_obj_t *button = make_button(nav, 106, CANVIEW_TOUCH_PRIMARY, labels[i], actions[i]);
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

static const char *quality_text(canview_ui_quality_t quality)
{
    switch (quality) {
    case CANVIEW_UI_QUALITY_CANDIDATE:
        return "후륜 결합 · 후보";
    case CANVIEW_UI_QUALITY_ESTIMATED:
        return "후륜 결합 · 추정";
    case CANVIEW_UI_QUALITY_VERIFIED:
        return "후륜 결합 · 확인";
    case CANVIEW_UI_QUALITY_UNAVAILABLE:
    default:
        return "후륜 결합 · 없음";
    }
}

static const char *drive_mode_text(canview_ui_drive_mode_t mode)
{
    switch (mode) {
    case CANVIEW_UI_DRIVE_NORMAL:
        return "NORMAL";
    case CANVIEW_UI_DRIVE_SPORT:
        return "SPORT";
    case CANVIEW_UI_DRIVE_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

lv_obj_t *canview_ui_create(lv_obj_t *parent, const canview_ui_config_t *config)
{
    memset(&ui, 0, sizeof(ui));
    if (config != NULL) {
        ui.config = *config;
    }
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
    create_automation_screen();
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

    lv_label_set_text_fmt(ui.speed_label, "%u km/h", model->speed_tenth_kph / 10U);
    lv_label_set_text_fmt(ui.rpm_label, "%u rpm", model->engine_rpm);
    lv_bar_set_value(ui.rpm_bar, model->engine_rpm, LV_ANIM_OFF);
    lv_arc_set_value(ui.coupling_arc, model->rear_coupling_percent);
    lv_label_set_text_fmt(ui.coupling_label, "%u%%", model->rear_coupling_percent);
    lv_label_set_text(ui.coupling_quality_label, quality_text(model->four_wd_quality));
    lv_label_set_text_fmt(ui.clutch_torque_label, "%u Nm", model->clutch_torque_nm);
    lv_label_set_text(ui.drive_mode_label, drive_mode_text(model->drive_mode));

    if (model->dpf_lamp_on) {
        lv_label_set_text(ui.dpf_label, "DPF 확인 필요");
        lv_label_set_text(ui.dpf_badge, "경고");
        lv_obj_set_style_text_color(ui.dpf_badge, CANVIEW_COLOR_WARNING, 0);
    } else {
        lv_label_set_text(ui.dpf_label, "DPF 정상");
        lv_label_set_text(ui.dpf_badge, "정상");
        lv_obj_set_style_text_color(ui.dpf_badge, CANVIEW_COLOR_ACCENT, 0);
    }
    lv_label_set_text(ui.dpf_detail_label,
                      model->dpf_detail_verified ? "상세 센서 확인됨" : "경고등 기준 · 세부 센서 미검증");

    set_checked(ui.quiet_button, model->quiet_mode_enabled);
    set_checked(ui.rear_button, model->rear_boost_enabled);
    set_checked(ui.adaptive_button, model->adaptive_volume_enabled);
    lv_label_set_text(ui.adaptive_button_label, model->adaptive_volume_enabled ? "사용 중" : "꺼짐");
    lv_label_set_text_fmt(ui.noise_label, "상대 %u dB", model->relative_noise_db);
    lv_label_set_text_fmt(ui.volume_offset_label, "음량 %+d", model->volume_offset_step);

    int32_t focus_x = 48 + (int32_t)model->sound_balance_step * 4;
    int32_t focus_y = 78 - (int32_t)model->sound_fader_step * 4;
    focus_x = LV_CLAMP(24, focus_x, 88);
    focus_y = LV_CLAMP(24, focus_y, 100);
    lv_obj_set_pos(ui.sound_focus, (lv_coord_t)focus_x, (lv_coord_t)focus_y);

    set_checked(ui.sport_button, model->sport_monitor_enabled);
    lv_label_set_text(ui.sport_button_label, model->sport_monitor_enabled ? "감시 중" : "사용 안 함");
    lv_arc_set_value(ui.sport_dial, model->drive_mode == CANVIEW_UI_DRIVE_SPORT ? 2 : 1);
    lv_label_set_text(ui.sport_mode_label, drive_mode_text(model->drive_mode));
    int32_t accel_abs = model->longitudinal_accel_centi_mps2;
    const char *accel_sign = "";
    if (accel_abs < 0) {
        accel_sign = "-";
        accel_abs = -accel_abs;
    }
    lv_label_set_text_fmt(ui.accel_label, "%s%ld.%02ld m/s²", accel_sign,
                          (long)(accel_abs / 100), (long)(accel_abs % 100));
    lv_label_set_text_fmt(ui.diagnostic_label, "%s · TX %s · RTT %u ms",
                          model->esp_now_encrypted ? "암호화" : "평문/대기",
                          model->control_tx_enabled ? "ON" : "OFF",
                          model->esp_now_rtt_ms);
}
