set(CANVIEW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
idf_component_register(
    SRCS "canview_bridge_auth.c"
    INCLUDE_DIRS "include" "${CANVIEW_ROOT}/shared/interface"
    REQUIRES
)
set_target_properties(${COMPONENT_LIB} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c99 -Wall -Wextra -Werror -Wpedantic
    -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
