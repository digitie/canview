#include "canview_controller_can.h"

#include "esp_log.h"

static const char *const TAG = "canview-controller";

void app_main(void)
{
    canview_controller_can_filter_store_t filter_store;
    canview_controller_can_filter_store_init(&filter_store);

    ESP_LOGI(TAG,
             "Controller scaffold ready: protocol filter revision=%lu, slots=%u",
             (unsigned long)filter_store.config_revision,
             (unsigned int)CANVIEW_CAN_FILTER_MAX_COUNT);
}
