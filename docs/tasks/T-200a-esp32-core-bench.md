# T-200a Communicator ESP32 최소 core와 host 검증

- 상태: `IN_PROGRESS`
- branch: `codex/t200a-esp32-core-bench`
- PR: [#22](https://github.com/digitie/canview/pull/22)
- 우선순위: `P0`
- Gate: `G0 / G1 준비`
- 선행: `T-001`, `T-004`

## 목표와 분리 이유

T-200의 실물 bring-up 전에 N16R8에서 빌드 가능한 boot/health/watchdog와 고정 pool 기반을 제공한다. T-200의 실제 PSRAM·heap·USB log·GPIO·UART 무송신 계측을 소프트웨어 시험으로 대신하지 않는다. STM32 T-102a merge 뒤 Communicator ESP32 core를 먼저 구현한다.

## 고정 결정

- ESP-IDF6.0.3, WROOM-1-N16R8, Flash16MiB, Octal PSRAM80MHz/ECC 가용7.5MiB와 USB19/20를 유지한다.
- SDK 독립 C99 boot/health와 고정 pool을 interface로 분리한다. vendor·FreeRTOS API는 platform adapter에서만 호출한다.
- RUN_OK7 LOW, BOOT0 요청2 LOW, reset1 해제, recovery9 open-drain 해제, RTS15 HIGH/flow-stop와 TX17 idle HIGH를 유지한다. 48/38은 pull 없는 입력이며 RUN 허용으로 승격하지 않는다.
- bench core에서는 UART driver·radio·CAN·OTA·NVS write·key/eFuse provisioning을 시작하지 않는다. capability/TX permit은 항상0이다.
- main service owner 하나가 고정 주기100ms로 health를 검사한다. 이전 진척250ms deadline·한 번 실행20ms budget을 검사한 뒤에만 자신의 TWDT 진척을 갱신한다. TWDT2초 panic과 idle core 감시는 SDK 설정/adapter에서 확인한다. 이 수치는 소프트웨어 제한이며 실측 WCET가 아니다.
- core/pool은 정적 저장소를 사용한다. SDK의 boot-time watchdog 등록과 내부 allocation은 별도로 구분하며 runtime core에 heap API를 넣지 않는다.
- 디버거 미연결 기준 panic PRINT_REBOOT/추가 지연0초를 고정한다. bootloader factory reset·Flash 변경/provisioning 옵션은 거부하며 금지 항목 삭제 변이도 독립 fixture로 검증한다.

## 구현 범위·예상 파일

- `firmware/communicator/esp32/app/`, `module/`, `interface/`: boot/health state·고정 pool·typed 포트
- 같은 project의 `bsp/`, `platform/esp32s3/`, `main/CMakeLists.txt`: safe GPIO·IDF watchdog/heap/reset/USB metadata adapter
- `firmware/boards/boards.json`, `tools/generate_boards.py`와 생성 설정: watchdog/메모리 계약
- `tools/check_sdkconfig.py`, `tools/check_esp32_core_coverage.py`, host core/adapter 및 config 부정 fixture, root CTest/CI
- `docs/core-bench.md`, README와 public API 문서: 상태·소유권·수명·주기·budget·실패·미실행

## 수용 기준

- [ ] safe GPIO→watchdog 등록→메모리 검사→bench health 순서와 단계별 실패·재진입·재초기화 거부를 host에서 검증한다.
- [ ] Flash16MiB, PSRAM7.5MiB, internal free heap80KiB/largest32KiB, main stack 여유1024B와 backward/늦은 clock/실행 budget 실패가 feed를 막는다.
- [ ] 고정 pool의 exhaustion·high-water/drop counter·copy ownership·double/stale release·generation/크기/alias 경계를 시험한다.
- [ ] 실제 IDF adapter 논리를 host SDK fixture로 시험하고, 실제 SDK compile과 구분해 기록한다.
- [ ] generated defaults와 실제 sdkconfig를 검사한다. MINI·reserved pin·ECC off·120MHz·watchdog panic off·UART console 등 부정 입력이 실패한다.
- [ ] BSP safe pin 순서·각 GPIO 실패·pull 없는 sense와 control capability/TX0를 검증한다.
- [ ] Windows Debug/Release, GCC·ASan+UBSan 전체 회귀, core coverage·strict API와 STM32/ESP32 최종 binary warning0을 확인한다.
- [ ] 독립 전문 리뷰어 2명의 적대적 리뷰·finding 재확인·최종 CI를 통과한다.

## 검증과 evidence

root `cmake --preset host-debug/host-release/host-coverage`와 CTest, `check_sdkconfig.py`, `check_esp32_core_coverage.py`, strict API, pinned IDF build를 사용한다. 새 검사기는 이 task의 산출물이며 생성/실행 전 PASS로 표시하지 않는다. Linux GCC를 초기 검증에 포함한다. target 산출물은 commit·SDK·BIN/ELF/MAP·SHA-256·warning scan에 연결한다.

## 범위 밖·rollback

실제 UART/ESP-NOW transport와 peer/security는 T-201/T-202, OTA/NVS migration은 T-204 이후다. 실제 보드 PSRAM/heap/stack·reset·USB log·GPIO 파형·UART 무송신은 T-200/G1·G2의 NOT_RUN으로 남긴다. 회귀 시 이 task를 revert해 이전 safe-idle BSP로 되돌린다. 완료해도 T-200 전체와 물리 gate를 자동 해제하지 않는다.
