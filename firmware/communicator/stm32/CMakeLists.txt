cmake_minimum_required(VERSION 3.25...4.4)

project(canview_communicator_stm32
    VERSION 0.1.0
    LANGUAGES C ASM
)

if(CMAKE_C_COMPILER_VERSION VERSION_LESS 15.3 OR
   CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 15.4)
    message(FATAL_ERROR
        "Arm GNU Toolchain GCC 15.3.x from release 15.3.Rel1 is required; found ${CMAKE_C_COMPILER_VERSION}."
    )
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)

set(STM32CUBE_G4_ROOT "" CACHE PATH "Path to a pinned STM32CubeG4 checkout")
if(NOT STM32CUBE_G4_ROOT)
    message(FATAL_ERROR
        "STM32CUBE_G4_ROOT is required. See README.md for the pinned dependency setup."
    )
endif()

set(CMSIS_DEVICE_ROOT
    "${STM32CUBE_G4_ROOT}/Drivers/CMSIS/Device/ST/STM32G4xx"
)
set(CMSIS_CORE_ROOT
    "${STM32CUBE_G4_ROOT}/Drivers/CMSIS"
)
set(STARTUP_SOURCE
    "${CMSIS_DEVICE_ROOT}/Source/Templates/gcc/startup_stm32g474xx.s"
)
set(SYSTEM_SOURCE
    "${CMSIS_DEVICE_ROOT}/Source/Templates/system_stm32g4xx.c"
)

foreach(required_path
        "${CMSIS_CORE_ROOT}/Core/Include/core_cm4.h"
        "${CMSIS_DEVICE_ROOT}/Include/stm32g474xx.h"
        "${STARTUP_SOURCE}"
        "${SYSTEM_SOURCE}"
)
    if(NOT EXISTS "${required_path}")
        message(FATAL_ERROR "Required STM32CubeG4 file not found: ${required_path}")
    endif()
endforeach()

set(CANVIEW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
include("${CANVIEW_ROOT}/cmake/CanviewWarnings.cmake")
set(TARGET_NAME canview-communicator-stm32)
set(CANVIEW_BUILD_MODE "CAPTURE_ONLY" CACHE STRING "Only capture-only bench is supported")
if(NOT CANVIEW_BUILD_MODE STREQUAL "CAPTURE_ONLY")
    message(FATAL_ERROR "T-102a supports CAPTURE_ONLY only; no CAN TX implementation is linked")
endif()

add_executable(${TARGET_NAME}
    app/main.c
    bsp/board.c
    bsp/core.c
    platform/stm32g474/safe_gpio.c
    platform/stm32g474/core_hw.c
    platform/stm32g474/syscalls.c
    "${STARTUP_SOURCE}"
    "${SYSTEM_SOURCE}"
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    STM32G474xx
    CANVIEW_TARGET_ROLE=CANVIEW_APP_COMMUNICATOR_STM32
)

target_include_directories(${TARGET_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/bsp"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32g474"
    "${CMAKE_CURRENT_SOURCE_DIR}/interface"
    "${CANVIEW_ROOT}/firmware/interface"
)
target_include_directories(${TARGET_NAME} SYSTEM PRIVATE
    "${CMSIS_CORE_ROOT}/Core/Include"
    "${CMSIS_DEVICE_ROOT}/Include"
)

set(MCU_OPTIONS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

add_subdirectory("${CANVIEW_ROOT}/shared/protocol" "${CMAKE_CURRENT_BINARY_DIR}/shared-protocol")
add_subdirectory("${CANVIEW_ROOT}/shared/app" "${CMAKE_CURRENT_BINARY_DIR}/shared-app")
add_subdirectory(module)
foreach(core canview_wire canview_app canview_stm_core)
    target_compile_options(${core} PRIVATE ${MCU_OPTIONS} -ffunction-sections -fdata-sections -fstack-usage)
endforeach()
target_link_libraries(${TARGET_NAME} PRIVATE canview_wire canview_app canview_stm_core)
# Keep vendor implementation outside the project's ISO warning contract.
set_source_files_properties(
    app/main.c bsp/board.c bsp/core.c platform/stm32g474/safe_gpio.c platform/stm32g474/core_hw.c
    platform/stm32g474/syscalls.c
    PROPERTIES COMPILE_OPTIONS "-Werror;-Wpedantic;-Wconversion;-Wstrict-prototypes;-Wmissing-prototypes;-Wundef")

target_compile_options(${TARGET_NAME} PRIVATE
    ${MCU_OPTIONS}
    -ffunction-sections
    -fdata-sections
    -fstack-usage
    -Wall
    -Wextra
    -Wshadow
    -Wdouble-promotion
    -Wformat=2
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Release>:-Os>
    $<$<CONFIG:Release>:-g0>
)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32G474CEUx_FLASH.ld")
set_property(TARGET ${TARGET_NAME} APPEND PROPERTY LINK_DEPENDS
    "${LINKER_SCRIPT}" "${CANVIEW_ROOT}/tools/check_stm32_core.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/register_model.h")
target_link_options(${TARGET_NAME} PRIVATE
    ${MCU_OPTIONS}
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--cref
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
)

set_target_properties(${TARGET_NAME} PROPERTIES
    SUFFIX ".elf"
)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
        $<TARGET_FILE:${TARGET_NAME}>
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary
        $<TARGET_FILE:${TARGET_NAME}>
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
    COMMAND ${Python3_EXECUTABLE} -X utf8 -B "${CANVIEW_ROOT}/tools/check_stm32_core.py"
        --elf $<TARGET_FILE:${TARGET_NAME}> --compiler "${CMAKE_C_COMPILER}"
        --sdk "${STM32CUBE_G4_ROOT}"
    COMMENT "Generating HEX/BIN and reporting image size"
)
