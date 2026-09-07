# 펌웨어 기반 구조와 후속 구현 계약

## 범위와 현 상태

2026-09-06 사용자 요청에 따른 기반 구현이다. 세 장치·네 MCU target의 프로젝트 구조,
MCU 독립 framing/CRC/COBS/classic CAN batch/sequence window, 안전 idle 부팅, host 검증을 제공한다.
언어와 문서 도구 결정은 [ADR-008](../adr/008-portable-foundation-and-api-docs.md)을 따른다.

2026-09-07 T-200a에서 Communicator ESP32는 별도 [bench core](../../firmware/communicator/esp32/docs/core-bench.md)를 사용한다. `communicator/esp32/app`이 boot/health·고정 pool을 조합하고, 같은 project의 `interface`/`module`은 SDK 독립 C99, `platform/esp32s3/runtime.c`와 project-local `canview_communicator_runtime` component만 IDF/TWDT 경계를 소유한다. 아래 공용 startup 설명은 Controller/Bridge의 아직 교체하지 않은 safe-idle 경로에 해당한다.

전체 ESP-NOW/UART message ABI 동결, 인증·pairing·ACK/command lifecycle, filter·QoS, FDCAN/DMA,
LVGL·센서·차량 자동화, OTA loader·복구·서명 검증은 **아직 구현하지 않았다**.
기존 architecture의 안전 요구를 축소하거나 제품 gate를 통과 처리하는 기반이 아니다.
Controller/Bridge에 raw CAN TX 경로를 추가하지 않았으며, 차량 송신은 계속 NO-GO다.

## 디렉터리와 의존 방향

| 계층 | 실제 코드 | 책임·허용 의존 |
|---|---|---|
| app | firmware/app/startup.c, shared/app, communicator/stm32/app | ESP 역할 고정·안전 idle, STM32 boot와 cooperative worker 조합; interface만 의존 |
| 공용 module | shared/protocol | 전송 바이트 검증/인코딩; 표준 C99와 shared/interface만 의존 |
| interface | shared/interface, firmware/interface | 상태·함수 계약만 선언. SDK 타입/전역 singleton 없음 |
| BSP | firmware/<target>/bsp | 생성 pin 이름으로 보드별 초기화 순서 조합 |
| platform | firmware/platform/esp32s3, communicator/stm32/platform/stm32g474 | IDF GPIO/FreeRTOS 대기 또는 CMSIS register 접근 |
| SDK 경계 | firmware/components/canview_foundation, 각 target CMake | 고정 외부 SDK와 공용 library 연결 |
| schema | protocol/schema/transport-foundation-v1.json, firmware/boards | generation 입력. 생성 header를 손으로 수정하지 않음 |

구체 드라이버·미들웨어·RTOS task를 구현하지 않았으므로 비어 있는 미래 디렉터리나 가짜 HAL을 만들지 않는다.
후속 드라이버는 bus port와 device context로 분리하고 app/codec에 HAL header를 가져오지 않는다.
Controller/Bridge IDF app_main은 공용 startup, Communicator ESP app_main과 STM main은 각 core bench composition root를 사용한다. 역할은 빌드 상수이고 무선 입력으로 변경할 수 없다.

## 프로토콜 구현 계약

정본은 [ESP-NOW/UART 설계](protocols/README.md)이며 새 schema는 그중 envelope 부분의 구현 입력이다.
기존 protocol/canview_protocol.h(v1.2 prototype)는 변경하지 않고 새 firmware target에서 제외한다.
navigation-v1 schema도 별도다. 새 header를 전체 v1.3 ABI 동결 결과라고 표시하지 않는다.

| 항목 | 동작 |
|---|---|
| ESP-NOW envelope | version 1.3 정확 일치, 32바이트 header, 전체 최대 240바이트 |
| UART envelope | version 1.0 정확 일치, 32바이트 header, 전체 최대 1024바이트 |
| 직렬화 | 명시적인 little-endian byte access. packed struct cast/memcpy ABI 없음 |
| CRC | ISO-HDLC, header CRC 필드 4바이트를 0으로 계산; 인증 대체 불가 |
| UART framing | bounded COBS + 0 delimiter, 최대 1030바이트. 초과 packet은 1회 OVERSIZE 후 delimiter까지 폐기 |
| classic CAN batch | 12바이트 prefix + 최대 12개 16바이트 record. 3bus, DLC 0..8, ID/flags/padding/시간 overflow 검사 |
| sequence | session별 64개 window, modulo32 wrap, duplicate/stale 거부. half-range는 모호하므로 거부 |
| 오류 출력 | encode written=0; 출력 일부 변경 가능. decode view/batch는 0 초기화 |
| 소유권 | caller 버퍼/context, heap 없음, 단일 worker. view는 다음 입력 변경/feed/reset 전까지 유효 |

공용 header의 parameter·반환·수명 설명은 [생성 API 입력](../api/index.rst)에 모았다.
일반 decode는 message_type을 opaque byte로 보존한다. **알 수 없는 message도 구조 검증에 성공할 수 있지만 dispatch는 존재하지 않는다.**
후속 dispatcher는 catalog allowlist, role/capability, 인증/session, sequence, queue admission을 통과한 payload만 넘겨야 한다.
CRC 성공 직후 ACK하는 연결은 금지한다. sequence는 인증된 입력을 queue에 수용한 뒤 commit한다.
필요하면 window 복사본으로 duplicate/stale를 미리 검사하고 최종 commit은 한 owner가 직렬화한다.

자동 retry/명령 완료 판정도 없다. 후속 T-003/T-004는 수신 확인과 적용 완료를 분리하고
transaction ID, timeout, 중복 결과 캐시, 상태 재조회, 전원 재부팅 session 갱신을 기존 protocol 설계대로 구현한다.

## 보드와 초기 출력

핀의 제작/전기적 정본은 hardware/*/pinmap.csv와 하드웨어 문서다.
BSP header와 sdkconfig.defaults/partition은 `python tools/generate_boards.py`로 생성하고 `--check`로 drift를 검출한다.
Controller 본체 핀은 고정 Waveshare commit과 adapter 계약을 기록한 waveshare35-pins.json을 입력으로 쓴다.
I²S 데이터 방향은 MCU 기준 재생 DOUT16/녹음 DIN14다. SoC 유효 GPIO와 실제 모듈 외부 pad·메모리 점유 범위를 구분해 JSON/CSV 양쪽을 검사한다. Controller S3R8 및 N16R8은 GPIO22..37을, N8R2는 GPIO22..34를 허용하지 않는다. 근거는 [고정 IDF GPIO 계약](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/docs/en/api-reference/peripherals/gpio/esp32s3.inc)과 해당 보드 pinmap이다.

| MCU | 메모리 | 기반 안전 초기화 |
|---|---|---|
| Controller ESP32-S3R8 | 16 MiB Flash, 8 MiB Octal PSRAM | BL6 low, 복구41 입력. LCD/I2C/카메라 미시작 |
| Comm WROOM-1-N16R8 | 16 MiB Flash, 8 MiB Octal PSRAM, ECC 설정 | RUN_OK7 low 먼저; BOOT0_REQ2/GPS_PWR47 low; RESET_CMD1 high release; RECOVERY9 open-drain release |
| Bridge WROOM-1-N8R2 | 8 MiB Flash, 2 MiB Quad PSRAM | LED5 low, 버튼4 입력. RF/웹 미시작 |
| STM32G474CEU6 | Flash512 KiB/SRAM96 KiB/CCM32 KiB | STB PA4/5 high; FT_EN PA6, ARM PA7, WD PB0 low; TX PA12/PB13/PA15 high |

출력 latch를 mode보다 먼저 설정한다. GPIO 오류는 FAULT에 고정하고 다음 출력을 실행하지 않는다.
reset 이전·brownout·rail 이상 시 안전은 외부 pull/gate/supervisor에 의존한다. host BSP mock은 전기적 안전 증거가 아니다.
UART TX/RTS, FDCAN alternate function, watchdog pulse는 활성화하지 않는다.

STM32 clock 계획값은 HSE16 MHz → PLL M4/N80/R2 → CPU160 MHz,
PCLK1/FDCAN80 MHz, UART4 Mbps/BRR20이다. T-102a는 HSE/PLL·TIM2/SysTick·IWDG의 bounded 초기화를 구현한다.
host register 모델과 실제 CMSIS 상수 대조는 실물 clock/error/timeout/bitrate 계측을 대신하지 않는다. UART/FDCAN은 아직 활성화하지 않는다.
STM32 linker는 bench용 전체 Flash, heap 예약0, stack 예약8 KiB다. stack 사용량/HIL/OTA loader 영역 검증은 별도다.
PSRAM 용량과 ECC 실제 가용량, cache/DMA 제한은 target bring-up에서 확인한다.

## OTA partition의 명확한 경계

기본 partitions.csv는 NVS/key + factory bench image이며 OTA 미구현 상태에서 rollback 성공을 가장하지 않는다.
각 partitions.ota-template.csv에는 [OTA 정본](ota.md)의 recovery/test, A/B app, journal/config/provisioning/staging 배치를
검토용으로 제공하지만 SDK에서 선택하지 않는다. OTA template의 table offset은 0x18000이며 기본은 0x8000이다.

STM32 전체 Flash bench linker도 제품 immutable loader/slot linker와 다르다.
**이 기반 image를 OTA 제품이나 기존 데이터가 있는 장치에 바로 flash/업데이트하지 않는다.**
partition 변경은 데이터 손실/boot 불가 위험이 있어 T-007·T-204·T-304와 signed bundle/복구/단전 gate를 함께 닫아야 한다.
어떤 빌드 flag도 이 기반 코드를 VEHICLE_TX 제품으로 승격하지 않는다.

## 빌드·코딩·문서

실행 명령과 실제 검증 결과는 [기반 개발 절차](../development/foundation.md)에 한 번만 둔다.
공용 코드는 strict C99, 경고를 오류로 처리하며 C99 typedef 기반 정적 검사를 유지한다.
IDF platform adapter는 SDK GNU 언어 모드를 유지하고 자체 코드의 -Wall/-Wextra/-Werror를 적용한다.
공용 GPIO/idle은 canview_esp32_platform, Communicator ESP health의 IDF adapter는 canview_communicator_runtime component가 소유한다. main의 app/startup/BSP 및 canview_foundation은 strict C99이고 네 역할의 실제 startup은 host object compile gate에도 포함한다.
기존 C11 자동화 prototype 시험은 별도 target이다. 이를 C99 gate나 새 firmware 기능으로 합산하지 않는다.

.clang-format은 formatting만 설정하며 C++ 언어 전환을 의미하지 않는다.
.editorconfig, .clangd와 compile_commands.json을 통해 coding 설정을 공유한다.
generated header는 formatter 대상에서 제외한다. public function의 brief/모든 param/return은 문서 build gate로 검사한다.

## 후속 agent가 시작할 순서

1. 자기 task와 관련 architecture만 읽고 기반 테스트를 실행한다. T-001 전체 수용 조건은 아직 닫지 않는다.
2. T-002에서 message catalog 및 golden/malformed/version/capability를 schema에 추가한다. 생성기·codec·독립 참조를 같은 변경으로 갱신한다.
3. T-003/T-004에서 인증/queue/replay/ACK 상태기계를 공용 module에 추가한다. HAL 없이 가짜 clock/queue로 먼저 시험한다.
4. T-102/T-105/T-200/T-300의 대상별 driver/task를 platform에 연결한다. DMA buffer 수명, IRQ/task owner, timeout/backpressure를 문서화한다.
5. T-204/T-304 OTA를 구현하기 전 bench partition과 제품 layout을 명시적으로 분리하고 복구 image/서명/단전 시험을 준비한다.

함수 구현을 채운다는 이유로 NOT_IMPLEMENTED를 OK로 바꾸지 않는다. 완료 판단은 task별 수용 기준과 실제 evidence로 한다.
