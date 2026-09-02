# 하드웨어 및 개발환경

## 1. 프로젝트 범위

`canview`는 차량 네트워크에 직접 연결되는 CAN 게이트웨이와, 데이터를 표시·제어하는 화면 장치를 분리한다.

```text
차량 CAN 1 ─┐
차량 CAN 2 ─┼─> 3채널 CAN 게이트웨이 ── ESP-NOW ──> ESP32-S3-Touch-LCD-3.5
차량 CAN 3 ─┘          │                              │
                      │                              ├─ ST7796 LCD
                      │                              ├─ FT6336 터치
                      │                              └─ UI/상태/제어 요청
                      └<──────── ESP-NOW 응답 ─────────┘
```

여기서 “최대 3개 CAN 버스”는 시스템 요구사항이다. Waveshare 보드 자체에 CAN 트랜시버가 3개 들어 있다는 뜻이 아니며, 보드에는 차량 CAN 커넥터나 CAN 물리계층 회로가 없다. 별도 게이트웨이에 CAN 채널을 3개 구성해야 한다.

기본 동작은 **listen-only/read-only**다. 차량 제어는 실차에서 바로 활성화하지 않고, 벤치 테스트·명령 허용 목록·속도 제한·사용자 확인·통신 단절 시 fail-safe를 모두 통과한 뒤 별도 기능으로 다룬다.

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

- CAN 게이트웨이와 화면 장치는 `ESP-NOW`로 무선 연결하므로, 차량 CAN 선을 화면 보드까지 끌어오지 않는다.
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
| camera | `GPIO17`, `18`, `21`, `38`–`42`, `45`–`48` | 카메라 사용 시 외부 GPIO로 사용 금지 |
| USB | `GPIO19`, `GPIO20` | USB D−/D+ |
| UART | `GPIO43`, `GPIO44` | TX/RX |
| BOOT | `GPIO0` | 부트 스트랩 |

Arduino 예제의 화면 관련 기준값은 `320×480`, rotation `0`, `ST7796`이다. ESP-IDF 예제도 `SPI2_HOST`, MOSI `GPIO1`, SCLK `GPIO5`, DC `GPIO3`, backlight `GPIO6`, I2C SDA `GPIO8`, SCL `GPIO7`을 사용한다. LCD CS와 RST는 예제 설정에서 `GPIO_NC`/`-1`로 표시되고, reset은 `TCA9554`를 통해 처리되므로 회로도와 예제 드라이버의 초기화 순서를 유지한다.

## 5. CAN 게이트웨이 하드웨어 요구사항

### 5.1 채널 수와 물리계층

게이트웨이는 최소 다음을 만족해야 한다.

1. 서로 독립된 `CAN1`, `CAN2`, `CAN3` 수신 경로
2. 채널별 CAN controller와 CAN transceiver
3. 채널별 CANH/CANL, 공통 접지, standby/enable 제어
4. 표준 11-bit와 필요 시 29-bit ID를 보존하는 수집 포맷
5. CAN classic 0–8 byte payload 보존
6. bus-off, error counter, overflow, timestamp를 채널별로 보고
7. 차량 연결부의 역전압·서지·ESD·과전류를 고려한 전원·보호 회로
8. 종단저항은 차량 네트워크의 실제 종단 상태를 확인한 뒤 필요한 위치에만 배치

ESP32-S3의 ESP-IDF TWAI 문서 기준으로 S3에는 TWAI controller가 1개 있고, 내부 CAN transceiver가 없으며, 외부 transceiver가 필요하다. 또한 내장 TWAI는 CAN-FD frame을 지원하지 않는다. 따라서 화면 보드 하나만으로 3채널 차량 CAN을 직접 처리하는 구조는 채택하지 않는다. 3채널은 별도 멀티채널 게이트웨이 또는 외부 CAN controller 조합으로 구현한다.

1차 차량이 classic CAN인지 실제 차량 캡처로 확인하기 전까지 bitrate, connector pin, bus 이름을 고정하지 않는다. 특히 `CAN1/2/3`은 프로젝트 내부 논리 이름이며 차량의 실제 CAN 버스 명칭과 같다고 가정하지 않는다.

### 5.2 수신 우선 설계

- 초기 펌웨어는 모든 채널을 listen-only로 시작한다.
- 수신 프레임에는 `bus_id`, monotonic timestamp, arbitration ID, IDE, RTR, DLC, data, error 상태를 붙인다.
- 화면 표시를 위해 게이트웨이에서 DBC decode한 신호를 별도 telemetry로 만들되, 원시 프레임도 진단 모드에서 전달한다.
- 전송(TX)은 기본 비활성화한다. 활성화하더라도 메시지·신호·주기·조건을 allow-list로 제한하고, 화면에서 만든 임의 raw frame은 전송하지 않는다.

## 6. ESP-NOW 링크 설계

### 6.1 역할

| 노드 | 책임 |
|---|---|
| CAN gateway | 최대 3개 CAN 수집, timestamp/상태 부여, DBC 후보 decode, TX 안전 게이트 |
| 화면 보드 | ESP-NOW peer, telemetry 수신, LVGL 렌더링, 터치 이벤트, 사용자 확인 |

ESP-NOW는 연결 없는 Wi-Fi 기반 통신이므로 “MAC 계층에서 전송 성공”과 “애플리케이션이 명령을 처리함”을 같은 뜻으로 보지 않는다. 응답 ACK, sequence number, timeout, 중복 제거를 애플리케이션 계층에 둔다.

### 6.2 권장 패킷

초기 호환성을 위해 한 번의 payload를 250 byte 이하로 설계한다. 이 크기는 ESP-NOW v1 호환 상한을 넘지 않으며, CAN raw frame 여러 개를 묶어도 여유를 관리하기 쉽다.

공통 헤더 예시는 다음과 같다.

| 필드 | 내용 |
|---|---|
| `version` | 프로토콜 버전 |
| `type` | `HELLO`, `HEARTBEAT`, `CAN_BATCH`, `TELEMETRY`, `CONTROL_REQ`, `CONTROL_ACK`, `ERROR` |
| `source` / `destination` | gateway/display 논리 주소 |
| `sequence` | 단조 증가 번호. 중복·순서 검출에 사용 |
| `timestamp_us` | gateway monotonic timestamp |
| `bus_id` | `1`, `2`, `3`; `0`은 여러 버스 묶음 |
| `flags` | listen-only, stale, overflow, error, ack-required 등 |
| `count` | 뒤따르는 항목 수 |
| `items` | CAN frame 또는 decoded signal 항목 |

CAN 항목은 `frame_id`, `is_extended`, `is_rtr`, `dlc`, `data[8]`, `bus_timestamp`를 보존한다. 화면에는 신호의 최신값과 마지막 수신 시각을 함께 저장해 stale 표시를 가능하게 한다.

### 6.3 무선 운용 규칙

- gateway와 display는 고정 Wi-Fi channel을 사용한다. 두 peer의 channel이 다르면 전송이 실패할 수 있다.
- production 빌드에서는 peer MAC을 allow-list에 두고, ESP-NOW encryption/LMK를 사용한다.
- `CONTROL_REQ`는 반드시 `request_id`, 사용자 확인 상태, 허용 명령 ID, 만료 시각을 포함한다.
- gateway는 화면이 끊기거나 heartbeat가 만료되면 진행 중인 제어 명령을 취소하고 TX 금지 상태로 돌아간다.
- CAN raw frame을 무제한 전송하지 않고, display refresh rate에 맞춰 batch·필터링한다. 진단 캡처는 별도 high-rate 모드로 분리한다.

## 7. 펌웨어 개발환경

### 7.1 권장 ESP-IDF 환경

Waveshare 공식 ESP-IDF 문서는 이 보드에 `ESP-IDF V5.5.0` 이상을 요구하고, 예제 화면은 `V5.5.2`에서 작성돼 있다. 새 프로젝트는 ESP-IDF 5.5.x 이상을 기준으로 고정한다.

권장 도구는 다음과 같다.

- Visual Studio Code
- Espressif `ESP-IDF` VS Code extension 2.0 이상
- ESP-IDF 5.5.x 이상
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

Arduino 예제 폴더에는 audio, AXP2101, camera, ES8311, RTC, IMU, TF, GFX, JPEG/PNG, LVGL 예제가 있다. Arduino는 화면 장치의 기능 확인에 사용하고, CAN gateway와 ESP-NOW 프로토콜의 정식 빌드는 ESP-IDF 기준으로 통합하는 방향을 권장한다.

### 7.3 개발 단계

1. **보드 단독 확인**: LCD, backlight, touch, I2C 장치, TF, RTC, IMU를 Waveshare 예제로 각각 확인한다.
2. **무선 최소 예제**: gateway와 display 간 `HELLO`·`HEARTBEAT`·ACK·sequence 검증을 먼저 한다.
3. **벤치 CAN**: 차량 대신 CAN simulator 또는 두 번째 노드로 3채널의 bitrate, filter, timestamp, bus-off 처리를 시험한다.
4. **DBC decode**: 저장된 후보 DBC로 raw frame을 decode하고, 미확인 신호는 raw/candidate 상태로 표시한다.
5. **무부하 실차 수신**: 차량 전원 상태별 캡처를 만들고 listen-only로만 확인한다.
6. **표시 기능**: 속도, RPM, 냉각수 온도, 배터리 전압, 기어, 휠 속도, 4WD 상태부터 안정화한다.
7. **제어 기능**: 별도 요구사항과 안전 검토 후, 벤치에서만 최소 명령을 검증한다.

## 8. 화면에 우선 표시할 데이터

1차 화면은 다음 순서로 만든다.

- 연결 상태: gateway heartbeat, 채널별 CAN 상태, 마지막 frame 시각
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
- [Waveshare ESP32-S3-Touch-LCD-3.5 예제 저장소](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/tree/283ec84c566c096f8c30493b93dcd4b0bb608de7)
- [Waveshare 회로도 PDF](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5/ESP32-S3-Touch-LCD-3.5-Schematic.pdf)
- [Espressif ESP32-S3 TWAI 문서](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/twai.html)
- [Espressif ESP32-S3 ESP-NOW 문서](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_now.html)
- [commaai/opendbc 고정 commit](https://github.com/commaai/opendbc/tree/3e92d112129507debe45364891954db70238997a)
