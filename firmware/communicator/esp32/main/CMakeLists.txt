set(CANVIEW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
idf_component_register(
    SRCS "${CANVIEW_ROOT}/firmware/app/startup.c" "../bsp/board.c"
    INCLUDE_DIRS "../bsp" "${CANVIEW_ROOT}/firmware/interface"
    REQUIRES canview_foundation canview_esp32_platform esp_psram
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    CANVIEW_ENTRY_ESP_IDF=1 CANVIEW_TARGET_ROLE=CANVIEW_APP_COMMUNICATOR_ESP32)
set_target_properties(${COMPONENT_LIB} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c99 -Wall -Wextra -Werror -Wpedantic
    -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
