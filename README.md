# MotoGauge — ESP32-S3 AMOLED 1.43" gauge

Motocyklowy wskaźnik (obrotomierz/prędkość) na bazie **Waveshare ESP32-S3-Touch-AMOLED-1.43"**
(okrągły AMOLED 466×466, kontroler **SH8601/CO5300** po QSPI, dotyk **FT3168** po I²C).
Interfejs zrobiony w **SquareLine Studio** na **LVGL 8**. Częstotliwość sygnału wejściowego
przeliczana jest na kąt wychylenia wskaźnika; aktualizacja firmware przez **OTA po WiFi (SoftAP)**.

Projekt **PlatformIO** oparty o platformę **pioarduino** (Arduino-ESP32 3.x / ESP-IDF 5.x —
wymagane przez QSPI `esp_lcd`, którego nie ma oficjalna platforma `espressif32`).

## Mapa pinów

| Funkcja | GPIO |
|---|---|
| LCD CS / PCLK | 9 / 10 |
| LCD QSPI D0..D3 | 11, 12, 13, 14 |
| LCD RST | 21 |
| Touch I²C SDA / SCL (FT3168, addr 0x38) | 47 / 48 |
| **Wejście pomiaru częstotliwości** (MCPWM capture) | **1** |
| Testowy generator sygnału (LEDC) | 2 |

## Budowanie i wgrywanie

```bash
pio run            # kompilacja
pio run -t upload  # wgranie przez USB
pio device monitor # log szeregowy (115200)
```

## Obsługa

- **Ekran Main** — gauge ze wskaźnikiem (`ui_wskaznik`).
- **Long-press** (przytrzymanie) na ekranie gauge — wejście do **Config**; long-press na pustym
  obszarze Config — powrót do gauge.
- **Ekran Config** (budowany w kodzie, `config_screen.cpp`):
  - **Mnożnik RPM** przyciskami `−/+` (`częstotliwość × mnożnik → RPM`): tap = ±0.1,
    przytrzymanie = ±1.0. Wartość **zapamiętywana po restarcie** (NVS).
  - **Motyw** (całe ekrany główne): `NFS` (działa), `Simson Clasic`, `Simson Clasic Night`
    (na razie `(wkrotce)` — aktywują się po dodaniu ich ekranów w `theme_screen()`).
    Wybór zapamiętywany.
  - przycisk **OTA**.
- Ręczne wpisywanie prędkości zostało usunięte (tymczasowe) — docelowo prędkość z tachometru.
- Po starcie jest ~8,5 s opóźnienia (intro), potem wskaźnik aktualizuje się co 50 ms.

### Sygnał wejściowy
Na **GPIO1** podajemy przebieg prostokątny. Zmierzona częstotliwość (MCPWM capture,
do ~20 kHz) jest mnożona przez mnożnik z ekranu Config i przeliczana wielomianem
(`rpm_to_angle`) na kąt wskazówki.

### Test bez generatora
W [src/test_signal.cpp](src/test_signal.cpp) jest generator (`ENABLE_TEST_SIGNAL`) dający
np. 1 kHz na **GPIO2**. Połącz zworką **GPIO2 → GPIO1**, by zasilić gauge testowym sygnałem.
Do produkcji ustaw `ENABLE_TEST_SIGNAL 0`.

## Aktualizacja OTA (z telefonu)

1. Wejdź w **Config** (long-press) i naciśnij **OTA** — pojawi się nakładka z danymi.
2. Na telefonie połącz się z WiFi:
   - SSID: **`MotoGauge-OTA`**
   - hasło: **`motogauge`**
3. Otwórz w przeglądarce **`http://192.168.4.1`**.
4. Wybierz plik `firmware.bin` (z `.pio/build/esp32-s3-amoled-1in43/firmware.bin`) i wyślij.
5. Po zapisie płytka sama się zrestartuje na nowym firmware.

OTA korzysta z dwóch partycji `app` (schemat `default_8MB.csv`) — nowy obraz trafia do
nieaktywnej partycji, więc nieudane wgranie nie blokuje urządzenia. Konfiguracja AP w
[src/ota.cpp](src/ota.cpp).

## Struktura `src/`

| Plik | Rola |
|---|---|
| `main.cpp` | `setup()/loop()` — sklejenie modułów |
| `gauge.c/.h` | pomiar → kąt wskazówki, timery, mnożnik |
| `config_screen.cpp/.h` | ekran Config (mnożnik, motywy), nawigacja, przycisk + nakładka OTA |
| `settings.cpp/.h` | trwałe ustawienia w NVS (mnożnik, motyw) |
| `ui_callbacks.c` | `speedTextCB` wymagane przez wygenerowany ekran SquareLine |
| `ota.cpp/.h` | SoftAP + serwer HTTP + `Update` |
| `test_signal.cpp/.h` | testowy generator częstotliwości |
| `mcpwm_frequency.c/.h` | pomiar częstotliwości (MCPWM capture) |
| `lcd_bsp.*`, `FT3168.*`, `esp_lcd_sh8601.*`, `read_lcd_id_bsp.*`, `lcd_config.h` | BSP wyświetlacza/dotyku |
| `ui/` | interfejs ze SquareLine Studio |

## Uwagi

- Rotacja programowa **wyłączona** (powodowała artefakty przy częściowym odświeżaniu);
  UI projektowany w natywnej orientacji 466×466.
- `LV_COLOR_16_SWAP 1` w [include/lv_conf.h](include/lv_conf.h) — zgodne z formatem obrazów UI.
- PSRAM (8 MB OPI) włączone przez `board_build.arduino.memory_type = qio_opi`.
