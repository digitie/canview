# SDK 전용 read-only adapter. app/portable core에 vendor header를 노출하지 않는다.
idf_component_register(
    SRCS "../../platform/esp32s3/ota_image.c" "../../platform/esp32s3/ota_crypto.c"
    INCLUDE_DIRS "../../platform/esp32s3" "../../../shared/ota/src" "../../../shared/interface"
    REQUIRES bootloader_support esp_partition esp_app_format spi_flash efuse mbedtls)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -Wshadow -Wformat=2)
