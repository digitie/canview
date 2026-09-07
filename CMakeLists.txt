cmake_minimum_required(VERSION 3.25)
project(canview_foundation VERSION 0.2.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(cmake/CanviewWarnings.cmake)
option(CANVIEW_COVERAGE "Clang source coverage for portable code" OFF)
option(CANVIEW_SANITIZE "Address/undefined sanitizer (non-Windows)" OFF)
if(CANVIEW_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Coverage requires Clang llvm-profdata/llvm-cov")
    endif()
    target_compile_options(canview_warnings INTERFACE -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(canview_warnings INTERFACE -fprofile-instr-generate)
endif()
if(CANVIEW_SANITIZE)
    if(WIN32)
        message(FATAL_ERROR "Use Linux portability job for ASan+UBSan; not a Windows PASS.")
    endif()
    target_compile_options(canview_warnings INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(canview_warnings INTERFACE -fsanitize=address,undefined)
endif()
find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)
add_subdirectory(shared/protocol)
add_subdirectory(shared/app)
add_subdirectory(firmware/communicator/stm32/module)
add_subdirectory(firmware/module/esp_core)
include(CTest)
if(BUILD_TESTING)
    foreach(bridge 0 1)
        if(bridge)
            set(esp_prefix canview-esp32-bridge)
            set(esp_test_prefix esp32-bridge)
            set(esp_board diagnostic-bridge)
        else()
            set(esp_prefix canview-esp32)
            set(esp_test_prefix esp32)
            set(esp_board communicator/esp32)
        endif()
        add_executable(${esp_prefix}-app-tests tests/esp_core/test_app.c firmware/app/esp_core.c)
        target_link_libraries(${esp_prefix}-app-tests PRIVATE canview_esp_core canview_warnings)
        foreach(scenario open gpio watchdog memory pool wait late healthy null-safe null-idle preflight)
            add_test(NAME ${esp_test_prefix}-app-${scenario} COMMAND ${esp_prefix}-app-tests ${scenario})
            set_tests_properties(${esp_test_prefix}-app-${scenario} PROPERTIES TIMEOUT 30)
        endforeach()
        add_executable(${esp_prefix}-runtime-tests tests/esp_core/test_runtime.c
            firmware/platform/esp32s3/core_runtime.c firmware/${esp_board}/bsp/runtime.c)
        target_compile_definitions(${esp_prefix}-runtime-tests PRIVATE CANVIEW_ESP_SDK_TEST=1)
        target_include_directories(${esp_prefix}-runtime-tests PRIVATE
            tests/esp_core firmware/${esp_board}/bsp)
        target_link_libraries(${esp_prefix}-runtime-tests PRIVATE canview_esp_core canview_warnings)
        add_test(NAME ${esp_test_prefix}-runtime-sdk-model COMMAND ${esp_prefix}-runtime-tests)
        set_tests_properties(${esp_test_prefix}-runtime-sdk-model PROPERTIES TIMEOUT 30)
        add_executable(${esp_prefix}-core-tests tests/esp_core/test_core.c)
        target_link_libraries(${esp_prefix}-core-tests PRIVATE canview_esp_core canview_warnings)
        foreach(scenario boot health faults pool)
            add_test(NAME ${esp_test_prefix}-core-${scenario} COMMAND ${esp_prefix}-core-tests ${scenario})
            set_tests_properties(${esp_test_prefix}-core-${scenario} PROPERTIES TIMEOUT 30)
        endforeach()
        foreach(kind app runtime core)
            target_compile_definitions(${esp_prefix}-${kind}-tests PRIVATE CANVIEW_TEST_BRIDGE=${bridge})
        endforeach()
    endforeach()
    # Run the real Bridge app+BSP+SDK adapter composition against the SDK fixture.
    # The fixture stops app_main at its terminal idle callback, so this is not a
    # substitute for an ESP-IDF build or physical board/HIL test.
    add_executable(canview-esp32-bridge-integration-tests
        tests/esp_core/test_bridge_integration.c
        firmware/app/esp_core.c
        firmware/platform/esp32s3/core_runtime.c
        firmware/diagnostic-bridge/bsp/board.c
        firmware/diagnostic-bridge/bsp/runtime.c)
    target_compile_definitions(canview-esp32-bridge-integration-tests PRIVATE
        CANVIEW_ESP_SDK_TEST=1 CANVIEW_TEST_BRIDGE=1)
    target_include_directories(canview-esp32-bridge-integration-tests PRIVATE
        tests/esp_core firmware/diagnostic-bridge/bsp)
    target_link_libraries(canview-esp32-bridge-integration-tests PRIVATE
        canview_esp_core canview_warnings)
    add_test(NAME esp32-bridge-app-integration COMMAND canview-esp32-bridge-integration-tests)
    set_tests_properties(esp32-bridge-app-integration PROPERTIES TIMEOUT 30)
    # 실제 서로 다른 BSP object와 runtime object를 교차 link해 app preflight가
    # GPIO 또는 platform open보다 먼저 mismatch를 거부하는지 확인한다.
    add_executable(canview-esp32-wrong-bsp-communicator-runtime
        tests/esp_core/test_wrong_bsp.c
        firmware/app/esp_core.c
        firmware/communicator/esp32/bsp/runtime.c
        firmware/diagnostic-bridge/bsp/board.c)
    target_link_libraries(canview-esp32-wrong-bsp-communicator-runtime PRIVATE
        canview_esp_core canview_warnings)
    add_test(NAME esp32-wrong-bsp-communicator-runtime
        COMMAND canview-esp32-wrong-bsp-communicator-runtime)
    set_tests_properties(esp32-wrong-bsp-communicator-runtime PROPERTIES TIMEOUT 30)
    add_executable(canview-esp32-wrong-bsp-bridge-runtime
        tests/esp_core/test_wrong_bsp.c
        firmware/app/esp_core.c
        firmware/diagnostic-bridge/bsp/runtime.c
        firmware/communicator/esp32/bsp/board.c)
    target_link_libraries(canview-esp32-wrong-bsp-bridge-runtime PRIVATE
        canview_esp_core canview_warnings)
    add_test(NAME esp32-wrong-bsp-bridge-runtime COMMAND canview-esp32-wrong-bsp-bridge-runtime)
    set_tests_properties(esp32-wrong-bsp-bridge-runtime PROPERTIES TIMEOUT 30)
    find_package(Threads REQUIRED)
    add_executable(canview-esp32-pool-threads tests/esp_core/test_pool_threads.c)
    target_link_libraries(canview-esp32-pool-threads PRIVATE canview_esp_core canview_warnings Threads::Threads)
    add_test(NAME esp32-pool-threads COMMAND canview-esp32-pool-threads)
    set_tests_properties(esp32-pool-threads PROPERTIES TIMEOUT 30)
    add_executable(canview-stm32-core-tests firmware/communicator/stm32/tests/test_core.c)
    add_executable(canview-stm32-register-tests
        firmware/communicator/stm32/tests/test_registers.c
        firmware/communicator/stm32/platform/stm32g474/core_hw.c)
    target_compile_definitions(canview-stm32-register-tests PRIVATE CANVIEW_STM_REGISTER_TEST=1)
    target_include_directories(canview-stm32-register-tests PRIVATE
        firmware/communicator/stm32/tests firmware/communicator/stm32/platform/stm32g474)
    target_link_libraries(canview-stm32-register-tests PRIVATE canview_stm_core canview_warnings)
    add_test(NAME stm32-core-register-model COMMAND canview-stm32-register-tests)
    set_tests_properties(stm32-core-register-model PROPERTIES TIMEOUT 30)
    target_link_libraries(canview-stm32-core-tests PRIVATE canview_stm_core canview_warnings)
    foreach(scenario boot scheduler-validation scheduler-healthy scheduler-faults queue)
        add_test(NAME stm32-core-${scenario} COMMAND canview-stm32-core-tests ${scenario})
        set_tests_properties(stm32-core-${scenario} PROPERTIES TIMEOUT 30)
    endforeach()
    add_subdirectory(tests/foundation)
    # ESP-NOW v1.3 and Communicator UART v1.0 ABIs are generated separately and
    # tested independently before runtime transport integration is enabled.
    add_executable(canview-protocol-header-c11 tests/protocol/header_c11.c)
    set_target_properties(canview-protocol-header-c11 PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF)
    target_include_directories(canview-protocol-header-c11 PRIVATE ${PROJECT_SOURCE_DIR}/protocol)
    target_link_libraries(canview-protocol-header-c11 PRIVATE canview_warnings)
    add_test(NAME protocol-header-c11 COMMAND canview-protocol-header-c11)
    add_executable(canview-protocol-header-c99 tests/protocol/header_c11.c)
    set_target_properties(canview-protocol-header-c99 PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF)
    target_include_directories(canview-protocol-header-c99 PRIVATE ${PROJECT_SOURCE_DIR}/protocol)
    target_link_libraries(canview-protocol-header-c99 PRIVATE canview_warnings)
    add_test(NAME protocol-header-c99 COMMAND canview-protocol-header-c99)
    add_executable(canview-espnow-tests tests/protocol/test_espnow.c)
    target_include_directories(canview-espnow-tests PRIVATE
        ${PROJECT_SOURCE_DIR}/protocol
        ${PROJECT_SOURCE_DIR}/shared/protocol/include
        ${PROJECT_SOURCE_DIR}/shared/interface)
    target_compile_definitions(canview-espnow-tests PRIVATE
        CANVIEW_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    target_link_libraries(canview-espnow-tests PRIVATE canview_wire canview_warnings)
    foreach(scenario vectors malformed tlv-contracts session security control qos pool-fuzz)
        add_test(NAME espnow-${scenario} COMMAND canview-espnow-tests ${scenario})
        set_tests_properties(espnow-${scenario} PROPERTIES TIMEOUT 60)
    endforeach()
    add_executable(canview-uart-tests tests/protocol/test_uart.c)
    target_include_directories(canview-uart-tests PRIVATE
        ${PROJECT_SOURCE_DIR}/protocol
        ${PROJECT_SOURCE_DIR}/shared/protocol/include
        ${PROJECT_SOURCE_DIR}/shared/interface)
    target_link_libraries(canview-uart-tests PRIVATE canview_wire canview_warnings)
    foreach(scenario matrix direction malformed stream long-stream soak-smoke payload-lifetime plan plan-boundaries command command-boundaries link link-boundaries limits session replay)
        add_test(NAME uart-${scenario} COMMAND canview-uart-tests ${scenario})
        set_tests_properties(uart-${scenario} PROPERTIES TIMEOUT 60)
    endforeach()
    add_subdirectory(tests/automation)
    add_test(NAME generated-transport COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tools/generate_transport.py --check)
    add_test(NAME generated-boards COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tools/generate_boards.py --check)
    add_test(NAME generated-check COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tools/check_generated.py)
    add_test(NAME generated-protocol COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tools/generate_protocol.py --check)
    add_test(NAME generated-uart COMMAND ${Python3_EXECUTABLE} -X utf8 -B
        ${PROJECT_SOURCE_DIR}/tools/generate_protocol.py --schema
        ${PROJECT_SOURCE_DIR}/protocol/schema/uart-v1.0.yaml --check)
    add_test(NAME uart-schema COMMAND ${Python3_EXECUTABLE} -X utf8 -B
        ${PROJECT_SOURCE_DIR}/tests/protocol/test_uart_schema.py -v)
    add_test(NAME uart-fault-stream COMMAND ${Python3_EXECUTABLE} -X utf8 -B
        ${PROJECT_SOURCE_DIR}/tests/protocol/uart_fault_stream.py --seed 1 --duration-seconds 86400)
    set_tests_properties(uart-fault-stream PROPERTIES TIMEOUT 300)
    add_test(NAME protocol-schema COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tests/protocol/test_schema.py -v)
    add_test(NAME espnow-reference COMMAND ${Python3_EXECUTABLE} -X utf8 -B
        ${PROJECT_SOURCE_DIR}/tests/protocol/test_espnow_reference.py -v)
    add_test(NAME espnow-fault-transport COMMAND ${Python3_EXECUTABLE} -X utf8 -B
        ${PROJECT_SOURCE_DIR}/tests/protocol/fault_transport.py --seed 1 --loss 0,1,5,20,50)
    add_test(NAME budget-check COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tools/check_budgets.py)
    add_test(NAME negative-fixtures COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tools/check_negative_fixtures.py)
    add_test(NAME foundation-python COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tests/foundation/test_generators.py -v)
    add_test(NAME esp32-sdkconfig-negative COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tests/foundation/test_sdkconfig.py -v)
    add_test(NAME python-unit COMMAND ${Python3_EXECUTABLE} -X utf8 -B -m unittest discover -s ${PROJECT_SOURCE_DIR}/tests -p "test_*.py")
    add_test(NAME plan-validation COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tools/validate_plan.py)
    add_test(NAME document-links COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tools/validate_document_links.py)
endif()
