# Vendor SDK 문법은 adapter 한 component로 제한한다.
idf_component_register(
    SRCS "../../platform/esp32s3/runtime.c"
    INCLUDE_DIRS "../../interface" "../../../../../shared/interface"
    REQUIRES esp_driver_gpio esp_system esp_timer esp_psram spi_flash heap freertos esp_app_format)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -Wshadow -Wformat=2)
