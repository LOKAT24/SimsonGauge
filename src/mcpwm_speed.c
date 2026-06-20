#include "mcpwm_speed.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "driver/mcpwm_cap.h"
#include "esp_log.h"
#include "esp_check.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "MCPWM_SPEED";

// --- Wspolne parametry --------------------------------------------------------
#define RING_BUFFER_SIZE            8192     // ~512 zdarzen (alg. B): pokrywa 50ms odczytu przy duzej czestotliwosci
#define MCPWM_FREQ_TIMEOUT_MS       500      // brak impulsu dluzej niz tyle => realny postoj => 0
#define MCPWM_FREQ_MAX_HZ           20000.0  // gorna granica wiarygodnosci
#define MCPWM_FREQ_MIN_HZ           5.0      // dolna granica wiarygodnosci
#define MIN_PULSE_WIDTH_US          100      // minimalna szerokosc impulsu (alg. B) - odrzucanie szpilek
#define MEDIAN_WIN                  5        // okno filtra medianowego

// Struktura do przechowywania danych o zdarzeniu przechwycenia (alg. B)
typedef struct {
    uint32_t timestamp;
    mcpwm_capture_edge_t edge;
} capture_event_t;

// --- Statyczne (prywatne) zmienne modulu -------------------------------------
static mcpwm_cap_timer_handle_t cap_timer = NULL;
static mcpwm_cap_channel_handle_t cap_chan = NULL;
static RingbufHandle_t ring_buf = NULL;
static volatile uint32_t last_cap_time_ms = 0;
static portMUX_TYPE s_spinlock = portMUX_INITIALIZER_UNLOCKED;

// Wybor algorytmu: 0 = A (pomiar okresu w ISR + mediana), 1 = B (bufor zboczy + mediana).
static volatile int s_algo = 0;

// Wartosci wyliczone raz w init (zaleza od rozdzielczosci timera)
static uint32_t s_timer_resolution = 0;
static uint32_t s_min_period_ticks = 0;   // odpowiada MCPWM_FREQ_MAX_HZ (krotszy => szpilka)
static uint32_t s_max_period_ticks = 0;   // odpowiada MCPWM_FREQ_MIN_HZ (dluzszy => brak sygnalu)
static uint32_t s_min_pulse_ticks  = 0;   // MIN_PULSE_WIDTH_US w tickach

// --- Stan algorytmu A (okres miedzy zboczami narastajacymi liczony w ISR) ----
static volatile uint32_t a_last_rise_ts = 0;
static volatile uint32_t a_period_buf[MEDIAN_WIN];
static volatile uint8_t  a_period_idx = 0;
static volatile uint8_t  a_period_count = 0;

// --- Stan algorytmu B --------------------------------------------------------
static double b_last_known_freq = 0.0;
static double b_freq_hist[MEDIAN_WIN];
static uint8_t b_hist_idx = 0;
static uint8_t b_hist_count = 0;

// --- Pomocnicze: mediana -----------------------------------------------------
static uint32_t median_u32(const uint32_t *src, int n)
{
    uint32_t tmp[MEDIAN_WIN];
    for (int i = 0; i < n; i++) tmp[i] = src[i];
    for (int i = 1; i < n; i++) {           // sortowanie przez wstawianie (n<=5)
        uint32_t k = tmp[i];
        int j = i - 1;
        while (j >= 0 && tmp[j] > k) { tmp[j + 1] = tmp[j]; j--; }
        tmp[j + 1] = k;
    }
    return tmp[n / 2];
}

static double median_d(const double *src, int n)
{
    double tmp[MEDIAN_WIN];
    for (int i = 0; i < n; i++) tmp[i] = src[i];
    for (int i = 1; i < n; i++) {
        double k = tmp[i];
        int j = i - 1;
        while (j >= 0 && tmp[j] > k) { tmp[j + 1] = tmp[j]; j--; }
        tmp[j + 1] = k;
    }
    return tmp[n / 2];
}

// --- ISR ---------------------------------------------------------------------
static IRAM_ATTR bool capture_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data)
{
    uint32_t ts = edata->cap_value;
    mcpwm_capture_edge_t edge = edata->cap_edge;

    if (s_algo == 0) {
        // --- Algorytm A: licz okres bezposrednio w przerwaniu ---
        if (edge == MCPWM_CAP_EDGE_POS) {
            uint32_t prev = a_last_rise_ts;
            if (prev != 0) {
                uint32_t period = ts - prev;             // uint32 sam ogarnia przepelnienie licznika
                if (period < s_min_period_ticks) {
                    return false;                        // za wczesnie => szpilka: ignoruj, NIE ruszaj referencji
                }
                if (period <= s_max_period_ticks) {      // wiarygodny okres
                    a_period_buf[a_period_idx] = period;
                    a_period_idx = (a_period_idx + 1) % MEDIAN_WIN;
                    if (a_period_count < MEDIAN_WIN) a_period_count++;
                    last_cap_time_ms = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
                }
            }
            a_last_rise_ts = ts;
        }
    } else {
        // --- Algorytm B: wrzuc kazde zbocze do bufora kolowego ---
        capture_event_t event = { .timestamp = ts, .edge = edge };
        xRingbufferSendFromISR(ring_buf, &event, sizeof(capture_event_t), NULL);
        last_cap_time_ms = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    }
    return false;
}

// --- Reset stanu (uzywany przy przelaczaniu algorytmu) -----------------------
static void reset_state(void)
{
    a_last_rise_ts = 0;
    a_period_idx = 0;
    a_period_count = 0;
    b_last_known_freq = 0.0;
    b_hist_idx = 0;
    b_hist_count = 0;
    if (ring_buf) {
        size_t item_size;
        void *item = NULL;
        while ((item = xRingbufferReceive(ring_buf, &item_size, 0)) != NULL) {
            vRingbufferReturnItem(ring_buf, item);
        }
    }
    last_cap_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void mcpwm_speed_set_algorithm(int algo)
{
    s_algo = algo ? 1 : 0;
    reset_state();
    ESP_LOGI(TAG, "Wybrano algorytm pomiaru: %s", s_algo == 0 ? "A (ISR/okres)" : "B (bufor zboczy)");
}

int mcpwm_speed_get_algorithm(void)
{
    return s_algo;
}

esp_err_t mcpwm_speed_init(int gpio_num)
{
    ESP_LOGI(TAG, "Konfiguracja MCPWM Capture dla GPIO%d", gpio_num);
    ring_buf = xRingbufferCreate(RING_BUFFER_SIZE, RINGBUF_TYPE_NOSPLIT);
    if (!ring_buf) {
        ESP_LOGE(TAG, "Nie udalo sie utworzyc bufora kolowego");
        return ESP_FAIL;
    }
    mcpwm_capture_timer_config_t cap_timer_config = { .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT, .group_id = 1 };
    ESP_RETURN_ON_ERROR(mcpwm_new_capture_timer(&cap_timer_config, &cap_timer), TAG, "Blad tworzenia timera capture");
    mcpwm_capture_channel_config_t cap_config = {
        .gpio_num = gpio_num, .prescale = 1,
        .flags.neg_edge = true, .flags.pos_edge = true,
        .flags.pull_up = true, .flags.pull_down = false,
    };
    ESP_RETURN_ON_ERROR(mcpwm_new_capture_channel(cap_timer, &cap_config, &cap_chan), TAG, "Blad tworzenia kanalu capture");
    mcpwm_capture_event_callbacks_t cbs = { .on_cap = capture_callback };
    ESP_RETURN_ON_ERROR(mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, NULL), TAG, "Blad rejestracji callback");
    ESP_RETURN_ON_ERROR(mcpwm_capture_channel_enable(cap_chan), TAG, "Blad wlaczania kanalu");
    ESP_RETURN_ON_ERROR(mcpwm_capture_timer_enable(cap_timer), TAG, "Blad wlaczania timera");
    ESP_RETURN_ON_ERROR(mcpwm_capture_timer_start(cap_timer), TAG, "Blad startu timera");

    // Zbuforuj rozdzielczosc timera i wyliczone progi (zaleza od czestotliwosci zegara capture)
    mcpwm_capture_timer_get_resolution(cap_timer, &s_timer_resolution);
    if (s_timer_resolution > 0) {
        s_min_period_ticks = (uint32_t)(s_timer_resolution / MCPWM_FREQ_MAX_HZ);
        s_max_period_ticks = (uint32_t)(s_timer_resolution / MCPWM_FREQ_MIN_HZ);
        s_min_pulse_ticks  = (s_timer_resolution / 1000000) * MIN_PULSE_WIDTH_US;
    }
    ESP_LOGI(TAG, "Rozdzielczosc timera: %lu Hz (min_period=%lu, max_period=%lu ticks)",
             (unsigned long)s_timer_resolution, (unsigned long)s_min_period_ticks, (unsigned long)s_max_period_ticks);

    last_cap_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return ESP_OK;
}

// --- Algorytm A: odczyt -------------------------------------------------------
static double get_hz_algo_a(void)
{
    if (a_period_count == 0) return 0.0;

    // Skopiuj okno pod ochrona przerwania (krotka sekcja krytyczna)
    uint32_t periods[MEDIAN_WIN];
    int n;
    portENTER_CRITICAL(&s_spinlock);
    n = a_period_count;
    for (int i = 0; i < n; i++) periods[i] = a_period_buf[i];
    portEXIT_CRITICAL(&s_spinlock);

    uint32_t med = median_u32(periods, n);
    if (med == 0) return b_last_known_freq;
    return (double)s_timer_resolution / med;
}

// --- Algorytm B: odczyt (poprawiony) -----------------------------------------
// Roznice wzgledem oryginalu:
//   - prev/last sa LOKALNE => brak fikcyjnego okresu "przez dziure" 50 ms miedzy odczytami,
//   - kazda poprawna czestotliwosc wpada do filtra medianowego (odporne na pojedyncze szpilki),
//   - zero tylko po realnym timeoucie (trzymanie ostatniej wartosci, brak migotania do 0).
static double get_hz_algo_b(void)
{
    capture_event_t prev = {0}, last = {0};
    bool have_last = false, have_prev = false;

    size_t item_size;
    capture_event_t *item = NULL;
    while ((item = (capture_event_t *)xRingbufferReceive(ring_buf, &item_size, 0)) != NULL) {
        capture_event_t cur = *item;
        vRingbufferReturnItem(ring_buf, (void *)item);

        if (!have_last)      { last = cur; have_last = true; continue; }
        if (!have_prev)      { prev = last; last = cur; have_prev = true; continue; }

        // Sekwencja ZBOCZE_NARASTAJACE -> ZBOCZE_OPADAJACE -> ZBOCZE_NARASTAJACE
        if (prev.edge == MCPWM_CAP_EDGE_POS &&
            last.edge == MCPWM_CAP_EDGE_NEG &&
            cur.edge  == MCPWM_CAP_EDGE_POS) {

            uint32_t high_duration = last.timestamp - prev.timestamp;
            uint32_t low_duration  = cur.timestamp  - last.timestamp;

            if (high_duration >= s_min_pulse_ticks && low_duration >= s_min_pulse_ticks) {
                uint32_t period_ticks = cur.timestamp - prev.timestamp;
                if (period_ticks > 0) {
                    double new_freq = (double)s_timer_resolution / period_ticks;
                    if (new_freq <= MCPWM_FREQ_MAX_HZ) {
                        b_freq_hist[b_hist_idx] = new_freq;
                        b_hist_idx = (b_hist_idx + 1) % MEDIAN_WIN;
                        if (b_hist_count < MEDIAN_WIN) b_hist_count++;
                        b_last_known_freq = median_d(b_freq_hist, b_hist_count);
                    }
                }
            }
        }
        prev = last;
        last = cur;
    }
    return b_last_known_freq;
}

double mcpwm_speed_get_hz(void)
{
    if (!cap_chan || !cap_timer) {
        ESP_LOGE(TAG, "Modul MCPWM nie zostal zainicjalizowany!");
        return 0.0;
    }

    // Wspolny timeout: realny brak sygnalu => 0 (i wyczysc stan)
    if ((xTaskGetTickCount() * portTICK_PERIOD_MS - last_cap_time_ms) > MCPWM_FREQ_TIMEOUT_MS) {
        if (a_period_count != 0 || b_last_known_freq != 0.0 || b_hist_count != 0) {
            ESP_LOGI(TAG, "Timeout sygnalu, zerowanie czestotliwosci.");
            reset_state();
        }
        return 0.0;
    }

    if (s_timer_resolution == 0) return b_last_known_freq;

    return (s_algo == 0) ? get_hz_algo_a() : get_hz_algo_b();
}
