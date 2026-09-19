# 승인된 빌드/제조 입력만 받는다. 업데이트 image/manifest에서 추출하지 않는다.
set(CANVIEW_BOOT_PUBLIC_DER "" CACHE FILEPATH "Trusted P-256 public SPKI DER; never a private key")
set(CANVIEW_BOOT_SECURITY_EPOCH "" CACHE STRING "Explicit manufacturing security epoch")
set(CANVIEW_BOOT_MANIFEST_KEY_ID "" CACHE STRING "Explicit separate manifest trust-root ID")
set(CANVIEW_BOOT_STM_ABI "" CACHE STRING "Explicit supported STM image ABI")
if(NOT CANVIEW_BOOT_PUBLIC_DER STREQUAL "" OR NOT CANVIEW_BOOT_SECURITY_EPOCH STREQUAL "" OR
   NOT CANVIEW_BOOT_MANIFEST_KEY_ID STREQUAL "" OR NOT CANVIEW_BOOT_STM_ABI STREQUAL "")
    foreach(value CANVIEW_BOOT_PUBLIC_DER CANVIEW_BOOT_SECURITY_EPOCH CANVIEW_BOOT_MANIFEST_KEY_ID CANVIEW_BOOT_STM_ABI)
        if("${${value}}" STREQUAL "")
            message(FATAL_ERROR "Boot trust requires explicit ${value}; no default identity/key")
        endif()
    endforeach()
    if(NOT IS_ABSOLUTE "${CANVIEW_BOOT_PUBLIC_DER}" OR NOT EXISTS "${CANVIEW_BOOT_PUBLIC_DER}" OR
       IS_DIRECTORY "${CANVIEW_BOOT_PUBLIC_DER}")
        message(FATAL_ERROR "Boot public DER must be an existing absolute file path")
    endif()
    set(boot_trust_dir "${CMAKE_CURRENT_BINARY_DIR}/trust")
    file(MAKE_DIRECTORY "${boot_trust_dir}")
    set(boot_trust_header "${boot_trust_dir}/canview_boot_trust_generated.h")
    add_custom_command(OUTPUT "${boot_trust_header}"
        COMMAND "${Python3_EXECUTABLE}" -X utf8 -B
            "${canview_boot_repo}/tools/ota/generate_stm32_boot_trust.py"
            --public-der "${CANVIEW_BOOT_PUBLIC_DER}" --security-epoch "${CANVIEW_BOOT_SECURITY_EPOCH}"
            --manifest-key-id "${CANVIEW_BOOT_MANIFEST_KEY_ID}" --stm-abi "${CANVIEW_BOOT_STM_ABI}"
            --output "${boot_trust_header}"
        DEPENDS "${CANVIEW_BOOT_PUBLIC_DER}" "${canview_boot_repo}/tools/ota/generate_stm32_boot_trust.py"
        VERBATIM)
    add_library(canview_boot_identity STATIC
        "${canview_boot_repo}/firmware/communicator/stm32/bsp/boot_identity.c" "${boot_trust_header}")
    target_include_directories(canview_boot_identity PRIVATE "${boot_trust_dir}"
        "${canview_bootutil_source}/include")
    target_link_libraries(canview_boot_identity PRIVATE canview_boot_config canview_warnings)
else()
    message(STATUS "Boot BSP trust NOT_CONFIGURED: no production/default key or identity")
endif()
