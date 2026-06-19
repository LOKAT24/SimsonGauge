#ifndef PCNT_FREQUENCY_H
#define PCNT_FREQUENCY_H

#include "esp_err.h"

/**
 * @brief Inicjalizuje licznik PCNT na podanym pinie GPIO.
 *
 * @param gpio_num Numer pinu GPIO, na którym będą zliczane impulsy.
 * @return esp_err_t ESP_OK w przypadku sukcesu, w przeciwnym razie kod błędu.
 */
esp_err_t pcnt_freq_init(int gpio_num);

/**
 * @brief Oblicza i zwraca częstotliwość sygnału w Hz.
 *
 * Funkcja mierzy liczbę impulsów oraz dokładny czas, jaki upłynął
 * od jej ostatniego wywołania, a następnie oblicza częstotliwość.
 *
 * @return double Zmierzona częstotliwość w hercach (Hz).
 */
double pcnt_freq_get_hz(void);

#endif // PCNT_FREQUENCY_H