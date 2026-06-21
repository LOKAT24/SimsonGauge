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
| **Wejście pomiaru RPM** (MCPWM capture) | **1** |
| **Wejście pomiaru prędkości** (MCPWM capture) | **3** |
| Testowy generator sygnału (RMT/LEDC) | 2 |

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
  - **Motyw** (całe ekrany główne): `NFS`, `Simson Clasic`, `Simson Clasic Night` —
    przełączane `< >`, wybór zapamiętywany (NVS).
  - **Jasność ekranu** — suwak 10–100%, zapamiętywana.
  - **Pomiar** — przycisk otwiera nakładkę „Ustawienia pomiaru":
    - **Mnożnik RPM** `−/+` (`częstotliwość × mnożnik → RPM`): tap = ±0.1, przytrzymanie = ±1.0.
    - **Mnożnik prędkości** `−/+` (analogicznie, dla wejścia prędkości).
    - **Algorytm pomiaru** — przełącznik `ISR (A)` / `Bufor (B)`.
    - wszystkie zapamiętywane (NVS).
  - przycisk **OTA**.
  - na dole ekranu — **wersja firmware** (`FW_VERSION`, brana z tagu git przy budowaniu;
    patrz [Wydawanie wersji](#wydawanie-wersji-release)).
- **Prędkość** jest mierzona z osobnego wejścia częstotliwości (GPIO3) × mnożnik prędkości
  i pokazywana na `ui_SpeedLabel` (motyw NFS).
- Po starcie motywu NFS jest ~8,5 s opóźnienia (intro), potem gauge odświeża się co 50 ms
  (motywy Simson startują od razu, po zakończeniu własnego intro).

### Sygnały wejściowe
Dwa niezależne wejścia częstotliwości (MCPWM capture), konfigurowane w `main.cpp`:
- **RPM — GPIO1:** zmierzona częstotliwość × mnożnik RPM, przeliczana wielomianem 3. stopnia
  (`rpm_to_angle` w `gauge.c`) na kąt wskazówki.
- **Prędkość — GPIO3:** zmierzona częstotliwość × mnożnik prędkości daje wartość prędkości.

### Test bez generatora
W [src/test_signal.cpp](src/test_signal.cpp) jest wbudowany generator (`ENABLE_TEST_SIGNAL`)
na **GPIO2**, w dwóch trybach (`TEST_SIGNAL_MODE`):
- **2 (domyślny):** odtwarzanie realnego przebiegu Simsona (~120 Hz) z `simson_waveform.h`,
  sprzętowo przez RMT.
- **1:** stały prostokąt `TEST_SIGNAL_HZ` (3500 Hz) przez LEDC.

Połącz zworką **GPIO2 → GPIO1**, by podać sygnał na wejście RPM. Do produkcji ustaw
`ENABLE_TEST_SIGNAL 0`.

## Aktualizacja OTA (z telefonu)

1. Wejdź w **Config** (long-press) i naciśnij **OTA** — pojawi się nakładka z danymi.
2. Na telefonie połącz się z WiFi:
   - SSID: **`MotoGauge-OTA`**
   - hasło: **`motogauge`**
3. Otwórz w przeglądarce **`http://192.168.4.1`**.
4. Wybierz plik `firmware.bin` (z `.pio/build/esp32-s3-amoled-1in43/firmware.bin`) i wyślij.
5. Po zapisie płytka sama się zrestartuje na nowym firmware.

OTA korzysta z dwóch partycji `app` (schemat `default_16MB.csv`, flash 16 MB) — nowy obraz trafia do
nieaktywnej partycji, więc nieudane wgranie nie blokuje urządzenia. Konfiguracja AP w
[src/ota.cpp](src/ota.cpp).

## Wydawanie wersji (release)

Numer wersji jest **wkompilowany automatycznie z tagu git** (`git describe`) — patrz
[scripts/git_version.py](scripts/git_version.py) (pre-build) i [src/version.h](src/version.h).
Wyświetla się na dole ekranu **Config** i w nakładce **OTA**. Build bez tagu pokazuje hash
commita (np. `1f2e983-dirty`), a build dokładnie na tagu — czystą wersję (np. `v1.0.0`).

> **Zasada:** tag musi powstać **przed** buildem, inaczej w firmware wyląduje hash zamiast wersji.

### Dwa pliki w release

| Plik | Zawartość | Dla kogo | Wgrywanie |
|---|---|---|---|
| `MotoGauge_<wersja>.bin` (`firmware.bin`) | sama partycja aplikacji | aktualizacja | przez **OTA** (sekcja wyżej) |
| `MotoGauge_<wersja>_full.bin` (`firmware.factory.bin`) | bootloader + partycje + app | **czysty układ** | programatorem, `write_flash 0x0` |

Pełny obraz na czysty ESP32 przez programator (esptool):

```bash
esptool --chip esp32s3 write_flash 0x0 MotoGauge_<wersja>_full.bin
# w razie potrzeby najpierw:  esptool --chip esp32s3 erase_flash
```

### Sposób A — skrypt (najprościej)

```powershell
.\scripts\release.ps1 v1.0.0
```

Tworzy tag → buduje → wrzuca oba pliki do `release\`. Jeśli masz [GitHub CLI](https://cli.github.com/)
(`gh`) i skonfigurowany remote — od razu pushuje tag i tworzy GitHub Release z oboma plikami.
Bez `gh` zostawia pliki lokalnie (folder `release/` jest w `.gitignore`).

### Sposób B — ręcznie (sam git + VS Code + strona GitHub, bez `gh`)

1. Zacommituj zmiany.
2. Utwórz tag: `Ctrl+Shift+P` → **Git: Create Tag** → `v1.0.0`
   (lub `git tag -a v1.0.0 -m "Release v1.0.0"`).
3. Zbuduj (`pio run`) — firmware dostaje `v1.0.0`.
4. Wypchnij tag: **Git: Push (Follow Tags)** — zwykły Sync domyślnie **nie** pcha tagów
   (można włączyć ustawienie `git.followTagsWhenSync`).
5. Na GitHub: **Releases → Draft a new release →** wybierz tag → przeciągnij oba pliki z
   `.pio/build/esp32-s3-amoled-1in43/` (`firmware.bin` i `firmware.factory.bin`) → **Publish**.

## Struktura `src/`

| Plik | Rola |
|---|---|
| `main.cpp` | `setup()/loop()` — sklejenie modułów, definicje pinów wejść (RPM/prędkość) |
| `gauge.c/.h` | pomiar → kąt wskazówki / prędkość, timery, mnożniki, `rpm_to_angle` |
| `config_screen.cpp/.h` | ekran Config (motyw, jasność, popup pomiaru, OTA, stopka wersji) + nawigacja |
| `settings.cpp/.h` | trwałe ustawienia w NVS (mnożniki RPM/prędkości, motyw, jasność, algorytm) |
| `version.h` | `FW_VERSION` z tagu git (patrz [scripts/git_version.py](scripts/git_version.py)) |
| `ui_callbacks.c` | `speedTextCB` wymagane przez wygenerowany ekran SquareLine |
| `ota.cpp/.h` | SoftAP + serwer HTTP + `Update` |
| `test_signal.cpp/.h`, `simson_waveform.h` | generator testowy: replay Simsona (RMT) / prostokąt (LEDC) |
| `mcpwm_frequency.c/.h` | pomiar częstotliwości RPM (MCPWM capture) |
| `mcpwm_speed.c/.h` | pomiar częstotliwości prędkości (MCPWM capture) |
| `lcd_bsp.*`, `FT3168.*`, `esp_lcd_sh8601.*`, `read_lcd_id_bsp.*`, `lcd_config.h` | BSP wyświetlacza/dotyku |
| `ui/` | interfejs ze SquareLine Studio |

## Uwagi

- Rotacja programowa **wyłączona** (powodowała artefakty przy częściowym odświeżaniu);
  UI projektowany w natywnej orientacji 466×466.
- `LV_COLOR_16_SWAP 1` w [include/lv_conf.h](include/lv_conf.h) — zgodne z formatem obrazów UI.
- PSRAM (8 MB OPI) włączone przez `board_build.arduino.memory_type = qio_opi`.
