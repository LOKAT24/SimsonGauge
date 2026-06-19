#ifndef OTA_H
#define OTA_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Request entering OTA mode (safe to call from the LVGL task).
 *
 * The actual WiFi SoftAP + web server are started lazily from ota_loop(),
 * which must run in the Arduino loop() task.
 */
void ota_request(void);

/** @brief Pump the OTA web server. Call from loop(). */
void ota_loop(void);

/** @brief true once the SoftAP + server are up. */
bool ota_is_active(void);

/* AP credentials / URL, for showing on screen. */
const char * ota_ssid(void);
const char * ota_pass(void);
const char * ota_url(void);

#ifdef __cplusplus
}
#endif

#endif // OTA_H
