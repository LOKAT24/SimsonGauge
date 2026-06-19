// Plik: main/main.c

// #include <Arduino.h>

#include "ui.h"  // Dołączamy interfejs użytkownika

extern "C" {

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "bsp.h" // Dołączamy nasz nowy interfejs
// #include "pcnt_frequency.h" // Dołączamy interfejs do licznika PCNT
#include "mcpwm_frequency.h"

// #define PCNT_GPIO 1 // Numer GPIO, na którym będzie działał licznik PCNT
#define MCPWM_GPIO 1 // Numer GPIO, na którym będzie działał MCPWM

static const char *TAG = "MAIN_APP";

// void action_pressed(lv_event_t * e){
//     // Funkcja obsługi zdarzenia naciśnięcia przycisku
//     ESP_LOGI(TAG, "Button pressed: ");
// }

#include <stdint.h> // Potrzebne dla typów int32_t i int64_t

/**
 * @brief Przekształca obroty (RPM) na kąt na podstawie wzoru.
 *
 * Funkcja wykorzystuje arytmetykę stałoprzecinkową (skala 2^32)
 * oraz metodę Hornera do wydajnej kalkulacji na liczbach całkowitych,
 * co jest idealne dla mikrokontrolerów.
 *
 * @param rpm Aktualne obroty na minutę (RPM) jako 32-bitowa liczba całkowita.
 * @return Obliczony kąt jako 32-bitowa liczba całkowita.
 */
int32_t rpm_to_angle(int32_t rpm)
{
    /*
     * Współczynniki z podanego wzoru:
     * f(x) = -0.0000000009463277 x³ + 0.0000135123083132 x² + 0.2731027037933820 x + 6.0
     *
     * Zostały one przeskalowane przez 2^32 (4294967296), aby stały się liczbami całkowitymi.
     */
    const int64_t C3_SCALED = -4;                  // -0.0000000009463277 * 2^32
    const int64_t C2_SCALED = 58036;               //  0.0000135123083132 * 2^32
    const int64_t C1_SCALED = 1172835389;          //  0.2731027037933820 * 2^32
    const int64_t C0_SCALED = 25769803776LL;       //  6.0 * 2^32

    // Skalujemy o 32 bity
    const int SCALE_BITS = 32;

    // Używamy 64-bitowych liczb całkowitych do obliczeń, aby uniknąć przepełnienia
    int64_t x = rpm;

    // Obliczamy wartość wielomianu za pomocą wydajnej metody Hornera:
    // y = c0 + x * (c1 + x * (c2 + x * c3))
    int64_t y_scaled = C0_SCALED + x * (C1_SCALED + x * (C2_SCALED + x * C3_SCALED));

    // Dzielimy wynik przez 2^32 (przesuwając bity w prawo), aby uzyskać ostateczną wartość.
    // Dodajemy połowę skali przed przesunięciem, aby uzyskać matematycznie poprawne zaokrąglenie.
    int32_t angle = (y_scaled + (1LL << (SCALE_BITS - 1))) >> SCALE_BITS;

    return angle;
}
/**
 * @brief Funkcja obsługi zdarzenia LVGL, która jest wywoływana cyklicznie.
 *
 * Ta funkcja jest wywoływana przez timer LVGL i aktualizuje interfejs użytkownika
 * na podstawie zmierzonej częstotliwości.
 *
 * @param timer Wskaźnik do timera LVGL.
 */
void lvgl_event_handler_frequency(lv_timer_t * timer)
{
    // Funkcja obsługi zdarzenia LVGL
    // double frequency = pcnt_freq_get_hz(); // Pobierz częstotliwość z licznika PCNT
    double frequency = mcpwm_freq_get_hz(); // Pobierz częstotliwość z MCPWM Capture
    ESP_LOGI(TAG, "Current frequency: %.2f Hz", frequency);

    //get multiplier from text area
    lv_obj_t * text_area = ui_TextArea2;
    const char * text = lv_textarea_get_text(text_area);
    double multiplier = 1.0; // Domyślny mnożnik
    if (text && text[0] != '\0') {
        // Spróbuj przekonwertować tekst na liczbę zmiennoprzecinkową
        char * endptr;
        multiplier = strtod(text, &endptr);
        if (endptr == text || *endptr != '\0') {
            // Jeśli konwersja się nie powiodła, użyj domyślnego mnożnika
            ESP_LOGW(TAG, "Nieprawidłowy mnożnik: '%s', używam domyślnego 1.0", text);
            multiplier = 1.0;
        }
    } else {
        ESP_LOGW(TAG, "Mnożnik nie został ustawiony, używam domyślnego 1.0");
    }

    int32_t angle = rpm_to_angle(frequency * multiplier); // Przekształć częstotliwość na kąt
    // ESP_LOGI(TAG, "RPM: %d, Angle: %d", rpm, angle);
    //set rotation of wskaznik/image asset rotation
    lv_img_set_angle(ui_wskaznik,angle);


    
    // Możesz tutaj zaktualizować interfejs użytkownika, np. wyświetlić częstotliwość na etykiecie
    // lv_label_set_text(ui_label_frequency, lv_snprintf("%.2f Hz", frequency));
}


static void screen_long_press_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    // Sprawdź, czy zdarzenie to długie przytrzymanie
    if (code == LV_EVENT_LONG_PRESSED) {
        ESP_LOGI(TAG, "Wykryto długie przytrzymanie! Przełączam ekran.");

        // Sprawdź, który ekran jest aktualnie aktywny
        lv_obj_t * active_screen = lv_scr_act();

        // Załaduj drugi ekran z animacją
        // UWAGA: Upewnij się, że nazwy ui_Screen1 i ui_Screen2 są poprawne
        // i zgodne z tym, co wygenerowało SquareLine Studio w pliku ui.h
        if (active_screen == ui_MainScreen) {
            lv_scr_load_anim(ui_ConfigScreen, LV_SCR_LOAD_ANIM_FADE_ON, 400, 0, false);
        } else {
            lv_scr_load_anim(ui_MainScreen, LV_SCR_LOAD_ANIM_FADE_ON, 400, 0, false);
        }
    }
}
/**
 * @brief Funkcja wywoływana przez jednorazowy timer, aby uruchomić cykliczny pomiar.
 */
static void start_measurement_timer_cb(lv_timer_t * timer)
{
    ESP_LOGI(TAG, "Uruchamiam cykliczny timer odczytu częstotliwości.");
    lv_timer_create(lvgl_event_handler_frequency, 50, NULL);
}

void speedTextCB(lv_event_t * e)
{
    //get text from text area
    lv_obj_t * text_area = lv_event_get_target(e);    
    const char * text = lv_textarea_get_text(text_area);
    //wpisanie tesktu do etykiety
    lv_label_set_text(ui_SpeedLabel, text);
}

void app_main(void)
{
    // 1. Inicjalizacja licznika PCNT
    // ESP_LOGI(TAG, "Initializing PCNT on GPIO%d...", PCNT_GPIO);
    // esp_err_t ret = pcnt_freq_init(PCNT_GPIO);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to initialize PCNT: %s", esp_err_to_name(ret));
    //     return; // Zakończ aplikację, jeśli inicjalizacja nie powiodła się
    // }

    // 1. Inicjalizacja MCPWM Capture
    ESP_LOGI(TAG, "Initializing MCPWM Capture on GPIO%d...", MCPWM_GPIO);
    esp_err_t ret = mcpwm_freq_init(MCPWM_GPIO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MCPWM Capture: %s", esp_err_to_name(ret));
        return; // Zakończ aplikację, jeśli inicjalizacja nie powiodła się
    }

    // 1. Uruchom i skonfiguruj cały sprzęt i biblioteki za pomocą jednej funkcji
    ESP_LOGI(TAG, "Initializing board support package...");
    bsp_display_start();

    // 2. Bezpiecznie uzyskaj dostęp do LVGL i zainicjalizuj UI
    ESP_LOGI(TAG, "Initializing UI...");
    if (bsp_lvgl_lock(-1)) {
        ui_init(); // Inicjalizacja UI ze SquareLine Studio
        // Set long press time if supported by your LVGL version and input driver
        // Example for LVGL v8.x (if available):

        // If not available, configure long press time in your input device driver initialization.
        lv_obj_add_event_cb(ui_MainScreen, screen_long_press_handler, LV_EVENT_LONG_PRESSED, NULL);
        lv_obj_add_event_cb(ui_ConfigScreen, screen_long_press_handler, LV_EVENT_LONG_PRESSED, NULL);
         ESP_LOGI(TAG, "Ustawiam opóźniony start timera odczytu częstotliwości (8500 ms)...");
        lv_timer_t * one_shot_timer = lv_timer_create(start_measurement_timer_cb, 8500, NULL);
        lv_timer_set_repeat_count(one_shot_timer, 1);
        bsp_lvgl_unlock();
    }
    
    ESP_LOGI(TAG, "Initialization complete.");
}

} // extern "C"

