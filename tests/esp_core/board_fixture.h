/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CANVIEW_ESP_CORE_BOARD_FIXTURE_H
#define CANVIEW_ESP_CORE_BOARD_FIXTURE_H
/* 구현/생성 header를 인용하지 않는 독립 보드 oracle. */
#if CANVIEW_TEST_BRIDGE
#define TEST_FLASH_BYTES (8388608U)
#define TEST_PSRAM_BYTES (2097152U)
#define TEST_INPUT_COUNT (1U)
#define TEST_INPUT_PIN0 (4U)
#define TEST_INPUT_PIN1 (0U)
#define TEST_INPUT_MASK (1U)
#define TEST_OTHER_FLASH (16777216U)
#define TEST_OTHER_PSRAM (7864320U)
#define TEST_PROJECT_NAME "canview_diagnostic_bridge"
#else
#define TEST_FLASH_BYTES (16777216U)
#define TEST_PSRAM_BYTES (7864320U)
#define TEST_INPUT_COUNT (2U)
#define TEST_INPUT_PIN0 (48U)
#define TEST_INPUT_PIN1 (38U)
#define TEST_INPUT_MASK (3U)
#define TEST_OTHER_FLASH (8388608U)
#define TEST_OTHER_PSRAM (2097152U)
#define TEST_PROJECT_NAME "canview_communicator_esp32"
#endif
#endif
