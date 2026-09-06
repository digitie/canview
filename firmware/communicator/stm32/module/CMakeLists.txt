add_library(canview_stm_core STATIC scheduler.c queue.c ../app/boot.c)
target_include_directories(canview_stm_core PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/../interface"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../shared/interface")
target_compile_features(canview_stm_core PUBLIC c_std_99)
target_link_libraries(canview_stm_core PRIVATE canview_warnings)
