add_library(canview_wire STATIC
    src/canview_wire.c
    src/canview_uart.c
    src/canview_espnow.c
    src/canview_espnow_contract.c)
target_include_directories(canview_wire PUBLIC
    include
    ../interface
    "${CMAKE_CURRENT_LIST_DIR}/../../protocol")
target_compile_features(canview_wire PUBLIC c_std_99)
if(TARGET canview_warnings)
    target_link_libraries(canview_wire PRIVATE canview_warnings)
endif()
