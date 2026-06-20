#include "ui/ui.h"
#include "ui_SimsonScreen.h"
#include "config_screen.h"
#include <stdio.h>
#include "gauge.h"

// Two variants of the Simson "classic" gauge that differ only in colours and
// background graphic (day / night). The face - bezel, scale arc and tick
// labels - is baked into the background image, so we no longer draw an arc;
// only the live speed / rpm / gear values are overlaid as labels.
lv_obj_t * ui_SimsonScreen;        // classic - day  (theme 1)
lv_obj_t * ui_SimsonNightScreen;   // classic - night (theme 2)

// Background images. Supplied as separate (re-converted) C files; any square
// resolution works - the image is auto-scaled to the 466x466 panel below.
LV_IMG_DECLARE(classic_dzien_466);
LV_IMG_DECLARE(classic_noc_466);
LV_FONT_DECLARE(Saira_Condensed_158);
LV_FONT_DECLARE(Saira_Condensed_50);
LV_FONT_DECLARE(SplineSansMono_11);

#define SCREEN_PX 466

// Colour palettes taken verbatim from motyw_design (day = 'tag', night = 'nacht').
typedef struct {
    uint32_t face_bg;   // solid fill behind the image (matches the face colour)
    uint32_t speed;     // big speed number
    uint32_t sub;       // small rpm + unit text
    uint32_t accent;    // gear number
} simson_palette_t;

static const simson_palette_t PAL_DAY   = { 0xe6dcc4, 0x1b1a17, 0x6f6a5e, 0xc0202a };
static const simson_palette_t PAL_NIGHT = { 0x15110d, 0xece3cd, 0x9a9080, 0xe23b32 };

// Live widgets for one variant. The gauge writes to whichever variant is on
// screen (s_active), updated when a Simson screen reports SCREEN_LOADED.
typedef struct {
    lv_obj_t * arc;          // live red RPM indicator (track itself is in the image)
    lv_obj_t * speed_label;
    lv_obj_t * rpm_label;
    lv_obj_t * gear_label;
} simson_widgets_t;

static simson_widgets_t   s_day_w;
static simson_widgets_t   s_night_w;
static simson_widgets_t * s_active = &s_day_w;

// ----------------------------------------------------------------------------
// Live values (called from the gauge timer) + intro sweep
// ----------------------------------------------------------------------------
void simson_set_values(int rpm, int speed)
{
    if (!s_active->speed_label) return;

    lv_arc_set_value(s_active->arc, rpm);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", speed);
    lv_label_set_text(s_active->speed_label, buf);

    snprintf(buf, sizeof(buf), "%.1f", (float)rpm / 1000.0f);
    lv_label_set_text(s_active->rpm_label, buf);
}

static void simson_intro_anim_cb(lv_anim_t * a, int32_t v)
{
    LV_UNUSED(a);
    // Sweep speed and rpm together (7000 rpm -> ~100 km/h), like a gauge self-test.
    simson_set_values(v, (int)(v / 70));
}

static void simson_intro_anim_ready_cb(lv_anim_t * a)
{
    LV_UNUSED(a);
    g_simson_intro_done = true;
}

static void simson_screen_loaded_cb(lv_event_t * e)
{
    lv_obj_t * scr = lv_event_get_target(e);
    s_active = (scr == ui_SimsonNightScreen) ? &s_night_w : &s_day_w;

    // Start the intro exactly when the screen becomes visible.
    g_simson_intro_done = false;
    lv_label_set_text(s_active->speed_label, "0");
    lv_label_set_text(s_active->rpm_label, "0.0");
    lv_arc_set_value(s_active->arc, 0);

    // Intro sweep 0 -> 7000 -> 0. The anim var is a child label (not the screen)
    // so a later screen change's lv_anim_del(screen) can't cancel it mid-sweep
    // and leave g_simson_intro_done stuck false.
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_active->speed_label);
    lv_anim_set_values(&a, 0, 7000);
    lv_anim_set_time(&a, 1000);            // 1.0s up
    lv_anim_set_playback_time(&a, 1000);   // 1.0s down
    lv_anim_set_playback_delay(&a, 150);   // brief hold at the top
    lv_anim_set_custom_exec_cb(&a, simson_intro_anim_cb);
    lv_anim_set_ready_cb(&a, simson_intro_anim_ready_cb);
    lv_anim_start(&a);
}

// ----------------------------------------------------------------------------
// Build
// ----------------------------------------------------------------------------
static lv_obj_t * simson_build(const lv_img_dsc_t * bg,
                               const simson_palette_t * pal,
                               simson_widgets_t * w)
{
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(pal->face_bg), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(scr, simson_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);

    // Background face (full gauge: bezel, scale arc, ticks and labels baked in).
    lv_obj_t * bg_img = lv_img_create(scr);
    lv_img_set_src(bg_img, bg);
    lv_obj_center(bg_img);
    if (bg->header.w > 0) {
        lv_img_set_zoom(bg_img, (uint16_t)(SCREEN_PX * 256 / bg->header.w));
    }
    lv_obj_add_flag(bg_img, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);
    lv_obj_add_event_cb(bg_img, to_config_cb, LV_EVENT_LONG_PRESSED, NULL);

    // Live red RPM indicator drawn over the scale baked into the background.
    // Only the indicator is visible - the track is part of the image - so the
    // MAIN (background) arc is made transparent. Size 454 -> centre-line radius
    // 227, width 10 (outer edge ~232, just inside the 466 px panel).
    w->arc = lv_arc_create(scr);
    lv_obj_set_size(w->arc, 454, 454);
    lv_obj_center(w->arc);
    lv_arc_set_bg_angles(w->arc, 135, 45);
    lv_arc_set_range(w->arc, 0, 7000);
    lv_arc_set_value(w->arc, 0);
    lv_obj_remove_style(w->arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(w->arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_opa(w->arc, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(w->arc, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(w->arc, lv_color_hex(pal->accent), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(w->arc, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_rounded(w->arc, true, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // Speed (big, centre)
    w->speed_label = lv_label_create(scr);
    lv_label_set_text(w->speed_label, "0");
    lv_obj_set_style_text_color(w->speed_label, lv_color_hex(pal->speed), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(w->speed_label, &Saira_Condensed_158, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(w->speed_label, LV_ALIGN_CENTER, 0, -10);

    // RPM x1000 (small, above the speed)
    w->rpm_label = lv_label_create(scr);
    lv_label_set_text(w->rpm_label, "0.0");
    lv_obj_set_style_text_color(w->rpm_label, lv_color_hex(pal->sub), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(w->rpm_label, &SplineSansMono_11, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(w->rpm_label, LV_ALIGN_CENTER, -60, -96);

    // Gear (bottom). No gear sensor yet -> stays "N".
    w->gear_label = lv_label_create(scr);
    lv_label_set_text(w->gear_label, "N");
    lv_obj_set_style_text_color(w->gear_label, lv_color_hex(pal->accent), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(w->gear_label, &Saira_Condensed_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(w->gear_label, LV_ALIGN_CENTER, 0, 125);

    return scr;
}

void ui_SimsonScreen_screen_init(void)
{
    // Idempotent: built from two places during boot - lazily by _ui_screen_change()
    // (HeloScreen fires SCREEN_LOADED synchronously inside ui_init(), before
    // config_screen_init() runs) and eagerly by config_screen_init(). Without this
    // guard the screens are built twice and the global pointers point at the
    // off-screen copy, so "classic" appears frozen until a config round-trip.
    if (ui_SimsonScreen) return;

    ui_SimsonScreen      = simson_build(&classic_dzien_466, &PAL_DAY,   &s_day_w);
    ui_SimsonNightScreen = simson_build(&classic_noc_466,   &PAL_NIGHT, &s_night_w);
}
