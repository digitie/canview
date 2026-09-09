# T-103 STM32 3채널 FDCAN capture-only 경로

- 상태: `IN_PROGRESS`
- branch: `codex/t103-fdcan-capture`
- PR: [#32](https://github.com/digitie/canview/pull/32)
- 우선순위: `P0`
- Gate: `G2`
- 선행: `T-004`, `T-102`, `T-500`
- 후속: `T-203`, `T-501`

## 2026-09-09 C source 구현

사용자가 `G1 이전 fw 구현 허용`과 `C로 작성`을 명시했으므로 실물 board/HIL을
기다리지 않고 T-103의 C99 source와 host/target compile을 진행했다. 이 구현은
physical G2를 닫거나 차량 연결 권한을 부여하지 않는다.

- `module/fdcan_capture.c`에 80 MHz nominal timing table, channel별 PHY contract,
  standard/extended/RTR/DLC/padding 검증, classic-only FD 분리와 64-slot static
  channel ring을 추가했다.
- u32 TIM2 source timestamp를 half-range 규칙으로 확장하고, accepted frame만
  global state를 변경한다. 같은 channel 역행·forward gap·observe 역행은
  `FAULT`/error로 닫고 cross-channel의 작은 reorder만 허용한다.
- worker가 세 ring을 merge해 wire batch를 만들고, delta overflow는 다음 batch로
  남긴다. callback reentry와 observer filter, raw/module ring drop은 queue를
  막지 않으며 exact/saturating counter로 보존한다.
- generic ID inventory는 `(bus, flags, DLC, ID)` key와 frame/change count,
  bit-change mask, period p50/p95, rate만 제공하며 DBC signal 의미를 확정하지
  않는다. fixed table 포화도 capture를 중단하지 않는다.
- `platform/stm32g474/fdcan_capture.c`는 CMSIS FDCAN monitor/RX FIFO0 adapter다.
  IRQ는 FIFO element W1..W4와 TIM2 timestamp만 SPSC raw ring에 복사하고,
  worker `service()`가 decode와 capture/drop callback을 수행한다. RX interrupt는
  drain 전에 acknowledge하고 drain 중 신규 RX event는 다음 IRQ로 남긴다. FIFO
  loss/raw-ring overflow는 latch해 worker status에 전달하며 RF0L/MRAF는 high-level
  `BUS_FAULT`, RF0F는 status error로 보존한다. stop/start는 raw session state를
  버리고, callback 재진입을 차단하며 실패한 raw frame은 callback 성공 전까지
  release하지 않고 재시도한다. safe output은 한 출력 실패 뒤에도 모든 출력을
  시도한다. TX register, TX callback과 command path는 없다.
- module에는 worker 전용 `canview_stm_fdcan_capture_reset()`을 추가했다. reset은
  profile/critical/filter 계약을 보존하고 frame·timestamp·inventory·drop history를
  새 capture session으로 되돌리며, batch callback 중 재진입은 `RESOURCE_BUSY`로
  닫는다.

상세 owner·pin·message RAM·timestamp·ISR 경계는 [FDCAN capture 문서](../../firmware/communicator/stm32/docs/fdcan-capture.md)에 기록했다.

## 목표

세 논리 CAN channel을 silent/listen-only로 수집하고 hardware timestamp, bus 상태, drop을 UART에 전달한다. 차량 ACK와 command frame은 0건이어야 한다.

## 구현 범위

- FDCAN1 PA11/12, FDCAN2 PB12/13, FDCAN3 PA8/15 pin/clock
- profile-driven nominal/data timing table와 invalid profile 거부
- message RAM/FIFO partition static assertion
- RX IRQ→bounded ring→batch builder
- bus별 monotonic timestamp, standard/extended ID, DLC/flags validation
- classic 0–8 byte record만 UART/ESP-NOW v1.3에 전달
- error active/passive/bus-off/no-data/unknown-bitrate status
- generic ID inventory와 observer software filter hook

## 안전 규칙

`CAPTURE_ONLY`는 transceiver receive가 필요할 때 normal physical mode를 쓸 수 있지만 FDCAN은 bus-monitoring mode이며 TX request API가 link되지 않는다. startup에서 bitrate/profile이 없거나 MAX3055 bus type이 미확정이면 해당 PHY는 standby다.

## 수용 기준

- [x] 세 simulator stream의 bus ID, ID, data, ordering이 보존된다. (strict C99 host test)
- [x] classic CAN 최대 예상 load에서 ring/drop counter가 정확하다. (64-slot saturation와 raw-drop API test)
- [x] CAN FD frame은 corruption 없이 unsupported counter로 분리된다. (FD/BRS/DLC malformed matrix)
- [x] bitrate mismatch, bus-off, no-data가 서로 다른 상태가 된다. (profile/status host test)
- [x] source timestamp wrap과 batch delta overflow가 새 batch로 안전하게 나뉜다. (wrap/65535 boundary test)
- [x] `CAPTURE_ONLY` host runner가 runner report의 `source`, `execution_id`, firmware
  `source_sha256`를 모든 event에 exact match로 요구하고 unknown kind/field, 실행된
  `COMMAND_REPLAY`와 forbidden TX kind를 fail-closed로 거부한다.
  `run_can_capture.py`는 expected candidate·firmware·harness source digest를 현재
  checkout 및 report와 대조한 뒤 생성된 host event log에 이 helper를 직접 적용한다.
- [ ] 실제 analyzer가 `CAPTURE_ONLY`에서 ACK와 data TX 0건을 측정한다. 장비가 없어
  physical/HIL gate는 `NOT_RUN`이며 차량 CAN TX는 `NO-GO`다.
- [x] safety path가 observer queue saturation에 막히지 않는다. (filter/reentry/
  bounded filter consumption/fixed inventory host test)

## 검증

```bash
ctest --preset host-sanitize -R fdcan --output-on-failure
cmake --build firmware/communicator/stm32/build/debug
cmake --build firmware/communicator/stm32/build/release
python -B tests/hil/run_can_capture.py --channels 3 --mode capture-only `
  --expected-commit <candidate-commit> `
  --expected-firmware-source-sha256 <firmware-source-sha256> `
  --expected-harness-source-sha256 <harness-source-sha256>
python -B tests/hil/assert_no_tx.py tests/hil/fixtures/t103-capture-only.jsonl `
  --expected-source t103-fixture `
  --expected-execution-id T103-FIXTURE-001 `
  --expected-firmware-identity f9ea109772edef0743fd22899f9c6c6d8c6035c3a43c03090a6d709bc2309212
```

추가 host 검증은 `cmake --build build/host-coverage`와
`python -B tools/check_stm32_coverage.py --build build/host-coverage`로
module 및 fake-register adapter의 독립 profile을 검사한다. `run_can_capture.py`는
current fixture를 host에서 can-load하는 smoke인 동시에 report의 execution/source
identity로 생성 event를 strict no-TX 검사하는 실행기이고, `assert_no_tx.py`는 별도 fixture와
physical JSONL을 검사하는 bounded 계약 시험이다. 둘 다 physical harness의 G2 결과를
대신하지 않는다. 실제 analyzer에서 ACK/data TX 0건을 측정하는 acceptance는 장비가 없어
`NOT_RUN`이다. event identity와 strict no-TX runner 연결도 함께 검증한다.

최종 source candidate는 `91ec28f550f18575f22a205155cffe2cbf18422d`이고, target
Debug/Release clean-first build input은 `9d27fc2f6eb73e99127184193575e680c358cc7b`다.
최종 evidence-only closure candidate `365a92193dc66b33fcd42b077bf4c058d21fac0c`에
target manifest를 고정했다. firmware/shared/protocol source SHA-256은
`f9ea109772edef0743fd22899f9c6c6d8c6035c3a43c03090a6d709bc2309212`, `tests/hil`
harness SHA-256은 `00f0d68afcf3e30707f642e808ff645c1a60f62f91ee33e23ac0e7570d72ffbb`다.
Debug/Release ELF·MAP·BIN·HEX와 fixed FDCAN layout checker가 통과했고 warning/error
scan은 0건이다. 상세 disposition은 [T-103 통합 hostile review](../reviews/adversarial/2026-09-09-T-103.md),
[target manifest](../reviews/adversarial/evidence/2026-09-09-T-103-target-91ec28f.md),
[Reviewer A closure](../reviews/adversarial/evidence/2026-09-09-T-103-final-365a921-reviewer-a.md),
[Reviewer B closure](../reviews/adversarial/evidence/2026-09-09-T-103-final-365a921-reviewer-b.md)에 둔다.
최종 A는 `PASS`, B는 physical gate를 반영한 `CONDITIONAL`이며 active P0/P1은 없다.
소프트웨어 source/target-evidence closure는 끝났지만, plan DAG상 선행 T-102가 아직
`IN_PROGRESS`이고 physical G2도 열려 있으므로 이 상세 task 상태는 `IN_PROGRESS`로
유지한다. remote CI run은 PR #32 head에서 확인 전까지 pending으로 취급한다. physical board flash·
전원·CAN analyzer·차량 evidence는 장비가 없어 `NOT_RUN`이며, 차량 CAN TX release는
`NO-GO`다.

## evidence

CAN simulator seed/profile, analyzer log, frame count·drop·latency report, firmware digest를 G2 bundle에 넣는다. 현재 target manifest에는 Debug/Release artifact 8개 SHA-256, toolchain, layout/warning 결과와 physical/HIL `NOT_RUN`을 보존했다. generic fixture raw-content digest pinning과 standalone helper identity widening은 P2로 T-500에 defer한다(owner: T-500, G2 evidence reproducibility, 목표 2026-09-30).


## 산출물·범위 경계

- 산출물은 STM32의 FDCAN bus adapter·capture ring·timestamp API와 strict C99
  regression이다. T-500이 소유한 HIL runner는 별도 harness를 소비하며 이
  task가 없는 physical evidence를 생성하지 않는다. DBC 화면 decode·임의 CAN
  TX는 범위 밖이다.
- platform raw ring slot은 IRQ producer가 snapshot 소유권을 넘긴 뒤 worker가 읽고 release한다. module capture ring은 worker callback이 producer이고 batch worker가 consumer다. DMA/cache/overflow·3채널 wrap 경쟁을 시험하며 불명확한 timestamp를 fresh로 표시하지 않는다. 실패 시 listen-only/default-deny로 남는다.
