#ifndef GAUGE_H
#define GAUGE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the gauge logic.
 *
 * Schedules a one-shot timer that (after the intro delay) starts a periodic
 * timer reading the measured frequency and rotating the needle (ui_wskaznik).
 *
 * Must be called while holding the LVGL lock (it creates LVGL timers).
 */
void gauge_start(void);

/** Set the RPM multiplier used by the gauge (frequency * multiplier = RPM). */
void gauge_set_multiplier(double m);

void gauge_set_speed_multiplier(float sm);

extern bool g_simson_intro_done;

#ifdef __cplusplus
}
#endif

#endif // GAUGE_H
