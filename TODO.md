# general

- Wifi: Fallback
- Wifi: generate password based on chip id/...?

- refine the sensordata logging logic

- display:
    - let user choose pumpDuration / pumpInterval / nextpumptime

- event queue rework?


# Time

- use internal time keeping

- use RTC only once per day to synchronize
- network based time synchronization as an alternative to RTC


# web UI

- simplify table / input creation
- async HTTP
- https?

- AUTH!

- more frequent updates .. websockets?
- graph... xaxis/yaxis


# Logging

- rework `fullSystemLog` into `eventLog` + last 100 sensor lines
- binary sensor data log (?)


# settings

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
