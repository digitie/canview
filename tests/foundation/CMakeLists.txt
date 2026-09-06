add_executable(canview-foundation-tests test_foundation.c)
target_link_libraries(canview-foundation-tests PRIVATE canview_wire canview_app canview_warnings)
foreach(scenario crc envelope cobs stream can sequence nulls noise app)
    add_test(NAME foundation-${scenario} COMMAND canview-foundation-tests ${scenario})
    set_tests_properties(foundation-${scenario} PROPERTIES TIMEOUT 60)
endforeach()

add_executable(canview-wire-vectors wire_vectors.c)
target_link_libraries(canview-wire-vectors PRIVATE canview_wire canview_warnings)
add_test(NAME foundation-golden COMMAND ${Python3_EXECUTABLE} -X utf8 -B
    ${CMAKE_CURRENT_SOURCE_DIR}/test_vectors.py $<TARGET_FILE:canview-wire-vectors>)
set(board_index 0)
set(board_roles CANVIEW_APP_CONTROLLER CANVIEW_APP_COMMUNICATOR_ESP32
    CANVIEW_APP_DIAGNOSTIC_BRIDGE CANVIEW_APP_COMMUNICATOR_STM32)
foreach(board controller communicator/esp32 diagnostic-bridge communicator/stm32)
    set(target canview-bsp-${board_index})
    add_executable(${target} test_bsp.c ${PROJECT_SOURCE_DIR}/firmware/${board}/bsp/board.c)
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/firmware/interface
        ${PROJECT_SOURCE_DIR}/shared/interface
        ${PROJECT_SOURCE_DIR}/firmware/${board}/bsp
        ${PROJECT_SOURCE_DIR}/firmware/communicator/stm32/platform/stm32g474)
    target_compile_definitions(${target} PRIVATE CANVIEW_TEST_BOARD=${board_index})
    target_link_libraries(${target} PRIVATE canview_warnings)
    add_test(NAME foundation-bsp-${board_index} COMMAND ${target})
    # Compile the actual composition root in every role; never run its infinite loop.
    set(startup_target canview-startup-${board_index})
    add_library(${startup_target} OBJECT ${PROJECT_SOURCE_DIR}/firmware/app/startup.c)
    target_include_directories(${startup_target} PRIVATE ${PROJECT_SOURCE_DIR}/firmware/interface
        ${PROJECT_SOURCE_DIR}/shared/interface ${PROJECT_SOURCE_DIR}/shared/app/include)
    list(GET board_roles ${board_index} role)
    target_compile_definitions(${startup_target} PRIVATE CANVIEW_TARGET_ROLE=${role})
    if(NOT board_index EQUAL 3)
        target_compile_definitions(${startup_target} PRIVATE CANVIEW_ENTRY_ESP_IDF=1)
    endif()
    target_link_libraries(${startup_target} PRIVATE canview_warnings)
    math(EXPR board_index "${board_index}+1")
endforeach()
