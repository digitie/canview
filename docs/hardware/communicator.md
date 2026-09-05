# Communicator 하드웨어

현재 상세 설계는 [R1 문서 지도](r1/README.md)에서 읽는다. 실제 [KiCad/PDF/BOM/netlist](../../hardware/README.md)와 전 패드 [pinmap.csv](../../hardware/communicator/pinmap.csv)가 함께 제공된다. 이 산출물은 설계 검토본이며 아직 PCB 제작·차량 연결 승인본은 아니다.

| 목적 | 상세 문서 |
|---|---|
| 차량 surge·USB-C·5V/3.3V·CAN hard gate | [회로 설명](r1/communicator-circuit.md) |
| STM32/ESP32·UART4Mbps·커넥터·부팅 | [FW pinmap](r1/firmware-pinmap.md) |
| 외부 케이스 GNSS·MTi7 DR·BMP384 | [센서 회로](r1/navigation-hardware.md) |
| 수치 여유·land pattern·PCB/HIL 미완료 | [검증 조건](r1/verification.md) |
| 제조사 원본과 적용 페이지 | [reference evidence](../../hardware/references/README.md) |

주요 변경은 USB-C 전원 허용과 차량 전용 PHY rail 분리, 독립 PHY reset, 재무장이 필요한 watchdog latch, TCAN1046AV의 정확한5/7/8/11번 pin 교정이다. Controller는 Waveshare 보드를 유지하며 Communicator는 MINI-1-N4R2, Bridge는 WROOM-1-N8R2다.

과거 block diagram/pin proposal은 [이력](history/communicator-pre-r1.md)에 보존했다. 현재 회로를 코딩할 때 이력을 선제적으로 읽지 않는다. 기존 `pinmap-proposed.csv`도 이력 자료이며 현행 FW 입력으로 사용하지 않는다.
