# T-100a 네 보드 PCB 배치·배선과 제작 설계 gate

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G1 제작 전`
- 선행: `T-100`
- 외부 선행: PCB 제조·실장 업체 규격, 승인된 manufacturer land pattern·기구 조건

## 목표

승인 schematic에서 Communicator·Controller adapter·Bridge·원격 mic PCB를 실제 제작 가능한 상태로 연결한다. 회로/ERC와 PCB/DRC를 구분하고 생산 산출물의 누락된 실행 책임을 닫는다.

## 고정 결정

[R1 하드웨어](../hardware/r1/README.md)와 [OTA §4·6](../architecture/ota.md)을 적용한다. N16R8 WROOM의 본체·antenna keepout 때문에 기존 70×45 mm 목표를 재검증한다. J31은 케이스 서비스 덮개에서 도구 없이 접근 가능해야 한다. 기존 block diagram/BOM을 제작 승인으로 보지 않는다.

## 구현 범위

- 네 보드별 배치/배선, stackup, net class, 전력·CAN·USB·RF·센서·mic/clock 반환 경로
- 제조사 package pin↔symbol↔footprint 대조와 열·전류·간격·mechanical 간섭 검토
- J31/J32·RECOVERY·RESET·SWD·test point 접근성과 실장/DNP variant별 조립 도면
- DRC/ERC/netlist 일치, Gerber/drill/position/BOM 생성·manifest 및 제조 검토 회신

## 범위 밖

실제 발주·결제·차량 설치, firmware 기능, 실물 power/EMC/thermal 합격 판정(T-101/T-508).

## 예상 변경 파일

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
hardware/communicator/kicad/*.kicad_pcb
hardware/controller-adapter/kicad/*.kicad_pcb
hardware/bridge/kicad/*.kicad_pcb
hardware/microphone/kicad/*.kicad_pcb
hardware/*/manufacturing/
docs/hardware/r1/verification.md
```

## 수용 기준

- [ ] 정확한 현재 보드 directory/파일명을 T-100 handoff에서 확인해 산출물 manifest에 고정하고 임의 footprint를 대입하지 않는다.
- [ ] 네 보드 모두 unrouted 0, 미해결 DRC 0 또는 별도 승인된 비안전 waiver로 설명하며 safety net waiver로 gate를 우회하지 않는다.
- [ ] WROOM antenna/기구/전원·thermal·UART/USB·3 CAN PHY·LVDS/센서 경로를 분야별로 검토하고 미실측 한계를 남긴다.
- [ ] N16R8 GPIO35/36/37 비사용, GPIO38 sense, 독립 reset·U56·J31/J32의 physical pin과 connectivity가 OTA 계약과 같다.
- [ ] Gerber/drill/placement/BOM의 board revision/digest/variant가 일치하고 제조사 DFM 승인 이후에만 제작 입력으로 표시한다.
- [ ] 제작 주문·비용 집행은 별도 사용자 승인이다. T-101에 시험 가능한 조립 PCB·회로/PCB revision·시험점 지도를 넘기며 실제 발주/실측 없이 G1을 통과로 표시하지 않는다.

## 검증 계획

이 task에서 확정한 실제 `.kicad_pcb`마다 KiCad CLI DRC 및 제조 출력 검사를 실행한다. 결과·KiCad version·입력/출력 hash를 기록한다. PCB가 없으면 미실행이지 DRC 통과가 아니다.

## evidence와 rollback

제조 release manifest와 독립 전기/PCB·기구 리뷰를 보관한다. 실패 시 제작 승인만 철회하고 원본 회로·이전 제조 revision은 보존한다.
