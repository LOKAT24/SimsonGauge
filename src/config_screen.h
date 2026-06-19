#ifndef CONFIG_SCREEN_H
#define CONFIG_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build the (code-defined) config screen and wire navigation.
 *
 * Loads persisted settings (multiplier, theme), creates the config screen with
 * the multiplier +/- controls, theme selector and OTA button, and attaches the
 * long-press navigation to the active main screen and the config screen.
 *
 * Call after ui_init() and while holding the LVGL lock.
 */
void config_screen_init(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_SCREEN_H
