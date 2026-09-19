idf_component_register(SRCS "main.c" "metadata.c" "receiver.c"
    "../../../../firmware/communicator/esp32/bsp/ota.c"
    PRIV_INCLUDE_DIRS "../../../../shared/ota/src" "../../../../shared/interface"
    "../../../../firmware/communicator/esp32/bsp"
    REQUIRES canview_esp32_ota canview_ota_parser
    EMBED_FILES "../../ota-signed-golden/communicator.cvota")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -fstack-usage)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u canview_ota_fixture_metadata")
