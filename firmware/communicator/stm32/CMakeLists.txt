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
set(FDCAN_HAL_SOURCE
    "${STM32CUBE_G4_ROOT}/Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_fdcan.c"
)

foreach(required_path
        "${CMSIS_CORE_ROOT}/Core/Include/core_cm4.h"
        "${CMSIS_DEVICE_ROOT}/Include/stm32g474xx.h"
        "${STARTUP_SOURCE}"
        "${SYSTEM_SOURCE}"
        "${FDCAN_HAL_SOURCE}"
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
    platform/stm32g474/fdcan_capture.c
    platform/stm32g474/uart_dma.c
    platform/stm32g474/syscalls.c
    "${STARTUP_SOURCE}"
    "${SYSTEM_SOURCE}"
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    STM32G474xx
    CANVIEW_TARGET_ROLE=CANVIEW_APP_COMMUNICATOR_STM32
    CANVIEW_STM_CAPTURE_ONLY_CONTRACT=1
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
    "${STM32CUBE_G4_ROOT}/Drivers/STM32G4xx_HAL_Driver/Inc"
)

set(MCU_OPTIONS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

add_subdirectory("${CANVIEW_ROOT}/shared/protocol" "${CMAKE_CURRENT_BINARY_DIR}/shared-protocol")
add_subdirectory("${CANVIEW_ROOT}/shared/app" "${CMAKE_CURRENT_BINARY_DIR}/shared-app")
add_library(canview_stm_build_provider STATIC
    bsp/build_metadata.c)
target_include_directories(canview_stm_build_provider PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/interface"
    "${CMAKE_CURRENT_SOURCE_DIR}/bsp"
    "${CANVIEW_ROOT}/firmware/interface"
    "${CANVIEW_ROOT}/shared/interface"
    "${CANVIEW_ROOT}/protocol")
target_compile_features(canview_stm_build_provider PUBLIC c_std_99)
target_compile_definitions(canview_stm_build_provider PRIVATE CANVIEW_STM_CAPTURE_ONLY_CONTRACT=1)
target_compile_options(canview_stm_build_provider PRIVATE
    -include "${CMAKE_CURRENT_SOURCE_DIR}/interface/canview_build_mode.h")
target_link_libraries(canview_stm_build_provider PRIVATE canview_warnings)
add_subdirectory(module)
set(STM32_CAPTURE_ONLY_COMPILE_OPTIONS
    "$<$<COMPILE_LANGUAGE:C>:-include>"
    "$<$<COMPILE_LANGUAGE:C>:${CMAKE_CURRENT_SOURCE_DIR}/interface/canview_build_mode.h>")
foreach(core canview_wire canview_app)
    target_compile_definitions(${core} PRIVATE CANVIEW_STM_CAPTURE_ONLY_CONTRACT=1)
    target_compile_options(${core} PRIVATE ${STM32_CAPTURE_ONLY_COMPILE_OPTIONS})
endforeach()
foreach(core canview_wire canview_app canview_stm_build_provider canview_stm_core)
    target_compile_options(${core} PRIVATE ${MCU_OPTIONS} -ffunction-sections -fdata-sections -fstack-usage)
endforeach()
target_link_libraries(${TARGET_NAME} PRIVATE canview_wire canview_app canview_stm_core)
target_link_libraries(${TARGET_NAME} PRIVATE canview_stm_build_provider)
# Keep vendor implementation outside the project's ISO warning contract.
set_source_files_properties(
    app/main.c bsp/board.c bsp/core.c platform/stm32g474/safe_gpio.c platform/stm32g474/core_hw.c
    platform/stm32g474/fdcan_capture.c platform/stm32g474/uart_dma.c
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
    "$<$<COMPILE_LANGUAGE:C>:-include>"
    "$<$<COMPILE_LANGUAGE:C>:${CMAKE_CURRENT_SOURCE_DIR}/interface/canview_build_mode.h>"
)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32G474CEUx_FLASH.ld")
set_property(TARGET ${TARGET_NAME} APPEND PROPERTY LINK_DEPENDS
    "${LINKER_SCRIPT}" "${CANVIEW_ROOT}/tools/check_stm32_core.py"
    "${CANVIEW_ROOT}/tools/check_stm32_fdcan_layout.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32g474/fdcan_message_ram.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/register_model.h")
target_link_options(${TARGET_NAME} PRIVATE
    ${MCU_OPTIONS}
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--build-id=sha1
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
    COMMAND ${Python3_EXECUTABLE} -X utf8 -B
        "${CANVIEW_ROOT}/tools/check_stm32_fdcan_layout.py"
        --sdk "${STM32CUBE_G4_ROOT}"
        --contract "${CMAKE_CURRENT_SOURCE_DIR}/platform/stm32g474/fdcan_message_ram.h"
    COMMENT "Generating HEX/BIN, reporting image size, and checking FDCAN RAM layout"
)
