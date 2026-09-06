#include <stddef.h>

#include "canview_protocol.h"

int main(void)
{
    return (sizeof(canview_frame_header_t) == CANVIEW_HEADER_SIZE &&
            offsetof(canview_frame_header_t, crc32_le) == 28U &&
            sizeof(canview_command_request_t) == 72U) ? 0 : 1;
}
