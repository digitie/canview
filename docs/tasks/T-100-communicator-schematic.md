# T-100 Communicator 회로도, BOM과 hard TX gate

- 상태: `READY`
- 우선순위: `P0`
- Gate: `G1`
- 선행: 없음
- 병렬 가능: `T-001`

## 목표

현재 제안 pinmap과 block diagram을 제작·review 가능한 KiCad 회로, BOM, 계산서로 승격한다. reset/brownout/firmware fault에서 차량 bus 송신이 불가능한 상태를 회로로 만든다.

## 고정 부품

`STM32G474CEU6`, `ESP32-S3-MINI-1-N4R2`, `TCAN1046AV-Q1`, `MAX3055ASD+`, `LM74800-Q1`, `MAX20040B`, `TPS629210`, `TLV803EA30DPWR` 조합과 현재 STM32/UART pinmap을 출발점으로 한다. 대체하려면 별도 설계 결정과 근거가 필요하다.

## 미결정을 이 task에서 닫을 항목

- exact FET, TVS, fuse, inductor, capacitor, crystal, connector MPN
- ACC/IGN 입력 사용 여부와 CAN wake/sleep 정책, ignition-off 전류 목표
- vehicle harness adapter pinout과 board connector
- MAX3055 WAKE/INH/ERR 처리
- vehicle power와 USB VBUS 동시 인가 시 역급전 방지와 data/service 전원 정책
- three-channel tri-state TXD gate, output pull-up, default-disable `/OE`, physical TX_ARM, rail-good와 100 ms 이하 외부 watchdog/monostable
- hard gate sense용 free STM32 pin
- regulator compensation/thermal, reset line capacitance, UART series resistor
- CAN termination population variant
- ACC/KL15 기반 OFF/STARTING/RUN_RX/RUN_TX_ARMED/SHUTDOWN 상태와 MAX20040 EN·PGOOD/BIAS net
- PROTECTED_VBAT UV comparator가 MAX3055 EN/TXD permit을 비동기 차단하는 threshold/hysteresis

## 필수 산출물

```text
hardware/communicator/kicad/*.kicad_sch
hardware/communicator/kicad/*.kicad_pcb
hardware/communicator/bom.csv
hardware/communicator/power-budget.md
hardware/communicator/protection-calculation.md
hardware/communicator/net-constraints.md
hardware/communicator/pinmap.csv
hardware/communicator/erc-report.txt
```

## 수용 기준

- [ ] MCU 무전원/reset에서 TCAN standby, MAX Power-On Standby, UART flow-stop이다.
- [ ] hard gate off에서 세 TXD가 recessive이며 RX path는 사용 가능하다.
- [ ] gate logic single fault와 power sequencing 검토가 문서화됐다.
- [ ] firmware가 watchdog을 전혀 또는 잘못 갱신하는 fault에서 gate-off timeout과 한계를 명시하고, 정상 형식 frame flood는 software executor가 별도 제한한다.
- [ ] CAN1/2 termination과 CAN3 RTH/RTL을 혼용하지 않는다.
- [ ] 모든 DNP와 assembly variant가 BOM field로 구분된다.
- [ ] MAX3055 BATT/WAKE/INH/ERR가 floating 상태로 남지 않으며 CAN3 실장 variant의 ERR 감시는 mandatory다.
- [ ] vehicle rail on/off와 USB VBUS의 모든 조합에서 다른 rail/host로 역급전하지 않는다.
- [ ] USB는 data-only, SWD VTref는 target-output-only이며 bench 12 V가 같은 보호 path를 사용한다.
- [ ] 3.3 V RF burst와 PHY worst case에서 regulator margin이 있다.
- [ ] load dump/reverse/OV의 FET SOA와 TVS 에너지 조건이 계산됐다.
- [ ] SWD reset, ESP CHIP_PU, supervisor wired-OR 전류/시간이 검산됐다.
- [ ] TXD/RXD/STB/EN, rails, reset, UART, gate에 test point가 있다.
- [ ] KiCad ERC의 waiver가 0개이거나 각 waiver 근거가 있다.
- [ ] ACC low OFF current 목표가 25 °C 1 mA 이하이고 CAN wake net이 회로에 암묵적으로 남지 않는다.

## 검증

KiCad CLI ERC와 BOM/pinmap consistency script를 CI에서 실행한다. 회로 PDF, ERC report, power/protection spreadsheet export, netlist SHA-256을 PR artifact로 남긴다.

## 금지

차량 connector pin을 추측해 silkscreen에 C-CAN/M-CAN으로 확정하지 않는다. hard gate 없이 `VEHICLE_TX capable` 표기를 하지 않는다.
