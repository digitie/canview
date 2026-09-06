cmake_minimum_required(VERSION 3.22)

project(canview_automation_tests LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

add_executable(canview-automation-tests
    test_automation.c
    ../../firmware/controller/components/canview_automation/canview_controller_automation.c
    ../../firmware/controller/components/canview_can/canview_controller_can.c
    ../../firmware/controller/components/canview_can/canview_controller_signal.c
    ../../firmware/controller/components/canview_can/canview_command_tracker.c
    ../../firmware/communicator/stm32/src/canview_auto_sport.c
)

target_include_directories(canview-automation-tests PRIVATE
    ../../firmware/controller/components/canview_automation/include
    ../../firmware/controller/components/canview_can/include
    ../../firmware/communicator/stm32/include
    ../../protocol
)

add_test(NAME automation-state-machines COMMAND canview-automation-tests)

add_executable(canview-automation-regressions
    test_automation_regressions.c
    ../../firmware/controller/components/canview_automation/canview_controller_automation.c
    ../../firmware/communicator/stm32/src/canview_auto_sport.c
    ../../firmware/controller/components/canview_can/canview_command_tracker.c
)
target_include_directories(canview-automation-regressions PRIVATE
    ../../firmware/controller/components/canview_automation/include
    ../../firmware/communicator/stm32/include
    ../../firmware/controller/components/canview_can/include
    ../../protocol
)
foreach(target canview-automation-tests canview-automation-regressions)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /UNDEBUG /utf-8
            /FI${CMAKE_CURRENT_SOURCE_DIR}/msvc_wire_layout.h)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -Wpedantic -UNDEBUG)
    endif()
endforeach()
foreach(scenario brightness-stale brightness-ramp brightness-speed
                 volume-pending volume-invalid headlamp-freshness
                 sport-feedback sport-ownership automation-time-gap command-double-ack)
    add_test(NAME automation-${scenario} COMMAND canview-automation-regressions ${scenario})
endforeach()
