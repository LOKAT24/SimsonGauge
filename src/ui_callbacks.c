// Application-side UI event callbacks and screen navigation.

#include "ui_callbacks.h"
#include "ui.h"

#define SCREEN_ANIM_MS  400

/**
 * Referenced directly by the SquareLine-generated screens (declared in
 * ui_events.h). Copies the typed text into the speed label.
 */
void speedTextCB(lv_event_t * e)
{
    lv_obj_t * text_area = lv_event_get_target(e);
    const char * text = lv_textarea_get_text(text_area);
    lv_label_set_text(ui_SpeedLabel, text);
}

/** Long-press anywhere toggles between the Main and Config screens. */
static void screen_long_press_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_LONG_PRESSED) {
        return;
    }
    if (lv_scr_act() == ui_MainScreen) {
        lv_scr_load_anim(ui_ConfigScreen, LV_SCR_LOAD_ANIM_FADE_ON, SCREEN_ANIM_MS, 0, false);
    } else {
        lv_scr_load_anim(ui_MainScreen, LV_SCR_LOAD_ANIM_FADE_ON, SCREEN_ANIM_MS, 0, false);
    }
}

void ui_register_callbacks(void)
{
    lv_obj_add_event_cb(ui_MainScreen, screen_long_press_handler, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(ui_ConfigScreen, screen_long_press_handler, LV_EVENT_LONG_PRESSED, NULL);
}
