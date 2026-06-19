// speedTextCB is referenced directly by the SquareLine-generated config screen
// (ui_ConfigScreen.c, declared in ui_events.h), so it must be defined with C
// linkage. Navigation, the OTA button and the config UI now live in
// config_screen.cpp.

#include "ui.h"

void speedTextCB(lv_event_t * e)
{
    lv_obj_t * text_area = lv_event_get_target(e);
    const char * text = lv_textarea_get_text(text_area);
    lv_label_set_text(ui_SpeedLabel, text);
}
