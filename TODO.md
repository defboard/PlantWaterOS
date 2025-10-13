 # general

- prevent race conditions using event queue + mutexes
    - pump on + reboot
    - log file read + write
    - pump on + sensor readings -> allow


# Time

- use internal time keeping

- use RTC only once per day to synchronize
- network based time synchronization as an alternative to RTC


# web UI

- hyperapp UI + REST endpoints (JSON)

- reboot button

- async HTTP

- https?

- pump now


# Logging

- rework `fullSystemLog` into `eventLog` + last 100 sensor lines
- binary sensor data log (?)
- MultiplexPrint


# JoyStick control


# settings

- use `Preferences.h` library
- WiFi SSID + password
    - if not defined, start as WiFi station

- sensor pin(s)
- sensor log frequency
- sensor log repeat count + delay

- pump pin(s)
- pump time

- pump condition:
    - min/max time
    - min/max moisture level
    - day time

- SPI pins (SD)
- I2C pins (RTC, display)

- Display resolution
- Button functionality

- time sync: RTC/network
- current time


# OTA

- replace ElegantOTA by our own tool


# Mobile App
