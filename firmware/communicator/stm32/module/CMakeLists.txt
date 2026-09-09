add_library(canview_stm_core STATIC
    scheduler.c
    queue.c
    stack_watermark.c
    service_policy.c
    diagnostic.c
    fdcan_capture.c
    uart_link.c
    ../app/boot.c)
target_include_directories(canview_stm_core PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/../interface"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../shared/interface"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../shared/protocol/include"
)
target_compile_features(canview_stm_core PUBLIC c_std_99)
target_compile_definitions(canview_stm_core PRIVATE CANVIEW_STM_CAPTURE_ONLY_CONTRACT=1)
target_compile_options(canview_stm_core PRIVATE
    -include "${CMAKE_CURRENT_LIST_DIR}/../interface/canview_build_mode.h")
target_link_libraries(canview_stm_core PRIVATE canview_wire canview_warnings canview_stm_build_provider)
