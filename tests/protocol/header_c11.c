#include <stddef.h>

#include "canview_protocol.h"

int main(void)
{
    canview_control_scope_t canonical = CANVIEW_SCOPE_AUDIO_PROFILE;
    canview_control_scope_t compatibility = CANVIEW_CONTROL_SCOPE_AUDIO_PROFILE;
    return (canonical == compatibility &&
            sizeof(canview_frame_header_t) == CANVIEW_HEADER_SIZE &&
            offsetof(canview_frame_header_t, crc32_le) == 28U &&
            sizeof(canview_command_request_t) == 72U) ? 0 : 1;
}
