// Persistent settings stored in NVS via the Arduino Preferences library.

#include "settings.h"
#include <Preferences.h>

static Preferences prefs;

#define NS          "motogauge"
#define KEY_MULT    "mult"
#define KEY_SPEED_MULT "smult"
#define KEY_THEME   "theme"
#define KEY_BRIGHT  "bright"

void settings_begin(void)
{
    prefs.begin(NS, false);   // read/write namespace
}

float settings_get_multiplier(void)
{
    return prefs.getFloat(KEY_MULT, 1.0f);
}

void settings_set_multiplier(float v)
{
    prefs.putFloat(KEY_MULT, v);
}

float settings_get_speed_multiplier(void)
{
    return prefs.getFloat(KEY_SPEED_MULT, 1.0f);
}

void settings_set_speed_multiplier(float v)
{
    prefs.putFloat(KEY_SPEED_MULT, v);
}

int settings_get_theme(void)
{
    return prefs.getInt(KEY_THEME, 0);
}

void settings_set_theme(int t)
{
    prefs.putInt(KEY_THEME, t);
}

int settings_get_brightness(void)
{
    return prefs.getInt(KEY_BRIGHT, 100);
}

void settings_set_brightness(int b)
{
    prefs.putInt(KEY_BRIGHT, b);
}
