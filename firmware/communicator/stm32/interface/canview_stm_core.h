/* SPDX-License-Identifier: GPL-3.0-only */
/** @file canview_stm_core.h
 * @brief SDK 독립 최소 boot와 단일-owner cooperative scheduler 계약.
 * 모든 context는 caller가 zero-init하고 정적 수명으로 보유한다. ISR에서
 * 호출하지 않으며 callback은 같은 context에 재진입하거나 설정을 변경하지 않는다.
 * 성공은 bench core 준비일 뿐 boot 인증·CAN/RX/TX·OTA 권한이 아니다.
 */
#ifndef CANVIEW_STM_CORE_H
#define CANVIEW_STM_CORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "canview_status.h"

#define CANVIEW_STM_WORKERS_MAX (8U)
#define CANVIEW_STM_HEALTH_WINDOW_MS (20U)
#define CANVIEW_STM_FEED_PERIOD_MS (10U)
#define CANVIEW_STM_PERIOD_MAX_MS (1000U)
#define CANVIEW_STM_WORKER_BUDGET_MAX_US (1000U)

typedef enum
{
    CANVIEW_STM_FAULT_NONE = 0,
    CANVIEW_STM_FAULT_BOOT,
    CANVIEW_STM_FAULT_GAP,
    CANVIEW_STM_FAULT_DEADLINE,
    CANVIEW_STM_FAULT_WORKER,
    CANVIEW_STM_FAULT_BUDGET,
    CANVIEW_STM_FAULT_REENTRY,
    CANVIEW_STM_FAULT_PORT
} canview_stm_fault_t;

typedef canview_status_t canview_stm_action_fn(void *context);
typedef uint32_t canview_stm_time_fn(void *context);
typedef void canview_stm_fault_fn(void *context, canview_stm_fault_t reason);

/** boot callback은 bounded이며 반환 전 완료한다. fault는 안전 출력을 재설정하고
 * health 갱신을 금지한다. context와 callback의 수명은 boot context보다 길어야 한다. */
typedef struct
{
    canview_stm_action_fn *safe;
    canview_stm_action_fn *watchdog_start;
    canview_stm_action_fn *clock_start;
    canview_stm_action_fn *time_start;
    canview_stm_fault_fn *fault;
    void *context;
} canview_stm_boot_port_t;

typedef enum
{
    CANVIEW_STM_BOOT_UNINITIALIZED = 0,
    CANVIEW_STM_BOOT_STARTING,
    CANVIEW_STM_BOOT_READY,
    CANVIEW_STM_BOOT_FAULT
} canview_stm_boot_state_t;

typedef struct
{
    canview_stm_boot_state_t state;
    canview_status_t result;
    bool watchdog_started;
} canview_stm_boot_t;

/** @brief safe→IWDG→clock→time 순서. 첫 실패에서 중단하고 FAULT를 latch한다.
 * @param boot caller 소유 zero-init context. 반복 시작은 거부한다.
 * @param port 필수 callback과 수명이 보장된 context. 값만 동기 사용한다.
 * @return 첫 실패 상태 또는 CANVIEW_OK. 실패 뒤 자동 retry/권한 복원 없음.
 */
canview_status_t canview_stm_boot_start(canview_stm_boot_t *boot,
                                        const canview_stm_boot_port_t *port);

/** worker는 실제 bounded service/check를 완료했을 때만 OK를 반환한다.
 * RESOURCE_BUSY는 진척 없음이다. 단순 호출 횟수만으로 진척을 허위 보고하지 않는다. */
typedef struct
{
    canview_stm_action_fn *run;
    void *context;
    uint32_t period_ms;
    uint32_t deadline_ms;
    uint32_t budget_us;
    bool required;
} canview_stm_worker_t;

typedef struct
{
    canview_stm_time_fn *now_us;
    canview_stm_action_fn *feed;
    canview_stm_fault_fn *fault;
    void *context;
} canview_stm_scheduler_port_t;

typedef struct
{
    canview_stm_worker_t workers[CANVIEW_STM_WORKERS_MAX];
    uint32_t last_run_ms[CANVIEW_STM_WORKERS_MAX];
    uint32_t last_progress_ms[CANVIEW_STM_WORKERS_MAX];
    uint32_t last_progress_us[CANVIEW_STM_WORKERS_MAX];
    canview_stm_scheduler_port_t port;
    uint32_t last_step_ms;
    uint32_t last_feed_ms;
    uint32_t feeds;
    size_t count;
    uint16_t required_mask;
    uint16_t votes;
    uint16_t started_mask;
    canview_stm_fault_t fault;
    bool initialized;
    bool running;
} canview_stm_scheduler_t;

/** @brief worker/port 설정을 복사한다. 필수 worker는 최소 하나, deadline≤20ms다.
 * @param scheduler zero-init 단일 owner context. 재초기화 거부.
 * @param workers count개 descriptor. descriptor는 복사하나 callback context 수명은 caller 책임이다.
 * @param count 1..CANVIEW_STM_WORKERS_MAX.
 * @param port 필수 clock/feed/fault callback; clock은 init부터 연속 monotonic us를 제공한다. ISR
 * 호출 금지.
 * @param now_ms monotonic u32 ms. RTC와 섞지 않는다.
 * @return 입력 오류 시 context 불변. 성공 이후 descriptor/context 변경 금지.
 */
canview_status_t canview_stm_scheduler_init(canview_stm_scheduler_t *scheduler,
                                            const canview_stm_worker_t *workers, size_t count,
                                            const canview_stm_scheduler_port_t *port,
                                            uint32_t now_ms);

/** @brief due worker를 순서대로 최대 한 번씩 처리한다. catch-up loop 없음.
 * @param scheduler initialized context. fault는 reset 전까지 terminal이다.
 * @param now_ms 이전 tick과의 간격≤20ms. u32 wrap은 허용, backward/gap은 fault.
 * @return OK 또는 latched fault의 TIMEOUT. callback 오류도 fail-closed.
 * 필수 worker 전체의 새로운 vote와 10ms cadence 없이는 watchdog을 갱신하지 않는다.
 * callback 시작/완료 now_us로 이전 진척 deadline을 확인한 뒤 새 vote를 인정한다.
 * now_ms는 dispatch/cadence용, now_us는 실제 완료 간격·budget과 전체20ms 제한용이다.
 */
canview_status_t canview_stm_scheduler_step(canview_stm_scheduler_t *scheduler, uint32_t now_ms);
#endif
