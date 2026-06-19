#include <Arduino.h>
#include "lcd_bsp.h"        // display + LVGL bring-up, lvgl_lock()/lvgl_unlock()
#include "FT3168.h"         // touch
#include "ui.h"            // SquareLine Studio generated UI
#include "gauge.h"          // frequency -> needle logic
#include "ui_callbacks.h"   // UI event callbacks / screen navigation
#include "test_signal.h"    // on-board test square wave

extern "C" {
#include "mcpwm_frequency.h"   // frequency measurement via MCPWM capture
}

#define MCPWM_GPIO 1           // GPIO carrying the signal whose frequency we measure

void setup()
{
    Serial.begin(115200);

    // Hardware: frequency capture + (optional) on-board test stimulus
    mcpwm_freq_init(MCPWM_GPIO);
    test_signal_start();

    // Display + touch + LVGL (starts the LVGL handler task)
    Touch_Init();
    lcd_lvgl_Init();

    // Build the UI and wire up the app logic under the LVGL lock
    if (lvgl_lock(-1)) {
        ui_init();
        ui_register_callbacks();
        gauge_start();
        lvgl_unlock();
    }
}

void loop()
{
}
