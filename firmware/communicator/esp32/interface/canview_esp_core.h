/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_esp_core.h
 * @brief N16R8 bench boot/health 정책. SDK·차량 권한과 독립이다.
 */
#ifndef CANVIEW_ESP_CORE_H
#define CANVIEW_ESP_CORE_H
#include "canview_status.h"
#include <stdbool.h>
#include <stdint.h>

#define CANVIEW_ESP_CORE_PERIOD_MS (100U)
#define CANVIEW_ESP_CORE_DEADLINE_US (UINT64_C(250000))
#define CANVIEW_ESP_CORE_BUDGET_US (UINT64_C(20000))
#define CANVIEW_ESP_CORE_WATCHDOG_MS (2000U)
#define CANVIEW_ESP_CORE_FLASH_BYTES (16777216U)
#define CANVIEW_ESP_CORE_PSRAM_BYTES (7864320U)
#define CANVIEW_ESP_CORE_HEAP_MIN (81920U)
#define CANVIEW_ESP_CORE_BLOCK_MIN (32768U)
#define CANVIEW_ESP_CORE_INTERNAL_MAX (524288U)
#define CANVIEW_ESP_CORE_STACK_MIN (1024U)
#define CANVIEW_ESP_CORE_STACK_BYTES (8192U)

typedef enum
{
    CANVIEW_ESP_CORE_UNINITIALIZED = 0,
    CANVIEW_ESP_CORE_STARTING,
    CANVIEW_ESP_CORE_SAFE_BENCH,
    CANVIEW_ESP_CORE_FAULT
} canview_esp_core_state_t;

typedef enum
{
    CANVIEW_ESP_FAULT_NONE = 0,
    CANVIEW_ESP_FAULT_SAFE_GPIO,
    CANVIEW_ESP_FAULT_WATCHDOG,
    CANVIEW_ESP_FAULT_MEMORY,
    CANVIEW_ESP_FAULT_CLOCK,
    CANVIEW_ESP_FAULT_DEADLINE,
    CANVIEW_ESP_FAULT_BUDGET,
    CANVIEW_ESP_FAULT_REENTRY
} canview_esp_core_fault_t;

/** 측정 단위는 모두 byte. sense 값은 진단이며 RUN/TX 승인 입력이 아니다. */
typedef struct
{
    uint32_t flash_bytes;
    uint32_t psram_bytes;
    uint32_t heap_free_bytes;
    uint32_t largest_block_bytes;
    uint32_t stack_free_bytes;
    uint32_t reset_reason;
    bool service_run_sense;
    bool usb_service_sense;
} canview_esp_core_sample_t;

typedef canview_status_t canview_esp_core_stage_fn_t(void *context);
typedef canview_status_t canview_esp_core_clock_fn_t(void *context, uint64_t *now_us);
typedef canview_status_t canview_esp_core_sample_fn_t(void *context,
                                                      canview_esp_core_sample_t *sample);
/** Adapter는 갱신 직전 clock을 다시 확인한다. 성공 시 실제 검사 timestamp를 반환한다. */
typedef canview_status_t canview_esp_core_feed_fn_t(void *context, uint64_t not_before_us,
                                                    uint64_t deadline_us, uint64_t *fed_at_us);
typedef struct
{
    canview_esp_core_stage_fn_t *safe_gpio;
    canview_esp_core_stage_fn_t *watchdog_start;
    canview_esp_core_clock_fn_t *now_us;
    canview_esp_core_sample_fn_t *sample;
    canview_esp_core_feed_fn_t *feed;
    void *context;
} canview_esp_core_port_t;

/** Zero-init 후 단일 main service owner만 사용한다. callback 재진입은 terminal fault다. */
typedef struct
{
    canview_esp_core_state_t state;
    canview_esp_core_fault_t fault;
    canview_esp_core_port_t port;
    canview_esp_core_sample_t sample;
    uint64_t last_progress_us;
    uint32_t checks;
    uint32_t feeds;
    bool busy;
    bool watchdog_ready;
} canview_esp_core_t;

/**
 * @brief safe GPIO·watchdog·메모리 검사를 수행하고 bench 상태에 진입한다.
 * @param core zero-init된 caller 소유 context. restart/reset API는 없다.
 * @param port 복사할 필수 callback과 수명 보장된 context. callback은 task 문맥 전용이다.
 * @return 성공도 UART/radio/CAN 허용이 아니다. 실패는 최초 fault를 보존한다.
 */
canview_status_t canview_esp_core_boot(canview_esp_core_t *core,
                                       const canview_esp_core_port_t *port);

/**
 * @brief health 진척과 시간 예산을 확인한 뒤 현재 owner의 TWDT를 갱신한다.
 * @param core boot 성공 context. 같은 owner가 100ms 고정 주기로 호출한다.
 * @return 이전 진척 deadline·실행 budget·메모리·clock 위반은 terminal failure다.
 *
 * 단일 owner이며 ISR·다른 task의 호출은 금지한다. 다른 RTOS worker는 자기 TWDT
 * subscription을 별도로 소유해야 한다. 이 service가 그 진척을 대신하지 않는다.
 */
canview_status_t canview_esp_core_step(canview_esp_core_t *core);
#endif
