cmake_minimum_required(VERSION 3.22.1...4.4)

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR
        "IDF_PATH is not set. Dot-source tools/environment/setup-windows.ps1 first."
    )
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(canview_communicator_esp32)
