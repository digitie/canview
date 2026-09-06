set(CANVIEW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
idf_component_register(
    SRCS "${CANVIEW_ROOT}/firmware/app/startup.c" "../bsp/board.c"
         "${CANVIEW_ROOT}/firmware/platform/esp32s3/canview_gpio.c"
    INCLUDE_DIRS "../bsp" "${CANVIEW_ROOT}/firmware/interface"
    REQUIRES canview_foundation esp_driver_gpio esp_psram
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    CANVIEW_ENTRY_ESP_IDF=1 CANVIEW_TARGET_ROLE=CANVIEW_APP_COMMUNICATOR_ESP32)
# SDK adapter retains SDK GNU mode; portable component is strict ISO C99.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -Wshadow -Wformat=2)
