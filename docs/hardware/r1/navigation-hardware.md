# 외부 GNSS·보드 내 INS·기압계

## 선택

보드 내 센서는 **Xsens MTI-7-5A-T v5 + Bosch BMP384**다. MTi7은 GNSS/INS 융합과 제한 시간의 위치 dead reckoning을 수행한다. 일반 AHRS의 quaternion 출력을 “데드레코닝 알고리즘 내장”이라고 부르지 않는다. **GNSS outage45초 이후 MTi7의 위치·속도 출력은 중단**되므로 무제한 터널 항법을 약속하지 않는다. [MTi 제조사 문서 사본](../../../hardware/references/pdf/xsens-mti1-current.pdf), [GNSS 연동 application note](../../../hardware/references/pdf/xsens-gnss.pdf)를 근거로 한다.

가격은 일반 IMU보다 높지만 별도 융합 알고리즘 개발 부담을 줄이고12.1×12.1mm module에 통합한다. 온보드 MCU에서 임의 적분한 위치와 제조사가 valid로 표시한 위치를 구분한다. 자기장 센서는 차량 철판·스피커·전력 인덕터 영향을 받으므로 yaw valid 판정과 장착 보정이 필요하다.

## 상용 케이스 포함 GNSS 후보

2026-09-05 공식 판매/사양 페이지 확인값이다. 가격은 USD, 세금·배송 제외이며 재고 보장은 아니다. **Standard UART 제품과 DroneCAN 제품을 혼동하지 않는다.**

| 후보 | 가격·성능 출발점 | 적용 판단 |
|---|---|---|
| [Holybro H-RTK F9P Helical SKU12018](https://holybro.com/products/h-rtk-f9p-helical) | $219, 알루미늄 케이스/헬리컬 안테나,115200baud/5Hz 기본, RAW20Hz/RTK8Hz 상한, 약250mA | R1 배선 기준. UART1 GH10 데이터와 UART2 GH6 PPS를 별도로 사용. 실제 MTi/F9P firmware 조합 검증 전 qualified INS로 표시하지 않음 |
| [Holybro Micro M10 With Case SKU12044](https://holybro.com/products/micro-m10-gps) | $27.99,34×28×11mm,GH6,115200baud,4GNSS10Hz/단일GNSS25Hz,200mA 미만 | 저가·소형 raw GNSS 후보. 외부 PPS 미노출/MTi native protocol 미확인으로 DR 대체품 아님 |
| [Holybro M10 SKU12040](https://holybro.com/products/m10-gps) | $43.99,케이스/안테나/GH10,115200baud/5Hz 기본 | 단순 위치·시간 후보. 신품 V2와 pinmap/케이스를 혼용하지 않음 |
| [Holybro Micro M9N With Case](https://holybro.com/products/micro-m9n-gps) | 4GNSS 동시 최대25Hz,GH6,케이스 옵션 | 고속·저가군 후보. 실제 판매 옵션 가격/재고 및 PPS·MTi 호환성은 구매 전 확인 |

M10의 단일 GNSS25Hz를 다중 GNSS25Hz로 잘못 옮기지 않는다. 일반 GPS 출하 출력은115200/5Hz지만 활성 UBX/NMEA 목록은 확인 대상이다. RTK centimetre급 정확도에는 correction source/수신 환경이 필요하며 이 보드 자체가 RTK 보정국을 제공하지 않는다.

저가 M9/M10은 입력 상한5.2V인 제품이 있다. **R1 AUTO5V의 DC·ripple 상한을 그대로 연결해도 된다고 승인하지 않는다.** 후보별 전원 허용치를 만족시키는 별도5.0V 레귤레이터/전원 variant를 검증해야 한다. current limit는 과전압 보호가 아니다. 기본 F9P 허용4.75~5.25V에 맞춘 회로를 다른 제품에 무조건 공용하지 않는다.

## 실제 연결과 ownership

GNSS TX→U44 Ioff buffer→MTi pad18 AUX_RX와 ESP GPIO5 tap, MTi pad19 AUX_TX→U43 GPS 전원-domain buffer→GNSS RX, PPS→U44→MTi pad20/ESP GPIO6다. Holybro **GH10-5는 SDA, GH6-5가 PPS**다. [Holybro 공식 pinout](https://docs.holybro.com/gps-and-rtk-system/f9p-h-rtk-series/standard-f9p-uart/pinout)을 따른다. UART는3.3V TTL이며 RS232±전압/12V navigation puck은 연결하지 않는다.

외부 GNSS configuration의 기본 소유자는 MTi다. ESP TX 링크는 DNP이며 동시에 두 송신원을 연결하지 않는다. ESP-direct variant로 바꾸면 MTi TX 저항을 제거하고 ESP 링크를 실장한 뒤 DR capability를 끈다. 케이스 USB-C와 보드 GPS5V를 동시에 연결하는 경우 외부 제품 내부 power mux/역류 동작이 확인되기 전 금지한다. USB 설정 시 보드의 GPS 전원을 끈다.

MTi host는 SPI mode3,≤2MHz, MTSSP의 opcode와3 filler byte 및 최소3µs guard를 따른다. DRDY GPIO는 notification만 하고 task가 pipe status 길이를 읽은 후 bounded buffer로 받는다. PSEL0=0,PSEL1=1 strap을 변경하지 않는다. nRST는 open-drain으로만 구동한다. module v5의 단일 전원 필터는 VDDA 앞 Würth74279279(0402,600Ω100MHz)와470nF다. module 밑 중앙8.8mm 영역과 antenna/전력부 이격을 PCB에서 확인한다.

BMP384는 MTi **AUX SPI**에 전용 연결한다. MTi firmware≥1.18과 해당 barometer 지원을 읽어 확인한다. ESP I²C 주소 스캔으로 발견될 부품이 아니다. Bosch package는10pad2×2mm이며 bottom drawing을 mirror한 PCB top view를 local footprint에 저장했다. 기압 port는 코팅·접착제로 막지 않으며, 외기/차실 압력 경로·팬 바람·온도 변화 영향을 측정한다.

## FW 초기화와 품질

1. 읽기 전용 device ID/revision/firmware/config snapshot을 가져온다. 기대 chip과 다르면 설정을 쓰지 않는다.
2. MTi 제조사 protocol로 GNSS 종류/baud/rate 및 required UBX 메시지·PPS 설정을 적용하고 readback한다. 지원 모델 문자열이 같다는 이유만으로 버전 차이를 무시하지 않는다.
3. GNSS epoch·PPS pulse edge·MTi status가 대응하는지 확인한다. PPS가 없거나 UTC/leap 상태가 미확정이면 그 품질을 낮춘다.
4. 차량 전방 X/좌측 Y/상방 Z, 북=0°·동=90° heading으로 normalized output을 만들고 장착 quaternion/revision을 남긴다. 제조사 body/NED/ENU를 그대로 섞지 않는다.
5. ESP task는 NAV/IMU/BARO/UTC 최신 slot만 유지한다. payload·validity·stale·구독 권한은 [protocol 계약](../../architecture/protocols/navigation.md)을 따른다.
6. GNSS outage 시 GNSS age와 error bound를 표시하고45초/제조사 invalid 중 먼저 오는 조건으로 position validity를 내린다. 마지막 좌표를 valid로 반복하지 않는다.

기압고도는 날씨/기압 기준에 영향을 받는다. `44330*(1-(P/P0)^0.1903)` 근사식을 상대 고도 표시의 출발점으로만 쓰고 P0 기본101325Pa, 선택 범위80000~110000Pa, revision을 저장한다. GNSS MSL/타원체 고도와 다른 필드다. 절대 고도 자동 보정은 정차·안정된 GNSS 오차 조건을 만족하는 별도 정책으로 구현하며 추정치를 확정 고도로 승격하지 않는다.
