// Plik: main/bsp.c

#include "bsp.h" // Nasz nowy nagłówek

// Wszystkie potrzebne nagłówki
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_sh8601.h"
#include "touch_bsp.h"
#include "read_lcd_id_bsp.h"

// Definicje i zmienne statyczne
static const char *TAG = "BSP";
static SemaphoreHandle_t lvgl_mux = NULL;

#define LCD_HOST    SPI2_HOST

#define SH8601_ID 0x86
#define CO5300_ID 0xff
static uint8_t READ_LCD_ID = 0x00; 

#if CONFIG_LV_COLOR_DEPTH == 32
#define LCD_BIT_PER_PIXEL       (24)
#elif CONFIG_LV_COLOR_DEPTH == 16
#define LCD_BIT_PER_PIXEL       (16)
#endif

#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_PIN_NUM_LCD_CS         (GPIO_NUM_9)
#define EXAMPLE_PIN_NUM_LCD_PCLK       (GPIO_NUM_10) 
#define EXAMPLE_PIN_NUM_LCD_DATA0      (GPIO_NUM_11)
#define EXAMPLE_PIN_NUM_LCD_DATA1      (GPIO_NUM_12)
#define EXAMPLE_PIN_NUM_LCD_DATA2      (GPIO_NUM_13)
#define EXAMPLE_PIN_NUM_LCD_DATA3      (GPIO_NUM_14)
#define EXAMPLE_PIN_NUM_LCD_RST        (GPIO_NUM_21)
#define EXAMPLE_PIN_NUM_BK_LIGHT       (-1)
#define EXAMPLE_LCD_H_RES              466
#define EXAMPLE_LCD_V_RES              466
#define EXAMPLE_LVGL_BUF_HEIGHT        (EXAMPLE_LCD_V_RES / 4)
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2

// Tablice z komendami inicjalizacyjnymi
static const sh8601_lcd_init_cmd_t sh8601_lcd_init_cmds[] = {
    {0x11, (uint8_t []){0x00}, 0, 120}, {0x44, (uint8_t []){0x01, 0xD1}, 2, 0},
    {0x35, (uint8_t []){0x00}, 1, 0},   {0x53, (uint8_t []){0x20}, 1, 10},
    {0x51, (uint8_t []){0x00}, 1, 10},  {0x29, (uint8_t []){0x00}, 0, 10},
    {0x51, (uint8_t []){0xFF}, 1, 0},
};
static const sh8601_lcd_init_cmd_t co5300_lcd_init_cmds[] = {
    {0x11, (uint8_t []){0x00}, 0, 80},  {0xC4, (uint8_t []){0x80}, 1, 0},
    {0x53, (uint8_t []){0x20}, 1, 1},   {0x63, (uint8_t []){0xFF}, 1, 1},
    {0x51, (uint8_t []){0x00}, 1, 1},   {0x29, (uint8_t []){0x00}, 0, 10},
    {0x51, (uint8_t []){0xFF}, 1, 0},
};

// Deklaracje statycznych zmiennych i funkcji, które będą używane wewnątrz tego pliku
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static void bsp_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
static void bsp_increase_lvgl_tick(void *arg);
static void bsp_lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);
static void bsp_lvgl_port_task(void *arg);

// Implementacja funkcji zwrotnej - teraz z instrukcją return
static bool bsp_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false; // <-- BRAKUJĄCY RETURN
}

// Pełne implementacje pozostałych funkcji
static void bsp_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    const int offsetx1 = (READ_LCD_ID == SH8601_ID) ? area->x1 : area->x1 + 0x06;
    const int offsetx2 = (READ_LCD_ID == SH8601_ID) ? area->x2 : area->x2 + 0x06;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

void bsp_lvgl_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

static void bsp_lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t tp_x, tp_y;
    if (getTouch(&tp_x, &tp_y)) {
        data->point.x = tp_x;
        data->point.y = tp_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void bsp_increase_lvgl_tick(void *arg)
{
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

bool bsp_lvgl_lock(int timeout_ms)
{
    assert(lvgl_mux);
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

void bsp_lvgl_unlock(void)
{
    assert(lvgl_mux);
    xSemaphoreGive(lvgl_mux);
}

static void bsp_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    while (1) {
        if (bsp_lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            bsp_lvgl_unlock();
        }
        if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

// Główna funkcja inicjalizacyjna z pełną implementacją
void bsp_display_start(void)
{
    READ_LCD_ID = read_lcd_id();

    // Inicjalizacja magistrali SPI
    const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_PCLK, EXAMPLE_PIN_NUM_LCD_DATA0, EXAMPLE_PIN_NUM_LCD_DATA1, EXAMPLE_PIN_NUM_LCD_DATA2, EXAMPLE_PIN_NUM_LCD_DATA3, EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * LCD_BIT_PER_PIXEL / 8);
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Instalacja panelu I/O
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_CS, bsp_notify_lvgl_flush_ready, &disp_drv);
    sh8601_vendor_config_t vendor_config = { .flags = { .use_qspi_interface = 1, }, };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // Instalacja sterownika panelu
    esp_lcd_panel_handle_t panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = { .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST, .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, .bits_per_pixel = LCD_BIT_PER_PIXEL, .vendor_config = &vendor_config, };
    vendor_config.init_cmds = (READ_LCD_ID == SH8601_ID) ? sh8601_lcd_init_cmds : co5300_lcd_init_cmds;
    vendor_config.init_cmds_size = (READ_LCD_ID == SH8601_ID) ? sizeof(sh8601_lcd_init_cmds) / sizeof(sh8601_lcd_init_cmds[0]) : sizeof(co5300_lcd_init_cmds) / sizeof(co5300_lcd_init_cmds[0]);
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // Inicjalizacja panelu dotykowego
    Touch_Init();

    // Inicjalizacja LVGL
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT);

    // Rejestracja sterownika wyświetlacza
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = bsp_lvgl_flush_cb;
    disp_drv.rounder_cb = bsp_lvgl_rounder_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    // Instalacja tick timera LVGL
    const esp_timer_create_args_t lvgl_tick_timer_args = { .callback = &bsp_increase_lvgl_tick, .name = "lvgl_tick" };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    // Rejestracja sterownika panelu dotykowego
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = bsp_lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);

    // Utworzenie mutexu i taska LVGL
    lvgl_mux = xSemaphoreCreateMutex();
    assert(lvgl_mux);
    xTaskCreate(bsp_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);
}