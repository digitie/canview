cmake_minimum_required(VERSION 3.25...4.4)
if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR "Set pinned IDF_PATH via tools/environment/setup-windows.ps1")
endif()
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components"
                         "${CMAKE_CURRENT_LIST_DIR}/components")
# Legacy draft components are deliberately excluded from the foundation image;
# the generated public protocol component is an explicit dependency of main.
set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(canview_communicator_esp32)
