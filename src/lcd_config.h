#ifndef LCD_CONFIG_H
#define LCD_CONFIG_H

#define EXAMPLE_LCD_H_RES              466
#define EXAMPLE_LCD_V_RES              466

#define LCD_BIT_PER_PIXEL              16

#define EXAMPLE_PIN_NUM_LCD_CS            9
#define EXAMPLE_PIN_NUM_LCD_PCLK          10 
#define EXAMPLE_PIN_NUM_LCD_DATA0         11
#define EXAMPLE_PIN_NUM_LCD_DATA1         12
#define EXAMPLE_PIN_NUM_LCD_DATA2         13
#define EXAMPLE_PIN_NUM_LCD_DATA3         14
#define EXAMPLE_PIN_NUM_LCD_RST           21
#define EXAMPLE_PIN_NUM_BK_LIGHT          (-1)

// Two LVGL draw buffers live in internal DMA RAM (see lcd_lvgl_Init). At V_RES/4
// they take ~216 KB, leaving too little internal RAM for the WiFi driver: starting
// the OTA SoftAP then crashes in ieee80211_hostap_attach (LoadProhibited on a
// failed allocation). V_RES/10 (~86 KB total) frees ~130 KB for WiFi; the image is
// unchanged, only flushed in more, smaller chunks (negligible for this UI).
//
// NB: full-screen double buffering in PSRAM was tried and does NOT work on this
// panel: the SPI driver cannot DMA straight from PSRAM, it allocates an equally
// large internal "priv" TX buffer and a whole 466x466 frame (~434 KB) does not
// fit in internal RAM (setup_dma_priv_buffer fails -> black screen).
//
// Anti-tearing: taller buffers mean the moving area (needle) is flushed in fewer
// horizontal bands, so fewer visible seams. V_RES/6 (two buffers ~143 KB) is a
// compromise that still leaves room for WiFi/OTA. If the OTA SoftAP starts
// crashing again (too little internal RAM), step this back toward V_RES/8 or /10.
#define EXAMPLE_LVGL_BUF_HEIGHT        (EXAMPLE_LCD_V_RES / 6)
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2                          //Timer time
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500                        //LVGL Indicates the maximum time for a task to run
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1                          //LVGL Minimum time to run a task
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)                 //LVGL runs the task stack
#define EXAMPLE_LVGL_TASK_PRIORITY     2                          //LVGL Running task priority

#define I2C_ADDR_FT3168 0x38
#define EXAMPLE_PIN_NUM_TOUCH_SCL 48
#define EXAMPLE_PIN_NUM_TOUCH_SDA 47

#endif