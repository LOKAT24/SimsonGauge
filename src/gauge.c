// Gauge logic: read the measured frequency and drive the needle (ui_wskaznik).

#include "gauge.h"
#include "ui.h"
#include "ui_SimsonScreen.h"
#include "settings.h"
#include "mcpwm_frequency.h"
#include "mcpwm_speed.h"

#include <stdint.h>

#define INTRO_DELAY_MS      8500   // wait before the first reading (lets the intro play)
#define UPDATE_PERIOD_MS    50     // gauge refresh period

// Multiplier (frequency * multiplier = RPM). Owned here, set from the config
// screen which loads/persists it via settings.
static double s_multiplier = 1.0;
static float s_speed_mult = 1.0;
bool g_simson_intro_done = false;

void gauge_set_multiplier(double m)
{
    s_multiplier = m;
}

void gauge_set_speed_multiplier(float sm)
{
    s_speed_mult = sm;
}

void gauge_set_algorithm(int algo)
{
    mcpwm_freq_set_algorithm(algo);
    mcpwm_speed_set_algorithm(algo);
}

/**
 * Convert RPM to the needle angle using a fixed-point (2^32) Horner polynomial.
 * f(x) = -0.0000000009463277 x^3 + 0.0000135123083132 x^2 + 0.2731027037933820 x + 6.0
 */
static int32_t rpm_to_angle(int32_t rpm)
{
    const int64_t C3_SCALED = -4;             // -0.0000000009463277 * 2^32
    const int64_t C2_SCALED = 58036;          //  0.0000135123083132 * 2^32
    const int64_t C1_SCALED = 1172835389;     //  0.2731027037933820 * 2^32
    const int64_t C0_SCALED = 25769803776LL;  //  6.0 * 2^32
    const int SCALE_BITS = 32;

    int64_t x = rpm;
    int64_t y_scaled = C0_SCALED + x * (C1_SCALED + x * (C2_SCALED + x * C3_SCALED));
    return (int32_t)((y_scaled + (1LL << (SCALE_BITS - 1))) >> SCALE_BITS);
}

static void gauge_update_cb(lv_timer_t * timer)
{
    double frequency = mcpwm_freq_get_hz();
    int32_t rpm = (int32_t)(frequency * s_multiplier);
    
    double speed_freq = mcpwm_speed_get_hz();
    // Przeliczenie czestotliwosci predkosci
    int32_t speed = (int32_t)(speed_freq * s_speed_mult); 
    char speed_str[16];
    snprintf(speed_str, sizeof(speed_str), "%d", (int)speed);
    
    int theme = settings_get_theme();
    if (theme == 0) {
        int32_t angle = rpm_to_angle(rpm);
        lv_img_set_angle(ui_wskaznik, angle);
        lv_label_set_text(ui_SpeedLabel, speed_str);
    } else { // theme 1 (day) / 2 (night): Simson classic
        if (!g_simson_intro_done) return;
        if (rpm > 7000) rpm = 7000;
        simson_set_values(rpm, (int)speed);
    }
}

/** One-shot: after the intro delay, start the periodic gauge update. */
static void gauge_start_cb(lv_timer_t * timer)
{
    lv_timer_create(gauge_update_cb, UPDATE_PERIOD_MS, NULL);
}

void gauge_start(void)
{
    if (settings_get_theme() == 0) {
        lv_timer_t * one_shot = lv_timer_create(gauge_start_cb, INTRO_DELAY_MS, NULL);
        lv_timer_set_repeat_count(one_shot, 1);
    } else {
        lv_timer_create(gauge_update_cb, UPDATE_PERIOD_MS, NULL);
    }
}
