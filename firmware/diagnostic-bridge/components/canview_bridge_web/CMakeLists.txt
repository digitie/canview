set(CANVIEW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
idf_build_get_property(web_python PYTHON)
set(WEB_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(WEB_ASSET_SCRIPT "${CANVIEW_ROOT}/tools/generate_bridge_web_assets.py")
set(WEB_ASSET_INPUT "${CANVIEW_ROOT}/ui/diagnostic-web/bridge-shell.html")
file(MAKE_DIRECTORY "${WEB_GEN_DIR}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${WEB_ASSET_SCRIPT}" "${WEB_ASSET_INPUT}")
execute_process(
    COMMAND "${web_python}" -B "${WEB_ASSET_SCRIPT}" --input "${WEB_ASSET_INPUT}"
            --output-dir "${WEB_GEN_DIR}"
    RESULT_VARIABLE web_asset_result)
if(NOT web_asset_result EQUAL 0)
    message(FATAL_ERROR "Diagnostic Bridge web asset generation failed")
endif()

idf_component_register(
    SRCS "canview_bridge_web.c" "dns_server.c" "${WEB_GEN_DIR}/bridge_assets.c"
    INCLUDE_DIRS "include" "${WEB_GEN_DIR}" "${CANVIEW_ROOT}/shared/interface"
    REQUIRES canview_bridge_auth esp_http_server esp_wifi esp_event esp_netif esp_timer
             esp_system nvs_flash cjson lwip mbedtls)
# This component includes ESP-IDF headers. The IDF component toolchain language mode
# is retained here; the implementation itself remains C99-compatible. The portable
# authentication component below is the strict C99 boundary.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -Wshadow -Wformat=2)
