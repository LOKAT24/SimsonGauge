#ifndef GAUGE_H
#define GAUGE_H

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

#ifdef __cplusplus
}
#endif

#endif // GAUGE_H
