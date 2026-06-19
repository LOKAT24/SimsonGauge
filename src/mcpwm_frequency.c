#include "mcpwm_frequency.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "driver/mcpwm_cap.h"
#include "esp_log.h"
#include "esp_check.h"
#include <stdlib.h>

static const char *TAG = "MCPWM_FREQ";

#define RING_BUFFER_SIZE            256
#define MCPWM_FREQ_TIMEOUT_MS       500
#define MCPWM_FREQ_MAX_HZ           20000.0
#define MIN_PULSE_WIDTH_US          100      // Minimalny czas trwania stanu wysokiego/niskiego, aby uznać go za prawidłowy

// Struktura do przechowywania danych o zdarzeniu przechwycenia
typedef struct {
    uint32_t timestamp;
    mcpwm_capture_edge_t edge;
} capture_event_t;

// Statyczne (prywatne) zmienne modułu
static mcpwm_cap_timer_handle_t cap_timer = NULL;
static mcpwm_cap_channel_handle_t cap_chan = NULL;
static RingbufHandle_t ring_buf = NULL;
static volatile uint32_t last_cap_time_ms = 0;
static double last_known_freq = 0.0; // Przechowuje ostatnią poprawnie zmierzoną częstotliwość

static IRAM_ATTR bool capture_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data)
{
    capture_event_t event = { .timestamp = edata->cap_value, .edge = edata->cap_edge };
    xRingbufferSendFromISR(ring_buf, &event, sizeof(capture_event_t), NULL);
    last_cap_time_ms = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    return false;
}

esp_err_t mcpwm_freq_init(int gpio_num)
{
    ESP_LOGI(TAG, "Konfiguracja MCPWM Capture dla GPIO%d", gpio_num);
    ring_buf = xRingbufferCreate(RING_BUFFER_SIZE, RINGBUF_TYPE_NOSPLIT);
    if (!ring_buf) {
        ESP_LOGE(TAG, "Nie udało się utworzyć bufora kołowego");
        return ESP_FAIL;
    }
    mcpwm_capture_timer_config_t cap_timer_config = { .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT, .group_id = 0 };
    ESP_RETURN_ON_ERROR(mcpwm_new_capture_timer(&cap_timer_config, &cap_timer), TAG, "Błąd tworzenia timera capture");
    mcpwm_capture_channel_config_t cap_config = {
        .gpio_num = gpio_num, .prescale = 1,
        .flags.neg_edge = true, .flags.pos_edge = true,
        .flags.pull_up = true, .flags.pull_down = false,
    };
    ESP_RETURN_ON_ERROR(mcpwm_new_capture_channel(cap_timer, &cap_config, &cap_chan), TAG, "Błąd tworzenia kanału capture");
    mcpwm_capture_event_callbacks_t cbs = { .on_cap = capture_callback };
    ESP_RETURN_ON_ERROR(mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, NULL), TAG, "Błąd rejestracji callback");
    ESP_RETURN_ON_ERROR(mcpwm_capture_channel_enable(cap_chan), TAG, "Błąd włączania kanału");
    ESP_RETURN_ON_ERROR(mcpwm_capture_timer_enable(cap_timer), TAG, "Błąd włączania timera");
    ESP_RETURN_ON_ERROR(mcpwm_capture_timer_start(cap_timer), TAG, "Błąd startu timera");
    last_cap_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return ESP_OK;
}

double mcpwm_freq_get_hz(void)
{
    if (!cap_chan || !cap_timer) {
        ESP_LOGE(TAG, "Moduł MCPWM nie został zainicjalizowany!");
        return 0.0;
    }

    if ((xTaskGetTickCount() * portTICK_PERIOD_MS - last_cap_time_ms) > MCPWM_FREQ_TIMEOUT_MS) {
        if (last_known_freq != 0.0) {
            ESP_LOGI(TAG, "Timeout sygnału, zerowanie częstotliwości.");
            last_known_freq = 0.0;
            // Opróżnij bufor
            size_t item_size;
            void *item = NULL;
            while ((item = xRingbufferReceive(ring_buf, &item_size, 0)) != NULL) {
                vRingbufferReturnItem(ring_buf, item);
            }
        }
        return 0.0;
    }

    uint32_t timer_resolution = 0;
    mcpwm_capture_timer_get_resolution(cap_timer, &timer_resolution);
    if (timer_resolution == 0) return last_known_freq; // Zwróć ostatnią wartość w razie błędu

    const uint32_t min_pulse_width_ticks = (timer_resolution / 1000000) * MIN_PULSE_WIDTH_US;

    size_t item_size;
    capture_event_t *item = NULL;
    static capture_event_t prev_event = {0}, last_event = {0};

    // Przetwarzaj wszystkie elementy z bufora kołowego
    while ((item = (capture_event_t *)xRingbufferReceive(ring_buf, &item_size, 0)) != NULL) {
        capture_event_t current_event = *item;
        vRingbufferReturnItem(ring_buf, (void *)item);

        if (prev_event.timestamp == 0) {
            prev_event = last_event;
            last_event = current_event;
            continue;
        }

        // Szukaj sekwencji ZBOCZE_NARASTAJĄCE -> ZBOCZE_OPADAJĄCE -> ZBOCZE_NARASTAJĄCE
        if (prev_event.edge == MCPWM_CAP_EDGE_POS &&
            last_event.edge == MCPWM_CAP_EDGE_NEG &&
            current_event.edge == MCPWM_CAP_EDGE_POS) {

            uint32_t high_duration = last_event.timestamp - prev_event.timestamp;
            uint32_t low_duration = current_event.timestamp - last_event.timestamp;

            // Walidacja: odrzuć impuls, jeśli którakolwiek faza jest za krótka (to szpilka)
            if (high_duration >= min_pulse_width_ticks && low_duration >= min_pulse_width_ticks) {
                uint32_t period_ticks = current_event.timestamp - prev_event.timestamp;
                
                if (period_ticks > 0) {
                    double new_freq = (double)timer_resolution / period_ticks;
                    
                    if (new_freq <= MCPWM_FREQ_MAX_HZ) {
                        // NATYCHMIASTOWA AKTUALIZACJA WARTOŚCI
                        last_known_freq = new_freq;
                    }
                }
            }
        }
        
        prev_event = last_event;
        last_event = current_event;
    }

    // Zwróć ostatnią poprawnie obliczoną wartość
    return last_known_freq;
}
