To use this driver, the Bluefruit library has to be available.
Here is a valid platformio init file:

```
[platformio]
default_envs = adafruit_feather_nrf52840

[env:adafruit_feather_nrf52840]
platform = nordicnrf52
board = adafruit_feather_nrf52840
framework = arduino
monitor_speed = 115200


lib_deps =
  ; It's highly recommended to use a specific tagged release for stability
  ; Check the Adafruit_nRF52_Arduino GitHub page for the latest release tag
  ; Example: adafruit/Adafruit_nRF52_Arduino#1.3.1 (replace 1.3.1 with actual latest tag)
  https://github.com/adafruit/Adafruit_nRF52_Arduino


; --- Library Management ---
; Try 'chain+' first. If issues persist, 'deep+' is more exhaustive but slower.
lib_ldf_mode = deep+

; This is a key setting: it tells PlatformIO to build the Adafruit_nRF52_Arduino
; "library" from sources directly into the project. This can significantly help
; the LDF correctly discover and link its internal/bundled libraries like TinyUSB.
lib_archive = no
```