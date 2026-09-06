# Bridge 회로

USB-C 전원의 ESP32-S3-WROOM-1-N8R2 read-only 진단 Bridge다. CAN PHY/차량 TX 경로는 없다.

[KiCad](kicad/bridge.kicad_sch) · [PDF](schematic.pdf) · [netlist](bridge.net) · [BOM](bom.csv) · [pinmap](pinmap.csv)

USB-C는5V1.5A 이상 C-to-C source를 요구한다. 상세 전원/핀/검증은 [R1 설명](../../docs/hardware/r1/bridge-controller-microphone.md)과 [공통 재생성 안내](../README.md)를 따른다. ERC와 정합성 통과는 제작·RF·USB 인증이 아니다.
