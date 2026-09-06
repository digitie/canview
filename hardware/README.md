# CANView KiCad 하드웨어

KiCad **10.0.6 Windows native**로 생성·내보내는 R1 상세 회로 검토본이다. 네 보드 모두 로컬 심벌·footprint, 부품별 사양, 연결표와 실제 KiCad netlist를 포함한다. **PCB 라우팅·Gerber·실물 시험은 포함하지 않으며 제작/차량 연결 승인본이 아니다.**

| 보드 | 편집 프로젝트 | 사람이 읽는 회로 | netlist |
|---|---|---|---|
| Communicator | [KiCad](communicator/kicad/communicator.kicad_pro) | [PDF](communicator/schematic.pdf) | [.net](communicator/communicator.net), [XML](communicator/netlist.xml) |
| Bridge | [KiCad](bridge/kicad/bridge.kicad_pro) | [PDF](bridge/schematic.pdf) | [.net](bridge/bridge.net), [XML](bridge/netlist.xml) |
| Controller 어댑터 | [KiCad](controller-adapter/kicad/controller-adapter.kicad_pro) | [PDF](controller-adapter/schematic.pdf) | [.net](controller-adapter/controller-adapter.net), [XML](controller-adapter/netlist.xml) |
| 원격 마이크 | [KiCad](microphone/kicad/microphone.kicad_pro) | [PDF](microphone/schematic.pdf) | [.net](microphone/microphone.net), [XML](microphone/netlist.xml) |

각 보드의 `bom.csv`는 reference/MPN/footprint/정격/허용오차/source/DNP/sheet를, `pinmap.csv`는 부품의 **물리 패드 번호별** net과 NC를 기록한다. `GEN-R-*`, `GEN-C-*`는 제조사 주문번호가 아니라 명시 사양의 구매 규격이다. 실제 구매품을 고를 때 동일 크기·전압·전력·온도·유효용량을 확인해야 한다. `PCB-PAD-1MM`는 구매 부품이 없는 테스트 패드다. `MP`는 JST 금속 고정 탭이며 신호 접점이 아니므로 명시적으로 NC다. USB 실드 패드는 `SH`로 GND에 연결한다.

설계 이유는 [R1 문서 지도](../docs/hardware/r1/README.md), 제조사 원본과 참조 페이지는 [references](references/README.md), 자동검사 결과는 [validation.json](validation.json), 수치 검산은 [margin-check.json](margin-check.json)에 있다.

## Windows 재생성

```powershell
Set-Location F:/dev/canview
.\tools\hardware\export-review.ps1
& 'C:\Program Files\KiCad\10.0\bin\python.exe' tools\hardware\validate_exports.py
& 'C:\Program Files\KiCad\10.0\bin\python.exe' tools\hardware\check_margins.py
& 'C:\Program Files\KiCad\10.0\bin\python.exe' -m unittest discover -s tools\protocol -p test_navigation_codec.py -v
# commit 이후, 저장된 evidence와 실제 Git blob을 대조 (read-only)
& 'C:\Program Files\KiCad\10.0\bin\python.exe' tools\hardware\validate_exports.py --git-revision HEAD
```

회로 source → KiCad XML/sexpr netlist → ERC → PDF 순서이며 각 native process의 종료를 기다린다. validator는 모든 부품의 실제 footprint를 KiCad `pcbnew`로 열고, 번호 있는 패드 집합/NC/net/BOM/주요 안전 pin을 대조한다. 자동검사 통과는 정격·PCB parasitic·고장 주입 검증을 대신하지 않는다. 로컬 symbol과 footprint에는 생성 입력과 출처가 포함되며 3D 모델은 KiCad 설치 경로 참조일 수 있다.
