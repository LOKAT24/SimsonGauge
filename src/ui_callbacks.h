#ifndef UI_CALLBACKS_H
#define UI_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Attach application event callbacks to the SquareLine UI.
 *
 * Currently registers the long-press handler that toggles between the Main and
 * Config screens. Must be called after ui_init() and while holding the LVGL
 * lock.
 *
 * Note: speedTextCB (referenced directly by the generated screens) is defined
 * in ui_callbacks.c and does not need explicit registration here.
 */
void ui_register_callbacks(void);

#ifdef __cplusplus
}
#endif

#endif // UI_CALLBACKS_H
