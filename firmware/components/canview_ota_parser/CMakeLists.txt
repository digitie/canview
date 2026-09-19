# Portable C99 parser만 제공한다. Flash/SDK/RTOS 소유자는 후속 target 통합 계층이다.
set(OTA_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../shared/ota")
idf_component_register(
    SRCS "${OTA_ROOT}/src/cbor_head.c" "${OTA_ROOT}/src/cbor_document.c"
         "${OTA_ROOT}/src/envelope.c" "${OTA_ROOT}/src/manifest.c" "${OTA_ROOT}/src/body.c"
         "${OTA_ROOT}/src/floor.c" "${OTA_ROOT}/src/native_metadata.c" "${OTA_ROOT}/src/native_stm.c"
         "${OTA_ROOT}/src/stage.c"
    INCLUDE_DIRS "${OTA_ROOT}/src" "${OTA_ROOT}/../interface")
set_target_properties(${COMPONENT_LIB} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c99 -Wall -Wextra -Werror -Wpedantic
    -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fstack-usage)
