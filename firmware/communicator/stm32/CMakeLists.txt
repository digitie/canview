cmake_minimum_required(VERSION 3.22)

project(canview_communicator_stm32
    VERSION 0.1.0
    LANGUAGES C ASM
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

set(TARGET_NAME canview-communicator-stm32)

add_executable(${TARGET_NAME}
    src/main.c
    src/canview_auto_sport.c
    "${STARTUP_SOURCE}"
    "${SYSTEM_SOURCE}"
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    STM32G474xx
)

target_include_directories(${TARGET_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMSIS_CORE_ROOT}/Core/Include"
    "${CMSIS_DEVICE_ROOT}/Include"
)

set(MCU_OPTIONS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

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
target_link_options(${TARGET_NAME} PRIVATE
    ${MCU_OPTIONS}
    -T${LINKER_SCRIPT}
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--cref
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
    COMMENT "Generating HEX/BIN and reporting image size"
)
