# Controller 하드웨어 및 개발환경

이 문서는 [문서 지도](../README.md)에 속한 Waveshare Controller의 board·pin·주변장치 상세 자료다. 전체 장치 책임은 [시스템 아키텍처](../architecture/system.md)가 정본이다.

## 1. 프로젝트 범위

`canview`는 차량 네트워크에 직접 연결되는 **Communicator**와, 데이터를 표시·제어하는 **Controller**를 분리한다. 전체 장치 경계는 [시스템 아키텍처](../architecture/system.md), Communicator의 부품·회로·핀 배정은 [Communicator hardware](communicator.md)에 정의한다.

```text
차량 CAN 1 ─┐
차량 CAN 2 ─┼─> Communicator ── ESP-NOW ──> Controller
차량 CAN 3 ─┘       │                           │
                    │                           ├─ ST7796 LCD
                    │                           ├─ FT6336 터치
                    │                           └─ UI/상태/제어 요청
                    └<────── ESP-NOW 응답 ───────┘
```

여기서 “최대 3개 CAN 버스”는 시스템 요구사항이다. Waveshare 보드 자체에 CAN 트랜시버가 3개 들어 있다는 뜻이 아니며, 보드에는 차량 CAN 커넥터나 CAN 물리계층 회로가 없다. 별도 Communicator에 CAN 채널을 3개 구성한다.

미확정 CAN 신호 검증에는 차량 CAN에 직접 연결되지 않는 선택 장치 `Diagnostic Bridge`를 추가할 수 있다. Bridge는 Communicator의 ESP-NOW read-only peer이자 휴대폰 SoftAP 웹서버이며, 권장 module·전원·버튼·저장장치와 안전 경계는 [Diagnostic Bridge·모바일 CAN 검증 UI](../architecture/diagnostic-bridge.md)에 정의한다.

기본 동작은 **listen-only/read-only**다. 차량 제어를 활성화한 설치에서는 Primary Controller에 검증된 음량·profile 패닝·mute·SPORT 등 필요한 의미 명령만 기능별로 허용한다. 실차에서 바로 활성화하지 않고, 벤치 테스트·명령 허용 목록·속도 제한·사용자 확인·통신 단절 시 fail-safe를 모두 통과해야 하며 최종 CAN 송신 판정은 Communicator STM32가 담당한다.

## 2. Waveshare 보드 사양

| 항목 | 확인된 내용 |
|---|---|
| 제품 | `ESP32-S3-Touch-LCD-3.5`, SKU `30733`; 케이스 버전 `30934` |
| MCU | ESP32-S3R8, dual-core Xtensa 32-bit LX7, 최대 240 MHz |
| 무선 | 2.4 GHz Wi-Fi 802.11 b/g/n, Bluetooth 5 LE, onboard antenna |
| 메모리 | 512 KB SRAM, 384 KB ROM, 8 MB PSRAM, 외부 16 MB Flash |
| LCD | 3.5인치 capacitive HD IPS, 320×480, 262K colors, ST7796, SPI |
| Touch | FT6336, I2C |
| IMU | QMI8658, 3축 가속도계 + 3축 자이로 |
| RTC | PCF85063, AXP2101을 통한 전원 공급, RTC 배터리 패드 예약 |
| 오디오 | ES8311 codec, onboard speaker, onboard SMD microphone |
| 전원 | USB Type-C, 3.7 V MX1.25 리튬 배터리 충전·방전 인터페이스, AXP2101 PMIC |
| 저장장치 | onboard TF card slot |
| 카메라 | 24-pin camera interface, OV5640/OV2640 계열 지원 |
| 조작 | `PWR` 버튼, `BOOT` 버튼, capacitive touch |
| 확장 | I2C, UART, USB, 다수 GPIO header |
| CAN | onboard CAN controller/transceiver/connecter 없음. 별도 CAN 하드웨어 필요 |

보드의 LCD, 터치, TF, 카메라, PMIC와 센서는 이미 GPIO를 공유하거나 점유한다. 따라서 남아 보이는 GPIO를 임의로 CAN 또는 외부 제어선에 배정하지 말고, 아래 점유표와 Waveshare 회로도를 함께 확인한다.

## 3. 32핀 헤더 핀맵

아래 표는 Waveshare 공식 pinout 이미지의 헤더 번호와 라벨을 옮긴 것이다. 실제 배선 전에는 사용하는 보드 리비전의 실크와 회로도를 최종 확인한다.

| 핀 | 라벨 | 용도·주의 |
|---:|---|---|
| 1 | `BAT` | 배터리 전원 계통. 외부 회로 전원으로 임의 사용하지 않는다. |
| 2 | `5V` | 5 V 전원 핀. CAN 트랜시버 전원은 별도 전원 설계를 사용한다. |
| 3 | `GND` | 공통 접지 |
| 4 | `GND` | 공통 접지 |
| 5 | `GPIO21` | 카메라 데이터 계통에 점유 |
| 6 | `DN(GPIO19)` | USB D− |
| 7 | `GPIO38` | 카메라 데이터 계통에 점유 |
| 8 | `DP(GPIO20)` | USB D+ |
| 9 | `GPIO39` | 카메라 데이터 계통에 점유 |
| 10 | `GPIO11` | TF/SPI 계통에 점유 |
| 11 | `GPIO40` | 카메라 데이터 계통에 점유 |
| 12 | `GPIO10` | TF/SPI 계통에 점유 |
| 13 | `GPIO41` | 카메라 데이터 계통에 점유 |
| 14 | `GPIO9` | TF/SPI 계통에 점유 |
| 15 | `GPIO42` | 카메라 데이터 계통에 점유 |
| 16 | `GPIO17` | 카메라 동기 신호 계통에 점유 |
| 17 | `GPIO45` | 카메라 데이터 계통에 점유 |
| 18 | `GPIO18` | 카메라 동기 신호 계통에 점유 |
| 19 | `GPIO46` | 카메라 데이터 계통에 점유 |
| 20 | `BOOT(GPIO0)` | 부트 모드 입력. 외부 회로가 리셋·부트 진입을 방해하지 않게 한다. |
| 21 | `GPIO47` | 카메라 데이터 계통에 점유 |
| 22 | `RST` | 보드 리셋. 외부 회로에서 구동하지 않는다. |
| 23 | `GPIO48` | 카메라 데이터 계통에 점유 |
| 24 | `PWR` | 전원 관리 버튼/계통. GPIO처럼 사용하지 않는다. |
| 25 | `RXD(GPIO44)` | UART RX |
| 26 | `SCL(GPIO7)` | onboard I2C SCL |
| 27 | `TXD(GPIO43)` | UART TX |
| 28 | `SDA(GPIO8)` | onboard I2C SDA |
| 29 | `GND` | 공통 접지 |
| 30 | `GND` | 공통 접지 |
| 31 | `3V3` | 3.3 V 계통 |
| 32 | `3V3` | 3.3 V 계통 |

### 외부 신호 배선 원칙

- Communicator와 Controller는 `ESP-NOW`로 무선 연결하므로, 차량 CAN 선을 Controller까지 끌어오지 않는다.
- 디버그 UART를 사용할 때는 `GPIO43/44`만 사용하고 3.3 V logic level인지 확인한다.
- `GPIO7/8`은 PMIC·터치·IMU·RTC·IO expander가 공유하는 I2C다. 외부 장치를 추가할 때 주소 충돌과 pull-up 전압을 확인한다.
- `5V`, `BAT`, `3V3`는 서로 대체 가능한 일반 전원 출력으로 간주하지 않는다. 차량 전원은 별도 automotive 전원 보호 회로를 거친다.
- `BOOT`, `RST`, `PWR`와 USB D+/D−에는 차량 신호를 연결하지 않는다.

## 4. 보드 내부 GPIO 점유

Waveshare의 Arduino 예제와 ESP-IDF 예제에서 확인한 기본 점유는 다음과 같다.

| 기능 | GPIO/설정 | 비고 |
|---|---|---|
| LCD SPI MOSI | `GPIO1` | 변경하지 않는 것을 권장 |
| LCD SPI MISO | `GPIO2` | 예제의 `SPI_MISO` |
| LCD DC | `GPIO3` | 예제의 `LCD_DC` |
| LCD SPI SCLK | `GPIO5` | 예제의 `SPI_SCLK` |
| LCD backlight | `GPIO6` | LEDC, 예제는 5 kHz/10-bit |
| onboard I2C SCL/SDA | `GPIO7` / `GPIO8` | 400 kHz 예제; 여러 장치가 공유 |
| LCD reset/확장 | `TCA9554`, I2C address `0x20` | 예제는 TCA9554 output 1로 LCD reset 제어 |
| TF card | `GPIO9`, `GPIO10`, `GPIO11` | 보드 저장장치와 충돌 |
| audio I²S | `GPIO12` MCLK, `GPIO13` BCLK, `GPIO14` playback DIN, `GPIO15` LRCLK, `GPIO16` recording DOUT | ES8311·speaker·SMD microphone; Arduino 예제 기준 |
| camera | `GPIO17`, `18`, `21`, `38`–`42`, `45`–`48` | 카메라 사용 시 외부 GPIO로 사용 금지 |
| USB | `GPIO19`, `GPIO20` | USB D−/D+ |
| UART | `GPIO43`, `GPIO44` | TX/RX |
| BOOT | `GPIO0` | 부트 스트랩 |

Arduino 예제의 화면 관련 기준값은 `320×480`, rotation `0`, `ST7796`이다. ESP-IDF 예제도 `SPI2_HOST`, MOSI `GPIO1`, SCLK `GPIO5`, DC `GPIO3`, backlight `GPIO6`, I2C SDA `GPIO8`, SCL `GPIO7`을 사용한다. LCD CS와 RST는 예제 설정에서 `GPIO_NC`/`-1`로 표시되고, reset은 `TCA9554`를 통해 처리되므로 회로도와 예제 드라이버의 초기화 순서를 유지한다.

Waveshare FAQ는 보드에 ES8311, speaker와 SMD microphone이 있다고 명시한다. 주변 소음 측정 prototype은 onboard microphone부터 사용한다. `GPIO12`–`16`은 audio 경로에 점유된 핀이므로 외부 microphone을 병렬 연결하지 않는다. 설치 위치 때문에 송풍음·speaker 누설이 지배적이면 별도 microphone node를 검토하며, 판단 기준과 신호처리는 [기능 설계](../architecture/features.md)에 정리했다.

### 4.1 I²C RTC와 시간 원천

보드의 `GPIO7=SCL`, `GPIO8=SDA` 공유 I²C에는 onboard `PCF85063` RTC가 있다. `TCA9554`(`0x20`), FT6336 touch, QMI8658 IMU와 같은 bus를 공유하므로 드라이버 초기화 시 address probe와 bus recovery를 수행한다. PCF85063의 7-bit I²C 주소는 `0x51`이다. Waveshare 보드 리비전과 부품 실장은 schematic 및 실제 probe로 최종 확인한다. [NXP PCF85063TP 데이터시트](https://www.nxp.com/docs/en/data-sheet/PCF85063TP.pdf)

PCF85063의 BCD 시간·날짜 레지스터, oscillator stop/invalid 상태, backup 전원 상태를 읽어 `rtc_quality`를 만든다. Controller가 RTC 소유자이며 UI의 시·분 선택은 Controller local transaction으로 RTC와 NVS에 직접 적용한다. 휴대폰에서 바꿀 때도 Diagnostic Bridge와 Controller 사이의 owner-targeted remote config를 사용하며 차량 command로 보내지 않는다. RTC wall clock은 화면·로그·일몰 계산용이고 CAN timestamp ordering은 Communicator STM32의 monotonic clock을 사용한다.

저장된 Hyundai DBC에는 1차 대상의 확정 GPS 좌표·현재 시각 신호가 없으므로 일출·일몰은 설정값 또는 별도 위치 원천에서 공급한다. 조사 결과는 [GPS·시간 조사](../vehicle/gps-time-investigation.md)에 기록한다.

## 5. Communicator 하드웨어 요구사항

### 5.1 채널 수와 물리계층

Communicator는 `ESP32-S3-MINI-1-N4R2`와 `STM32G474CEU6`, 2채널 `TCAN1046AV-Q1`, 1채널 `MAX3055`로 구성한다. 상세 회로 조건과 제안 핀맵은 [Communicator hardware](communicator.md), 기계 판독용 표는 [`hardware/communicator/pinmap-proposed.csv`](../../hardware/communicator/pinmap-proposed.csv)를 따른다.

Communicator는 최소 다음을 만족해야 한다.

1. 서로 독립된 `CAN1`, `CAN2`, `CAN3` 수신 경로
2. 채널별 CAN controller와 CAN transceiver
3. 채널별 CANH/CANL, 공통 접지, standby/enable 제어
4. 표준 11-bit와 필요 시 29-bit ID를 보존하는 수집 포맷
5. CAN classic 0–8 byte payload 보존
6. bus-off, error counter, overflow, timestamp를 채널별로 보고
7. 차량 연결부의 역전압·서지·ESD·과전류를 고려한 전원·보호 회로
8. 종단저항은 차량 네트워크의 실제 종단 상태를 확인한 뒤 필요한 위치에만 배치

CAN controller 3개는 STM32G474CEU6의 FDCAN1–3을 사용한다. CAN1·CAN2는 `TCAN1046AV-Q1`의 두 high-speed 채널에, CAN3는 `MAX3055`의 125 kbps fault-tolerant 채널에 연결한다. MAX3055를 고속 CAN 버스에 연결하거나 일반 120 Ω 종단을 그대로 적용하지 않는다. ESP32-S3-MINI-1-N4R2는 CAN frame 처리보다 ESP-NOW, provisioning, update를 맡고 STM32와 4 Mbps UART/RTS/CTS로 통신한다.

전원 기준안은 `LM74800-Q1 + back-to-back N-FET → MAX20040B 5 V → PGOOD-gated TPS629210 3.3 V → TLV803EA30DPWR 공통 reset`이다. MCU reset과 무전원 상태에서는 외부 pull resistor만으로 CAN1·2가 standby, CAN3가 Power-On Standby, UART가 flow-stop 상태가 되어야 한다. 구체 회로와 핀은 [Communicator hardware](communicator.md)를 정본으로 사용한다.

1차 차량이 classic CAN인지 실제 차량 캡처로 확인하기 전까지 bitrate, connector pin, bus 이름을 고정하지 않는다. 특히 `CAN1/2/3`은 프로젝트 내부 논리 이름이며 차량의 실제 CAN 버스 명칭과 같다고 가정하지 않는다.

### 5.2 수신 우선 설계

- 초기 펌웨어는 모든 채널을 listen-only로 시작한다.
- 수신 프레임에는 `bus_id`, monotonic timestamp, arbitration ID, IDE, RTR, DLC, data, error 상태를 붙인다.
- 화면 표시용 DBC decode는 Controller의 catalog/decoder에서 수행한다. Communicator는 원시 CAN frame과 bus 상태를 전달하며, 진단 모드에서도 같은 raw record를 사용한다.
- 전송(TX)은 기본 비활성화한다. 활성화하더라도 메시지·신호·주기·조건을 allow-list로 제한하고, 화면에서 만든 임의 raw frame은 전송하지 않는다.

## 6. ESP-NOW 링크 설계

역할은 Communicator가 최대 3개 CAN raw 수집·TX 안전 gate를 담당하고, Controller가 수신 allow-list·DBC decode·telemetry·LVGL·사용자 의도 명령을 담당하는 것으로 분리한다. wire protocol v1의 고정 frame은 ESP-NOW v1과 v2가 함께 처리할 수 있도록 240 byte 이하로 제한한다.

전체 명세는 [ESP-NOW protocol](../architecture/protocols/esp-now.md), C wire 구조는 [`protocol/canview_protocol.h`](../../protocol/canview_protocol.h)에 있다. 명세에는 다음을 포함한다.

- 32 byte little-endian header, CRC-32, sequence와 session
- QoS 0 telemetry와 QoS 1 command의 분리
- ACK와 실제 `COMMAND_RESULT`의 분리, 중복 제거와 재전송
- USB pair package, 장치 로컬 PMK, pair별 link root/LMK, pairing window와 transcript HMAC/HKDF
- channel 불일치, heartbeat 만료, queue overflow, bus-off 복구
- control lease, state revision, snapshot 복원, 사용자 물리 조작 우선
- capability bitset, TLV, major/minor version과 제한된 bulk transfer

production에서는 기본 PMK, LMK 없는 평문 unicast, 자동 보안 downgrade를 금지한다. ESP-NOW 송신 callback 성공은 애플리케이션 처리 성공이 아니므로 제어 완료는 STM32의 end-to-end tag와 차량 feedback까지 확인한 `COMMAND_RESULT(COMPLETED)`로만 판단한다.

## 7. 펌웨어 개발환경

### 7.1 Controller 권장 ESP-IDF 환경

Waveshare 공식 ESP-IDF 문서는 이 보드에 `ESP-IDF V5.5.0` 이상을 요구한다. 새 프로젝트의 최신 안정 baseline은 `ESP-IDF v6.0.3`으로 고정하되, Waveshare example/BSP의 IDF 6 호환 여부는 실제 board bring-up gate에서 확인한다.

권장 도구는 다음과 같다.

- Visual Studio Code
- Espressif `ESP-IDF` VS Code extension 2.0 이상
- ESP-IDF `v6.0.3` (`esp32s3`)
- `git`
- USB Type-C 데이터 케이블과 보드의 USB serial/JTAG 포트
- Python 기반 보조 도구가 필요한 경우 가상환경
- DBC 검사·decode용 `cantools`(선택)

기본 프로젝트 흐름은 다음과 같다.

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```

프로젝트 설정의 기준값은 `esp32s3`, 16 MB Flash, PSRAM enabled다. Waveshare 예제에는 PSRAM octal/80 MHz와 240 MHz CPU 설정이 포함돼 있지만, 실제 보드와 사용하는 IDF 버전에 맞춰 `sdkconfig`를 다시 검토한다. 예제 안에 더 오래된 `sdkconfig` 흔적이 있더라도 공식 현재 문서의 5.5.0 이상 요구사항을 우선한다.

### 7.2 Arduino 대안

빠른 화면·센서 bring-up에는 Arduino IDE도 사용할 수 있다. Waveshare가 안내한 기준 라이브러리는 다음과 같다.

| 라이브러리 | 기준 버전 |
|---|---:|
| LVGL | 8.4.0 |
| GFX_Library_for_Arduino | 1.5.5 |
| JPEGDEC | 1.8.2 |
| PNGdec | 1.8.2 |
| XPowersLib | 0.2.9 |
| SensorLib | 0.3.1 |
| ESP32-audioI2S-master | 3.3.0 |
| TCA9554 | 0.1.2 |
| `es8311` | Waveshare example 포함 offline library |

Arduino 예제 폴더에는 audio, AXP2101, camera, ES8311, RTC, IMU, TF, GFX, JPEG/PNG, LVGL 예제가 있다. Arduino는 Controller의 기능 확인에 사용하고, Communicator ESP32와 ESP-NOW 프로토콜의 정식 빌드는 ESP-IDF 기준으로 통합한다. STM32 firmware는 CMake 기반으로 빌드하며 상세 명령은 [장치별 toolchain](../development/toolchains.md)을 따른다.

### 7.3 개발 단계

1. **보드 단독 확인**: LCD, backlight, touch, I2C 장치, TF, RTC, IMU를 Waveshare 예제로 각각 확인한다.
2. **무선 최소 예제**: Communicator와 Controller 간 `HELLO`·`HEARTBEAT`·ACK·sequence 검증을 먼저 한다.
3. **벤치 CAN**: 차량 대신 CAN simulator 또는 두 번째 노드로 3채널의 bitrate, filter, timestamp, bus-off 처리를 시험한다.
4. **DBC decode**: 저장된 후보 DBC로 raw frame을 decode하고, 미확인 신호는 raw/candidate 상태로 표시한다.
5. **무부하 실차 수신**: 차량 전원 상태별 캡처를 만들고 listen-only로만 확인한다.
6. **표시 기능**: 속도, RPM, 냉각수 온도, 배터리 전압, 기어, 휠 속도, 4WD 상태부터 안정화한다.
7. **제어 기능**: 별도 요구사항과 안전 검토 후, 벤치에서만 최소 명령을 검증한다.

## 8. 화면에 우선 표시할 데이터

1차 화면은 다음 순서로 만든다.

- 연결 상태: Communicator heartbeat, 채널별 CAN 상태, 마지막 frame 시각
- 주행: 차량 속도, 네 바퀴 속도, 엔진 RPM, 변속기 현재 기어
- 엔진: 냉각수 온도, 스로틀, 연료량/엔진 상태, 배터리 전압
- 4WD: `_4WD_TYPE`, `_2H_ACT`, `_4H_ACT`, `LOW_ACT`, `AUTO_ACT`, `LOCK_ACT`, 클러치 상태 후보
- 안전 참고: 브레이크·가속 페달 상태, yaw/lateral/longitudinal acceleration, steering angle
- 진단: DPF lamp, MIL, TPMS raw/candidate, stale/error flag

SCC/LKAS와 같은 운전자 보조 신호는 우선 읽기 전용으로 표시한다. 해당 메시지를 송신해 차량 기능을 흉내 내거나 개입시키는 기능은 초기 범위에 포함하지 않는다.

## 9. 안전 경계

- 이 프로젝트는 계기판 대체품이나 차량 안전장치가 아니다.
- DBC의 이름·단위·scale은 차량의 모든 연식·트림·시장 사양에 대해 보장되지 않는다.
- 2017 Tucson TL 2.0 diesel 4WD BlueLink의 실제 bus 연결 지점·bitrate·신호 존재 여부는 캡처로 확인해야 한다.
- 차량이 움직이는 동안 firmware update, debug logging 변경, raw TX를 하지 않는다.
- CAN 연결 전에 차량 배선도·커넥터 pinout·종단 상태를 확인하고 fuse/current protection을 사용한다.
- 화면이 꺼지거나 ESP-NOW가 끊겨도 차량의 기존 제어 신호를 방해하지 않는 수신 전용 경로를 기본값으로 한다.

## 10. 출처

- [Waveshare 제품 문서](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5)
- [Waveshare Resources and Documents](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/Resources-And-Documents)
- [Waveshare Arduino 안내](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/Arduino)
- [Waveshare ESP-IDF 안내](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/ESP-IDF)
- [Waveshare FAQ — ES8311, speaker, SMD microphone](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/FAQ)
- [Waveshare ESP32-S3-Touch-LCD-3.5 예제 저장소](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/tree/283ec84c566c096f8c30493b93dcd4b0bb608de7)
- [Waveshare 회로도 PDF](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5-Schematic.pdf)
- [Espressif ESP32-S3 TWAI 문서](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/twai.html)
- [Espressif ESP-IDF 6.0.3 ESP-NOW 문서](https://docs.espressif.com/projects/esp-idf/en/v6.0.3/esp32s3/api-reference/network/esp_now.html)
- [commaai/opendbc 고정 commit](https://github.com/commaai/opendbc/tree/3e92d112129507debe45364891954db70238997a)
