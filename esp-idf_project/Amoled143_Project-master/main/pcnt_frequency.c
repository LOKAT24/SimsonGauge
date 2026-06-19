#include "pcnt_frequency.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "esp_timer.h" // <-- Potrzebne do precyzyjnego pomiaru czasu
#include "esp_check.h"

static const char *TAG = "PCNT_FREQ";
#define PCNT_HIGH_LIMIT 32767

// Zmienne statyczne (prywatne) modułu
static pcnt_unit_handle_t pcnt_unit = NULL;
static int64_t last_read_time_us = 0; // Czas ostatniego odczytu w mikrosekundach

esp_err_t pcnt_freq_init(int gpio_num)
{
    ESP_LOGI(TAG, "Konfiguracja PCNT dla GPIO%d", gpio_num);

    pcnt_unit_config_t unit_config = {
        .low_limit = -100,
        .high_limit = PCNT_HIGH_LIMIT,
    };
    ESP_RETURN_ON_ERROR(pcnt_new_unit(&unit_config, &pcnt_unit), TAG, "Błąd tworzenia jednostki PCNT");

    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = gpio_num,
        .level_gpio_num = -1,
    };
    pcnt_channel_handle_t pcnt_chan = NULL;
    ESP_RETURN_ON_ERROR(pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan), TAG, "Błąd tworzenia kanału PCNT");
    ESP_RETURN_ON_ERROR(pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD), TAG, "Błąd ustawiania zbocza");
    ESP_RETURN_ON_ERROR(pcnt_unit_enable(pcnt_unit), TAG, "Błąd włączania jednostki");
    ESP_RETURN_ON_ERROR(pcnt_unit_clear_count(pcnt_unit), TAG, "Błąd czyszczenia licznika");
    ESP_RETURN_ON_ERROR(pcnt_unit_start(pcnt_unit), TAG, "Błąd startu jednostki");

    // Zapisz czas startu jako punkt odniesienia dla pierwszego pomiaru
    last_read_time_us = esp_timer_get_time();

    return ESP_OK;
}

double pcnt_freq_get_hz(void)
{
    if (pcnt_unit == NULL) {
        ESP_LOGE(TAG, "Jednostka PCNT nie została zainicjalizowana!");
        return 0.0;
    }

    // --- Krok 1: Odczytaj dane ---
    int pulse_count;
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
    int64_t current_time_us = esp_timer_get_time();

    // --- Krok 2: Oblicz różnicę czasu ---
    int64_t time_delta_us = current_time_us - last_read_time_us;

    // --- Krok 3: Zresetuj stan do następnego pomiaru ---
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    last_read_time_us = current_time_us;

    // --- Krok 4: Oblicz częstotliwość ---
    // Unikaj dzielenia przez zero, jeśli funkcja zostanie wywołana zbyt szybko
    if (time_delta_us == 0) {
        return 0.0;
    }

    // f = (liczba impulsów) / (czas w sekundach)
    // Czas w sekundach = czas w mikrosekundach / 1_000_000
    double frequency = (double)pulse_count * 1000000.0 / time_delta_us;

    return frequency;
}