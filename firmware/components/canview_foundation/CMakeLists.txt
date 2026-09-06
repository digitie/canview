idf_component_register(
    SRCS "../../../shared/protocol/src/canview_wire.c"
         "../../../shared/app/src/canview_app.c"
    INCLUDE_DIRS "../../../shared/protocol/include"
                 "../../../shared/app/include"
                 "../../../shared/interface")
set_property(TARGET ${COMPONENT_LIB} PROPERTY C_STANDARD 99)
set_property(TARGET ${COMPONENT_LIB} PROPERTY C_EXTENSIONS OFF)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c99 -Wall -Wextra -Werror
    -Wpedantic -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes)
