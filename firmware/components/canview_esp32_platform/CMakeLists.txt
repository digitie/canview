# Only this SDK adapter uses the vendor GNU language mode.
idf_component_register(
    SRCS "../../platform/esp32s3/canview_gpio.c"
    INCLUDE_DIRS "../../interface" "../../../shared/interface"
    REQUIRES esp_driver_gpio freertos)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -Wshadow -Wformat=2)
