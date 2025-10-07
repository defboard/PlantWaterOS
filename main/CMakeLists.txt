idf_component_register(
    SRCS "SensorBoard.cpp" "Wifi.cpp" "WifiCredentials.cpp"
    INCLUDE_DIRS "."
    REQUIRES ElegantOTA arduino-esp32
)
