idf_component_register(
    SRCS "canview_controller_can.c" "canview_controller_signal.c"
         "canview_command_tracker.c"
    INCLUDE_DIRS "include"
    REQUIRES canview_protocol
)
set_property(TARGET ${COMPONENT_LIB} PROPERTY C_STANDARD 99)
set_property(TARGET ${COMPONENT_LIB} PROPERTY C_STANDARD_REQUIRED ON)
set_property(TARGET ${COMPONENT_LIB} PROPERTY C_EXTENSIONS OFF)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c99 -Wall -Wextra -Werror -Wpedantic
    -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
