# Controller 마이크 adapter

Waveshare의32pin female header에 연결하는 male adapter다. 카메라 GPIO38/39/40을 원격 I²S mic에 사용하므로 카메라와 동시 사용할 수 없다. RTC는 Waveshare 내장 회로를 재사용한다.

[KiCad](kicad/controller-adapter.kicad_sch) · [PDF](schematic.pdf) · [netlist](controller-adapter.net) · [BOM](bom.csv) · [pinmap](pinmap.csv)

케이블 LVDS100Ω 종단과5V fuse가 포함된다. mating 높이·방향·간섭은 실물 검증 전이다. [케이블/핀/타이밍 설명](../../docs/hardware/r1/bridge-controller-microphone.md)을 먼저 읽는다.
