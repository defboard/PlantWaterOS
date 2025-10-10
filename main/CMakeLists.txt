idf_component_register(
    SRCS "WaterPlant.cpp" "Wifi.cpp" "WifiCredentials.cpp"
    INCLUDE_DIRS "."
    REQUIRES RTClib ElegantOTA Adafruit_SSD1306 arduino-esp32
)
