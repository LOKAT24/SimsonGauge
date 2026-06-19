// Plik: main/bsp.h

#ifndef BSP_H
#define BSP_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicjalizuje wyświetlacz, panel dotykowy i bibliotekę LVGL.
 * */
void bsp_display_start(void);

/**
 * @brief Blokuje mutex LVGL w celu bezpiecznego dostępu do API LVGL.
 * * @param timeout_ms Czas oczekiwania w milisekundach (-1 oznacza nieskończoność).
 * @return true jeśli udało się zablokować mutex, w przeciwnym razie false.
 */
bool bsp_lvgl_lock(int timeout_ms);

/**
 * @brief Zwalnia mutex LVGL.
 * */
void bsp_lvgl_unlock(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_H