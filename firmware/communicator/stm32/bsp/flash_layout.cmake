# Host/Arm에서 동일 C99 BSP 배치 검사를 빌드한다. 실제 Flash IO는 아직 연결하지 않는다.
add_library(canview_stm_flash_layout STATIC "${CMAKE_CURRENT_LIST_DIR}/flash_layout.c")
target_include_directories(canview_stm_flash_layout PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/../interface"
    "${CMAKE_CURRENT_LIST_DIR}/../../../../shared/interface")
target_link_libraries(canview_stm_flash_layout PRIVATE canview_warnings)
