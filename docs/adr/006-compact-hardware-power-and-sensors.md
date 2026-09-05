# ADR-006: 소형 R1 회로·USB service 전원·INS/원격 수음

- 상태: accepted — 설계 방향에 대한 결정이며 제작/실차 승인 아님
- 날짜: 2026-09-05
- 배경: 사용자 최신 회로 요청과 크기 명확화(가격보다 소형화 우선)

## 결정

1. Communicator의 기존 STM32G474CEU6+ESP32-S3-MINI-1-N4R2+TCAN1046AV+MAX3055를 유지한다. Bridge는 N8R2 WROOM을 사용한다. 작은 보드를 우선하되 보호·RF·열 조건을 희생하지 않는다.
2. USB-C에서도 전원을 받을 수 있도록 기존 data-only 지시를 대체한다. 차량 AUTO5V/PHY3V3와 mux 뒤 SYS5V/SYS3V3를 분리해 USB-only에서는 CAN PHY/GPS를 끈다. C-to-C5V1.5A 이상 광고만 허용하고 USB PD 고전압은 사용하지 않는다.
3. 최소 R1은 **외부 fused IGN/ACC에서 스위칭된12V** 입력을 받는다. 상시 BAT+와 별도 ACC sense/1mA parked/CAN wake는 이번 회로의 기능이 아니다. 기존 상시전원 입력 요구는 그 별도 variant에만 남긴다.
4. 차량 TX gate는 외부 watchdog, 두 rail supervisor, 실제5V/배터리 판정, physical arm과 재무장 latch를 사용한다. watch/power 회복이 자동 송신 복귀를 만들지 않게 한다. CWD120pF로100ms 이내 차단 목표를 유지한다.
5. true GNSS/INS 모듈 MTi7과 BMP384를 onboard, 케이스 포함 UART GNSS를 external로 둔다. 기본 F9P+PPS와 저가 raw GNSS variant를 구분한다. MTi7의45초 outage 제한을 protocol validity에 반영한다.
6. Controller는 기존 Waveshare RTC를 재사용한다. 긴 수음 cable은 raw I²S가 아닌 LVDS 세 pair+전원 pair이며 T5848/1.8V 변환을 remote에 둔다.
7. 센서 stream은 기존 generic CAN path와 독립된 ESP-NOW1.4/UART1.1 확장이다. 구독, rate/count, bandwidth, idempotency, 최종 적용 결과가 있어야 한다. Bridge 차량 제어 권한은 추가하지 않는다.

## 결과와 한계

전원 mux/PHY POR/드라이버 gate로 부품 수는 늘어나지만 board reset·USB 연결이 차량 bus를 구동하는 경로를 줄인다. PDF/footprint/BOM/핀맵을 로컬에 보존한다. ERC가 통과해도 MAX20040 land 원본 overlay, 전원/SI/thermal/fault 시험과 실제 차량 bus 확인 전 제작·송신 gate는 미완료다. 상세 근거와 남은 조건은 [R1](../hardware/r1/README.md)에만 두고 이 ADR에 부품 값을 중복 복제하지 않는다.
