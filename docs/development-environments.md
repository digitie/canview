# Controller·Communicator 개발환경

## 1. 공통 원칙

두 ESP32와 STM32는 한 저장소에서 관리하지만 firmware image와 toolchain은 분리한다.

| 대상 | 기준 framework/toolchain | 산출물 |
|---|---|---|
| Controller | ESP-IDF `v5.5.2`, LVGL `8.4.x`, Waveshare BSP/example | Controller Flash image |
| Communicator ESP32 | ESP-IDF `v5.5.2` | ESP-NOW·UART image |
| Communicator STM32 | CMake + Ninja + GNU Arm Embedded, STM32CubeG4 | CAN·safety image |
| DBC 생성·검증 | Python virtual environment, `cantools` | 생성된 signal catalog와 검증 report |

Controller와 Communicator ESP32는 같은 ESP-IDF baseline을 사용해 ESP-NOW API와 보안 설정 차이를 줄인다. 버전 업그레이드는 한 장치만 독립적으로 올리지 않고 wire compatibility, RF regression, flash/PSRAM 사용량을 함께 확인한다.

`sdkconfig`, compiler version, STM32CubeG4 tag, DBC upstream commit을 build metadata에 넣는다. release artifact는 다음 정보를 함께 보관한다.

- Git commit과 dirty 여부
- toolchain 및 framework 버전
- protocol major/minor와 capability digest
- DBC/catalog digest
- build timestamp와 target hardware revision

## 2. Controller 개발환경

### 2.1 하드웨어와 framework

Controller는 Waveshare `ESP32-S3-Touch-LCD-3.5`다. 보드에는 ESP32-S3R8, 16 MB external Flash, 8 MB PSRAM, ST7796 LCD, FT6336 touch, AXP2101 PMIC, QMI8658 IMU, PCF85063 RTC, ES8311 audio codec가 있다. 세부 핀은 [하드웨어 및 개발환경](hardware-and-development.md)을 따른다.

Waveshare 공식 문서는 ESP-IDF 5.5.0 이상을 요구하고 예제는 5.5.2 기준이므로 baseline을 `v5.5.2`로 고정한다. LVGL UI는 이 저장소의 [`../ui/lvgl/`](../ui/lvgl/)를 보드 BSP의 display/touch driver 위에 연결한다.

### 2.2 설치와 빌드

```bash
git clone --recursive --branch v5.5.2 https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3
. ./export.sh
```

Controller project가 추가된 뒤의 표준 명령은 다음과 같다.

```bash
cd firmware/controller
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

필수 설정은 실제 보드용 16 MB Flash, octal PSRAM, 240 MHz CPU, LVGL 8.4, ESP-NOW station mode다. Waveshare example의 `sdkconfig`를 출발점으로 삼되 새 IDF에서 생성한 설정 diff를 review한다. 생산 image에는 secure boot, flash encryption, NVS encryption을 별도 provisioning 절차와 함께 적용한다.

### 2.3 Controller 검증 순서

1. PMIC와 backlight power-on/off
2. ST7796 full-screen color와 tearing/flush
3. FT6336 좌표, rotation, edge touch
4. shared I2C의 touch·IMU·RTC 충돌
5. ESP-NOW pairing, reconnect, channel mismatch
6. 20 Hz UI update 중 frame time, heap, PSRAM, RF loss
7. ignition cycle 및 brownout 뒤 command lock 상태

## 3. Communicator ESP32 개발환경

### 3.1 target 설정

Communicator의 무선 MCU는 `ESP32-S3-MINI-1-N4R2`다. build target은 `esp32s3`, Flash는 4 MB Quad SPI, PSRAM은 2 MB Quad SPI로 설정한다. Controller의 16 MB/8 MB 설정을 복사하면 partition과 PSRAM mode가 틀리므로 별도 `sdkconfig.defaults`를 유지한다.

```bash
cd firmware/communicator/esp32
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyACM1 flash monitor
```

초기 configuration 기준은 다음과 같다.

- Flash size: 4 MB
- PSRAM: 2 MB Quad SPI, `GPIO26` 외부 사용 금지
- UART1: 4,000,000 baud, 8-N-1, RTS/CTS
- UART pins: TX `GPIO17`, RX `GPIO18`, RTS `GPIO15`, CTS `GPIO16`
- USB Serial/JTAG: `GPIO19/20` 유지
- ESP-NOW: encrypted unicast, 고정 운행 channel, callback에서는 queue copy만 수행
- watchdog: Wi-Fi task와 UART worker를 분리 감시

ESP32 firmware는 DBC에서 생성된 표시용 signal catalog를 가질 수 있지만, 차량 송신 판단용 안전 signal과 command frame 생성은 STM32에 독립적으로 남긴다.

## 4. Communicator STM32 CMake 환경

### 4.1 도구

권장 구성은 다음과 같다.

- CMake 3.22 이상
- Ninja
- `arm-none-eabi-gcc`, `arm-none-eabi-g++`, `arm-none-eabi-gdb`, `arm-none-eabi-objcopy`, `arm-none-eabi-size`
- STM32CubeG4 `v1.6.3`
- STM32CubeProgrammer와 ST-LINK GDB server
- ST-LINK/V3 또는 동등한 SWD probe
- STM32CubeMX는 pin/clock 검산과 초기화 코드 생성에만 선택적으로 사용

STM32CubeCLT는 GNU Arm toolchain, GDB, STM32CubeProgrammer를 한 번에 제공하며 Linux, Windows, macOS를 지원한다. 로컬 package manager의 GNU Arm toolchain을 사용해도 되지만 CI와 개발 PC의 compiler major를 맞춘다.

### 4.2 repository CMake scaffold

[`../firmware/communicator/stm32/`](../firmware/communicator/stm32/)의 CMake project는 STM32CubeG4를 repository 밖 dependency로 참조한다. vendor package를 이 저장소에 무분별하게 복사하지 않는다.

```bash
git clone --recursive --depth 1 --branch v1.6.3 \
  https://github.com/STMicroelectronics/STM32CubeG4.git /opt/STM32CubeG4

export STM32CUBE_G4_ROOT=/opt/STM32CubeG4
cd firmware/communicator/stm32
cmake --preset debug
cmake --build --preset debug
arm-none-eabi-size build/debug/canview-communicator-stm32.elf
```

Flash 예시는 다음과 같다. 실제 probe serial과 reset 방식은 개발 PC 설정에 맞춘다.

```bash
STM32_Programmer_CLI -c port=SWD \
  -w build/debug/canview-communicator-stm32.bin 0x08000000 \
  -v -rst
```

### 4.3 CMake source-of-truth 규칙

- `.ioc`를 사용할 수 있지만 build option, include path, linker script 선택은 CMake가 정본이다.
- CubeMX 재생성 영역과 수동 작성 영역을 디렉터리로 분리한다.
- generated source를 갱신하면 CMake source list와 pinmap diff를 함께 review한다.
- `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`를 compile/link 양쪽에 동일 적용한다.
- linker map과 size report를 CI artifact로 보관한다.
- warning은 최소 `-Wall -Wextra -Wshadow -Wdouble-promotion -Wformat=2`를 사용하고 project source는 warning-free를 요구한다.
- release는 LTO 적용 전 timing, interrupt latency, stack watermark를 다시 측정한다.

### 4.4 clock과 peripheral 기준

STM32 system clock 170 MHz, FDCAN kernel clock, USART2 4 Mbps를 정확히 만들 수 있는 clock tree를 CubeMX와 reference manual에서 검산한다. 최종 crystal 값이 정해질 때까지 CMake scaffold는 clock initialization을 구현 완료로 간주하지 않는다.

USART2는 PA0 CTS, PA1 RTS, PA2 TX, PA3 RX의 AF7을 사용한다. FDCAN은 다음과 같다.

| controller | RX | TX | transceiver |
|---|---|---|---|
| FDCAN1 | PA11 AF9 | PA12 AF9 | TCAN1046AV channel 1 |
| FDCAN2 | PB5 AF9 | PB6 AF9 | TCAN1046AV channel 2 |
| FDCAN3 | PB3 AF11 | PB4 AF11 | MAX3055, 125 kbps only |

## 5. DBC toolchain

```bash
python3 -m venv .venv
. .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install cantools
python -m cantools list dbc/opendbc/hyundai_can.dbc
```

DBC 원본은 수정하지 않고, generator가 다음 두 산출물을 만든다.

1. STM32용 최소 안전 signal/message table: command precondition과 feedback에 필요한 신호만 포함
2. ESP32용 표시 catalog: UI telemetry와 단위·quality·name 포함

두 산출물에는 DBC 파일 SHA-256, opendbc commit, generator version을 넣는다. 실차에서 검증하지 않은 신호는 이름이 존재해도 `UNVERIFIED` quality를 유지한다.

## 6. CI 권고 gate

- Controller/Communicator ESP32: `idf.py build`, partition size, `sdkconfig` drift
- STM32: CMake configure/build, warnings, ELF size, linker overflow
- protocol: C header static assertions, encode/decode golden vectors, malformed length/CRC fuzz
- DBC: source hash, generated output reproducibility, duplicate signal ID
- UI: host prototype screenshot diff와 LVGL host compile
- hardware docs: pinmap CSV 중복 pin/net 검사

## 7. 공식 출처

- [Waveshare ESP-IDF 안내](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/ESP-IDF)
- [Waveshare example repository](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/)
- [Espressif ESP-IDF 시작하기](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/index.html)
- [Espressif ESP-NOW API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_now.html)
- [Espressif UART API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/uart.html)
- [ST STM32CubeCLT](https://www.st.com/en/development-tools/stm32cubeclt.html)
- [ST STM32CubeG4](https://github.com/STMicroelectronics/STM32CubeG4)
- [ST CMake application note AN5952](https://www.st.com/resource/en/application_note/an5952-how-to-use-cmake-in-stm32cubeide-stmicroelectronics.pdf)
