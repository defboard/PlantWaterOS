idf_component_register(
    SRCS "PlantWaterOS.cpp" "Wifi.cpp" "WifiCredentials.cpp" "Server.cpp"
    INCLUDE_DIRS "."
    REQUIRES RTClib ElegantOTA Adafruit_SSD1306 arduino-esp32
    EMBED_FILES
        "${CMAKE_CURRENT_BINARY_DIR}/index.html.gz"
        "${CMAKE_CURRENT_BINARY_DIR}/hyperapp.js.gz"
)

function(gzip source dest)
    get_filename_component(name "${source}" NAME)
    get_filename_component(source "${source}" ABSOLUTE)
    set(target "${dest}/${name}.gz")
    add_custom_command(
        OUTPUT "${target}"
        COMMAND gzip --keep --stdout --best "${source}" > "${target}"
        DEPENDS "${source}"
        VERBATIM
    )
endfunction()

gzip("${COMPONENT_DIR}/static/index.html" ${CMAKE_CURRENT_BINARY_DIR})
gzip("${COMPONENT_DIR}/static/hyperapp.js" ${CMAKE_CURRENT_BINARY_DIR})

# Alternative to using EMBED_FILES above:
# target_add_binary_data(${COMPONENT_TARGET} "${CMAKE_CURRENT_BINARY_DIR}/index.html.gz" BINARY)
# target_add_binary_data(${COMPONENT_TARGET} "${CMAKE_CURRENT_BINARY_DIR}/hyperapp.js.gz" BINARY)
