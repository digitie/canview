idf_component_register(SRCS "main.c" REQUIRES canview_esp32_ota)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
