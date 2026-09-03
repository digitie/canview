# Communicator 하드웨어 설계 기준

## 1. 문서 상태

이 문서는 `Communicator`의 회로 설계 입력과 1차 핀 할당을 정의한다.

- IC 종류와 전기적 한계는 제조사 데이터시트에서 확인한 값이다.
- MCU GPIO 할당, 커넥터, 전원 topology는 **회로도 작성 전 제안값**이다.
- 아래 핀맵은 ERC, signal-integrity 검토, 실제 차량 bus 확인, prototype bring-up을 통과하기 전까지 PCB 확정값이 아니다.

## 2. 부품 구성과 선정 결과

| 부품 | 적용 부품 | 확인된 주요 사양 | 역할 |
|---|---|---|---|
| 무선 MCU | `ESP32-S3-MINI-1-N4R2` | dual-core LX7 최대 240 MHz, 4 MB Quad SPI Flash, 2 MB Quad SPI PSRAM, 3.0–3.6 V, −40–85 °C, PCB antenna | ESP-NOW, pairing, 설정, 무선/UART queue |
| CAN MCU | `STM32G474CEU6` | Cortex-M4F 170 MHz, Flash 512 KB, SRAM 128 KB, FDCAN 3개, UFQFPN48, 1.71–3.6 V, suffix 6은 −40–85 °C | 세 CAN 채널, timestamp, filter, 안전 gate |
| 고속 CAN | `TCAN1046AV-Q1` | 독립 2채널, ISO 11898-2:2016, classic CAN/CAN FD 최대 8 Mbps, VCC 4.5–5.5 V, VIO 1.7–5.5 V, standby/WUP | CAN 1·2 물리계층 |
| 저속 CAN | `MAX3055ASD+` | fault-tolerant/single-wire fallback, 125 kbps, ±80 V bus fault protection, VCC 5 V ±5%, BATT 5–42 V, −40–125 °C | CAN 3 저속 fault-tolerant 물리계층 |

### 2.1 ESP32-S3-MINI-1-N4R2 변경 영향

이 설계는 `ESP32-S3-PICO-1`을 사용하지 않는다. `N4R2`에서 `GPIO26`은 내부 PSRAM에 연결되어 외부 용도로 사용할 수 없다. 모듈은 65 pads이고 PCB antenna를 포함하므로, module antenna 끝을 base PCB 가장자리에 배치하고 antenna keep-out 안에 copper pour, trace, 부품, metal enclosure를 두지 않는다.

USB Serial/JTAG를 유지하기 위해 `GPIO19/20`은 UART flow control에 쓰지 않는다. MCU 간 UART는 GPIO matrix를 이용해 `GPIO17/18/15/16`에 배치한다. `GPIO0/3/45/46`은 strapping 영향을 받으므로 외부 회로가 reset 시 레벨을 강제하지 않게 한다.

### 2.2 MAX3055 적용 제한

`MAX3055`는 고속 CAN transceiver가 아니다. 데이터시트가 규정하는 대상 속도는 125 kbps이며, fault-tolerant bus의 RTH/RTL termination 구조를 사용한다. 따라서 다음을 금지한다.

- CAN 1·2의 500 kbps 고속 bus에 MAX3055 연결
- MAX3055 채널에 고속 CAN용 120 Ω 종단을 관성적으로 장착
- 실차 bus 종류가 확인되기 전에 CAN 3을 normal mode로 켜기
- CAN 3을 CAN 1·2와 같은 transceiver profile로 다루기

## 3. 전원과 보호 회로

```text
차량 BATT
   │
   ├─ fuse/PTC ─ reverse-polarity protection ─ load-dump TVS ─ EMI filter
   │                                                        │
   │                                                        ├─ automotive 5 V buck
   │                                                        │    ├─ TCAN1046AV VCC
   │                                                        │    └─ MAX3055 VCC
   │                                                        │
   │                                                        └─ automotive 3.3 V rail
   │                                                             ├─ STM32 VDD/VDDA
   │                                                             ├─ ESP32 module 3V3
   │                                                             └─ TCAN1046AV VIO
   │
   └──────────────── protected BATT sense/feed ─────────────────> MAX3055 BATT
```

전원 IC의 구체 부품은 아직 확정하지 않는다. 다음 조건으로 별도 선정한다.

1. 차량 cold crank와 load dump를 포함한 입력 범위 및 ISO 7637-2 pulse 요구조건
2. 역극성, jump start, ESD, 과전류, thermal shutdown
3. ESP32 Wi-Fi burst를 포함한 3.3 V peak current와 충분한 local bulk capacitance
4. 5 V transceiver rail과 3.3 V MCU rail의 power sequencing 및 역급전 방지
5. ignition-off 대기전류와 MAX3055 wake/INH 사용 여부

`TCAN1046AV`에는 VCC 5 V와 VIO 3.3 V를 공급한다. 각 supply pin 바로 옆에 데이터시트 권장 decoupling을 두고 두 CAN channel의 return path를 짧게 만든다. `MAX3055`에는 VCC 5 V와 protected BATT를 각각 공급하며 두 핀 모두 IC 가까이에 100 nF를 둔다.

STM32 VDD/VDDA와 ESP32 3V3에는 제조사 권장 decoupling과 bulk capacitor를 둔다. STM32 exposed pad/VSS는 연속 ground plane에 thermal via로 연결한다. analog 기능을 사용하지 않더라도 VDDA/VSSA/VREF+는 floating시키지 않는다.

## 4. CAN 물리계층 회로

### 4.1 CAN 1·2: TCAN1046AV-Q1

```text
STM32 FDCANx_TX ─────> TXDx        CANHx ── CMC/DNP ── ESD/TVS ── connector
STM32 FDCANx_RX <───── RXDx        CANLx ── CMC/DNP ── ESD/TVS ── connector
STM32 CANx_STB ──────> STBx
3V3 ─────────────────> VIO
5V  ─────────────────> VCC
GND ─────────────────> GND1/GND2
```

TCAN1046AV pin 기능은 다음과 같다.

| 핀 | 신호 | 핀 | 신호 |
|---:|---|---:|---|
| 1 | `TXD1` | 8 | `RXD2` |
| 2 | `GND1` | 9 | `STB2` |
| 3 | `VCC` | 10 | `CANL2` |
| 4 | `RXD1` | 11 | `CANH2` |
| 5 | `VIO` | 12 | `CANL1` |
| 6 | `TXD2` | 13 | `CANH1` |
| 7 | `GND2` | 14 | `STB1` |

- `STB1/2`에는 pull-up을 두어 STM32 reset 중 두 채널이 standby가 되게 한다.
- 차량 bus에 stub으로 붙을 때 120 Ω은 기본 `DNP`다. 별도 jumper나 switchable termination은 벤치 및 endpoint 구성에서만 사용한다.
- common-mode choke와 external ESD/TVS는 footprint를 두되 EMC 시험으로 장착값을 결정한다. transceiver 내부 ±58 V bus fault 보호만으로 차량 connector의 모든 transient 요구를 충족한다고 가정하지 않는다.
- CANH/CANL은 tightly coupled differential pair로 routing하고, connector–protection–transceiver 순서를 짧고 대칭으로 유지한다.

### 4.2 CAN 3: MAX3055

```text
STM32 FDCAN3_TX ───────────────> TXD
STM32 FDCAN3_RX <─ level guard ─ RXD
STM32 MAX_ERR    <─ level guard ─ ERR
STM32 MAX_STB ─────────────────> STB
STM32 MAX_EN  ─────────────────> EN
5V ────────────────────────────> VCC
protected BATT ────────────────> BATT
RTH/RTL network ───────────────> CANH/CANL ─ protection ─ connector
```

| 핀 | 신호 | 전기·설계 주의 |
|---:|---|---|
| 1 | `INH` | 외부 regulator 제어용 open/high-Z 동작. 사용하지 않으면 데이터시트 조건으로 처리 |
| 2 | `TXD` | 3.3 V STM32 high가 VIH 2.4 V 이상이므로 논리 입력 조건 충족 |
| 3 | `RXD` | high 출력이 5 V rail 기준. STM32의 5 V-tolerant 입력과 역급전 방지 회로 필요 |
| 4 | `ERR` | 5 V 출력. 5 V-tolerant 입력 또는 level translator 사용 |
| 5 | `STB` | `EN`과 함께 mode 선택 |
| 6 | `EN` | `STB=1`, `EN=1`일 때 normal operating |
| 7 | `WAKE` | local wake 입력. 미사용 처리와 transient 보호 필요 |
| 8 | `RTH` | CANH fault-tolerant termination network |
| 9 | `RTL` | CANL fault-tolerant termination network |
| 10 | `VCC` | 5 V ±5%, 100 nF local bypass |
| 11 | `CANH` | fault-tolerant bus high |
| 12 | `CANL` | fault-tolerant bus low |
| 13 | `GND` | ground |
| 14 | `BATT` | protected vehicle battery, 100 nF local bypass |

`RXD`와 `ERR`는 STM32의 `FT` 표기 pin을 쓰되, STM32가 꺼진 상태에서 MAX3055가 살아 있을 때의 injection current를 회로 검토해야 한다. 기본안은 automotive-qualified unidirectional level translator 또는 저항/클램프 조합을 사용하고, 부품 미장착 직결은 prototype에서 전원 sequencing과 absolute maximum을 계측으로 확인한 경우에만 허용한다.

MAX3055 termination은 데이터시트의 RTH/RTL 방식으로 설계한다. 각 node의 RTH/RTL 저항은 최소 500 Ω 조건을 지키고 전체 network 값은 실제 bus topology에 맞춘다. 120 Ω differential 종단을 그대로 복사하지 않는다.

## 5. 제안 핀맵

정본 CSV는 [`../hardware/communicator/pinmap-proposed.csv`](../hardware/communicator/pinmap-proposed.csv)다.

### 5.1 ESP32-S3-MINI-1-N4R2

| 모듈 pad | GPIO | 제안 기능 | 연결 | 비고 |
|---:|---:|---|---|---|
| 3 | — | `3V3` | 3.3 V rail | 3.0–3.6 V |
| 4 | 0 | `ESP_BOOT` | button/test pad | strapping, 평상시 pull-up |
| 19 | 15 | `UART1_RTS` | STM32 `PA0/USART2_CTS` | GPIO matrix 사용 |
| 20 | 16 | `UART1_CTS` | STM32 `PA1/USART2_RTS` | GPIO matrix 사용 |
| 21 | 17 | `UART1_TX` | STM32 `PA3/USART2_RX` | module native U1TXD |
| 22 | 18 | `UART1_RX` | STM32 `PA2/USART2_TX` | module native U1RXD |
| 23 | 19 | `USB_D-` | USB connector | 다른 기능에 배정 금지 |
| 24 | 20 | `USB_D+` | USB connector | 다른 기능에 배정 금지 |
| 26 | 26 | internal PSRAM | 연결 금지 | N4R2에서 외부 사용 불가 |
| 39 | 43 | `UART0_TX/debug` | test pad | 생산 connector 노출 정책 검토 |
| 40 | 44 | `UART0_RX/debug` | test pad | 생산 connector 노출 정책 검토 |
| 45 | — | `EN` | reset circuit/test pad | floating 금지 |
| 1, 2, 42, 43, 46–65 | — | `GND` | ground plane | antenna 영역 제외 제조사 land pattern 준수 |

`GPIO3/45/46`은 strapping pin, `GPIO26`은 N4R2 내부 PSRAM pin이다. `GPIO19/20`은 native USB를 위해 예약한다. UART1의 RTS/CTS를 GPIO15/16으로 routing하는 것은 ESP32-S3 GPIO matrix가 지원하지만, firmware에서 `uart_set_pin()`과 hardware flow control을 명시적으로 설정해야 한다.

### 5.2 STM32G474CEU6 UFQFPN48

| package pin | MCU pin | AF/모드 | 제안 기능 | 연결 |
|---:|---|---|---|---|
| 8 | `PA0` | AF7 `USART2_CTS` | `UART_CTS` | ESP GPIO15 RTS |
| 9 | `PA1` | AF7 `USART2_RTS_DE` | `UART_RTS` | ESP GPIO16 CTS |
| 10 | `PA2` | AF7 `USART2_TX` | `UART_TX` | ESP GPIO18 RX |
| 11 | `PA3` | AF7 `USART2_RX` | `UART_RX` | ESP GPIO17 TX |
| 12 | `PA4` | output | `CAN1_STB` | TCAN pin 14, external pull-up |
| 13 | `PA5` | output | `CAN2_STB` | TCAN pin 9, external pull-up |
| 14 | `PA6` | output | `CAN3_STB` | MAX pin 5, external pull-down |
| 15 | `PA7` | output | `CAN3_EN` | MAX pin 6, external pull-down |
| 30 | `PA8` | input, 5 V-tolerant | `CAN3_ERR` | MAX pin 4 via level guard |
| 33 | `PA11` | AF9 `FDCAN1_RX` | `CAN1_RX` | TCAN pin 4 |
| 34 | `PA12` | AF9 `FDCAN1_TX` | `CAN1_TX` | TCAN pin 1 |
| 41 | `PB3` | AF11 `FDCAN3_RX` | `CAN3_RX` | MAX pin 3 via level guard |
| 42 | `PB4` | AF11 `FDCAN3_TX` | `CAN3_TX` | MAX pin 2 |
| 43 | `PB5` | AF9 `FDCAN2_RX` | `CAN2_RX` | TCAN pin 8 |
| 44 | `PB6` | AF9 `FDCAN2_TX` | `CAN2_TX` | TCAN pin 6 |
| 36 | `PA13` | AF0 `SWDIO` | debug | SWD header |
| 37 | `PA14` | AF0 `SWCLK` | debug | SWD header |
| 7 | `PG10-NRST` | reset | `NRST` | supervisor/SWD header |
| 46 | `PB8-BOOT0` | boot strap | `BOOT0` | pull-down + service pad |

`PA11/PA12`는 USB D−/D+와도 multiplex되므로 이 설계에서 STM32 USB는 사용하지 않는다. Communicator의 service interface는 ESP32 native USB와 STM32 SWD를 사용한다.

### 5.3 reset 시 safe state

| 신호 | 외부 기본값 | reset 중 결과 |
|---|---|---|
| TCAN `STB1`, `STB2` | pull-up to VIO | CAN 1·2 standby |
| MAX `STB`, `EN` | pull-down | sleep/standby 계열, 송신 비활성 |
| STM32 `BOOT0` | pull-down | user Flash boot |
| ESP32 `GPIO0` | pull-up | SPI Flash boot |
| CAN termination switch | off/DNP | 차량 기존 종단을 변경하지 않음 |

## 6. PCB와 connector 체크리스트

- ESP32 module antenna를 차량 금속 bracket, display cable, CAN common-mode choke와 멀리 둔다.
- 4 Mbps UART 네 선과 ground reference를 두 MCU 사이 최단으로 routing하고, CAN differential pair와 평행하게 길게 지나지 않는다.
- UART TX/RX/RTS/CTS마다 source 근처 series resistor footprint를 둬 edge rate와 ringing을 조정할 수 있게 한다.
- CAN connector에는 채널별 CANH/CANL/GND를 명확히 분리하고 잘못된 cross-plug를 기계적으로 방지한다.
- 각 transceiver TXD/RXD/STB, CANH/CANL, 5 V, 3.3 V, reset, UART 네 선에 test point를 둔다.
- 세 CAN channel의 termination은 서로 독립적이며 기본 미장착이다.
- vehicle ground와 logic ground의 연결 방식, shield/chassis 접속은 EMC 시험 전에 의도적으로 정의한다.
- prototype에서 TXD dominant timeout, bus-off, 단선, short-to-battery/ground, MCU unpowered 조건을 시험한다.

## 7. 확정 전 필요한 검증

1. 2017 Tucson TL 실제 connector와 세 bus의 physical layer·bitrate 확인
2. CAN 3가 ISO 11898-3 계열 fault-tolerant 125 kbps인지 확인
3. MAX3055 `RXD/ERR` level guard 부품과 unpowered injection current 계산
4. 4 Mbps UART의 양 방향 eye, baud error, CTS stop latency, overflow 시험
5. ESP32 antenna 효율과 ESP-NOW packet loss를 enclosure 안에서 측정
6. load dump/cold crank/ESD/EMC 요구조건과 보호 부품 확정
7. 차량 bus에 병렬 연결했을 때 termination 및 common-mode 영향 확인

## 8. 공식 출처

- [Espressif ESP32-S3-MINI-1/MINI-1U 데이터시트 v1.7](https://www.espressif.com/sites/default/files/documentation/esp32-s3-mini-1_mini-1u_datasheet_en.pdf)
- [Espressif ESP32-S3 GPIO matrix 문서](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html)
- [Espressif ESP32-S3 UART 문서](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/uart.html)
- [ST STM32G474CE 제품 페이지](https://www.st.com/en/microcontrollers-microprocessors/stm32g474ce.html)
- [ST STM32G474xB/xC/xE 데이터시트](https://www.st.com/resource/en/datasheet/stm32g474ce.pdf)
- [TI TCAN1046AV-Q1 제품 페이지](https://www.ti.com/product/TCAN1046AV-Q1)
- [TI TCAN1046AV-Q1 데이터시트](https://www.ti.com/lit/ds/symlink/tcan1046av-q1.pdf)
- [Analog Devices MAX3054/MAX3055/MAX3056 데이터시트](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX3054-MAX3056.pdf)
