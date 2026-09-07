set(CANVIEW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
idf_component_register(
    SRCS "app_main.c" "../bsp/board.c" "../bsp/bridge_button.c" "../bsp/runtime.c"
    INCLUDE_DIRS "../bsp" "${CANVIEW_ROOT}/firmware/interface"
    REQUIRES canview_esp_core canview_esp32_platform canview_esp32_runtime canview_bridge_auth
             canview_bridge_web nvs_flash
)
# app_main includes ESP-IDF headers; keep the vendor component language mode while
# keeping the application implementation C99-compatible.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -Wshadow -Wformat=2)
