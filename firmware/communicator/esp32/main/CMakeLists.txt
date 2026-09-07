set(CANVIEW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
idf_component_register(
    SRCS "../app/main.c" "../bsp/board.c" "../bsp/runtime.c" "../module/health.c" "../module/pool.c"
    INCLUDE_DIRS "../bsp" "../interface" "${CANVIEW_ROOT}/firmware/interface"
    REQUIRES canview_foundation canview_esp32_platform canview_communicator_runtime
)
set_target_properties(${COMPONENT_LIB} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c99 -Wall -Wextra -Werror -Wpedantic
    -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
