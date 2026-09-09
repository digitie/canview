cmake_minimum_required(VERSION 3.25)
project(canview_foundation VERSION 0.2.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(cmake/CanviewWarnings.cmake)
option(CANVIEW_COVERAGE "Clang source coverage for portable code" OFF)
option(CANVIEW_SANITIZE "Address/undefined sanitizer (non-Windows)" OFF)
option(CANVIEW_LONG_TESTS "Enable explicitly requested long-duration host tests" OFF)
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
add_library(canview_stm_build_provider STATIC
    firmware/communicator/stm32/bsp/build_metadata.c)
target_include_directories(canview_stm_build_provider PUBLIC
    firmware/communicator/stm32/interface
    firmware/communicator/stm32/bsp
    shared/interface
    protocol)
target_compile_features(canview_stm_build_provider PUBLIC c_std_99)
target_compile_definitions(canview_stm_build_provider PRIVATE CANVIEW_STM_CAPTURE_ONLY_CONTRACT=1)
target_compile_options(canview_stm_build_provider PRIVATE
    -include "${PROJECT_SOURCE_DIR}/firmware/communicator/stm32/interface/canview_build_mode.h")
target_link_libraries(canview_stm_build_provider PRIVATE canview_warnings)
add_subdirectory(firmware/communicator/stm32/module)
add_subdirectory(firmware/module/esp_core)
add_library(canview_bridge_auth_host STATIC
    firmware/diagnostic-bridge/components/canview_bridge_auth/canview_bridge_auth.c)
target_include_directories(canview_bridge_auth_host PUBLIC
    firmware/diagnostic-bridge/components/canview_bridge_auth/include
    shared/interface)
target_link_libraries(canview_bridge_auth_host PUBLIC canview_warnings)
add_library(canview_bridge_web_session_host STATIC
    firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web_session.c)
target_include_directories(canview_bridge_web_session_host PUBLIC
    firmware/diagnostic-bridge/components/canview_bridge_web/include)
target_link_libraries(canview_bridge_web_session_host PUBLIC canview_warnings)
include(CTest)
if(BUILD_TESTING)
    add_executable(canview-bridge-auth-tests tests/bridge_web/test_auth.c)
    target_link_libraries(canview-bridge-auth-tests PRIVATE canview_bridge_auth_host canview_warnings)
    add_test(NAME bridge-auth COMMAND canview-bridge-auth-tests)
    set_tests_properties(bridge-auth PROPERTIES TIMEOUT 30)
    add_executable(canview-bridge-web-session-tests tests/bridge_web/test_session.c)
    target_link_libraries(canview-bridge-web-session-tests PRIVATE
        canview_bridge_web_session_host canview_warnings)
    add_test(NAME bridge-web-session COMMAND canview-bridge-web-session-tests)
    set_tests_properties(bridge-web-session PROPERTIES TIMEOUT 30)
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
    # Run the real Bridge bootstrap+BSP+SDK adapter composition against the SDK
    # fixture. Network/NVS are intentionally outside this portable fixture; the
    # target project builds the complete app composition separately.
    add_executable(canview-esp32-bridge-integration-tests
        tests/esp_core/test_bridge_integration.c
        firmware/diagnostic-bridge/main/bridge_bootstrap.c
        firmware/platform/esp32s3/core_runtime.c
        firmware/diagnostic-bridge/bsp/board.c
        firmware/diagnostic-bridge/bsp/runtime.c)
    target_compile_definitions(canview-esp32-bridge-integration-tests PRIVATE
        CANVIEW_ESP_SDK_TEST=1 CANVIEW_TEST_BRIDGE=1)
    target_include_directories(canview-esp32-bridge-integration-tests PRIVATE
        tests/esp_core firmware/diagnostic-bridge/bsp firmware/diagnostic-bridge/main)
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
    add_executable(canview-stm32-platform-tests firmware/communicator/stm32/tests/test_platform.c)
    target_compile_definitions(canview-stm32-platform-tests PRIVATE CANVIEW_STM_CAPTURE_ONLY_CONTRACT=1)
    target_link_libraries(canview-stm32-platform-tests PRIVATE canview_stm_core canview_warnings)
    add_test(NAME stm32-platform-contracts COMMAND canview-stm32-platform-tests)
    set_tests_properties(stm32-platform-contracts PROPERTIES TIMEOUT 30)
    add_executable(canview-stm32-fdcan-tests tests/stm32/test_fdcan_capture.c)
    target_link_libraries(canview-stm32-fdcan-tests PRIVATE canview_stm_core canview_wire canview_warnings)
    add_test(NAME stm32-fdcan-capture COMMAND canview-stm32-fdcan-tests)
    set_tests_properties(stm32-fdcan-capture PROPERTIES TIMEOUT 30)
    add_executable(canview-stm32-uart-link-tests tests/stm32/test_uart_link.c)
    target_link_libraries(canview-stm32-uart-link-tests PRIVATE canview_stm_core canview_wire canview_warnings)
    add_test(NAME stm32-uart-link COMMAND canview-stm32-uart-link-tests)
    set_tests_properties(stm32-uart-link PROPERTIES TIMEOUT 60)
    add_executable(canview-stm32-uart-platform-tests
        tests/stm32/test_uart_dma_platform.c
        tests/stm32/fake_stm32/fake_hardware.c
        firmware/communicator/stm32/platform/stm32g474/uart_dma.c)
    target_include_directories(canview-stm32-uart-platform-tests BEFORE PRIVATE
        tests/stm32/fake_stm32
        firmware/communicator/stm32/platform/stm32g474)
    target_compile_options(canview-stm32-uart-platform-tests PRIVATE
        "SHELL:-include \"${PROJECT_SOURCE_DIR}/tests/stm32/fake_stm32/core_hw.h\""
        "SHELL:-include \"${PROJECT_SOURCE_DIR}/tests/stm32/fake_stm32/safe_gpio.h\""
        "SHELL:-include \"${PROJECT_SOURCE_DIR}/tests/stm32/fake_stm32/board_pins.h\"")
    target_compile_definitions(canview-stm32-uart-platform-tests PRIVATE
        CANVIEW_STM_UART_PLATFORM_TEST=1)
    target_link_libraries(canview-stm32-uart-platform-tests PRIVATE
        canview_stm_core canview_wire canview_warnings)
    add_test(NAME stm32-uart-platform COMMAND canview-stm32-uart-platform-tests)
    set_tests_properties(stm32-uart-platform PROPERTIES TIMEOUT 30)
    add_executable(canview-stm32-fdcan-platform-tests
        tests/stm32/test_fdcan_platform.c
        tests/stm32/fake_stm32/fake_hardware.c
        firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c)
    target_include_directories(canview-stm32-fdcan-platform-tests BEFORE PRIVATE
        tests/stm32/fake_stm32
        firmware/communicator/stm32/platform/stm32g474)
    target_compile_options(canview-stm32-fdcan-platform-tests PRIVATE
        "SHELL:-include \"${PROJECT_SOURCE_DIR}/tests/stm32/fake_stm32/core_hw.h\""
        "SHELL:-include \"${PROJECT_SOURCE_DIR}/tests/stm32/fake_stm32/safe_gpio.h\""
        "SHELL:-include \"${PROJECT_SOURCE_DIR}/tests/stm32/fake_stm32/board_pins.h\"")
    target_compile_definitions(canview-stm32-fdcan-platform-tests PRIVATE
        CANVIEW_STM_FDCAN_TEST=1)
    target_link_libraries(canview-stm32-fdcan-platform-tests PRIVATE
        canview_stm_core canview_wire canview_warnings)
    add_test(NAME stm32-fdcan-platform COMMAND canview-stm32-fdcan-platform-tests)
    set_tests_properties(stm32-fdcan-platform PROPERTIES TIMEOUT 30)
    target_compile_definitions(canview-stm32-register-tests PRIVATE CANVIEW_STM_REGISTER_TEST=1)
    target_compile_definitions(canview-stm32-register-tests PRIVATE CANVIEW_STM_CAPTURE_ONLY_CONTRACT=1)
    target_include_directories(canview-stm32-register-tests PRIVATE
        firmware/communicator/stm32/tests firmware/communicator/stm32/platform/stm32g474
        firmware/communicator/stm32/bsp)
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
    add_test(NAME stm32-build-mode-negative COMMAND ${Python3_EXECUTABLE} -X utf8 -B
        ${PROJECT_SOURCE_DIR}/tools/check_stm32_build_mode.py
        --compiler ${CMAKE_C_COMPILER}
        --header ${PROJECT_SOURCE_DIR}/firmware/communicator/stm32/interface/canview_build_mode.h)
    add_test(NAME uart-schema COMMAND ${Python3_EXECUTABLE} -X utf8 -B
        ${PROJECT_SOURCE_DIR}/tests/protocol/test_uart_schema.py -v)
    # Keep the default CTest contract bounded. The 24-hour virtual soak remains
    # available only when explicitly enabled by a caller with a matching job
    # timeout; it must not make ordinary PR validation exceed the preset limit.
    add_test(NAME uart-fault-stream COMMAND ${Python3_EXECUTABLE} -X utf8 -B
        ${PROJECT_SOURCE_DIR}/tests/protocol/uart_fault_stream.py --seed 1 --duration-seconds 1)
    set_tests_properties(uart-fault-stream PROPERTIES TIMEOUT 60)
    if(CANVIEW_LONG_TESTS)
        add_test(NAME uart-fault-stream-24h COMMAND ${Python3_EXECUTABLE} -X utf8 -B
            ${PROJECT_SOURCE_DIR}/tests/protocol/uart_fault_stream.py --seed 1 --duration-seconds 86400)
        set_tests_properties(uart-fault-stream-24h PROPERTIES TIMEOUT 90000)
    endif()
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
    # The repository-mounted Windows/WSL path can make the complete Python
    # unit suite exceed one minute even though individual tests remain bounded.
    set_tests_properties(python-unit PROPERTIES TIMEOUT 180)
    add_test(NAME bridge-http-contract COMMAND ${Python3_EXECUTABLE} -X utf8 -B
        ${PROJECT_SOURCE_DIR}/tests/security/bridge_http.py)
    set_tests_properties(bridge-http-contract PROPERTIES TIMEOUT 15)
    add_test(NAME plan-validation COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tools/validate_plan.py)
    add_test(NAME document-links COMMAND ${Python3_EXECUTABLE} -X utf8 -B ${PROJECT_SOURCE_DIR}/tools/validate_document_links.py)
endif()
