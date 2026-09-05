# R1 상세 회로 설계

2026-09-05 요청의 **소형화 우선** 기준이다. 가격보다 보드 면적을 우선하지만 보호 부품, 안테나 이격, 전원 루프, 절연·방열 여유를 줄여 크기를 맞추지 않는다. 아래는 실제 KiCad 10 회로와 netlist를 갖춘 **설계 검토본**이며 PCB 배치·배선 또는 차량 연결 승인본이 아니다.

| 읽을 목적 | 정본 |
|---|---|
| 회로도·BOM·netlist 열기/재생성 | [하드웨어 산출물](../../../hardware/README.md) |
| 차량 전원·USB-C·CAN·고장 안전 | [Communicator 회로](communicator-circuit.md) |
| MCU/커넥터 pin과 FW 부팅 계약 | [핀맵·FW 인터페이스](firmware-pinmap.md) |
| GNSS 후보·실제 데드레코닝·고도 | [위치·관성 하드웨어](navigation-hardware.md) |
| Bridge·Controller 최소 어댑터·장거리 수음 | [Bridge와 마이크](bridge-controller-microphone.md) |
| 허용치 계산·미검증·PCB 제약 | [검증·제작 전 조건](verification.md) |
| PDF 원본·정확한 참조 절·포럼 | [근거 색인](../../../hardware/references/README.md) |
| 센서 메시지·구독·적용 응답 | [프로토콜 확장](../../architecture/protocols/navigation.md) |

## 회로 범위

- Communicator: STM32G474CEU6, ESP32-S3-MINI-1-N4R2, TCAN1046AV-Q1, MAX3055, 차량/USB-C 전원, 독립 TX 차단, 외부 GNSS, MTi-7, BMP384.
- Bridge: ESP32-S3-WROOM-1-N8R2, USB-C 전원·native USB·BOOT/RESET/PAIR. 이번 최소 보드에는 SD를 실장하지 않는다. 차량 CAN PHY나 제어 출력이 없다.
- Controller 어댑터: 기존 Waveshare 회로를 복제하지 않고 확장 헤더, LVDS 변환, 마이크 케이블 보호만 추가한다. 기존 I²C RTC를 재사용한다.
- 마이크: 별도 작은 보드의 1.8V T5848과 LVDS/레벨 변환. 소음을 측정하되 원음 저장·전송은 기본 꺼짐이다.

`tools/hardware/*_circuits.py`가 연결 생성 입력이다. `connectivity.json`은 그 입력을 펼친 감사용 생성물이며 두 번째 편집 정본이 아니다. 원본을 수정한 후 네 프로젝트를 같이 재생성한다. MCU/CAN 구형 제안 문서와 R1이 다르면 이 상세 설계와 생성된 pinmap을 따른다. 차량별 커넥터 배선과 CAN3 PHY 종류는 여전히 실차 확인 대상이다.

## 작은 보드의 현실적 범위

초기 배치 목표는 Communicator 약 70×45mm, Bridge 약 35×25mm, 마이크 약 20×18mm다. **실제 외곽 치수나 배치 가능성은 아직 검증하지 않았다.** MTi 모듈 12.1mm, 두 전력 FET, 전원 인덕터, MAX3055 SOIC14, 커넥터, 안테나 금지 영역 때문에 MCU 면적만 더해 크기를 확정할 수 없다. 4층 PCB를 출발점으로 하고 일반 R/C는 0402, 정밀 분압은 0603, 고전압·유효용량 부품은 0805~1210을 사용한다. 마이크는 음향 구멍과 조립 공간을 우선한다.
