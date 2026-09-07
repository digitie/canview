# SDK 없는 정책/pool만 strict C99로 컴파일한다.
idf_component_register(
    SRCS "../../module/esp_core/health.c" "../../module/esp_core/pool.c"
    INCLUDE_DIRS "../../interface" "../../../shared/interface")
set_target_properties(${COMPONENT_LIB} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c99 -Wall -Wextra -Werror -Wpedantic
    -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
