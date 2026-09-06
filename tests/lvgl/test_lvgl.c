#include <assert.h>
#include <stdio.h>
#include <string.h>
#if defined(_MSC_VER)
#include <crtdbg.h>
#include <windows.h>
#endif

/* 실제 adapter의 내부 객체를 검사한다. LVGL은 공식 소스 전체를 별도 링크한다. */
#include "../../ui/lvgl/canview_ui.c"

_Static_assert(LVGL_VERSION_MAJOR == 8 && LVGL_VERSION_MINOR == 4 &&
               LVGL_VERSION_PATCH == 0, "LVGL 8.4.0 required");

static lv_color_t draw_pixels[320U * 20U];
static lv_disp_draw_buf_t draw_buffer;
static lv_disp_drv_t display_driver;
static lv_color_t frame_pixels[320U * 480U];
static uint32_t commands[CANVIEW_UI_CMD_COUNT];
static canview_ui_command_t last_command;
static bool delete_on_command;
static bool confirm_on_command;
static void advance(uint32_t milliseconds);
static lv_point_t pointer_point;
static bool pointer_pressed;
static lv_indev_drv_t pointer_driver;

static void read_pointer(lv_indev_drv_t *driver, lv_indev_data_t *data)
{
    (void)driver;
    data->point = pointer_point;
    data->state = pointer_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void command_sink(const canview_ui_command_t *command, void *context)
{
    (void)context;
    assert(command != NULL);
    assert((uint32_t)command->id < CANVIEW_UI_CMD_COUNT);
    ++commands[command->id];
    last_command = *command;
    if (delete_on_command) {
        canview_ui_destroy();
    } else if (confirm_on_command && command->id == CANVIEW_UI_CMD_SET_QUIET) {
        canview_ui_model_t model = ui.model;
        model.quiet_mode_enabled = command->value.enabled;
        canview_ui_update(&model);
        canview_ui_command_complete(command->id);
    }
}

static canview_ui_model_t fixture(void)
{
    canview_ui_model_t model = {0};
    model.speed_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.engine_rpm_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.engine_rpm = 6500U;
    model.battery_voltage_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.battery_voltage_tenth_v = 320U;
    model.transmission_clutch_lock_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.engine_temperature_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.engine_temperature_c = -40;
    model.fuel_economy_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.instant_fuel_economy_tenth_kmpl = 999U;
    model.four_wd_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.rear_coupling_percent = 100U;
    model.tire_pressure_quality = CANVIEW_UI_QUALITY_VERIFIED;
    for (uint32_t i = 0U; i < CANVIEW_UI_WHEEL_COUNT; ++i) {
        model.wheel_drive_percent[i] = 100U;
        model.tire_pressure_tenth_psi[i] = 999U;
    }
    model.dpf_lamp_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.dpf_load_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.dpf_load_percent = 100U;
    model.drive_mode_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.drive_mode = CANVIEW_UI_DRIVE_COMFORT;
    model.sport_previous_mode_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.sport_previous_mode = CANVIEW_UI_DRIVE_ECO;
    model.audio_volume_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.audio_volume_level = 100U;
    model.audio_profile_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.volume_offset_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.volume_offset_step = 4;
    model.fft_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.fft_peak_hz = 7999U;
    model.fft_peak_tenth_db = -1;
    for (uint32_t i = 0U; i < CANVIEW_UI_FFT_BIN_COUNT; ++i) {
        model.fft_bins[i] = (uint8_t)(100U - i * 4U);
    }
    model.settings_valid = true;
    model.settings_edit_allowed = true;
    model.audio_control_allowed = true;
    model.display_brightness_percent = 42U;
    model.adaptive_noise_band = CANVIEW_UI_NOISE_BAND_BALANCED;
    model.adaptive_sensitivity = CANVIEW_UI_SENSITIVITY_NORMAL;
    model.adaptive_response = CANVIEW_UI_RESPONSE_NORMAL;
    model.adaptive_max_offset_steps = 4U;
    model.sport_entry_speed_kph = 70U;
    model.idle_timeout_seconds = 30U;
    model.rtc_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.rtc_source = CANVIEW_UI_TIME_RTC;
    model.rtc_year = 2024U;
    model.rtc_month = 2U;
    model.rtc_day = 29U;
    model.rtc_hour = 23U;
    model.rtc_minute = 59U;
    return model;
}

static void flush_display(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels)
{
    for (int32_t y = area->y1; y <= area->y2; ++y) {
        for (int32_t x = area->x1; x <= area->x2; ++x) {
            assert(x >= 0 && x < 320 && y >= 0 && y < 480);
            frame_pixels[y * 320 + x] = *pixels++;
        }
    }
    lv_disp_flush_ready(driver);
}

static void event_send(lv_obj_t *object, lv_event_code_t code)
{
    const lv_res_t result = lv_event_send(object, code, NULL);
    assert(result == LV_RES_OK || (delete_on_command && result == LV_RES_INV));
}

static void assert_text(lv_obj_t *label, const char *expected)
{
    if (strcmp(lv_label_get_text(label), expected) != 0) {
        fprintf(stderr, "expected [%s], received [%s]\n", expected, lv_label_get_text(label));
        abort();
    }
}

static void assert_no_intersection(lv_obj_t *a, lv_obj_t *b)
{
    lv_area_t first;
    lv_area_t second;
    lv_obj_get_coords(a, &first);
    lv_obj_get_coords(b, &second);
    if (!(first.x2 < second.x1 || second.x2 < first.x1 ||
          first.y2 < second.y1 || second.y2 < first.y1)) {
        fprintf(stderr, "overlap: [%s] (%d,%d,%d,%d), [%s] (%d,%d,%d,%d)\n",
            lv_label_get_text(a), first.x1, first.y1, first.x2, first.y2,
            lv_label_get_text(b), second.x1, second.y1, second.x2, second.y2);
        abort();
    }
}

static void check_labels(lv_obj_t *parent)
{
    const uint32_t count = lv_obj_get_child_cnt(parent);
    lv_area_t container;
    lv_obj_get_coords(parent, &container);
    for (uint32_t i = 0U; i < count; ++i) {
        lv_obj_t *child = lv_obj_get_child(parent, (int32_t)i);
        if (lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) {
            continue;
        }
        if (lv_obj_check_type(child, &lv_label_class)) {
            lv_area_t area;
            lv_obj_get_coords(child, &area);
            /* scroll viewport 바깥의 설정 행은 허용하되 행 내부의 텍스트 잘림은 금지한다. */
            if (!(area.x1 >= container.x1 && area.x2 <= container.x2 &&
                  (lv_obj_has_flag(parent, LV_OBJ_FLAG_SCROLLABLE) ||
                   (area.y1 >= container.y1 && area.y2 <= container.y2)))) {
                fprintf(stderr, "clipped [%s] (%d,%d,%d,%d) parent (%d,%d,%d,%d)\n",
                    lv_label_get_text(child), area.x1, area.y1, area.x2, area.y2,
                    container.x1, container.y1, container.x2, container.y2);
                abort();
            }
            for (uint32_t j = i + 1U; j < count; ++j) {
                lv_obj_t *other = lv_obj_get_child(parent, (int32_t)j);
                if (lv_obj_check_type(other, &lv_label_class) &&
                    !lv_obj_has_flag(other, LV_OBJ_FLAG_HIDDEN)) {
                    assert_no_intersection(child, other);
                }
            }
        }
        check_labels(child);
    }
}

static void test_layout(void)
{
    canview_ui_model_t model = fixture();
    model.speed_tenth_kph = 2400U;
    model.fft_peak_tenth_db = -1600;
    canview_ui_update(&model);
    for (uint32_t screen = 0U; screen < CANVIEW_UI_SCREEN_COUNT; ++screen) {
        canview_ui_show_screen((canview_ui_screen_t)screen);
        advance(200U);
        lv_obj_update_layout(ui.root);
        check_labels(ui.screens[screen]);
        if (screen != CANVIEW_UI_SCREEN_SETTINGS) {
            assert(lv_obj_get_scroll_bottom(ui.screens[screen]) == 0);
            assert(lv_obj_get_scroll_right(ui.screens[screen]) == 0);
        }
    }
    check_labels(lv_obj_get_parent(ui.header_speed_label));
    assert(lv_obj_get_height(lv_obj_get_child(ui.screens[CANVIEW_UI_SCREEN_DRIVE], 0)) >= 250);
    assert(lv_obj_get_height(ui.speed_arc) == 56);
    assert(lv_obj_get_height(ui.rpm_arc) == 56);
    lv_obj_t *main_gauges = lv_obj_get_parent(lv_obj_get_parent(ui.rpm_arc));
    lv_obj_t *main_aux = lv_obj_get_parent(ui.drive_aux_label);
    assert(lv_obj_get_parent(main_aux) == main_gauges);
    assert(lv_obj_get_parent(main_gauges) == ui.screens[CANVIEW_UI_SCREEN_DRIVE]);
    lv_area_t rpm_area;
    lv_area_t aux_area;
    lv_obj_get_coords(ui.rpm_arc, &rpm_area);
    lv_obj_get_coords(main_aux, &aux_area);
    assert(aux_area.x1 > rpm_area.x2);
    assert_text(ui.drive_aux_label, "BATT 32.0 V\nLOCK OFF\nTEMP -40 °C");
    assert_text(ui.automation_aux_label, lv_label_get_text(ui.drive_aux_label));
    assert_text(ui.fft_speed_label, "240 km/h");
    assert_text(ui.fft_rpm_label, "6500 RPM");
    lv_area_t chart_area;
    lv_area_t caption_area;
    lv_obj_get_coords(ui.fft_chart, &chart_area);
    lv_obj_get_coords(ui.fft_status_label, &caption_area);
    assert(chart_area.y1 >= caption_area.y2 + 6);
    puts("PASS: five-screen bounds, numeric/unit separation, 250px AWD, 56px gauges, FFT telemetry");
}

static void test_quality(void)
{
    canview_ui_model_t model = fixture();
    canview_ui_update(&model);
    assert_text(ui.audio_peak_level_label, "-0.1 dBFS");
    model.speed_quality = CANVIEW_UI_QUALITY_STALE;
    model.four_wd_quality = CANVIEW_UI_QUALITY_ESTIMATED;
    model.fuel_economy_quality = CANVIEW_UI_QUALITY_CANDIDATE;
    model.tire_pressure_tenth_psi[0] = 0U;
    model.tire_pressure_unavailable_mask = 2U;
    model.dpf_load_quality = CANVIEW_UI_QUALITY_STALE;
    model.battery_voltage_quality = CANVIEW_UI_QUALITY_CANDIDATE;
    model.transmission_clutch_lock_quality = CANVIEW_UI_QUALITY_STALE;
    canview_ui_update(&model);
    assert_text(ui.header_speed_label, "—");
    assert_text(ui.fft_speed_label, "— km/h");
    assert_text(ui.fft_rpm_label, "6500 RPM");
    assert_text(ui.wheel_drive_labels[0], "—%");
    assert_text(ui.wheel_pressure_labels[0], "0.0 psi");
    assert_text(ui.wheel_pressure_labels[1], "—");
    assert_text(ui.wheel_pressure_labels[2], "99.9 psi");
    assert_text(ui.instant_economy_label, "—");
    assert_text(ui.dpf_label, "");
    assert_text(ui.dpf_load_label, "—%");
    assert_text(ui.drive_aux_label, "BATT —\nLOCK —\nTEMP -40 °C");
    assert_text(ui.automation_aux_label, lv_label_get_text(ui.drive_aux_label));
    assert(lv_anim_get(ui.speed_arc, arc_animation_exec) == NULL);
    assert(lv_anim_get(ui.dpf_load_bar, bar_animation_exec) == NULL);
    advance(400U);
    assert(lv_arc_get_value(ui.speed_arc) == 0);
    assert(lv_bar_get_value(ui.dpf_load_bar) == 0);

    model = fixture();
    model.fft_quality = CANVIEW_UI_QUALITY_STALE;
    model.audio_volume_quality = CANVIEW_UI_QUALITY_CANDIDATE;
    model.volume_offset_quality = CANVIEW_UI_QUALITY_STALE;
    model.drive_mode_quality = CANVIEW_UI_QUALITY_ESTIMATED;
    model.sport_previous_mode_quality = CANVIEW_UI_QUALITY_STALE;
    canview_ui_update(&model);
    assert_text(ui.volume_label, "—");
    assert_text(ui.volume_offset_label, "—");
    assert_text(ui.drive_mode_label, "—");
    assert_text(ui.sport_mode_label, "—");
    assert_text(ui.sport_return_label, "—");
    assert_text(ui.fft_peak_frequency_label, "— Hz");
    assert_text(ui.fft_rpm_label, "6500 RPM");
    for (uint32_t i = 0U; i < CANVIEW_UI_FFT_BIN_COUNT; ++i) {
        assert(ui.fft_series->y_points[i] == LV_CHART_POINT_NONE);
        assert(ui.audio_fft_series->y_points[i] == LV_CHART_POINT_NONE);
    }
    puts("PASS: independent quality/stale, zero pressure, immediate invalidation and animation cancellation");
}

static void test_dbfs(void)
{
    static const int16_t levels[] = {-1600, -246, -1, 0, 1, -1601, INT16_MIN, INT16_MAX};
    static const char *expected[] = {
        "-160.0 dBFS", "-24.6 dBFS", "-0.1 dBFS", "0.0 dBFS",
        "— dBFS", "— dBFS", "— dBFS", "— dBFS"
    };
    canview_ui_model_t model = fixture();
    for (uint32_t i = 0U; i < sizeof(levels) / sizeof(levels[0]); ++i) {
        model.fft_peak_tenth_db = levels[i];
        canview_ui_update(&model);
        assert_text(ui.audio_peak_level_label, expected[i]);
        assert_text(ui.fft_peak_level_label, expected[i]);
    }
    model.fft_quality = CANVIEW_UI_QUALITY_STALE;
    model.fft_peak_tenth_db = -246;
    canview_ui_update(&model);
    assert_text(ui.fft_peak_level_label, "— dBFS");
    puts("PASS: signed digital dBFS units, fractional negatives, full-scale/range/stale boundaries");
}

static void test_dpf_lamp_evidence(void)
{
    canview_ui_model_t model = fixture();
    canview_ui_update(&model);
    assert_text(ui.dpf_label, "");
    assert_text(ui.dpf_load_label, "100%");
    model.dpf_lamp_on = true;
    canview_ui_update(&model);
    assert_text(ui.dpf_label, "확인");
    model.dpf_lamp_quality = CANVIEW_UI_QUALITY_STALE;
    canview_ui_update(&model);
    assert_text(ui.dpf_label, "—");
    assert_text(ui.dpf_load_label, "100%");
    model.dpf_lamp_on = false;
    model.dpf_lamp_quality = CANVIEW_UI_QUALITY_UNAVAILABLE;
    canview_ui_update(&model);
    assert_text(ui.dpf_label, "—");
    model.dpf_lamp_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.dpf_load_quality = CANVIEW_UI_QUALITY_UNAVAILABLE;
    canview_ui_update(&model);
    assert_text(ui.dpf_label, "");
    assert_text(ui.dpf_load_label, "—%");
    puts("PASS: DPF lamp off has no status claim; on/invalid and load remain independent");
}

static void test_invalid_model(void)
{
    canview_ui_model_t model = fixture();
    model.speed_tenth_kph = UINT16_MAX;
    model.engine_rpm = UINT16_MAX;
    model.drive_mode = (canview_ui_drive_mode_t)-1;
    model.sport_previous_mode = (canview_ui_drive_mode_t)999;
    model.adaptive_noise_band = (canview_ui_noise_band_t)-1;
    model.adaptive_sensitivity = (canview_ui_sensitivity_t)65536;
    model.adaptive_response = (canview_ui_response_t)999;
    model.adaptive_max_offset_steps = UINT8_MAX;
    model.sport_entry_speed_kph = 75U;
    model.display_brightness_percent = UINT8_MAX;
    model.idle_timeout_seconds = 31U;
    model.rtc_source = (canview_ui_time_source_t)-1;
    model.fft_bins[3] = 101U;
    model.tire_pressure_tenth_psi[3] = UINT16_MAX;
    model.wheel_drive_percent[2] = 101U;
    model.audio_volume_level = UINT8_MAX;
    canview_ui_update(&model);
    assert_text(ui.header_speed_label, "—");
    assert_text(ui.rpm_label, "—");
    assert_text(ui.drive_mode_label, "—");
    assert_text(ui.brightness_label, "—%");
    assert_text(ui.volume_label, "—");
    assert_text(ui.wheel_pressure_labels[3], "—");
    assert_text(ui.wheel_drive_labels[2], "—%");
    for (uint32_t i = 0U; i < CANVIEW_UI_SETTING_DROPDOWNS; ++i) {
        assert(lv_dropdown_get_selected(ui.setting_dropdowns[i]) == 3U);
    }
    assert(lv_dropdown_get_selected(ui.idle_timeout_dropdown) == 4U);
    assert(lv_dropdown_get_selected(ui.rtc_minute_dropdown) == 60U);
    assert(strstr(lv_label_get_text(ui.rtc_time_label), "invalid") != NULL);
    const canview_ui_screen_t screen = canview_ui_current_screen();
    canview_ui_show_screen((canview_ui_screen_t)-1);
    assert(canview_ui_current_screen() == screen);
    canview_ui_show_screen(CANVIEW_UI_SCREEN_COUNT);
    assert(canview_ui_current_screen() == screen);
    canview_ui_update(NULL);
    static const int32_t invalid_qualities[] = {-1, 0, 1, 2, 4, 5, 65536};
    for (uint32_t i = 0U; i < sizeof(invalid_qualities) / sizeof(invalid_qualities[0]); ++i) {
        model = fixture();
        model.speed_quality = (canview_ui_quality_t)invalid_qualities[i];
        model.engine_rpm_quality = (canview_ui_quality_t)invalid_qualities[i];
        model.fft_quality = (canview_ui_quality_t)invalid_qualities[i];
        canview_ui_update(&model);
        assert_text(ui.header_speed_label, "—");
        assert_text(ui.rpm_label, "—");
        assert_text(ui.fft_peak_frequency_label, "— Hz");
    }
    puts("PASS: invalid enum/range rejection, no wrapped enum index or plausible clamped reading");
}

static void test_commands_and_rtc(void)
{
    canview_ui_model_t model = fixture();
    canview_ui_update(&model);
    assert(lv_dropdown_get_selected(ui.rtc_minute_dropdown) == 59U);
    for (uint16_t minute = 0U; minute < 60U; ++minute) {
        model.rtc_minute = (uint8_t)minute;
        canview_ui_update(&model);
        assert(lv_dropdown_get_selected(ui.rtc_minute_dropdown) == minute);
    }
    const uint32_t quiet_before = commands[CANVIEW_UI_CMD_SET_QUIET];
    event_send(ui.quiet_button, LV_EVENT_CLICKED);
    event_send(ui.quiet_button, LV_EVENT_CLICKED);
    assert(commands[CANVIEW_UI_CMD_SET_QUIET] == quiet_before + 1U);
    assert(last_command.value.enabled);
    assert(!ui.model.quiet_mode_enabled);
    assert(!lv_obj_has_state(ui.quiet_button, LV_STATE_CHECKED));
    canview_ui_update(&model);
    assert(commands[CANVIEW_UI_CMD_SET_QUIET] == quiet_before + 1U);
    canview_ui_command_complete(CANVIEW_UI_CMD_SET_QUIET);
    assert(!lv_obj_has_state(ui.quiet_button, LV_STATE_CHECKED));
    confirm_on_command = true;
    event_send(ui.quiet_button, LV_EVENT_CLICKED);
    confirm_on_command = false;
    assert(lv_obj_has_state(ui.quiet_button, LV_STATE_CHECKED));
    assert(!ui.pending[CANVIEW_UI_CMD_SET_QUIET]);

    const uint32_t setting_before = commands[CANVIEW_UI_CMD_SET_ADAPTIVE_NOISE_BAND];
    lv_dropdown_set_selected(ui.setting_dropdowns[0], 2U);
    event_send(ui.setting_dropdowns[0], LV_EVENT_VALUE_CHANGED);
    assert(commands[CANVIEW_UI_CMD_SET_ADAPTIVE_NOISE_BAND] == setting_before + 1U);
    assert(last_command.value.option == 2U);
    assert(lv_dropdown_get_selected(ui.setting_dropdowns[0]) == 1U);
    assert(ui.model.adaptive_noise_band == CANVIEW_UI_NOISE_BAND_BALANCED);
    canview_ui_command_complete(CANVIEW_UI_CMD_SET_ADAPTIVE_NOISE_BAND);

    const uint32_t rtc_before = commands[CANVIEW_UI_CMD_SET_RTC_DATETIME];
    lv_dropdown_set_selected(ui.rtc_year_dropdown, 23U);
    event_send(ui.rtc_year_dropdown, LV_EVENT_VALUE_CHANGED);
    event_send(ui.rtc_apply_button, LV_EVENT_CLICKED);
    assert(commands[CANVIEW_UI_CMD_SET_RTC_DATETIME] == rtc_before);
    assert_text(lv_obj_get_child(ui.rtc_apply_button, 0), "날짜 확인");
    lv_dropdown_set_selected(ui.rtc_day_dropdown, 27U);
    event_send(ui.rtc_day_dropdown, LV_EVENT_VALUE_CHANGED);
    lv_dropdown_set_selected(ui.rtc_minute_dropdown, 17U);
    event_send(ui.rtc_minute_dropdown, LV_EVENT_VALUE_CHANGED);
    canview_ui_update(&model);
    assert(lv_dropdown_get_selected(ui.rtc_minute_dropdown) == 17U);
    event_send(ui.rtc_apply_button, LV_EVENT_CLICKED);
    assert(commands[CANVIEW_UI_CMD_SET_RTC_DATETIME] == rtc_before + 1U);
    assert(last_command.value.datetime.year == 2023U);
    assert(last_command.value.datetime.day == 28U);
    assert(last_command.value.datetime.minute == 17U);
    assert(ui.model.rtc_year == 2024U);
    event_send(ui.rtc_apply_button, LV_EVENT_CLICKED);
    assert(commands[CANVIEW_UI_CMD_SET_RTC_DATETIME] == rtc_before + 1U);
    canview_ui_command_complete(CANVIEW_UI_CMD_SET_RTC_DATETIME);
    assert(lv_dropdown_get_selected(ui.rtc_minute_dropdown) == model.rtc_minute);

    const canview_ui_datetime_t leap = {.year = 2000U, .month = 2U, .day = 29U};
    assert(datetime_valid(&leap));
    model.rtc_year = 2099U;
    model.rtc_month = 4U;
    model.rtc_day = 31U;
    canview_ui_update(&model);
    assert(!rtc_valid());
    assert(lv_dropdown_get_selected(ui.rtc_day_dropdown) == 31U);
    puts("PASS: all 60 RTC minutes, Gregorian dates, draft preservation, semantic requests, feedback-only state");
}

static void test_driving_lock(void)
{
    canview_ui_model_t model = fixture();
    canview_ui_update(&model);
    canview_ui_show_screen(CANVIEW_UI_SCREEN_SETTINGS);
    lv_dropdown_open(ui.rtc_minute_dropdown);
    assert(lv_dropdown_is_open(ui.rtc_minute_dropdown));
    model.speed_tenth_kph = 1U;
    canview_ui_update(&model);
    assert(!lv_dropdown_is_open(ui.rtc_minute_dropdown));
    assert(lv_obj_has_state(ui.brightness_slider, LV_STATE_DISABLED));
    assert(lv_obj_has_state(ui.sport_button, LV_STATE_DISABLED));
    const uint32_t before = commands[CANVIEW_UI_CMD_SET_SPORT_MONITOR];
    event_send(ui.sport_button, LV_EVENT_CLICKED);
    assert(commands[CANVIEW_UI_CMD_SET_SPORT_MONITOR] == before);
    lv_dropdown_set_selected(ui.setting_dropdowns[4], 2U);
    event_send(ui.setting_dropdowns[4], LV_EVENT_VALUE_CHANGED);
    assert(commands[CANVIEW_UI_CMD_SET_SPORT_ENTRY_SPEED] == 0U);
    model.speed_tenth_kph = 0U;
    model.speed_quality = CANVIEW_UI_QUALITY_STALE;
    canview_ui_update(&model);
    assert(lv_obj_has_state(ui.rtc_apply_button, LV_STATE_DISABLED));
    puts("PASS: moving/stale-speed editing lock, open-list closure, event-side policy guard");
}

static void test_overlays_and_idle(void)
{
    canview_ui_model_t model = fixture();
    canview_ui_update(&model);
    assert(lv_obj_has_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_HIDDEN));
    model.speed_limit_quality = CANVIEW_UI_QUALITY_VERIFIED;
    model.speed_limit_active = true;
    model.speed_limit_kph = 110U;
    model.speed_limit_warning_active = true;
    model.speed_limit_warning_visible = true;
    model.headlamp_warning_active = true;
    canview_ui_update(&model);
    for (uint32_t screen = 0U; screen < CANVIEW_UI_SCREEN_COUNT; ++screen) {
        canview_ui_show_screen((canview_ui_screen_t)screen);
        lv_obj_update_layout(ui.root);
        assert(lv_obj_get_width(ui.speed_limit_overlay) == 128);
        assert(lv_obj_get_x(ui.speed_limit_overlay) == 96);
        assert(lv_obj_get_style_opa(ui.speed_limit_overlay, 0) ==
               (screen == 0U ? LV_OPA_COVER : LV_OPA_60));
        assert(lv_obj_has_flag(ui.headlamp_warning_overlay, LV_OBJ_FLAG_HIDDEN));
        lv_point_t point = {.x = 160, .y = 220};
        lv_obj_t *hit = lv_indev_search_obj(lv_scr_act(), &point);
        assert(hit != ui.speed_limit_overlay && hit != ui.speed_limit_label);
        lv_obj_add_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_HIDDEN);
        assert(hit == lv_indev_search_obj(lv_scr_act(), &point));
        lv_obj_clear_flag(ui.speed_limit_overlay, LV_OBJ_FLAG_HIDDEN);
        const uint32_t activities = commands[CANVIEW_UI_CMD_USER_ACTIVITY];
        event_send(ui.nav_buttons[screen], LV_EVENT_PRESSED);
        assert(commands[CANVIEW_UI_CMD_USER_ACTIVITY] == activities + 1U);
    }
    const uint32_t activities = commands[CANVIEW_UI_CMD_USER_ACTIVITY];
    lv_dropdown_open(ui.rtc_minute_dropdown);
    event_send(lv_dropdown_get_list(ui.rtc_minute_dropdown), LV_EVENT_PRESSED);
    assert(commands[CANVIEW_UI_CMD_USER_ACTIVITY] == activities + 1U);
    lv_dropdown_close(ui.rtc_minute_dropdown);
    canview_ui_show_screen(CANVIEW_UI_SCREEN_AUDIO);
    advance(200U);
    pointer_point.x = 280;
    pointer_point.y = 72;
    assert(lv_indev_search_obj(lv_scr_act(), &pointer_point) == ui.rear_button);
    const uint32_t rear_before = commands[CANVIEW_UI_CMD_SET_REAR_BOOST];
    const uint32_t activity_before = commands[CANVIEW_UI_CMD_USER_ACTIVITY];
    pointer_pressed = true;
    advance(50U);
    pointer_pressed = false;
    advance(50U);
    assert(commands[CANVIEW_UI_CMD_SET_REAR_BOOST] == rear_before + 1U);
    assert(commands[CANVIEW_UI_CMD_USER_ACTIVITY] == activity_before + 1U);
    assert(!lv_obj_has_state(ui.rear_button, LV_STATE_CHECKED));
    canview_ui_command_complete(CANVIEW_UI_CMD_SET_REAR_BOOST);
    canview_ui_show_screen(CANVIEW_UI_SCREEN_SETTINGS);
    lv_dropdown_open(ui.rtc_minute_dropdown);
    model.idle_dimmed = true;
    canview_ui_update(&model);
    assert(canview_ui_current_screen() == CANVIEW_UI_SCREEN_DRIVE);
    assert(!lv_dropdown_is_open(ui.rtc_minute_dropdown));
    canview_ui_show_screen(CANVIEW_UI_SCREEN_FFT);
    canview_ui_update(&model);
    assert(canview_ui_current_screen() == CANVIEW_UI_SCREEN_FFT);
    model.speed_quality = CANVIEW_UI_QUALITY_STALE;
    canview_ui_update(&model);
    lv_obj_update_layout(ui.root);
    assert(!lv_obj_has_flag(ui.headlamp_warning_overlay, LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_style_opa(ui.headlamp_warning_overlay, 0) == LV_OPA_60);
    assert(lv_obj_get_width(ui.headlamp_warning_overlay) == 128);
    assert(lv_obj_get_width(ui.speed_limit_overlay) == 36);
    assert(lv_obj_get_y(ui.speed_limit_overlay) == 0);
    assert(lv_obj_has_flag(lv_obj_get_parent(ui.link_label), LV_OBJ_FLAG_HIDDEN));
    model.speed_limit_active = false;
    canview_ui_update(&model);
    assert(!lv_obj_has_flag(lv_obj_get_parent(ui.link_label), LV_OBJ_FLAG_HIDDEN));
    puts("PASS: neutral default, warning priority, per-screen transparency/touch, single idle edge, popup activity");
}

static void test_mode_consistency(void)
{
    canview_ui_model_t model = fixture();
    for (uint32_t mode = CANVIEW_UI_DRIVE_NORMAL; mode <= CANVIEW_UI_DRIVE_SPORT; ++mode) {
        model.drive_mode = (canview_ui_drive_mode_t)mode;
        canview_ui_update(&model);
        assert_text(ui.drive_mode_label, drive_mode_text(model.drive_mode));
        assert_text(ui.sport_mode_label, drive_mode_text(model.drive_mode));
        assert(lv_color_to32(lv_obj_get_style_text_color(ui.drive_mode_label, 0)) ==
               lv_color_to32(lv_obj_get_style_text_color(ui.sport_mode_label, 0)));
    }
    puts("PASS: current mode and color shared by drive/automation screens");
}

static void write_u32(uint8_t *destination, uint32_t value)
{
    for (uint32_t byte = 0U; byte < 4U; ++byte) {
        destination[byte] = (uint8_t)(value >> (8U * byte));
    }
}

static void write_snapshot(const char *name)
{
    uint8_t header[54] = {0};
    header[0] = 'B';
    header[1] = 'M';
    write_u32(&header[2], 54U + 320U * 480U * 3U);
    write_u32(&header[10], 54U);
    write_u32(&header[14], 40U);
    write_u32(&header[18], 320U);
    write_u32(&header[22], 480U);
    header[26] = 1U;
    header[28] = 24U;
    FILE *output = fopen(name, "wb");
    assert(output != NULL);
    assert(fwrite(header, sizeof(header), 1U, output) == 1U);
    for (int32_t y = 479; y >= 0; --y) {
        for (uint32_t x = 0U; x < 320U; ++x) {
            const lv_color_t pixel = frame_pixels[(uint32_t)y * 320U + x];
            const uint8_t bgr[] = {pixel.ch.blue, pixel.ch.green, pixel.ch.red};
            assert(fwrite(bgr, sizeof(bgr), 1U, output) == 1U);
        }
    }
    assert(fclose(output) == 0);
}

static void snapshots(void)
{
    static const char *names[] = {"drive.bmp", "audio.bmp", "fft.bmp", "automation.bmp", "settings.bmp"};
    canview_ui_model_t model = fixture();
    model.speed_tenth_kph = 830U;
    model.engine_rpm = 2100U;
    model.instant_fuel_economy_tenth_kmpl = 137U;
    model.drive_mode = CANVIEW_UI_DRIVE_NORMAL;
    model.fft_peak_hz = 1250U;
    model.fft_peak_tenth_db = -246;
    for (uint32_t bin = 0U; bin < CANVIEW_UI_FFT_BIN_COUNT; ++bin) {
        const uint32_t distance = bin > 14U ? bin - 14U : 14U - bin;
        model.fft_bins[bin] = (uint8_t)(100U - distance * 5U);
    }
    for (uint32_t i = 0U; i < 4U; ++i) {
        model.tire_pressure_tenth_psi[i] = 342U;
        model.wheel_drive_percent[i] = 30U;
    }
    canview_ui_update(&model);
    for (uint32_t screen = 0U; screen < CANVIEW_UI_SCREEN_COUNT; ++screen) {
        canview_ui_show_screen((canview_ui_screen_t)screen);
        advance(400U);
        write_snapshot(names[screen]);
    }
}

static void test_lifecycle(const canview_ui_config_t *config)
{
    lv_obj_t *original = ui.root;
    assert(canview_ui_create(NULL, config) == original);
    canview_ui_destroy();
    advance(400U);
    assert(ui.root == NULL);
    assert(lv_anim_count_running() == 0U);
    canview_ui_destroy();
    canview_ui_update(NULL);
    lv_mem_monitor_t baseline;
    lv_mem_monitor(&baseline);
    for (uint32_t cycle = 0U; cycle < 100U; ++cycle) {
        lv_obj_t *parent = lv_obj_create(lv_scr_act());
        assert(canview_ui_create(parent, config) != NULL);
        canview_ui_model_t model = fixture();
        canview_ui_update(&model);
        for (uint32_t i = 0U; i < 20U; ++i) {
            canview_ui_show_screen((canview_ui_screen_t)(i % CANVIEW_UI_SCREEN_COUNT));
            model.speed_tenth_kph = (uint16_t)(i * 100U);
            model.dpf_load_percent = (uint8_t)(i * 5U);
            canview_ui_update(&model);
            lv_tick_inc(10U);
        }
        lv_dropdown_open(ui.rtc_minute_dropdown);
        if (cycle % 2U == 0U) {
            canview_ui_destroy();
        }
        lv_obj_del(parent);
        advance(400U);
        assert(ui.root == NULL);
        assert(lv_anim_count_running() == 0U);
        assert(lv_mem_test() == LV_RES_OK);
        lv_mem_monitor_t current;
        lv_mem_monitor(&current);
        assert(current.free_size == baseline.free_size);
    }
    assert(canview_ui_create(NULL, config) != NULL);
    canview_ui_model_t model = fixture();
    canview_ui_update(&model);
    delete_on_command = true;
    event_send(ui.quiet_button, LV_EVENT_CLICKED);
    delete_on_command = false;
    assert(ui.root == NULL);
    advance(400U);
    puts("PASS: 100 create/delete cycles, 2000 rapid tab transitions, parent deletion, zero heap growth, callback deletion");
}

static void advance(uint32_t milliseconds)
{
    for (uint32_t i = 0U; i < milliseconds; i += 10U) {
        lv_tick_inc(10U);
        (void)lv_timer_handler();
    }
}

int main(void)
{
#if defined(_MSC_VER)
    (void)_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    (void)_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    (void)_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    (void)SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    (void)setvbuf(stdout, NULL, _IONBF, 0U);
    lv_init();
    lv_disp_draw_buf_init(&draw_buffer, draw_pixels, NULL, 320U * 20U);
    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = 320;
    display_driver.ver_res = 480;
    display_driver.draw_buf = &draw_buffer;
    display_driver.flush_cb = flush_display;
    assert(lv_disp_drv_register(&display_driver) != NULL);
    lv_indev_drv_init(&pointer_driver);
    pointer_driver.type = LV_INDEV_TYPE_POINTER;
    pointer_driver.read_cb = read_pointer;
    assert(lv_indev_drv_register(&pointer_driver) != NULL);
    const canview_ui_config_t config = {.font = &lv_font_montserrat_14,
                                        .metric_font = &lv_font_montserrat_28,
                                        .command_cb = command_sink};
    assert(canview_ui_create(NULL, &config) != NULL);
    advance(200U);
    test_layout();
    test_quality();
    test_dbfs();
    test_dpf_lamp_evidence();
    test_invalid_model();
    test_commands_and_rtc();
    test_driving_lock();
    test_overlays_and_idle();
    test_mode_consistency();
    snapshots();
    test_lifecycle(&config);
    puts("PASS: official LVGL 8.4.0 host regression (board/font/PWM/vehicle gates not executed)");
    return 0;
}
