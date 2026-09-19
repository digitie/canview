# 외부 SDK 원본은 보존한다. pin/clean 검사 후 제한된 build 사본 호환 변환을 적용한다.
find_package(Git REQUIRED)
get_filename_component(canview_boot_repo "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
file(READ "${canview_boot_repo}/tools/toolchain-versions.json" toolchain_versions)
string(JSON mcuboot_pin GET "${toolchain_versions}" sdk mcuboot gitCommit)
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${CANVIEW_MCUBOOT_ROOT}" rev-parse HEAD
    OUTPUT_VARIABLE mcuboot_head OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${CANVIEW_MCUBOOT_ROOT}" status --porcelain=v1 --untracked-files=all
    OUTPUT_VARIABLE mcuboot_dirty OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
if(NOT mcuboot_head STREQUAL mcuboot_pin OR NOT mcuboot_dirty STREQUAL "")
    message(FATAL_ERROR "MCUboot pin/clean mismatch")
endif()
add_library(canview_boot_config INTERFACE)
target_include_directories(canview_boot_config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(canview_boot_config INTERFACE MBEDTLS_CONFIG_FILE="config-asn1.h")
if(CANVIEW_SANITIZE)
    target_compile_options(canview_boot_config INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(canview_boot_config INTERFACE -fsanitize=address,undefined)
endif()
if(CANVIEW_COVERAGE)
    target_compile_options(canview_boot_config INTERFACE -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(canview_boot_config INTERFACE -fprofile-instr-generate)
endif()
if(NOT CMAKE_CROSSCOMPILING)
    target_compile_definitions(canview_boot_config INTERFACE CANVIEW_MCUBOOT_HOST_MODEL=1)
    if(WIN32 AND CMAKE_C_COMPILER_ID STREQUAL "Clang")
        # C volatile 의미 사용. MS acquire/release 확장이 FIH struct를 libatomic 호출로 바꾸지 않는다.
        target_compile_options(canview_boot_config INTERFACE -fno-ms-volatile)
    endif()
endif()
include(mcuboot_compat.cmake)
add_library(canview_boot_crypto STATIC
    "${CANVIEW_MCUBOOT_ROOT}/ext/tinycrypt/lib/source/utils.c"
    "${CANVIEW_MCUBOOT_ROOT}/ext/tinycrypt/lib/source/sha256.c"
    "${canview_ecc_source}"
    "${CANVIEW_MCUBOOT_ROOT}/ext/tinycrypt/lib/source/ecc_dsa.c"
    "${CANVIEW_MCUBOOT_ROOT}/ext/mbedtls-asn1/src/asn1parse.c"
    "${CANVIEW_MCUBOOT_ROOT}/ext/mbedtls-asn1/src/platform_util.c"
    asn1_memory.c verify_only_rng.c)
target_include_directories(canview_boot_crypto PUBLIC
    "${CANVIEW_MCUBOOT_ROOT}/ext/tinycrypt/lib/include"
    "${CANVIEW_MCUBOOT_ROOT}/ext/mbedtls-asn1/include")
target_link_libraries(canview_boot_crypto PUBLIC canview_boot_config)
add_subdirectory("${canview_bootutil_source}" "${CMAKE_CURRENT_BINARY_DIR}/upstream")
if(NOT CMAKE_CROSSCOMPILING)
    # x86에서는 Arm panic assembly만 abort로 대체한다. FIH double-var/CFI는 유지한다.
    get_target_property(bootutil_sources bootutil SOURCES)
    list(REMOVE_ITEM bootutil_sources src/fault_injection_hardening.c)
    set_property(TARGET bootutil PROPERTY SOURCES "${bootutil_sources}")
    target_sources(bootutil PRIVATE "${canview_boot_repo}/tests/ota/mcuboot_host_fih.c")
    target_compile_definitions(bootutil PRIVATE
        CANVIEW_UPSTREAM_FIH_SOURCE="${canview_bootutil_source}/src/fault_injection_hardening.c")
endif()
target_link_libraries(bootutil PUBLIC canview_boot_config canview_boot_crypto)
# upstream 안전 assert는 Release에서도 제거하지 않는다.
target_compile_options(bootutil PRIVATE -UNDEBUG)
# 고정 upstream은 _Static_assert를 사용한다. portable 프로젝트 C99 설정은 유지한다.
set_target_properties(bootutil canview_boot_crypto PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
foreach(vendor_target bootutil canview_boot_crypto)
    target_compile_options(${vendor_target} PRIVATE -Wall -Wextra -Werror)
endforeach()
add_library(canview_boot_flash_map STATIC flash_map.c)
target_include_directories(canview_boot_flash_map PRIVATE ../bsp)
target_link_libraries(canview_boot_flash_map PUBLIC canview_boot_config canview_stm_flash_layout
    PRIVATE canview_warnings)
