# T-103 STM32 3채널 FDCAN capture-only 경로

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2`
- 선행: `T-004`, `T-102`, `T-500`
- 후속: `T-203`, `T-501`

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

- [ ] 세 simulator stream의 bus ID, ID, data, ordering이 보존된다.
- [ ] classic CAN 최대 예상 load에서 ring/drop counter가 정확하다.
- [ ] CAN FD frame은 corruption 없이 unsupported counter로 분리된다.
- [ ] bitrate mismatch, bus-off, no-data가 서로 다른 상태가 된다.
- [ ] source timestamp wrap과 batch delta overflow가 새 batch로 안전하게 나뉜다.
- [ ] analyzer가 `CAPTURE_ONLY`에서 ACK와 data TX 0건을 확인한다.
- [ ] safety path가 observer queue saturation에 막히지 않는다.

## 검증

```bash
cmake --build firmware/communicator/stm32/build/debug
ctest --preset host-sanitize -R fdcan --output-on-failure
python tests/hil/run_can_capture.py --channels 3 --mode capture-only
python tests/hil/assert_no_tx.py evidence/latest/can-analyzer.log
```

## evidence

CAN simulator seed/profile, analyzer log, frame count·drop·latency report, firmware digest를 G2 bundle에 넣는다.


## 산출물·범위 경계

- 예상 산출물은 STM32의 FDCAN bus adapter·capture ring·timestamp API, `tests/hil/run_can_capture.py`와 raw fixture다. DBC 화면 decode·임의 CAN TX는 범위 밖이다.
- ring slot은 ISR producer가 소유권을 넘긴 뒤 worker만 읽고 release한다. DMA/cache/overflow·3채널 wrap 경쟁을 시험하며 불명확한 timestamp를 fresh로 표시하지 않는다. 실패 시 listen-only/default-deny로 남는다.
