# Communicator 하드웨어 설계 기준

이 문서는 [문서 지도](../README.md)에 속한 Communicator 전원·CAN PHY·MCU pinmap의 회로 입력 요구사항이다. 시스템 권한 경계는 [시스템 아키텍처](../architecture/system.md), 제작 산출물은 T-100이 정본이다.

## 1. 문서 상태와 안전 목표

이 문서는 `Communicator`의 전원, 회로, 핀 할당과 부팅 안전 상태를 정의하는 회로 입력 요구사항이다. 아직 승인 KiCad 회로도·BOM·ERC·firmware 독립 hard TX gate가 없으므로 제작 정본이나 차량 송신 승인으로 사용하지 않는다. 해당 산출물은 [T-100](../tasks/T-100-communicator-schematic.md)에서 확정한다. AEC/PPAP 적합성 판단은 범위 밖이며, 다음 전기적 목표를 우선한다.

- MCU가 무전원·reset·brownout·watchdog reset·firmware 정지 상태여도 CAN bus를 dominant로 붙잡지 않는다.
- 5 V가 유효해지기 전에 3.3 V MCU rail을 켜지 않고, 3.3 V가 안정된 뒤 두 MCU의 reset을 해제한다.
- CAN PHY는 clock, bitrate, filter와 interrupt가 모두 준비된 뒤에만 normal mode로 전환한다.
- 입력 과전압·역극성·전원 강하 시 PHY는 standby 또는 high-impedance 상태로 수렴한다.

IC 사양과 핀 multiplexing은 제조사 데이터시트에서 확인했지만, 보호소자 정격·MOSFET SOA·thermal·signal integrity는 회로도/ERC와 prototype 시험 전까지 확정값이 아니다.

## 2. 확정 부품과 역할

| 블록 | 적용 부품 | 확인된 주요 사양 | 역할 |
|---|---|---|---|
| 무선 MCU | `ESP32-S3-MINI-1-N4R2` | dual-core LX7 최대 240 MHz, 4 MB Flash, 2 MB PSRAM, 3.0–3.6 V | ESP-NOW, pairing, 설정, 무선/UART queue |
| CAN MCU | `STM32G474CEU6` | Cortex-M4F 170 MHz, Flash 512 KB, SRAM 128 KB, FDCAN 3개, UFQFPN48 | CAN timestamp/filter, UART, 최종 TX safety gate |
| 고속 CAN PHY | `TCAN1046AV-Q1` | dual channel, ISO 11898-2:2016, CAN FD 최대 8 Mbps, VCC 5 V, VIO 3.3 V | CAN 1·2 물리계층 |
| fault-tolerant CAN PHY | `MAX3055ASD+` | 125 kbps, ±80 V bus fault protection, VCC 5 V, 별도 BATT | CAN 3 물리계층 |
| 입력 보호 controller | `LM74800-Q1` | 3–65 V, common-source back-to-back N-FET 구동, reverse blocking, OV disconnect | 역극성·역전류·과전압 차단 |
| 5 V buck-boost | `MAX20040B` 계열 | 기동 후 2–36 V, 최대 1.2 A, open-drain PGOOD | PHY 5 V와 3.3 V 전단 |
| 3.3 V buck | `TPS629210` | 3–17 V 입력, 최대 1 A | STM32·ESP32·TCAN VIO 전원 |
| 3.3 V supervisor | `TLV803EA30DPWR` | 3.08 V nominal threshold, 200 ms nominal delay, open-drain active-low, X2SON | STM32 NRST·ESP CHIP_PU 공통 감시 |
| CAN clock | HSE crystal | STM32 지원 범위 4–48 MHz | FDCAN kernel clock 정확도 확보 |

`ESP32-S3-PICO-1`은 사용하지 않는다. `ESP32-S3-MINI-1-N4R2`의 `GPIO26`은 내부 PSRAM용이므로 외부에 배선하지 않는다. `GPIO19/20`은 native USB, `GPIO0/3/45/46`은 strapping 영향 때문에 UART와 제어선에서 제외한다.

`MAX3055`는 고속 CAN transceiver가 아니다. CAN 3의 실제 차량 bus가 125 kbps fault-tolerant 방식인지 확인되지 않으면 PHY를 enable하지 않는다.

현재 제안 pinmap에는 firmware와 독립적인 차량 송신 차단 net이 없다. 승인 회로에는 세 TXD와 PHY 사이의 tri-state gate, TXD output별 pull-up, default-disabled `/OE`, 물리 `TX_ARM`, rail-good와 100 ms 이하 외부 watchdog/monostable을 추가해야 한다. gate가 닫혀도 RX는 유지하고 상태를 별도 GPIO로 sense한다. exact logic MPN과 sense GPIO는 회로/ERC/SI 검토에서 정하며, 이 gate가 없는 revision은 `CAPTURE_ONLY`로만 사용한다.

## 3. 전원과 reset

### 3.1 전원 tree

```text
Vehicle BATT 12 V
        │
      Fuse
        │
  SMBJ36CA + 입력 EMI/ESD network
        │
  LM74800-Q1 + common-source back-to-back N-FET ×2
  OV rising cutoff nominal 약 31.9 V
        │
   PROTECTED_VBAT ───────────────────────────> MAX3055 BATT
        │
     MAX20040B
        │  5V_PHY
        ├────────────────────────────────────> TCAN1046AV VCC
        ├────────────────────────────────────> MAX3055 VCC
        │
        └─ PGOOD(open drain, 10 kΩ pull-up to BIAS)
                         │
                         └────────────────────> TPS629210 EN
                                                   │
                                              TPS629210
                                                   │  3V3
                     ┌─────────────────────────────┼───────────────┐
                     │                             │               │
               STM32 VDD/VDDA               ESP32 3V3       TCAN VIO
                     │                             │
                     └────────── SYS_RESET_N ──────┘
                                      ▲
                         TLV803EA30DPWR RESET
                         10 kΩ pull-up to 3V3
```

MAX20040 PGOOD은 출력이 regulation 값의 약 96%를 넘을 때 해제되고 약 93% 아래에서 다시 low가 된다. 데이터시트 기준으로 PGOOD을 BIAS에 10 kΩ로 pull-up하고 `TPS629210 EN`에 연결한다. TPS629210의 EN 내부 pull-down은 PGOOD이 유효하지 않은 구간에 3.3 V rail을 off로 수렴시킨다.

MAX20040B의 제안 주문형식은 `MAX20040BATPA/VY+`이고 FB를 VCC에 연결해 5 V fixed output으로 쓴다. TPS629210은 VSET을 open 또는 249 kΩ 이상으로 두는 3.3 V 설정을 사용한다. compensation, switching frequency, inductor와 capacitor는 두 regulator의 공식 design procedure로 계산한다.

MAX20040은 최초 기동에 `IN > 3.5 V`가 필요하며 기동 완료 뒤에만 약 2 V까지 동작 범위가 확장된다. 따라서 배터리가 이미 2–3 V인 상태에서 새 부팅은 보장하지 않는다. 이 구간에서는 reset 유지와 PHY 비활성 상태가 정상 동작이다.

TPS629210의 1 A 한계는 ESP32 RF burst, STM32, TCAN VIO를 모두 포함한다. 회로 확정 전에 3.3 V peak current, startup inrush, 출력 capacitor, junction temperature를 계산하고 Wi-Fi burst 중 rail droop을 계측한다. MAX20040의 1.2 A 한계에도 3.3 V 변환 입력전류와 세 PHY의 worst-case 전류를 포함한다.

### 3.1.1 전원 상태기계

첫 vehicle-capable revision은 명시적 `ACC/KL15`로 기동하고 CAN wake를 사용하지 않는다. ACC가 없는 harness는 bench/capture-only로만 지원한다.

| 상태 | 조건 | rail | CAN TX |
|---|---|---|---|
| `OFF` | ACC low, bench enable 없음 | LM74800/ACC front end만, 5V/3V3 off | hard gate off |
| `STARTING` | ACC rising | MAX20040 on, PGOOD 전 3V3 off | hard gate off |
| `RUN_RX` | 5V PGOOD, 3V3 supervisor 해제 | MCU와 PHY 수신 준비 | hard gate off |
| `RUN_TX_ARMED` | 승인 build+물리 TX_ARM+외부 watchdog+rail-good | 모두 on | generated executor만 허용 |
| `SHUTDOWN` | ACC falling 또는 UV/fault | TX permit을 먼저 비동기 제거한 뒤 rail off | 즉시 gate off |
| `SERVICE` | current-limited bench 12V | 차량과 같은 보호 경로 | 기본 capture-only |

`POWER_EN`은 firmware가 아니라 protected ACC input/bench enable 회로가 MAX20040 EN을 구동한다. ACC loss와 PROTECTED_VBAT UV는 rail이 내려가기 전에 hard TX permit과 MAX3055 EN을 hardware로 제거한다. PGOOD pull-up은 generic `BIAS`가 아니라 회로도에서 실제 `MAX20040_BIAS` package pin으로 이름·전압을 고정한다. OFF 25 °C parked current 목표는 1 mA 이하고 72시간·온도별로 측정한다.

native USB는 data-only로 고정해 target rail을 공급하지 않는다. SWD header의 VTref는 target에서 probe로 나가는 sense이고 전원 입력 pin을 두지 않는다. service 전원은 별도 current-limited 12 V connector로 동일 보호 경로에 넣는다. BATT/ACC/USB/SWD 투입·제거 순서 전체에서 phantom powering과 GPIO injection을 검증한다.

MAX3055 BATT는 protected VBAT에 남으므로 PROTECTED_VBAT UV comparator, 5V PGOOD와 hard TX permit을 CAN3 EN/TXD 차단 조건에 포함한다. WAKE input은 CAN wake 미사용 상태의 데이터시트 권장 bias로 고정하고 INH output은 명시적 no-connect/test point로 처리한다. threshold와 hysteresis는 MAX20040 start/hold, MAX3055 BATT/VCC 영역과 cold-crank 목표를 worst-case로 계산해 회로 산출물에 기록한다.

### 3.2 입력 보호와 OV divider

LM74800 OV divider의 제안값은 다음과 같다.

```text
LM74800_VIN ───── 100 kΩ ──┬── LM74800 OV
                            │
                          4.02 kΩ
                            │
                           GND
```

`VOVR = 1.233 V` nominal을 적용하면 rising cutoff는 `1.233 × (100 kΩ + 4.02 kΩ) / 4.02 kΩ ≈ 31.9 V`다. falling 기준 약 1.13 V를 적용한 nominal 재연결점은 약 29.2 V다. 합계 104.02 kΩ는 OV leakage 오차를 줄이기 위한 제조사 권고 `120 kΩ 미만`을 만족한다. 실제 cutoff 범위는 reference·저항 공차와 온도를 포함해 worst-case로 다시 계산한다.

`SMBJ36CA`는 TI reference 회로에서 ISO 7637 Pulse 1의 음전압을 LM74800의 −65 V absolute maximum 안으로 제한하는 역할이다. 양의 load dump 에너지를 이 TVS 하나가 모두 흡수한다고 가정하지 않는다. 양의 과전압은 OV 차단으로 downstream을 분리하며, 두 N-FET의 `VDS`, avalanche/SOA, gate 보호, TVS pulse rating과 PCB 열을 대상 pulse profile로 검증한다.

입력 connector에서 fuse, TVS, FET power loop를 짧게 배치한다. `PROTECTED_VBAT` 이후에 MAX20040과 MAX3055 BATT를 분기하며, 역급전 경로와 ignition-off 대기전류를 별도 점검한다.

ESP32 native USB service port의 VBUS와 vehicle power가 동시에 들어오는 상태를 별도 power case로 다룬다. USB VBUS가 5V_PHY/3V3/차량 rail을 역급전하거나 차량 rail이 USB host로 흘러가지 않도록 power-path 또는 data-only service 정책을 회로도에 명시하고 네 rail 조합을 prototype에서 측정한다.

### 3.3 3.3 V supervisor와 공통 reset

`TLV803EA30DPWR`의 부품명은 다음 의미다.

- `TLV803E`: active-low open-drain reset
- `A`: 200 ms nominal delay
- `30`: 3.08 V nominal falling threshold
- `DPW`: 0.8 mm × 0.8 mm X2SON-5

RESET에는 3.3 V로 10 kΩ pull-up을 두고 `STM32 NRST`와 `ESP32 CHIP_PU`를 묶은 `SYS_RESET_N`을 구동한다. VDD에는 IC 가까이 100 nF를 둔다. X2SON의 `MR`은 내부 pull-up을 사용하되 service reset test point를 제공한다. 공통선이므로 SWD reset도 ESP32를 함께 reset한다. 독립 reset이 필요하다는 시험 결과가 나오면 open-drain buffer로 두 branch를 분리한다.

ESP32는 3.3 V가 안정된 뒤 CHIP_PU가 high가 되기까지 최소 50 µs가 필요하다. 200 ms supervisor가 이 조건을 충족하므로 supervisor 출력에 큰 RC capacitor를 중복 장착하지 않는다. 정확한 reset delay 보증 범위와 threshold 공차는 BOM 확정 시 최신 데이터시트로 재확인한다.

## 4. CAN 물리계층 회로

### 4.1 CAN 1·2: TCAN1046AV-Q1

두 채널의 reset 기본 상태는 동일하다.

```text
3V3 ── 10 kΩ ──┬── TXD1 <──────── STM PA12 / FDCAN1_TX
                │
                └── STB1 <──────── STM PA4

3V3 ── 10 kΩ ──┬── TXD2 <──────── STM PB13 / FDCAN2_TX
                │
                └── STB2 <──────── STM PA5

TCAN RXD1 ────────────────────────> STM PA11 / FDCAN1_RX
TCAN RXD2 ────────────────────────> STM PB12 / FDCAN2_RX
```

실제 회로에서는 TXD1, STB1, TXD2, STB2 각각에 독립 10 kΩ pull-up을 둔다. MCU가 Hi-Z일 때 `TXD=HIGH`는 recessive, `STB=HIGH`는 standby이므로 부팅·reset 중 두 bus를 dominant로 만들지 않는다. TCAN 내부 bias와 undervoltage 보호가 있어도 외부 저항을 reset 상태의 정본으로 사용한다.

TCAN1046AV에는 VCC 5 V, VIO 3.3 V를 공급하고 각 supply pin 가까이에 데이터시트 권장 decoupling을 둔다. VCC/VIO undervoltage 또는 무전원 상태에서 bus와 logic pin이 high-impedance가 되는 조건도 power-sequence 시험에 포함한다.

| TCAN pin | 신호 | 연결 |
|---:|---|---|
| 1 | TXD1 | STM PA12, 10 kΩ pull-up |
| 2 | GND1 | ground plane |
| 3 | VCC | 5V_PHY |
| 4 | RXD1 | STM PA11 |
| 5 | VIO | 3V3 |
| 6 | TXD2 | STM PB13, 10 kΩ pull-up |
| 7 | GND2 | ground plane |
| 8 | RXD2 | STM PB12 |
| 9 | STB2 | STM PA5, 10 kΩ pull-up |
| 10 | CANL2 | CAN 2 protection·connector |
| 11 | CANH2 | CAN 2 protection·connector |
| 12 | CANL1 | CAN 1 protection·connector |
| 13 | CANH1 | CAN 1 protection·connector |
| 14 | STB1 | STM PA4, 10 kΩ pull-up |

차량 connector 쪽 회로 기준은 다음과 같다.

- connector 바로 뒤에 CAN FD data rate에 맞는 저용량 TVS를 배치한다.
- common-mode choke는 footprint만 두고 기본 `DNP`로 시작한다.
- 차량 bus의 중간 stub이면 종단 부품은 모두 `DNP`다.
- 이 보드가 검증된 endpoint일 때만 `60.4 Ω + 60.4 Ω` split termination과 center tap–GND `4.7 nF` footprint를 장착한다.
- termination 장착 여부는 CAN 1과 CAN 2에서 각각 독립 관리한다.

### 4.2 CAN 3: MAX3055

MAX3055는 STB를 high로 고정하고 EN 하나만 STM32가 제어한다.

```text
5V_PHY ── 10 kΩ ───────────────────────────> MAX3055 STB

STM PA6 / CAN3_EN ─────────────────────────> MAX3055 EN
                │
              10 kΩ
                │
               GND

3V3 ── 10 kΩ ──┬── MAX3055 TXD <────────── STM PA15 / FDCAN3_TX

MAX3055 RXD ── 5.1 kΩ ──┬────────────────> STM PA8 / FDCAN3_RX
                         │
                       10 kΩ
                         │
                        GND

MAX3055 ERR ── 5.1 kΩ ──┬────────────────> STM PB14 / CAN3_ERR
                         │
                       10 kΩ
                         │
                        GND
```

MAX3055 mode table에서 `STB=1, EN=0`은 Power-On Standby, `STB=1, EN=1`은 Normal Operating이다. reset 중 PA6은 Hi-Z이고 외부 10 kΩ pull-down이 EN을 low로 유지한다. firmware가 준비된 뒤 PA6만 high로 전환한다. TXD pull-up은 MCU reset 시 recessive를 보장하며, PA15 자체의 reset pull-up 방향과도 일치한다.

MAX3055 RXD/ERR high는 `VCC − 0.5 V`에서 VCC까지 올라갈 수 있다. PA8/PB14가 5 V tolerant이더라도 5.1 kΩ/10 kΩ divider를 기본 장착해 약 3.3 V로 낮추고 STM32 무전원 시 injection을 제한한다. 이 값은 125 kbps에서 검증하되, 입력 capacitance·threshold·상승시간과 STM32 전원 off injection current를 worst-case로 계산한다. CAN3를 실장·enable하는 variant에서는 ERR과 divider를 mandatory로 둔다. MAX3055 전체가 DNP인 variant에서만 함께 DNP 처리한다.

| MAX pin | 신호 | 연결·처리 |
|---:|---|---|
| 1 | INH | CAN wake 미사용 output, 명시적 NC + test point |
| 2 | TXD | STM PA15, 10 kΩ pull-up to 3V3 |
| 3 | RXD | 5.1 kΩ/10 kΩ divider를 거쳐 STM PA8 |
| 4 | ERR | divider를 거쳐 STM PB14; CAN3 실장 시 mandatory |
| 5 | STB | 10 kΩ pull-up to 5V_PHY, MCU에 연결하지 않음 |
| 6 | EN | STM PA6, 10 kΩ pull-down |
| 7 | WAKE | 데이터시트 권장 inactive bias·protection, floating 금지 |
| 8 | RTH | CANH distributed termination footprint |
| 9 | RTL | CANL distributed termination footprint |
| 10 | VCC | 5V_PHY, local bypass |
| 11 | CANH | CAN 3 protection·connector |
| 12 | CANL | CAN 3 protection·connector |
| 13 | GND | ground plane |
| 14 | BATT | PROTECTED_VBAT, local bypass |

FT-CAN 종단은 고속 CAN과 다르다. CANH–CANL 사이에 120 Ω을 넣지 않는다. RTH와 RTL 각각의 distributed termination footprint를 두고 기본 `DNP`로 시작한다. MAX3055 데이터시트 기준 각 node 저항은 500 Ω 미만으로 내리지 않으며, network의 CANH와 CANL 각각 total target은 100 Ω이다. 소규모·짧은 차량 network에서는 합성값이 더 커도 동작할 수 있으므로 실차 기존 종단을 측정한 뒤 값을 정한다.

## 5. STM32G474CEU6 UFQFPN48 최종 제안 핀맵

기계 판독용 정본은 [`../hardware/communicator/pinmap-proposed.csv`](../../hardware/communicator/pinmap-proposed.csv)다.

| package pin | MCU pin | AF/모드 | 기능 | 연결·reset 처리 |
|---:|---|---|---|---|
| 5 | `PF0` | `OSC_IN` | HSE input | 4–48 MHz crystal network |
| 6 | `PF1` | `OSC_OUT` | HSE output | crystal network |
| 7 | `PG10-NRST` | reset | `SYS_RESET_N` | TLV803E·SWD, floating 금지 |
| 8 | `PA0` | AF7 `USART2_CTS` | UART CTS | ESP GPIO15 RTS, 10 kΩ pull-up |
| 9 | `PA1` | AF7 `USART2_RTS_DE` | UART RTS | ESP GPIO16 CTS, 10 kΩ pull-up |
| 10 | `PA2` | AF7 `USART2_TX` | UART TX | ESP GPIO18 RX, weak pull-up footprint |
| 11 | `PA3` | AF7 `USART2_RX` | UART RX | ESP GPIO17 TX, weak pull-up footprint |
| 12 | `PA4` | output | TCAN STB1 | external 10 kΩ pull-up → standby |
| 13 | `PA5` | output | TCAN STB2 | external 10 kΩ pull-up → standby |
| 14 | `PA6` | output | MAX3055 EN | external 10 kΩ pull-down → disabled |
| 25 | `PB12` | AF9 `FDCAN2_RX` | CAN 2 RX | TCAN pin 8 |
| 26 | `PB13` | AF9 `FDCAN2_TX` | CAN 2 TX | TCAN pin 6, external 10 kΩ pull-up |
| 27 | `PB14` | input | MAX3055 ERR | divider; CAN3 전체 DNP variant에서만 미실장 |
| 30 | `PA8` | AF11 `FDCAN3_RX` | CAN 3 RX | MAX pin 3 via divider |
| 33 | `PA11` | AF9 `FDCAN1_RX` | CAN 1 RX | TCAN pin 4 |
| 34 | `PA12` | AF9 `FDCAN1_TX` | CAN 1 TX | TCAN pin 1, external 10 kΩ pull-up |
| 36 | `PA13` | AF0 `SWDIO` | debug | SWD header |
| 37 | `PA14` | AF0 `SWCLK` | debug | SWD header |
| 38 | `PA15` | AF11 `FDCAN3_TX` | CAN 3 TX | MAX pin 2, external 10 kΩ pull-up |
| 46 | `PB8-BOOT0` | boot strap | BOOT0 | 10 kΩ pull-down + service pad |

`PA11/PA12`는 STM32 USB와 multiplex되므로 STM32 USB는 사용하지 않는다. service interface는 ESP32 native USB와 STM32 SWD를 사용한다.

이 핀맵은 이전의 `PB3/PB4 = FDCAN3`, `PB5/PB6 = FDCAN2` 배치를 폐기한다. STM32는 reset 시 PA15·PA13·PB4에 debug용 pull-up, PA14에 pull-down을 적용한다. 또한 PB4/PB6에는 UCPD dead-battery 5.1 kΩ pull-down 경로가 개입할 수 있다. UCPD pull-down은 firmware에서 해제할 수 있지만 reset 전부터 보장되는 하드웨어 안전 상태에는 의존할 수 없다. PA15 reset pull-up은 CAN TXD recessive 방향이므로 FDCAN3 TX에는 오히려 적합하다.

## 6. ESP32 UART와 reset 핀

| ESP pad | GPIO | 기능 | STM32 peer | 외부 기본값 |
|---:|---:|---|---|---|
| 19 | 15 | UART1 RTS | PA0 CTS | 10 kΩ pull-up |
| 20 | 16 | UART1 CTS | PA1 RTS | 10 kΩ pull-up |
| 21 | 17 | UART1 TX | PA3 RX | 22–47 kΩ weak pull-up footprint |
| 22 | 18 | UART1 RX | PA2 TX | 22–47 kΩ weak pull-up footprint |
| 23 | 19 | USB D− | USB connector | 다른 기능 배정 금지 |
| 24 | 20 | USB D+ | USB connector | 다른 기능 배정 금지 |
| 26 | 26 | internal PSRAM | 연결 금지 | N4R2 내부 전용 |
| 45 | — | CHIP_PU | SYS_RESET_N | TLV803E + 10 kΩ pull-up |

RTS/CTS 두 선은 각각 high가 되어 상대 송신기가 전송을 멈추는 상태로 시작한다. TX/RX weak pull-up은 line idle을 정의하는 선택 footprint이며 4 Mbps eye 측정 결과로 장착값을 정한다. 두 MCU firmware에서 hardware flow-control polarity를 invert하지 않는다.

## 7. reset 상태와 firmware 부팅 순서

### 7.1 무전원·reset 기본 상태

| 대상 | 하드웨어 기본값 | 결과 |
|---|---|---|
| TCAN TXD1/TXD2 | 각 10 kΩ pull-up | recessive |
| TCAN STB1/STB2 | 각 10 kΩ pull-up | CAN 1·2 standby |
| MAX3055 TXD | 10 kΩ pull-up | recessive |
| MAX3055 STB | 10 kΩ to 5 V | high 고정 |
| MAX3055 EN | 10 kΩ pull-down | Power-On Standby, transmitter 비활성 |
| UART RTS/CTS | 각 10 kΩ pull-up | 양방향 flow stop |
| STM32 BOOT0 | 10 kΩ pull-down | user Flash boot |
| SYS_RESET_N | supervisor open-drain | 3.3 V 부족 시 두 MCU reset |
| CAN termination | switch off 또는 DNP | 차량 기존 종단 불변 |

외부 resistor가 reset 안전의 정본이다. firmware의 첫 GPIO write는 이 상태를 유지하는 보조 계층이다.

### 7.2 STM32 부팅 순서

1. PA4·PA5 output latch를 high, PA6를 low로 먼저 기록한 뒤 output mode로 전환한다.
2. HSE를 시작하고 PLL/FDCAN kernel clock을 검증한다.
3. USART2 4 Mbps와 RTS/CTS를 초기화하되 CTS가 허용되기 전 송신하지 않는다.
4. FDCAN 3채널의 bitrate, message RAM, filter, interrupt와 error 처리를 구성한다.
5. 세 controller를 먼저 start하고 target bus profile을 listen-only로 검증한다.
6. 검증된 채널에 한해 PA4·PA5를 low로 내려 TCAN을 normal로 만들고, PA6를 high로 올려 MAX3055를 normal로 만든다.
7. vehicle TX는 별도 control lease와 safety 조건이 충족되기 전까지 계속 금지한다.

HSE startup 또는 clock 검증에 실패하면 PHY enable 단계로 진행하지 않고 error를 기록한 뒤 watchdog reset한다. IWDG 목표 timeout은 250–500 ms이며, main loop·CAN ISR·UART worker가 모두 정상일 때만 refresh한다. firmware가 정지해도 GPIO는 자동으로 Hi-Z가 되지 않는다. 외부 TX guardian heartbeat가 100 ms 이내 만료되어 tri-state gate를 recessive로 만들고, 이후 IWDG reset이 PA4/PA5/PA6를 reset 기본 상태로 돌려야 한다. TCAN TXD dominant timeout과 MAX3055 permanent-dominant timer는 stuck-low에 대한 추가 방어일 뿐 정상 형식의 반복 frame을 차단하는 bus guardian으로 간주하지 않는다.

## 8. PCB·prototype 검증 체크리스트

- HSE crystal과 load capacitor를 STM32에 가깝고 CAN switching loop에서 멀리 배치한다.
- ESP32 antenna keep-out에 copper, trace, 부품, metal enclosure를 두지 않는다.
- 4 Mbps UART 네 선과 ground reference를 두 MCU 사이 최단으로 routing하고 각 source에 series resistor footprint를 둔다.
- CAN connector에는 채널별 CANH/CANL/GND와 protection을 독립 배치한다.
- TXD/RXD/STB/EN, CANH/CANL, 5 V, 3.3 V, PGOOD, SYS_RESET_N, UART 네 선에 test point를 둔다.
- power ramp up/down, cold crank, OV cutoff/reconnect, brownout, watchdog, MCU별 reset에서 세 bus가 비간섭 상태인지 계측한다.
- TXD stuck-low, CAN short-to-ground/battery, bus-off, MAX ERR, 한 MCU 무전원 조건을 fault-injection한다.
- CAN FD 채널은 5/8 Mbps까지 eye와 sample point를 확인하고, CAN 3은 125 kbps와 RTH/RTL network를 별도 검증한다.
- 실제 2017 Tucson TL connector, bus 종류, bitrate, 기존 종단을 확인하기 전 차량 TX와 onboard termination을 enable하지 않는다.

## 9. 공식 출처

- [Espressif ESP32-S3-MINI-1/MINI-1U 데이터시트](https://www.espressif.com/sites/default/files/documentation/esp32-s3-mini-1_mini-1u_datasheet_en.pdf)
- [Espressif ESP32-S3 전원·CHIP_PU 설계 가이드](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html)
- [ST STM32G474xB/xC/xE 데이터시트](https://www.st.com/resource/en/datasheet/stm32g474ce.pdf)
- [TI TCAN1046AV-Q1 데이터시트](https://www.ti.com/lit/ds/symlink/tcan1046av-q1.pdf)
- [Analog Devices MAX3054/MAX3055/MAX3056 데이터시트](https://www.analog.com/media/en/technical-documentation/data-sheets/max3054-max3056.pdf)
- [TI LM7480-Q1 데이터시트](https://www.ti.com/lit/ds/symlink/lm7480-q1.pdf)
- [Analog Devices MAX20039/MAX20040 데이터시트](https://www.analog.com/media/en/technical-documentation/data-sheets/max20039-max20040.pdf)
- [TI TPS629210 제품 페이지](https://www.ti.com/product/TPS629210)
- [TI TLV803E 데이터시트](https://www.ti.com/lit/gpn/TLV803E)
