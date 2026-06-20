#ifndef MCPWM_FREQUENCY_H
#define MCPWM_FREQUENCY_H

#include "esp_err.h"

/**
 * @brief Inicjalizuje moduł MCPWM Capture do pomiaru częstotliwości.
 *
 * @param gpio_num Numer pinu GPIO, na którym będzie mierzony sygnał.
 * @return esp_err_t ESP_OK w przypadku sukcesu, w przeciwnym razie kod błędu.
 */
esp_err_t mcpwm_freq_init(int gpio_num);

/**
 * @brief Oblicza i zwraca częstotliwość sygnału w Hz.
 *
 * Funkcja odczytuje precyzyjne znaczniki czasu zarejestrowane przez
 * sprzętowy moduł MCPWM Capture i na ich podstawie oblicza częstotliwość.
 * Jeśli przez 200ms nie zostanie wykryty żaden impuls, funkcja zwróci 0.0.
 *
 * @return double Zmierzona częstotliwość w hercach (Hz).
 */
double mcpwm_freq_get_hz(void);

/**
 * @brief Wybiera algorytm pomiaru czestotliwosci (mozna przelaczac w locie).
 *
 * @param algo 0 = Algorytm A (pomiar okresu w ISR + filtr medianowy),
 *             1 = Algorytm B (bufor zboczy + filtr medianowy).
 */
void mcpwm_freq_set_algorithm(int algo);

/** @return Aktualnie wybrany algorytm (0 = A, 1 = B). */
int mcpwm_freq_get_algorithm(void);

#endif // MCPWM_FREQUENCY_H