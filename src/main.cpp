#include <Arduino.h>
#include "lcd_bsp.h"        // display + LVGL bring-up, lvgl_lock()/lvgl_unlock()
#include "FT3168.h"         // touch
#include "ui.h"            // SquareLine Studio generated UI
#include "gauge.h"          // frequency -> needle logic
#include "config_screen.h"  // code-defined config screen + navigation
#include "settings.h"       // persistent settings (NVS)
#include "test_signal.h"    // on-board test square wave
#include "ota.h"            // WiFi SoftAP firmware update
extern "C" {
#include "mcpwm_frequency.h"   // frequency measurement via MCPWM capture
#include "mcpwm_speed.h"       // frequency measurement via MCPWM capture (group 1)
}

#define MCPWM_GPIO 1           // GPIO carrying the signal whose frequency we measure
#define SPEED_GPIO 3           // GPIO carrying the signal whose speed we measure

void setup()
{
    Serial.begin(115200);

    // Persistent settings (multiplier, theme)
    settings_begin();

    // Hardware: frequency capture + (optional) on-board test stimulus
    mcpwm_freq_init(MCPWM_GPIO);
    mcpwm_speed_init(SPEED_GPIO);
    test_signal_start();

    // Display + touch + LVGL (starts the LVGL handler task)
    Touch_Init();
    lcd_lvgl_Init();
    lcd_set_brightness(settings_get_brightness() * 255 / 100);

    // Build the UI and wire up the app logic under the LVGL lock
    if (lvgl_lock(-1)) {
        ui_init();
        config_screen_init();   // builds config screen, loads settings, wires navigation
        gauge_start();
        lvgl_unlock();
    }
}

void loop()
{
    ota_loop();     // services the OTA web server once OTA mode is requested
    delay(5);
}
