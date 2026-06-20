// Code-defined configuration screen (no longer maintained in SquareLine).
//
// Contents:
//   - RPM multiplier with -/+ buttons (tap = +-0.1, hold = +-1.0), persisted.
//   - Theme selector (whole main screens): NFS / Simson Clasic / Simson Clasic
//     Night. Only NFS exists today; the others are listed as "(wkrotce)" and
//     become active once their screens are added (see theme_screen()).
//   - OTA button.
//   - Long-press on an empty area returns to the active main screen.

#include "config_screen.h"
#include "ui.h"
#include "settings.h"
#include "gauge.h"
#include "ota.h"
#include "lcd_bsp.h"
#include "ui_SimsonScreen.h"

// LVGL's LV_PART_* | LV_STATE_* macros mix two enums; in C++ that triggers a
// deprecation warning on every style call. The generated UI (.c) files don't
// hit it; silence it for this C++ file.
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"

// ----------------------------------------------------------------------------
// Themes
// ----------------------------------------------------------------------------
#define THEME_COUNT 3
static const char * THEME_NAMES[THEME_COUNT] = {
    "NFS", "Simson Clasic", "Simson Clasic Night"
};

// Map a theme index to its main screen, or NULL if not implemented yet.
// When you add the Simson screens in SquareLine, return them here.
static lv_obj_t * theme_screen(int idx)
{
    switch (idx) {
        case 0: return ui_MainScreen;            // NFS (current)
        case 1: return ui_SimsonScreen;          // Simson Clasic (day)
        case 2: return ui_SimsonNightScreen;     // Simson Clasic Night
        default: return NULL;                    // not implemented yet
    }
}

// ----------------------------------------------------------------------------
// State
// ----------------------------------------------------------------------------
#define MULT_MIN        0.1f
#define MULT_MAX        1000.0f
#define MULT_STEP_TAP   0.1f
#define MULT_STEP_HOLD  1.0f
#define NAV_ANIM_MS     300

static lv_obj_t * s_config_scr  = NULL;
static lv_obj_t * s_active_main = NULL;   // current theme's main screen
static lv_obj_t * s_mult_label  = NULL;
static lv_obj_t * s_smult_label = NULL;
static lv_obj_t * s_theme_label = NULL;
static lv_obj_t * s_bright_label = NULL;

static float s_mult      = 1.0f;
static float s_smult     = 1.0f;
static int   s_theme_idx = 0;
static int   s_bright    = 100;

// ----------------------------------------------------------------------------
// Multiplier
// ----------------------------------------------------------------------------
static void mult_refresh_label(void)
{
    lv_label_set_text_fmt(s_mult_label, "%.1f", s_mult);
}

static void mult_change(float delta)
{
    s_mult += delta;
    if (s_mult < MULT_MIN) s_mult = MULT_MIN;
    if (s_mult > MULT_MAX) s_mult = MULT_MAX;
    mult_refresh_label();
    gauge_set_multiplier(s_mult);
}

static void mult_minus_cb(lv_event_t * e)
{
    switch (lv_event_get_code(e)) {
        case LV_EVENT_SHORT_CLICKED:        mult_change(-MULT_STEP_TAP);  break;
        case LV_EVENT_LONG_PRESSED_REPEAT:  mult_change(-MULT_STEP_HOLD); break;
        default: break;
    }
}

static void mult_plus_cb(lv_event_t * e)
{
    switch (lv_event_get_code(e)) {
        case LV_EVENT_SHORT_CLICKED:        mult_change(+MULT_STEP_TAP);  break;
        case LV_EVENT_LONG_PRESSED_REPEAT:  mult_change(+MULT_STEP_HOLD); break;
        default: break;
    }
}

// ----------------------------------------------------------------------------
// Speed Multiplier
// ----------------------------------------------------------------------------
static void smult_refresh_label(void)
{
    lv_label_set_text_fmt(s_smult_label, "%.1f", s_smult);
}

static void smult_change(float delta)
{
    s_smult += delta;
    if (s_smult < MULT_MIN) s_smult = MULT_MIN;
    if (s_smult > MULT_MAX) s_smult = MULT_MAX;
    smult_refresh_label();
    gauge_set_speed_multiplier(s_smult);
}

static void smult_minus_cb(lv_event_t * e)
{
    switch (lv_event_get_code(e)) {
        case LV_EVENT_SHORT_CLICKED:        smult_change(-MULT_STEP_TAP);  break;
        case LV_EVENT_LONG_PRESSED_REPEAT:  smult_change(-MULT_STEP_HOLD); break;
        default: break;
    }
}

static void smult_plus_cb(lv_event_t * e)
{
    switch (lv_event_get_code(e)) {
        case LV_EVENT_SHORT_CLICKED:        smult_change(+MULT_STEP_TAP);  break;
        case LV_EVENT_LONG_PRESSED_REPEAT:  smult_change(+MULT_STEP_HOLD); break;
        default: break;
    }
}

// ----------------------------------------------------------------------------
// Brightness
// ----------------------------------------------------------------------------
static void bright_refresh_label(void)
{
    lv_label_set_text_fmt(s_bright_label, "%d%%", s_bright);
}

static void bright_change(int delta)
{
    s_bright += delta;
    if (s_bright < 10) s_bright = 10;
    if (s_bright > 100) s_bright = 100;
    bright_refresh_label();
    lcd_set_brightness(s_bright * 255 / 100);
}

static void bright_minus_cb(lv_event_t * e)
{
    switch (lv_event_get_code(e)) {
        case LV_EVENT_SHORT_CLICKED:
        case LV_EVENT_LONG_PRESSED_REPEAT:
            bright_change(-10);
            break;
        default: break;
    }
}

static void bright_plus_cb(lv_event_t * e)
{
    switch (lv_event_get_code(e)) {
        case LV_EVENT_SHORT_CLICKED:
        case LV_EVENT_LONG_PRESSED_REPEAT:
            bright_change(+10);
            break;
        default: break;
    }
}

// ----------------------------------------------------------------------------
// Theme selection
// ----------------------------------------------------------------------------
static void theme_refresh_label(void)
{
    if (theme_screen(s_theme_idx)) {
        lv_label_set_text(s_theme_label, THEME_NAMES[s_theme_idx]);
    } else {
        lv_label_set_text_fmt(s_theme_label, "%s (wkrotce)", THEME_NAMES[s_theme_idx]);
    }
}

static void theme_apply(void)
{
    lv_obj_t * scr = theme_screen(s_theme_idx);
    if (scr) {
        lv_obj_remove_event_cb(s_active_main, to_config_cb);
        s_active_main = scr;
        settings_set_theme(s_theme_idx);
        lv_obj_add_event_cb(s_active_main, to_config_cb, LV_EVENT_LONG_PRESSED, NULL);
    }
    // not-implemented themes: just shown as a choice, active screen unchanged
}

static void theme_prev_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    s_theme_idx = (s_theme_idx + THEME_COUNT - 1) % THEME_COUNT;
    theme_refresh_label();
    theme_apply();
}

static void theme_next_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    s_theme_idx = (s_theme_idx + 1) % THEME_COUNT;
    theme_refresh_label();
    theme_apply();
}

// ----------------------------------------------------------------------------
// OTA
// ----------------------------------------------------------------------------
static lv_obj_t * make_button(lv_obj_t * parent, const char * text, int w, int h);  // defined below

// Leave OTA mode: ask the loop() task to tear the SoftAP down, then remove the
// overlay (async - we're inside the close button's own event).
static void ota_close_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ota_request_stop();
    lv_obj_del_async((lv_obj_t *)lv_event_get_user_data(e));
}

static void ota_show_info(void)
{
    lv_obj_t * box = lv_obj_create(lv_layer_top());
    lv_obj_set_size(box, 420, 420);
    lv_obj_center(box);
    lv_obj_set_style_radius(box, 210, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(box, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * lbl = lv_label_create(box);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, 320);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text_fmt(lbl,
                          "TRYB OTA\n\nWiFi: %s\nHaslo: %s\n\n%s\n\nWgraj plik .bin",
                          ota_ssid(), ota_pass(), ota_url());
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -35);

    // Exit OTA: stops the SoftAP + server and removes this overlay.
    lv_obj_t * close_btn = make_button(box, "Zamknij", 150, 46);
    lv_obj_align(close_btn, LV_ALIGN_CENTER, 0, 130);
    lv_obj_add_event_cb(close_btn, ota_close_cb, LV_EVENT_CLICKED, box);
}

static void ota_button_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ota_request();
    ota_show_info();
}

// ----------------------------------------------------------------------------
// Navigation (long-press toggles between main and config)
// ----------------------------------------------------------------------------
void to_config_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) {
        lv_scr_load_anim(s_config_scr, LV_SCR_LOAD_ANIM_FADE_ON, NAV_ANIM_MS, 0, false);
    }
}

static void to_main_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) {
        settings_set_multiplier(s_mult);
        settings_set_speed_multiplier(s_smult);
        settings_set_brightness(s_bright);
        lv_scr_load_anim(s_active_main, LV_SCR_LOAD_ANIM_FADE_ON, NAV_ANIM_MS, 0, false);
    }
}

// ----------------------------------------------------------------------------
// Small UI helpers
// ----------------------------------------------------------------------------
static lv_obj_t * make_caption(lv_obj_t * parent, const char * text)
{
    lv_obj_t * l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(0xAAAAAA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    return l;
}

// horizontal row container (transparent, content-height)
static lv_obj_t * make_row(lv_obj_t * parent)
{
    lv_obj_t * row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

static lv_obj_t * make_button(lv_obj_t * parent, const char * text, int w, int h)
{
    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_t * lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
    return btn;
}

// ----------------------------------------------------------------------------
// Build
// ----------------------------------------------------------------------------
void config_screen_init(void)
{
    // --- init additional screens ---
    ui_SimsonScreen_screen_init();

    // --- load persisted settings ---
    s_mult = settings_get_multiplier();
    gauge_set_multiplier(s_mult);

    s_smult = settings_get_speed_multiplier();
    gauge_set_speed_multiplier(s_smult);

    s_theme_idx = settings_get_theme();
    s_active_main = theme_screen(s_theme_idx);
    if (!s_active_main) {                 // persisted theme not available -> NFS
        s_theme_idx = 0;
        s_active_main = theme_screen(0);
    }

    s_bright = settings_get_brightness();

    // --- screen ---
    s_config_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_config_scr, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_config_scr, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(s_config_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_config_scr, to_main_cb, LV_EVENT_LONG_PRESSED, NULL);

    // centered column holding all settings (kept within the circle's safe area)
    lv_obj_t * col = lv_obj_create(s_config_scr);
    lv_obj_remove_style_all(col);
    lv_obj_set_width(col, 320);
    lv_obj_set_height(col, LV_SIZE_CONTENT);
    // Nudge up: the column nearly fills the panel, so centred it pushes the last
    // row (OTA) onto the round display's bottom edge where touch is unreliable.
    lv_obj_align(col, LV_ALIGN_CENTER, 0, -14);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(col, LV_OBJ_FLAG_EVENT_BUBBLE);   // long-press bubbles to screen

    // title
    lv_obj_t * title = lv_label_create(col);
    lv_label_set_text(title, "USTAWIENIA");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(title, 6, LV_PART_MAIN | LV_STATE_DEFAULT);

    // --- multiplier ---
    make_caption(col, "Mnoznik RPM");
    lv_obj_t * mult_row = make_row(col);

    lv_obj_t * btn_minus = make_button(mult_row, "-", 60, 50);
    lv_obj_add_event_cb(btn_minus, mult_minus_cb, LV_EVENT_ALL, NULL);

    s_mult_label = lv_label_create(mult_row);
    lv_obj_set_width(s_mult_label, 110);
    lv_obj_set_style_text_align(s_mult_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_mult_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_mult_label, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    mult_refresh_label();

    lv_obj_t * btn_plus = make_button(mult_row, "+", 60, 50);
    lv_obj_add_event_cb(btn_plus, mult_plus_cb, LV_EVENT_ALL, NULL);

    // --- speed multiplier ---
    make_caption(col, "Mnoznik Predkosci");
    lv_obj_t * smult_row = make_row(col);

    lv_obj_t * btn_sminus = make_button(smult_row, "-", 60, 50);
    lv_obj_add_event_cb(btn_sminus, smult_minus_cb, LV_EVENT_ALL, NULL);

    s_smult_label = lv_label_create(smult_row);
    lv_obj_set_width(s_smult_label, 110);
    lv_obj_set_style_text_align(s_smult_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_smult_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_smult_label, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    smult_refresh_label();

    lv_obj_t * btn_splus = make_button(smult_row, "+", 60, 50);
    lv_obj_add_event_cb(btn_splus, smult_plus_cb, LV_EVENT_ALL, NULL);

    // --- theme ---
    make_caption(col, "Motyw");
    lv_obj_t * theme_row = make_row(col);

    lv_obj_t * btn_prev = make_button(theme_row, "<", 50, 44);
    lv_obj_add_event_cb(btn_prev, theme_prev_cb, LV_EVENT_CLICKED, NULL);

    s_theme_label = lv_label_create(theme_row);
    lv_label_set_long_mode(s_theme_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_theme_label, 190);
    lv_obj_set_style_text_align(s_theme_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_theme_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_theme_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    theme_refresh_label();

    lv_obj_t * btn_next = make_button(theme_row, ">", 50, 44);
    lv_obj_add_event_cb(btn_next, theme_next_cb, LV_EVENT_CLICKED, NULL);

    // --- brightness ---
    make_caption(col, "Jasnosc ekranu");
    lv_obj_t * bright_row = make_row(col);

    lv_obj_t * btn_bminus = make_button(bright_row, "-", 60, 50);
    lv_obj_add_event_cb(btn_bminus, bright_minus_cb, LV_EVENT_ALL, NULL);

    s_bright_label = lv_label_create(bright_row);
    lv_obj_set_width(s_bright_label, 110);
    lv_obj_set_style_text_align(s_bright_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(s_bright_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s_bright_label, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    bright_refresh_label();

    lv_obj_t * btn_bplus = make_button(bright_row, "+", 60, 50);
    lv_obj_add_event_cb(btn_bplus, bright_plus_cb, LV_EVENT_ALL, NULL);

    // --- OTA --- bigger target + a forgiving touch zone (it sits low on the
    // round panel, where edge touches jitter and otherwise miss).
    lv_obj_t * ota_btn = make_button(col, "OTA", 160, 52);
    lv_obj_set_ext_click_area(ota_btn, 12);
    lv_obj_add_event_cb(ota_btn, ota_button_cb, LV_EVENT_CLICKED, NULL);

    // --- navigation: long-press on the (active) main screen opens config ---
    lv_obj_add_event_cb(s_active_main, to_config_cb, LV_EVENT_LONG_PRESSED, NULL);
}
