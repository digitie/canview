# 원격 I²S 마이크

T5848 1.8V mic, LVDS 수신/송신, AXC level translation,3.3/1.8V LDO를 사용한다.32kHz/64fs, BCLK2.048MHz가 기준이며 일반3.3V I²S를 케이블에 직접 보내지 않는다.

[KiCad](kicad/microphone.kicad_sch) · [PDF](schematic.pdf) · [netlist](microphone.net) · [BOM](bom.csv) · [pinmap](pinmap.csv)

3m는 검증 전 길이 목표다. acoustic hole/annular GND/paste 공정과 케이블 지연은 [설계·수용 기준](../../docs/hardware/r1/bridge-controller-microphone.md)에 따라 시험한다. ERC 통과만으로 길이·음질·소음 기반 볼륨 동작을 보장하지 않는다.
