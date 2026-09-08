# T-103 STM32 3채널 FDCAN capture-only 경로

- 상태: `IN_PROGRESS`
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
  loss/raw-ring overflow는 latch해 worker status에 전달하며 stop/start는 raw
  session state를 버린다. TX register, TX callback과 command path는 없다.

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
- [ ] analyzer가 `CAPTURE_ONLY`에서 ACK와 data TX 0건을 확인한다.
- [x] safety path가 observer queue saturation에 막히지 않는다. (filter/reentry/
  bounded filter consumption/fixed inventory host test)

## 검증

```bash
ctest --preset host-sanitize -R fdcan --output-on-failure
cmake --build firmware/communicator/stm32/build/debug
cmake --build firmware/communicator/stm32/build/release
python -B tests/hil/run_can_capture.py --channels 3 --mode capture-only
python -B tests/hil/assert_no_tx.py tests/hil/fixtures/t103-capture-only.jsonl
```

추가 host 검증은 `cmake --build build/host-coverage`와
`python -B tools/check_stm32_coverage.py --build build/host-coverage`로
module 및 fake-register adapter의 독립 profile을 검사한다. `run_can_capture.py`는
현재 fixture를 host에서 can-load하는 smoke이고, `assert_no_tx.py`는 strict
capture-only JSONL 계약 시험이다. 둘 다 physical harness의 G2 결과를 대신하지
않는다. 실제 analyzer에서 ACK/data TX 0건을 측정하는 acceptance는 장비가 없어
`NOT_RUN`이다.

## evidence

CAN simulator seed/profile, analyzer log, frame count·drop·latency report, firmware digest를 G2 bundle에 넣는다.


## 산출물·범위 경계

- 산출물은 STM32의 FDCAN bus adapter·capture ring·timestamp API와 strict C99
  regression이다. T-500이 소유한 HIL runner는 별도 harness를 소비하며 이
  task가 없는 physical evidence를 생성하지 않는다. DBC 화면 decode·임의 CAN
  TX는 범위 밖이다.
- platform raw ring slot은 IRQ producer가 snapshot 소유권을 넘긴 뒤 worker가 읽고 release한다. module capture ring은 worker callback이 producer이고 batch worker가 consumer다. DMA/cache/overflow·3채널 wrap 경쟁을 시험하며 불명확한 timestamp를 fresh로 표시하지 않는다. 실패 시 listen-only/default-deny로 남는다.
