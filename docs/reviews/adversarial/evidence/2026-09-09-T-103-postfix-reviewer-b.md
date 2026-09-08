# CV-HOSTILE-20260909-T103-B-03

- 후보: `a8d515849d98b89bfc7904356cf3ad8c5a2334bd`
- 기준: `6db3998092812354afe3b3918892b682022990b4`
- 시작: `2026-09-08T21:26:43.6064722Z` / KST `2026-09-09T06:26:43.6064722`
- 종료: `2026-09-08T21:41:03.0396815Z` / KST `2026-09-09T06:41:03.0431585`
- 방식: immutable Git object만 열람. 파일 변경·commit·push 없음.
- 현재 worktree는 dirty였으나 source evidence로 사용하지 않음.
- 실제 board/HIL/analyzer/build/CTest/CI는 실행하지 않음.

## 최종 판정: BLOCK

### P1-01 — no-TX analyzer가 ACK/TX 의미와 실행 identity를 검증하지 않음

위치:

- `tests/hil/assert_no_tx.py:76-88`
- `tests/hil/assert_no_tx.py:137-157`
- `tests/hil/assert_no_tx.py:159-196`

`kind`는 비어 있지 않기만 하면 되고, `fields`는 임의 key를 허용한다. TX 검사는 `CAN_TX`, `VEHICLE_CAN_TX`, `vehicle_tx`, `tx_permitted`, `tx_frames`, `ack_frames` 일부에만 한정된다. `source`도 임의의 non-empty 문자열이면 통과한다.

후보 fixture의 완전한 gate/summary 구조에 다음을 주입해 독립 실행했다.

- `kind="CAN_ACK"` → status `0`
- `kind="ACK"` → status `0`
- `fields={"tx":true,"ack":true,"vehicle_can_tx":true}` → status `0`
- `source="unrelated-run"` → status `0`

영향: 위조되거나 다른 실행에서 나온 evidence가 “CAN TX/ACK 없음”으로 인증될 수 있다.

권고: canonical event/field schema를 allowlist로 고정하고 unknown kind·field를 거부하며, expected execution ID/source와 firmware identity를 인자로 강제해야 한다. 위 negative fixture를 추가해야 한다.

### P2-01 — 실제 FDCAN FIFO loss가 high-level loss/fault로 승격되지 않음

위치:

- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:35-44`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:388-448`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:589-650`
- `tests/stm32/test_fdcan_platform.c:367-420`

`RF0L`은 interrupt snapshot에 포함되지만, `fifo_loss_unknown`은 FIFO fill이 3보다 클 때나 get index가 범위를 벗어날 때만 설정된다. 실제 FIFO depth가 3인 상황에서 message lost가 발생하면 fill level은 여전히 `<=3`일 수 있다. 이 경우 raw `RF0L` bit는 일부 보존되지만 custom `CANVIEW_STM_FDCAN_PLATFORM_ERROR_FIFO_LOSS`, `BUS_FAULT`, drop counter로 승격되지 않는다. `RF0F`도 `STATUS_INTERRUPTS`에서 제외된다.

현재 fake test는 불가능한 `fill=4`와 invalid get index만 시험하고 literal `RF0L` + valid fill을 시험하지 않는다.

ST 공식 HAL은 `RF0L`을 “Rx FIFO 0 message lost”, `RF0F`를 “Rx FIFO 0 full”로 정의한다. [STM32G4 FDCAN HAL header](https://raw.githubusercontent.com/STMicroelectronics/stm32g4xx-hal-driver/master/Inc/stm32g4xx_hal_fdcan.h)

영향: 실제 capture gap이 발생해도 channel이 active로 남고 completeness evidence가 과대평가될 수 있다.

권고: interrupt snapshot의 `RF0L`을 즉시 loss latch와 explicit drop/fault로 변환하고, `RF0F`도 진단 상태에 보존한다. 실제 가능한 FIFO 상태를 fake-register test에 추가해야 한다.

### P2-02 — frame sink 실패 후 raw frame이 재시도 없이 소실됨

위치:

- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:452-466`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:469-503`
- `tests/stm32/test_fdcan_platform.c:438-446`

`pop_raw_element()`이 callback 호출 전에 `raw_read_index`를 증가시킨다. 이후 `frame_sink()`가 `CANVIEW_TIMEOUT` 등 실패를 반환해도 raw item은 이미 소비되고 `sink_failures`만 증가한다.

실패 시나리오: module ring 일시 포화 또는 worker callback timeout.

영향: frame이 재시도되지 않고 capture/drop accounting에도 module drop으로 반드시 반영되지 않는다.

권고: sink 성공 전 ownership을 유지하거나, bounded retry 및 명시적 drop callback/counter 경로를 추가해야 한다.

## P0/P3

- P0: 0건
- P1: 1건
- P2: 2건
- P3: 0건

## 확인된 no-finding 영역

- module의 malformed/FD/DLC/padding 검증
- channel별 timestamp wrap 및 transactional batch commit
- filter callback reentry 및 bounded inventory
- ISR의 raw snapshot/SPSC ring 경계
- STM32 source 내 실제 FDCAN TX API/register 사용
- Diagnostic Bridge의 T-103 변경 범위 내 TX 경계
- CMake target wiring, board generator, static allocation 및 heap 금지 계약
- `tests/esp_core/test_wrong_bsp.c`, `test_runtime.c`
- STM32 G4 message-RAM 상수: `848/176/72/3`은 공식 HAL layout과 일치한다. G4 CMSIS register map에는 `RXF0S/RXF0A`가 있고 `RXF0C`가 없으므로 초기 `RXF0C` 의심은 finding으로 승격하지 않았다. [공식 STM32G474 CMSIS header](https://raw.githubusercontent.com/STMicroelectronics/cmsis-device-g4/master/Include/stm32g474xx.h)

## 미실행 및 제한

- 실제 STM32 board flash/boot
- ST-LINK, PHY, GPIO alternate-function, bitrate/electrical test
- IRQ latency 및 real bus load
- reset/brownout rail gate
- physical CAN analyzer의 ACK/data TX 0건
- 차량 CAN 및 vehicle profile evidence
- target build, CTest, sanitizer, coverage, CI 재실행

후보 문서의 기존 build/coverage 기록은 읽었지만 이번 immutable review에서 재실행하지 않았으므로 독립 실행 결과로 인정하지 않는다. 차량 CAN TX는 계속 `NO-GO`다.

## 실제 열람·실행 명령

주요 immutable 명령:

```powershell
git -c safe.directory=F:/dev/canview cat-file -e "a8d515849d98b89bfc7904356cf3ad8c5a2334bd^{commit}"
git -c safe.directory=F:/dev/canview rev-parse "a8d515849d98b89bfc7904356cf3ad8c5a2334bd^{commit}"
git -c safe.directory=F:/dev/canview diff --find-renames 6db3998092812354afe3b3918892b682022990b4 a8d515849d98b89bfc7904356cf3ad8c5a2334bd
git -c safe.directory=F:/dev/canview diff --check 6db3998092812354afe3b3918892b682022990b4 a8d515849d98b89bfc7904356cf3ad8c5a2334bd
git -c safe.directory=F:/dev/canview diff --name-status --find-renames 6db3998092812354afe3b3918892b682022990b4 a8d515849d98b89bfc7904356cf3ad8c5a2334bd
git -c safe.directory=F:/dev/canview ls-tree -r --name-only a8d515849d98b89bfc7904356cf3ad8c5a2334bd tests/esp_core
git -c safe.directory=F:/dev/canview grep -n -E 'RXF0C|RXF0SA|RXF0S|TXBAR|TXBCR|TXBTIE|FDCAN.*TX' a8d515849d98b89bfc7904356cf3ad8c5a2334bd -- firmware tests tools
git -c safe.directory=F:/dev/canview log --oneline --decorate -4 a8d515849d98b89bfc7904356cf3ad8c5a2334bd
```

후보 object에서 다음 파일들을 직접 읽었다.

- `.github/workflows/foundation.yml`
- `CMakeLists.txt`
- `firmware/communicator/stm32/CMakeLists.txt`
- `firmware/communicator/stm32/CMakePresets.json`
- `firmware/communicator/stm32/module/CMakeLists.txt`
- `firmware/boards/boards.json`
- `tools/check_sdkconfig.py`
- `tools/generate_boards.py`
- `tools/check_stm32_core.py`
- `tools/check_stm32_coverage.py`
- `tools/check_coverage.py`
- `tests/foundation/test_sdkconfig.py`
- `tests/esp_core/test_wrong_bsp.c`
- `tests/esp_core/test_runtime.c`
- `tests/hil/assert_no_tx.py`
- `tests/hil/adapter.py`
- `tests/hil/run_can_capture.py`
- `tests/hil/validate_evidence.py`
- `tests/hil/events.py`
- `tests/hil/scenario.py`
- `tests/hil/fixtures/t103-capture-only.jsonl`
- `tests/test_t103_capture_helpers.py`
- `tests/stm32/test_fdcan_capture.c`
- `tests/stm32/test_fdcan_platform.c`
- `tests/stm32/fake_stm32/*`
- `firmware/communicator/stm32/interface/canview_stm_fdcan_capture.h`
- `firmware/communicator/stm32/module/fdcan_capture.c`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.h`
- `firmware/communicator/stm32/app/main.c`
- STM32 BSP/core 및 warning/toolchain CMake 파일
- 관련 T-103 문서, `docs/resume.md`, `docs/journal.md`, 기존 Reviewer B report, target evidence

`firmware/communicator/stm32/platform/stm32g474/CMakeLists.txt`와 `sdkconfig.defaults`는 후보 tree에 존재하지 않아 열람할 수 없었다.
