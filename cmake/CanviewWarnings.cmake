# 프로젝트 C99 strict gate. Vendor SDK 언어 표준은 덮어쓰지 않는다.
add_library(canview_warnings INTERFACE)
if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$" AND
   NOT CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(canview_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion
        -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef)
else()
    message(FATAL_ERROR "Strict C99 foundation requires GCC or clang GNU frontend.")
endif()
