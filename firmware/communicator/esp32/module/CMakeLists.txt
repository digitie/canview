add_library(canview_esp_core STATIC health.c pool.c)
target_include_directories(canview_esp_core PUBLIC ../interface ../../../../shared/interface)
target_link_libraries(canview_esp_core PUBLIC canview_warnings)
