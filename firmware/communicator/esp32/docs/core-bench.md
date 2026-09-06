# Communicator ESP32 bench core

현재 실행 범위는 [T-200a](../../../../docs/tasks/T-200a-esp32-core-bench.md)다. 후속 UART/무선/OTA와 실물 T-200 gate는 포함하지 않는다.

## 구성·소유권

`app/main.c`는 SDK가 생성한 main task에서 BSP/runtime → portable boot → 고정 pool → 주기 health를 조합한다. `module/health.c`, `module/pool.c`와 interface는 strict C99이며 HAL·SDK·FreeRTOS·heap API를 포함하지 않는다. `bsp/board.c`는 생성 pin 정본으로 안전 출력을 설정하고, `bsp/runtime.c`는 sense pin을 SDK 독립 config에 연결한다. 실제 SDK include와 호출은 `platform/esp32s3/runtime.c` component 하나에 둔다.

core·pool·runtime은 app의 정적 저장소이며 reset 전 재초기화하지 않는다. app stack에는 작은 port 사본만 둔다. 입력 config와 port는 복사하며 callback context는 사용 완료까지 살아 있어야 한다. runtime의 main owner identity와 고정 주기 tick은 다른 task에서 접근하지 않는다. SDK adapter의 private 정적 spinlock 두 개는 watchdog과 pool용으로 분리한다. core callback 재진입은 terminal fault이며 ISR 호출은 금지한다.

## Boot와 실패

| 단계 | 성공 조건 | 실패 시 |
|---|---|---|
| BSP | RUN_OK7 LOW → BOOT0 요청2 LOW → GPS47 LOW → reset1 HIGH → recovery9 OD HIGH → RTS15 HIGH → TX17 HIGH, 입력8/16/18/48/38 pull 없음 | 이후 단계 중단, 외부 default-safe gate 유지 필요 |
| watchdog | TWDT2초 panic, 두 idle core 감시, 현재 main task 신규 등록 | 기존 subscription 탈취·자동 재시도 없음 |
| memory/clock | Flash16MiB, ECC 가용 PSRAM7.5MiB, internal heap≥80KiB/largest≥32KiB, main stack 여유≥1024B | 최초 fault 보존, feed 없음 |
| bench | pool 초기화와 USB metadata 진단 후 주기 health | UART/radio/CAN/OTA/NVS write 시작 없음 |

state는 UNINITIALIZED → STARTING → SAFE_BENCH 또는 FAULT다. boot 성공 자체는 watchdog feed가 아니다. clock backward/freeze, 이전 진척250ms 초과, health 실행20ms 초과, SDK 오류 또는 메모리 하한 위반이면 FAULT로 고정한다. callback에서 이미 latch한 fault도 덮어쓰지 않는다. wait/pool 오류도 app의 service status로 기록하고 loop를 빠져나가 더 이상 feed하지 않는다. `core.state`는 마지막 core 검사 상태이며 로그의 `status`와 함께 해석한다.

GPIO는 latch를 먼저 기록한 뒤 mode를 바꾼다. RTS HIGH는 송신 흐름 정지, TX HIGH는 idle이다. UART driver를 설치하지 않는다. sense48은 서비스 RUN 감지, sense38은 USB mux 선택 진단일 뿐 rail PGOOD나 차량 권한이 아니다. cap/TX는 항상0이다. MCU 코드 실행 이전 reset/brownout 안전은 외부 pull/gate와 실물 계측 없이는 증명할 수 없다.

## RTOS·시간·watchdog

| owner | 분류·우선순위·stack | 주기·예산 | 통신/감시 |
|---|---|---|---|
| IDF main service | soft real-time bench health, IDF main priority1/CPU0, stack8192B | 100ms fixed-rate, 이전 진척250ms, 실행20ms | 현재 task TWDT subscription 하나 |
| SDK idle CPU0/1 | SDK 소유, idle priority | SDK 스케줄링 | 각각 TWDT 감시; main은 대리 feed하지 않음 |

SDK 초기화·watchdog 등록에서 SDK 내부 allocation은 존재할 수 있다. core/pool runtime에는 malloc/free가 없다. Flash/PSRAM은 SDK 부팅이 초기화하며 application이 재초기화하거나 NVS를 열지 않는다. IDF6.0.3 `uxTaskGetStackHighWaterMark`는 **byte** 단위다. PSRAM get_size는 ECC 제외 가용 크기다. heap 수치는 internal·8-bit capability의 free와 largest block이며 PSRAM을 internal RAM으로 합산하지 않는다.

첫 wait에서 tick 기준을 잡고 이후 `xTaskDelayUntil`의 이전 wake tick을 보존한다. 지연 없이 반환되는 overrun은 TIMEOUT이다. catch-up loop로 여러 feed를 만들지 않는다. 32-bit tick wrap은 SDK API에 맡기며 health는 별도의 64-bit monotonic microsecond clock을 검사한다.

core는 sample 완료 뒤 시간을 검사하고, adapter는 자신의 critical section 진입 뒤 clock을 다시 검사한 후 `esp_task_wdt_reset`을 호출한다. `fed_at`은 SDK 호출 직전 **검사 시각**이지 물리 watchdog counter 재설정 계측값이 아니다. task 선점은 막지만 SDK 내부 spinlock 대기, NMI/고우선순위 interrupt·cache stall 상한과 watchdog reset latency는 HIL 미검증이다. 이 상한을 이미 보장했다고 주장하거나 차량 safety deadline으로 재사용하면 안 된다. 다른 worker를 추가할 때 각 worker가 자기 subscription과 진척을 소유해야 한다.

실패 후 app은 한 번 상태를 출력하고 idle만 호출한다. 등록 후에는 main이 feed하지 않아 TWDT panic/reset을 기다리며, 등록 전 실패는 safe idle로 남는다. USB가 연결되지 않은 경우 출력 지연도 health deadline에 포함된다. journal/자동 NVS erase/reboot 복구는 구현하지 않는다.

## 고정 pool 계약

정적16slot × payload256B, capacity1..16이다. task 또는 SDK callback이 동일 bounded lock을 통해 acquire/copy/release/stats를 호출한다. ISR·lock 내부 callback·token의 동시 다중 owner 사용은 금지한다. 256B 복사/clear는 lock 아래 수행하며 실제 critical WCET는 아직 계측하지 않았다. radio ingress에 사용하기 전 T-202에서 예산을 측정한다.

acquire는 payload를 복사하고 pool 주소·slot·generation token을 반환한다. 내부 pointer를 노출하지 않는다. context/output/source alias와 크기 오류를 거부한다. 해제는 payload를 지우고 generation이 UINT32_MAX인 slot은 영구 retire하여 wrap ABA를 막는다. token은 RAM 전용이며 serialize/reboot 뒤 재사용하지 않는다. exhaustion/stale은 UINT32_MAX에서 포화하며 used/high-water/retired를 lock 아래 snapshot으로 반환한다. 부족하거나 잘못된 token이면 출력 버퍼/길이를 변경하지 않는다.

## 설정과 검증

`boards.json`의 bench-health-v1이 defaults를 생성한다. 기존 sdkconfig는 defaults 변경으로 갱신되지 않으므로 별도 SDKCONFIG 경로로 빌드한다. `tools/check_sdkconfig.py`는 실제 생성 설정의 메모리·watchdog panic/idle·USB console·factory layout을 검사한다. OTA/security provisioning을 자동 선택하지 않는다.

host 시험은 boot 단계별 오류/재진입, memory/clock/deadline 경계, pool stale/alias/포화와 generation retire, 실제 adapter의 SDK 실패 및 owner/critical feed gate를 포함한다. 4 native host thread가 2slot에서8000개 payload를 교차 검증한다. 이는 ESP32 dual-core RTOS 스케줄링 검증이 아니다. coverage는 portable과 adapter profile을 분리하고 function100%/line≥95%/branch≥90%를 요구한다.

근거 SDK는 설치된 ESP-IDF v6.0.3의 `esp_system/include/esp_task_wdt.h`, `task_wdt/task_wdt.c`, `esp_psram/include/esp_psram.h`, `freertos/FreeRTOS-Kernel/include/freertos/task.h`다. 공식 원문은 [ESP-IDF v6.0.3](https://github.com/espressif/esp-idf/tree/v6.0.3/components)이며 고정 commit/digest 정본은 저장소 toolchain manifest다.

## 미실행과 후속 gate

실물 Flash/PSRAM 검사·온도·heap/stack 장시간 하한, USB log, GPIO/reset/brownout, UART 무송신 파형, watchdog actual reset·critical WCET는 NOT_RUN이다. T-200/G1·G2와 T-500 rig에서 확인한다. 차량 CAN TX는 NO-GO다. 현재 구현/리뷰/검증 진행 상태는 상세 task와 리뷰 evidence를 따른다.
