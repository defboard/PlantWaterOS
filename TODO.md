# general

- display + buttons:
    - let user choose pumpDuration / pumpInterval / nextpumptime

- event queue rework/simplification?
- webserver core

# Time

- use internal time keeping
- use RTC only once per day to synchronize
- network based time synchronization as an alternative to RTC

# Network

- Wifi: Fallback after N failed attempts -> previous wifi
- Wifi: generate password based on chip id/...?

# web UI

- https
- AUTH!

- async HTTP

- display moisture level in %

- more frequent updates .. websockets?
- graph... xaxis/yaxis

- display missing datetimes as "n/a"
- make datetime exchange format more consistent (transfer as ISO8661, parse immediately)
- more human-readable datetime display ("2 days ago"/...)

- progress bar for firmware update

- error reporting in WebUI according to server request status

- display firmware version (build date, commit)

# Logging

- logrotate + access to older logs
- refine the sensordata logging logic -> smoother averaging
- rework `fullSystemLog` into `eventLog` + last 100 sensor lines
- binary sensor data log (?)

# settings

- pump condition:
    - min/max time
    - min/max moisture level
    - day time

- time sync: RTC/network
- current time

- Display resolution
- Button functionality

- sensor log frequency
- sensor log repeat count + delay

- sensor pin(s)
- pump pin(s)

- SPI pins (SD)
- I2C pins (RTC, display)
