idf_component_register(
    SRCS "canview_controller_can.c" "canview_controller_signal.c"
         "canview_command_tracker.c"
    INCLUDE_DIRS "include"
    REQUIRES canview_protocol
)
