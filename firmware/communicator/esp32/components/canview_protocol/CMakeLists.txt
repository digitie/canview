set(CANVIEW_PROTOCOL_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../protocol")

if(NOT EXISTS "${CANVIEW_PROTOCOL_INCLUDE_DIR}/canview_protocol.h")
    message(FATAL_ERROR "Generated protocol header was not found: ${CANVIEW_PROTOCOL_INCLUDE_DIR}")
endif()

idf_component_register(
    INCLUDE_DIRS "${CANVIEW_PROTOCOL_INCLUDE_DIR}"
)
