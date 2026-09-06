# Controller·Communicator·Diagnostic Bridge 개발환경

이 문서는 [Windows 개발환경](windows.md) 아래에서 장치별 SDK, compiler, build 산출물을 구체화한다. branch·worktree·review 절차는 [agent workflow](../runbooks/agent-workflow.md)를 따른다.

아래 PowerShell 명령은 모두 저장소 정본 경로 `F:/dev/canview`에서 실행한다.

## 1. 공통 원칙

Controller, Communicator ESP32, 선택 장치 Diagnostic Bridge와 STM32는 한 저장소에서 관리하지만 firmware image와 target 설정은 분리한다.

| 대상 | 기준 framework/toolchain | 산출물 |
|---|---|---|
| Controller | ESP-IDF `v6.0.3`, LVGL `8.4.x`, Waveshare BSP/example | Controller Flash image |
| Communicator ESP32 | ESP-IDF `v6.0.3` | ESP-NOW·UART image |
| Diagnostic Bridge | ESP-IDF `v6.0.3`, built-in HTTP server | ESP-NOW observer·SoftAP·모바일 웹 image |
| Communicator STM32 | CMake `4.4.3` + Ninja `1.13.2` + Arm GNU `15.3.Rel1`, STM32CubeG4 `v1.6.3` | CAN·safety image |
| KiCad hardware review | KiCad `10.0.6` `kicad-cli` + bundled Python | schematic·XML netlist·ERC JSON·PDF |
| DBC/profile 생성·검증 | Python virtual environment, `cantools`와 schema validator | Controller signal catalog, STM32 safety profile와 검증 report |

Controller, Communicator ESP32와 Diagnostic Bridge는 같은 ESP-IDF baseline을 사용해 ESP-NOW API와 보안 설정 차이를 줄인다. ESP-IDF v6의 CMake 최소 요구사항은 3.22.1이며, 이 저장소의 Windows 정본은 CMake `4.4.3`으로 올려 고정한다. 버전 업그레이드는 한 장치만 독립적으로 올리지 않고 wire compatibility, RF regression, flash/PSRAM 사용량을 함께 확인한다.

정본 버전과 SDK commit은 [`tools/toolchain-versions.json`](../../tools/toolchain-versions.json)에 둔다. Windows 초기화와 commit 검증은 [`tools/environment/setup-windows.ps1`](../../tools/environment/setup-windows.ps1)를 사용한다.

하드웨어 생성물은 [`tools/hardware/export-review.ps1`](../../tools/hardware/export-review.ps1)가 KiCad `10.0.6`의 bundled Python과 `kicad-cli`를 사용해 재현한다. 이 별도 단계는 임베디드 SDK 준비와 분리되어 있어 KiCad가 없는 환경에서도 ESP-IDF·STM32 빌드 준비를 검증할 수 있다.

`sdkconfig`, compiler version, STM32CubeG4 tag, DBC upstream commit을 build metadata에 넣는다. release artifact는 다음 정보를 함께 보관한다.

- Git commit과 dirty 여부
- toolchain 및 framework 버전
- protocol major/minor와 capability digest
- DBC/catalog digest
- build timestamp와 target hardware revision

## 2. Controller 개발환경

### 2.1 하드웨어와 framework

Controller는 Waveshare `ESP32-S3-Touch-LCD-3.5`다. 보드에는 ESP32-S3R8, 16 MB external Flash, 8 MB PSRAM, ST7796 LCD, FT6336 touch, AXP2101 PMIC, QMI8658 IMU, PCF85063 RTC, ES8311 audio codec가 있다. 세부 핀은 [하드웨어 및 개발환경](../hardware/controller.md)을 따른다.

Waveshare 공식 문서는 ESP-IDF 5.5.0 이상을 요구한다. 최신 안정 baseline은 `v6.0.3`으로 선택했으며, 공식 example/BSP의 IDF 6 호환 여부는 실제 board bring-up에서 별도 확인한다. LVGL UI는 이 저장소의 [`../ui/lvgl/`](../../ui/lvgl/)를 보드 BSP의 display/touch driver 위에 연결한다.

### 2.2 설치와 빌드

```powershell
. .\tools\environment\foundation-windows.ps1
. .\tools\environment\setup-windows.ps1
```

Controller project가 추가된 뒤의 표준 명령은 다음과 같다.

```powershell
idf.py -C firmware/controller set-target esp32s3
idf.py -C firmware/controller menuconfig
idf.py -C firmware/controller build
idf.py -C firmware/controller -p COMx flash monitor
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

Communicator의 무선 MCU는 `ESP32-S3-WROOM-1-N16R8`다. build target은 `esp32s3`, Flash16MiB Quad SPI, PSRAM8MiB Octal SPI/80MHz/ECC로 설정한다. ECC 가용 용량은7.5MiB다. Controller와 Flash 크기가 같아도 GPIO·OTA 배치·역할이 다르므로 별도 `sdkconfig.defaults`를 유지한다. [OTA 설계](../architecture/ota.md)를 따른다.

```powershell
idf.py -C firmware/communicator/esp32 set-target esp32s3
idf.py -C firmware/communicator/esp32 menuconfig
idf.py -C firmware/communicator/esp32 build
idf.py -C firmware/communicator/esp32 -p COMx flash monitor
```

초기 configuration 기준은 다음과 같다.

- Flash size:16MiB
- PSRAM:8MiB Octal SPI,80MHz/ECC; GPIO35/36/37 외부 사용 금지
- UART1: 4,000,000 baud, 8-N-1, RTS/CTS
- UART pins: TX `GPIO17`, RX `GPIO18`, RTS `GPIO15`, CTS `GPIO16`
- USB Serial/JTAG: `GPIO19/20` 유지
- ESP-NOW: encrypted unicast, 고정 운행 channel, callback에서는 queue copy만 수행
- watchdog: Wi-Fi task와 UART worker를 분리 감시

ESP32 Communicator firmware는 DBC 표시 catalog를 필요로 하지 않는다. raw CAN batch를 전달하는 bridge만 유지하고, DBC에서 생성된 signal catalog와 decoder는 Controller에 둔다. 차량 송신 판단용 안전 signal과 command frame 생성은 STM32에 독립적으로 남긴다.

## 4. Diagnostic Bridge 개발환경

### 4.1 target과 module

첫 prototype은 `ESP32-S3-WROOM-1-N8R2` 개발보드를 권장한다. 8 MB Flash는 OTA A/B와 gzip web asset을 담고, 2 MB PSRAM은 제한된 pre-trigger ring과 HTTP buffer에 사용한다. 차량 상시 설치에서 온도 여유가 중요하므로 기본 권장 주변온도가 65 °C인 `N8R8`보다 -40~85 °C의 `N8R2`를 우선한다.

Diagnostic Bridge firmware directory는 foundation bootstrap project로 생성되어 있다. 실제 SoftAP·capture·web 기능은 T-400에서 추가하며 현재는 Controller·Communicator와 같은 `setup-windows.ps1` 및 `idf.py -C firmware/diagnostic-bridge ...` 순서로 image compile만 검증한다.

필수 configuration 기준은 다음과 같다.

- ESP-IDF `v6.0.3`, target `esp32s3`
- Flash 8 MB Quad SPI, PSRAM 2 MB Quad SPI
- `WIFI_MODE_APSTA`: STA는 ESP-NOW 전용, SoftAP는 휴대폰 한 대
- external STA credential과 NAPT 기능 제외
- `CONFIG_HTTPD_WS_SUPPORT`, WebSocket pre-handshake 인증 지원
- encrypted NVS, production Secure Boot·Flash Encryption 검토
- web asset은 external CDN 없이 gzip으로 Flash에 포함
- watchdog은 ESP-NOW protocol, capture, HTTP task를 각각 감시

표준 경로와 component 분리는 [Diagnostic Bridge·모바일 CAN 검증 UI](../architecture/diagnostic-bridge.md)의 `Bridge firmware 구조`를 따른다. 정적 UI prototype은 [`../ui/diagnostic-web/`](../../ui/diagnostic-web/)에 있다.

### 4.2 Web asset 검증

정적 UI는 Node runtime을 target에 넣지 않는다. 개발 시 HTML/CSS/JS lint와 screenshot을 수행한 뒤 gzip asset으로 내장한다. REST·WebSocket schema version과 web build hash를 화면 `시스템 정보`에 표시한다.

필수 browser matrix는 Android Chrome과 iOS Safari다. captive portal mini-browser가 file download를 제한하면 일반 browser로 여는 안내를 제공한다. 인터넷이 끊긴 SoftAP에서도 font, icon, chart와 모든 설정 화면이 동작해야 한다.

## 5. Communicator STM32 CMake 환경

### 5.1 도구

권장 구성은 다음과 같다.

- CMake `4.4.3`
- Ninja `1.13.2`
- Arm GNU Toolchain `15.3.Rel1` (`arm-none-eabi-gcc` 15.3.x)
- `arm-none-eabi-gcc`, `arm-none-eabi-g++`, `arm-none-eabi-gdb`, `arm-none-eabi-objcopy`, `arm-none-eabi-size`
- STM32CubeG4 `v1.6.3`
- STM32CubeProgrammer와 ST-LINK GDB server
- ST-LINK/V3 또는 동등한 SWD probe
- STM32CubeMX는 pin/clock 검산과 초기화 코드 생성에만 선택적으로 사용

STM32CubeCLT는 GNU Arm toolchain, GDB, STM32CubeProgrammer를 한 번에 제공하며 Linux, Windows, macOS를 지원한다. 이 저장소의 setup script는 PATH에 있는 GCC가 `15.3.x`인지 export 전후 모두 확인하고 CMake preset에 실제 실행 파일 경로를 전달한다.

### 5.2 repository CMake scaffold

[`../firmware/communicator/stm32/`](../../firmware/communicator/stm32/)의 CMake project는 STM32CubeG4를 repository 밖 dependency로 참조한다. vendor package를 이 저장소에 무분별하게 복사하지 않는다.

```powershell
. .\tools\environment\foundation-windows.ps1
. .\tools\environment\setup-windows.ps1
Push-Location firmware/communicator/stm32
cmake --preset debug
cmake --build --preset debug
arm-none-eabi-size build/debug/canview-communicator-stm32.elf
Pop-Location
```

Flash 예시는 다음과 같다. 실제 probe serial과 reset 방식은 개발 PC 설정에 맞춘다.

```powershell
STM32_Programmer_CLI -c port=SWD `
  -w firmware/communicator/stm32/build/debug/canview-communicator-stm32.bin 0x08000000 `
  -v -rst
```

### 5.3 CMake source-of-truth 규칙

- `.ioc`를 사용할 수 있지만 build option, include path, linker script 선택은 CMake가 정본이다.
- CubeMX 재생성 영역과 수동 작성 영역을 디렉터리로 분리한다.
- generated source를 갱신하면 CMake source list와 pinmap diff를 함께 review한다.
- `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`를 compile/link 양쪽에 동일 적용한다.
- linker map과 size report를 CI artifact로 보관한다.
- warning은 최소 `-Wall -Wextra -Wshadow -Wdouble-promotion -Wformat=2`를 사용하고 project source는 warning-free를 요구한다.
- release는 LTO 적용 전 timing, interrupt latency, stack watermark를 다시 측정한다.

### 5.4 clock과 peripheral 기준

PF0/OSC_IN과 PF1/OSC_OUT에 HSE16MHz crystal을 연결한다. R1은 system clock160MHz, APB/USART2와 FDCAN80MHz로4Mbps를 정수 분주하는 [clock 출발값](../hardware/r1/firmware-pinmap.md)을 사용한다. CubeMX/target에서 PLL·전압 scale·flash wait와 실제 clock을 검증하기 전 설정 완료로 표시하지 않는다. CAN FD5/8Mbps의 oscillator tolerance와 sample point, crystal load/drive를 검산한다. HSE startup 또는 clock 검증 실패 시 HSI로 차량 CAN 송신을 계속하지 않고 모든 PHY를 비활성 상태로 둔 채 watchdog reset한다.

USART2는 PA0 CTS, PA1 RTS, PA2 TX, PA3 RX의 AF7을 사용한다. FDCAN은 다음과 같다.

| controller | RX | TX | transceiver |
|---|---|---|---|
| FDCAN1 | PA11 AF9 | PA12 AF9 | TCAN1046AV channel 1 |
| FDCAN2 | PB12 AF9 | PB13 AF9 | TCAN1046AV channel 2 |
| FDCAN3 | PA8 AF11 | PA15 AF11 | MAX3055, 125 kbps only |

PB3/PB4와 PB5/PB6의 이전 배치는 사용하지 않는다. reset debug pull과 UCPD dead-battery pull-down 경로가 CAN TX의 하드웨어 기본 상태를 복잡하게 만들기 때문이다. PA12, PB13, PA15의 TXD에는 각각 외부 10 kΩ pull-up을 둔다.

### 5.5 reset-safe 초기화와 watchdog

현재 회로는 차량 PHY rail과 차량/USB mux 뒤 system rail을 분리한다. ESP/STM reset supervisor도 독립이다. [R1 핀맵의 부팅·복구 계약](../hardware/r1/firmware-pinmap.md#부팅복구-구현-순서)과 [OTA 서비스 인터록](../architecture/ota.md)이 정본이며 과거 공용 NRST/CHIP_PU 연결을 BSP에 다시 넣지 않는다. GPIO 초기화 이전에도 외부 회로가 TCAN standby, MAX3055 standby, TX gate off, UART flow stop을 유지해야 한다.

STM32 firmware 초기화 순서는 다음과 같이 고정한다.

1. PA4·PA5 latch high와 PA6·PA7 latch low를 output mode보다 먼저 기록한다. ESP_RUN_OK도 low로 시작한다.
2. HSE·PLL·FDCAN kernel clock을 검증한다.
3. UART와 FDCAN message RAM, filter, interrupt, bitrate를 모두 구성한다.
4. controller를 start하고 listen-only profile을 검증한다.
5. RX_ALLOWED·actual rail/gate sense와 filter를 확인한 뒤 필요한 PHY만 수신한다. TX gate는 계속 닫고 별도 권한·lease·build mode·physical arm 검사 뒤에만 ARM edge를 허용한다. USB-only/서비스 모드에서는 RX/TX를 모두 차단한다.

IWDG 목표 timeout은 250–500 ms다. main loop만으로 refresh하지 않고 CAN 처리, UART worker, safety state가 모두 정상일 때만 refresh한다. 외부 WDI falling pulse와 재무장 latch는 별도 계약이며 [핀맵](../hardware/r1/firmware-pinmap.md)을 따른다. reset/fault 뒤 자동 재무장하지 않는다.

## 6. DBC toolchain

```powershell
py -3.12 -m venv .venv
. .\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install cantools
python -m cantools list dbc/opendbc/hyundai_can.dbc
deactivate
```

DBC 원본은 수정하지 않고, generator가 Controller용 catalog와 STM32용 최소 safety profile을 분리 생성한다. 사람 편집 정본은 vehicle profile과 evidence manifest이며, signal 이름과 bit layout을 Communicator ESP32에 생성하지 않는다.

1. Controller용 signal descriptor table: `signal_id`, bus/ID, bit field, endian, signedness, factor/offset, range, unit, runtime quality와 evidence grade 포함
2. STM32용 safety/command table: 검증된 전제조건 signal, fixed command builder, counter/checksum, feedback만 포함
3. 차량 profile 검증 report: source DBC commit, 실차 등급, freshness, capture evidence, 후보/미지원 사유

두 산출물에는 DBC 파일 SHA-256, opendbc commit, generator version을 넣는다. 실차에서 검증하지 않은 신호는 runtime quality와 별개인 `CANDIDATE` evidence grade를 유지한다. 새로운 signal이 기존 CAN ID를 사용하면 Communicator firmware를 바꾸지 않고 Controller catalog만 갱신한다. 새로운 ID를 사용하면 Controller allow-list entry와 upstream subscription을 함께 추가한다.

## 7. foundation target build evidence

2026-09-06에 Arm GNU `15.3.Rel1`, ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`을 manifest commit/SHA로 준비했다. STM32 debug/release와 Communicator ESP32, Diagnostic Bridge, Controller의 ESP32-S3 foundation binary 및 public component fixture를 생성했다. 네 target log에서 컴파일·링커 warning/error 진단은 0개였다. 이 결과는 compile gate만 닫으며 실제 flash, reset/brownout, clock, PSRAM, UART/CAN, RF와 HIL은 닫지 않는다.

## 8. CI 권고 gate

- Controller/Communicator ESP32: `idf.py build`, partition size, `sdkconfig` drift
- STM32: CMake configure/build, warnings, ELF size, linker overflow
- protocol: C header static assertions, encode/decode golden vectors, malformed length/CRC fuzz
- DBC: source hash, generated output reproducibility, duplicate signal ID
- UI: host prototype screenshot diff와 LVGL host compile
- Diagnostic Bridge: ESP-IDF build, web asset offline check, REST schema, WebSocket reconnect, 390×844 screenshot diff
- hardware docs: pinmap CSV 중복 pin/net 검사

## 9. 공식 출처

- [Waveshare ESP-IDF 안내](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/ESP-IDF)
- [Waveshare example repository](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/)
- [Espressif ESP-IDF 시작하기](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/index.html)
- [Espressif ESP-NOW API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_now.html)
- [ESP-IDF 6.0.3 HTTP server](https://docs.espressif.com/projects/esp-idf/en/v6.0.3/esp32s3/api-reference/protocols/esp_http_server.html)
- [ESP-IDF 6.0.3 Wi-Fi APSTA](https://docs.espressif.com/projects/esp-idf/en/v6.0.3/esp32s3/api-guides/wifi.html)
- [ESP32-S3-WROOM-1/1U datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [Espressif UART API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/uart.html)
- [ST STM32CubeCLT](https://www.st.com/en/development-tools/stm32cubeclt.html)
- [ST STM32CubeG4](https://github.com/STMicroelectronics/STM32CubeG4)
- [ST CMake application note AN5952](https://www.st.com/resource/en/application_note/an5952-how-to-use-cmake-in-stm32cubeide-stmicroelectronics.pdf)
