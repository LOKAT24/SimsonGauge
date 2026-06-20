#ifndef UI_SIMSONSCREEN_H
#define UI_SIMSONSCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t * ui_SimsonScreen;        // Simson classic - day  (theme 1)
extern lv_obj_t * ui_SimsonNightScreen;   // Simson classic - night (theme 2)

void ui_SimsonScreen_screen_init(void);

// Push live values onto the on-screen Simson variant (rpm in 1/min, speed in km/h).
void simson_set_values(int rpm, int speed);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // UI_SIMSONSCREEN_H
