/* SPDX-License-Identifier: GPL-3.0-only */
#include "canview_esp_runtime.h"
#include "canview_board.h"
#include "board_pins.h"

canview_status_t canview_esp_board_runtime(canview_esp_runtime_t *runtime,
                                           canview_esp_runtime_port_t *port)
{
    const canview_platform_port_t board = canview_board_port();
    if (board.board_profile != CANVIEW_BOARD_PROFILE)
    {
        return CANVIEW_INVALID_ARGUMENT;
    }
    const canview_esp_runtime_config_t config = {board.enter_safe_state, board.context,
                                                 {CANVIEW_BOARD_FLASH_BYTES,
                                                  CANVIEW_BOARD_PSRAM_AVAILABLE_BYTES},
                                                 1U,
                                                 {CANVIEW_BOARD_PAIR_BUTTON_N_GPIO, 0U},
                                                 CANVIEW_BOARD_PROFILE};
    return canview_esp_runtime_open(runtime, &config, port);
}
