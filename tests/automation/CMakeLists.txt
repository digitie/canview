cmake_minimum_required(VERSION 3.22)

project(canview_automation_tests LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

add_executable(canview-automation-tests
    test_automation.c
    ../../firmware/controller/components/canview_automation/canview_controller_automation.c
    ../../firmware/communicator/stm32/src/canview_auto_sport.c
)

target_include_directories(canview-automation-tests PRIVATE
    ../../firmware/controller/components/canview_automation/include
    ../../firmware/communicator/stm32/include
    ../../protocol
)

target_compile_options(canview-automation-tests PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
)

add_test(NAME automation-state-machines COMMAND canview-automation-tests)
