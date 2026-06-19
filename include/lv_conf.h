/**
 * lv_conf.h  -  LVGL 8.3 configuration for the Waveshare ESP32-S3-Touch-AMOLED-1.43
 *
 * Only the options that differ from LVGL's built-in defaults are set here;
 * everything else falls back to the defaults in lv_conf_internal.h.
 * Loaded via -DLV_CONF_INCLUDE_SIMPLE (see platformio.ini).
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
 *   COLOR SETTINGS
 *====================*/

/* RGB565 panel */
#define LV_COLOR_DEPTH 16

/* esp_lcd sends color data MSB-first, while lv_color_t is little-endian on the
 * ESP32-S3 -> swap the two bytes of each pixel.
 * If the colors look wrong (e.g. red/blue mixed up), flip this to 0. */
#define LV_COLOR_16_SWAP 1

/*=========================
 *   MEMORY SETTINGS
 *=========================*/

/* Use the standard C allocator (heap). Avoids tuning LV_MEM_SIZE; the widgets
 * demo allocates comfortably from the ESP32-S3 heap. */
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC   malloc
#define LV_MEM_CUSTOM_FREE    free
#define LV_MEM_CUSTOM_REALLOC realloc

/*====================
 *   HAL / TICK
 *====================*/

/* The example drives lv_tick_inc() from an esp_timer, so keep the custom
 * tick source disabled. */
#define LV_TICK_CUSTOM 0

/*====================
 *   DEBUG / MONITOR
 *====================*/

#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR  0

/*====================
 *   FONTS
 *====================*/

/* Enable a range of sizes so the widgets demo can lay out nicely on the
 * 466x466 panel (montserrat_14 is the default font and is on by default). */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_48 1

/*====================
 *   DEMOS
 *====================*/

/* No LVGL built-in demo is used anymore; the UI comes from SquareLine Studio
 * (see src/ui/). Leave the demos disabled. */
#define LV_USE_DEMO_WIDGETS 0

#endif /* LV_CONF_H */
