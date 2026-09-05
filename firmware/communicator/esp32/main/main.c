#include "canview_protocol.h"

#include "esp_log.h"

static const char *const TAG = "canview-comm-esp32";

void app_main(void)
{
    ESP_LOGI(TAG,
             "Communicator ESP32 scaffold ready: protocol v%u.%u, max frame=%u",
             (unsigned int)CANVIEW_PROTOCOL_MAJOR,
             (unsigned int)CANVIEW_PROTOCOL_MINOR,
             (unsigned int)CANVIEW_MAX_FRAME_SIZE);
}
