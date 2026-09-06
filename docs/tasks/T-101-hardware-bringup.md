# T-101 Communicator PCB bring-up과 전원/reset fault 검증

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G1`
- 선행: `T-100a`, `T-500`
- 외부 선행: 조립 PCB, T-102/T-200의 시험용 boot image, bench 장비
- 후속: `T-501`, `T-106`

## 목표

설계 계산이 실제 PCB에서 성립하고 모든 비정상 전원 순서가 CAN TX 비활성으로 수렴함을 계측한다.

## 시험 준비

- current-limited programmable supply와 transient source 또는 검증된 pulse fixture
- oscilloscope/logic analyzer, electronic load, CAN simulator/analyzer
- 세 CAN channel dummy bus와 독립 termination
- hard gate off가 물리적으로 확인되는 `CAPTURE_ONLY` image
- test report template와 board serial/revision

## 순서

1. unpowered resistance/short와 termination population 확인
2. MCU 미실장 또는 reset held 상태의 rail sequence·TXD/STB/EN 측정
3. 12 V nominal, slow ramp, fast cycle, brownout, cold-crank profile
4. reverse input와 OV cutoff/reconnect
5. ESP32 RF burst·세 PHY load에서 5 V/3.3 V droop와 thermal
6. STM/ESP 개별 reset, SWD reset, boot failure, watchdog loop
7. TXD stuck-low injection과 hard gate off 차단
8. CAN short-to-ground/battery, bus-off, MAX ERR
9. 4 Mbps UART eye와 CTS stall
10. 외부 IGN/ACC 차단·USB service의 OFF/current 검증 (CAN wake 없음)
11. BATT/ACC/USB/SWD의 모든 투입·제거 순서와 phantom power
12. PROTECTED_VBAT UV에서 MAX3055 BATT/VCC/EN/TXD와 hard gate timing

## 수용 기준

- [ ] 어떤 reset/brownout 시험에서도 unintended dominant pulse가 검출되지 않는다.
- [ ] hard gate off에서 MCU pin low fault가 bus dominant를 만들지 않는다.
- [ ] 차량/USB mux·SYS/PHY 각 rail의 실제 경로와 독립 supervisor threshold/delay·상승/하강 순서가 현행 회로 계산 범위 안이다. 모든 경우를 단일 5 V PGOOD→3.3 V 순서로 가정하지 않는다.
- [ ] rail droop가 MCU/PHY UV threshold margin을 침범하지 않는다.
- [ ] OV cutoff/reconnect에서 oscillation/chatter가 없다.
- [ ] CAN fault 뒤 부품 손상·latch-up·역급전이 없다.
- [ ] UART 4 Mbps 오류율과 eye margin이 기준을 만족한다.
- [ ] 외부 IGN/ACC OFF·USB 유무 조합의 current와 phantom power를 측정하고 상시 BAT+의 1 mA/자동 CAN wake를 달성했다고 표시하지 않는다.
- [ ] 72시간 OFF 및 온도 corner에서 의도하지 않은 재기동·역급전이 없다.
- [ ] CPU halt, ISR spin, WDI 미갱신/과속·정상 cadence 오갱신에서 외부 guardian 동작과 한계가 기록된다.
- [ ] gate off에서는 TXD valid-frame/stuck-low injection에도 세 bus의 dominant/ACK/error/data frame이 0건이다.

## 계획 보완 수용 기준

- [ ] 독립 ESP/STM reset, J31 제거/재삽입, GPIO7·48 고착, stale ARM edge를 현행 회로로 측정한다. OTA flash cut-point 시험은 T-508에서 별도 수행한다.
- [ ] `run_power_faults.py`, `assert_no_tx.py`, `validate_hardware_evidence.py`는 T-500 공용 rig API로 이 task에서 구현·등록하고 실패 fixture부터 확인한다.

## 검증

```bash
python tests/hil/run_power_faults.py --rig-config private/rig.yaml --board-rev REV
python tests/hil/assert_no_tx.py evidence/latest/can-analyzer.log
python tests/hil/validate_hardware_evidence.py evidence/hardware/REV/SERIAL
```

## 증거

`evidence/hardware/<board-rev>/<serial>/`에 setup 사진, scope CSV/PNG, supply profile, firmware digest, 결과 JSON을 저장한다. 대용량 원본은 release storage URL과 SHA-256만 repo에 둔다.

## rollback

한 항목이라도 실패하면 board revision을 `CAPTURE_ONLY laboratory`로 표시한다. resistor 값만 조용히 바꿔 재시험하지 말고 BOM variant와 재시험 ID를 남긴다.


## 산출물·범위 경계

- 예상 산출물은 `tests/hil/` power/rail/gate script·negative 로그 fixture와 `docs/hardware/r1/verification.md`의 실측 evidence다. PCB routing 변경/발주 및 vehicle TX 기능은 범위 밖이다.
