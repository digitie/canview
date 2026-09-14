idf_component_register(SRCS "main.c" "../../../../shared/ota/src/native_metadata.c"
    PRIV_INCLUDE_DIRS "../../../../shared/ota/src" "../../../../shared/interface"
    REQUIRES canview_esp32_ota)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
