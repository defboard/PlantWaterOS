idf_component_register(
    SRCS "PlantWaterOS.cpp" "Wifi.cpp" "WifiCredentials.cpp" "Server.cpp"
    INCLUDE_DIRS "."
    REQUIRES RTClib ElegantOTA Adafruit_SSD1306 arduino-esp32
    EMBED_TXTFILES static/index.html static/hyperapp.js
)
