# T-100 Communicator 회로도, BOM과 hard TX gate

- 상태: `IN_PROGRESS`
- 우선순위: `P0`
- Gate: `G1`
- 선행: 없음
- 병렬 가능: `T-001`

## 목표

현재 생성된 KiCad 회로·BOM·pinmap을 원문/계산서와 대조해 제작 전 설계 gate를 닫는다. reset/brownout/firmware fault의 차단 논리·전기 조건을 검산하며 실물 안전 합격은 T-101/T-508에서 별도로 입증한다.

## R1 현재 산출물과 잔여 작업

네 보드의 [상세 회로/PDF/BOM/netlist](../../hardware/README.md)는 생성했다. [R1 검증 조건](../hardware/r1/verification.md)의 제조사 land 원본 대조·최신 PDF·전원/PCB/SI/HIL gate 때문에 이 task 전체는 DONE이 아니다. 새 요구는 [ADR-006](../adr/006-compact-hardware-power-and-sensors.md)이 종전 USB data-only/상시전원 ACC 회로 가정을 대체한다. firmware/sensor bring-up은 [T-100b](T-100b-navigation-audio-bringup.md)로 분리했다.

남은 설계 작업: MAX20040 land90-0409 원본과 KiCad footprint overlay, 미확보 원문/구판 변경 검토, 구매 R/C 확정, thermal/loop/SOA 계산서, 실제 harness 검증. PCB 설계·제작 입력은 T-100a, 실물 fault evidence는 T-101에서 수행한다. 현재 generated file만 보고 제작 발주하지 않는다.

## 고정 부품

`STM32G474CEU6`, `ESP32-S3-WROOM-1-N16R8`, `TCAN1046AV-Q1`, `MAX3055ASD+`, `LM74800-Q1`, `MAX20040B`, `TPS629210`, `TLV803EA30DPWR` 조합과 현재 STM32/UART pinmap을 출발점으로 한다. 대체하려면 별도 설계 결정과 근거가 필요하다.

## 현재 선정 후 검산·승인을 닫을 항목

- 현재 BOM의 FET/TVS/fuse/inductor/capacitor/crystal/connector 선정 MPN을 원문·worst-case·구매 조건과 검산하고 미승인 항목을 구분
- R1은 외부 fused IGN/ACC 입력, CAN wake 없음. 상시 BAT+ variant의 별도 ACC/parked current 요구를 혼용하지 않음
- vehicle harness adapter pinout과 board connector
- MAX3055 WAKE/INH/ERR 처리
- vehicle power와 USB VBUS 동시 인가 시 역급전 방지와 data/service 전원 정책
- three-channel tri-state TXD gate, output pull-up, FD active-low `/OE`/FT active-high `OE`의 default-disable, physical TX_ARM, rail-good와 100 ms 이하 외부 watchdog/monostable
- 현재 pinmap의 hard gate/서비스 sense와 reset/ARM pin을 실제 package·symbol·netlist로 검산; 임의 free pin 재선정 금지
- regulator compensation/thermal, reset line capacitance, UART series resistor
- CAN termination population variant
- 외부 IGN/ACC 공급과 USB-only 조합별 OFF/STARTING/RUN_RX/RUN_TX_ARMED/service 상태, MAX20040 EN·PGOOD/내부VCC net
- PROTECTED_VBAT UV comparator가 MAX3055 EN/TXD permit을 비동기 차단하는 threshold/hysteresis

## 필수 산출물

```text
hardware/communicator/kicad/*.kicad_sch
PCB 제약·제조사 land 승인 자료 (실제 PCB는 T-100a)
hardware/communicator/bom.csv
docs/hardware/r1/verification.md (전력·보호 계산·net 제약; 현재 정적 계산+미완료 실측 gate)
hardware/communicator/pinmap.csv
hardware/communicator/erc.json
hardware/validation.json
hardware/margin-check.json
```

## 수용 기준

이 task는 원문/계산/회로 검토의 수용 기준이다. 아래 전기적 동작을 설계로 설명하고 실물 측정 방법을 T-101/T-508에 넘긴다. PCB 제작 뒤 실측 결과를 이 task의 선행으로 요구하지 않으며, 설계 검토 완료를 물리 시험 합격으로 기록하지 않는다. 실제 차량 harness mapping은 T-501 전까지 UNASSIGNED로 남긴다.

- [ ] MCU 무전원/reset에서 TCAN standby, MAX Power-On Standby, UART flow-stop이다.
- [ ] RX_ALLOWED가 유효한 정상 RUN에서 TX만 disarmed일 때 세 TXD가 recessive이며 RX가 가능하다. J31 제거·USB-only·rail/reset fault에서는 RX/TX가 모두 차단될 수 있으며 이를 우회하지 않는다.
- [ ] gate logic single fault와 power sequencing 검토가 문서화됐다.
- [ ] firmware가 watchdog을 전혀 또는 잘못 갱신하는 fault에서 gate-off timeout과 한계를 명시하고, 정상 형식 frame flood는 software executor가 별도 제한한다.
- [ ] CAN1/2 termination과 CAN3 RTH/RTL을 혼용하지 않는다.
- [x] 모든 DNP와 assembly variant가 BOM field로 구분된다.
- [ ] MAX3055 BATT/WAKE/INH/ERR가 floating 상태로 남지 않으며 CAN3 실장 variant의 ERR 감시는 mandatory다.
- [ ] vehicle rail on/off와 USB VBUS의 모든 조합에서 다른 rail/host로 역급전하지 않는다.
- [ ] USB-C는 MCU service 전원만 공급하고 PHY/GPS rail은 automotive-only다. SWD VTref는 target-output-only이며 bench12V는 같은 보호 path다.
- [ ] 3.3 V RF burst와 PHY worst case에서 regulator margin이 있다.
- [ ] load dump/reverse/OV의 FET SOA와 TVS 에너지 조건이 계산됐다.
- [ ] SWD reset, ESP CHIP_PU, supervisor wired-OR 전류/시간이 검산됐다.
- [x] TXD/RXD/STB/EN, rails, reset, UART, gate의 test point가 생성 회로에 있다. OTA 추가점의 개수·접근성은 현행 netlist/PCB에서 다시 검산하며 기존 24개를 고정 총수로 사용하지 않는다.
- [x] KiCad ERC0개, waiver0개와 export 정합성 검증을 통과했다. 전기적 안전 승인을 뜻하지 않는다.
- [x] R1의 외부 IGN/ACC 차단과 USB service 조건이 명확하고 상시 BAT+ 허용/1mA parked 달성을 주장하지 않는다. CAN wake net이 암묵적으로 남지 않는다.

## 계획 보완 수용 기준

- [ ] [OTA §6](../architecture/ota.md#6-실제-회로-변경과-핀-계약)의 `comm-r2-n16r8`·독립 reset·J31/J32·U56·GPIO38과 현행 netlist/BOM을 교차 확인한다. 구형 MINI 기준은 제작 입력에서 제외한다.
- [ ] 네 보드 schematic·land·계산서 승인과 PCB 배치/배선 승인을 구분하고, 후자는 T-100a로 전달한다. 실측 gate는 T-101/T-508에 남긴다.

## 검증

KiCad CLI ERC와 BOM/pinmap consistency script를 CI에서 실행한다. 회로 PDF, ERC report, power/protection spreadsheet export, netlist SHA-256을 PR artifact로 남긴다.

## 금지

차량 connector pin을 추측해 silkscreen에 C-CAN/M-CAN으로 확정하지 않는다. hard gate 없이 `VEHICLE_TX capable` 표기를 하지 않는다.


## 산출물·범위 경계

- PCB 배치/배선·제작 입력(T-100a), 실물 fault 합격(T-101/T-508), 차량 harness의 근거 없는 연결 확정은 범위 밖이다. 설계 계산/원문 대조가 실패하면 제작 승인을 막고 기존 승인 revision과 미결정을 보존한다.
