# Communicator STM32 firmware

`STM32G474CEU6`용 C99 boot/fault bench firmware foundation이다. safe-state 뒤 bounded HSE/PLL·IWDG·TIM2/SysTick과 cooperative health scheduler를 실행하고 reset/build/stack 진단 record를 준비한다. T-103의 3채널 FDCAN capture-only module과 T-104의 USART2 4 Mbps DMA/link adapter를 target에 포함하며, 기본 `app/main.c`는 UART worker를 실제 scheduler에 연결한다. CAN TX·SPORT·OTA는 활성화하지 않는다. [core 구조·API 소유권·ISR·시험·미실행 gate](docs/core-bench.md), [FDCAN capture 계약](docs/fdcan-capture.md), [UART DMA·link 계약](docs/uart-dma.md)이 현재 구현 계약이다. 기존 자동 SPORT prototype은 별도 host 회귀에서만 유지한다.

## dependency

STM32CubeG4 `v1.6.3`을 repository 밖에 clone한다. 전체 버전과 commit은 [`tools/toolchain-versions.json`](../../../tools/toolchain-versions.json)에 고정되어 있다.

```powershell
. .\tools\environment\setup-windows.ps1
Push-Location firmware/communicator/stm32
cmake --preset debug
cmake --build --preset debug
Pop-Location
```

Arm GNU Toolchain `15.3.Rel1` (`arm-none-eabi-gcc` 15.3.x), CMake `4.4.3`, Ninja `1.13.2`가 필요하다. setup script가 설정하는 `CANVIEW_ARM_GNU_ROOT`가 해당 `bin`을 먼저 검색하며, 기존 개발 shell 호환을 위해 `ARM_GNU_TOOLCHAIN_ROOT`도 fallback으로 지원한다.

## build

```powershell
Push-Location firmware/communicator/stm32
cmake --preset debug
cmake --build --preset debug
arm-none-eabi-size build/debug/canview-communicator-stm32.elf
Pop-Location
```

release build는 다음과 같다.

```powershell
Push-Location firmware/communicator/stm32
cmake --preset release
cmake --build --preset release
Pop-Location
```

`build/<preset>/`에 ELF, HEX, BIN, MAP이 생성된다.

## 현재 안전 경계

- PA4/PA5를 high로 설정해 TCAN1046AV 두 채널을 standby로 둔다.
- PA6을 low로 설정해 STB가 high로 고정된 MAX3055를 Power-On Standby로 둔다.
- 외부 pull resistor가 MCU 코드 실행 전부터 TCAN STB high, TCAN/MAX TXD high, MAX EN low를 보장해야 한다.
- 생산 firmware는 PF0/PF1 HSE, FDCAN1 PA11/PA12, FDCAN2 PB12/PB13, FDCAN3 PA8/PA15 배치를 사용한다.
- UART 4 Mbps transport는 USART2 PA0–PA3와 DMA1/DMAMUX1 adapter로 연결되며, 기본 app의 UART worker는 parser·queue·CTS·watchdog 진척을 담당한다. T-103 FDCAN adapter는 검증된 profile에서만 capture mode를 시작할 수 있고, CAN TX request GPIO는 high로 유지되며 외부 gate/PHY receive 전환은 실제 G2 시험 전까지 안전 상태다. UART의 실제 전기·PRBS/HIL은 [UART DMA·link 계약](docs/uart-dma.md)의 `NOT_RUN` gate다.
- IWDG는 명목 375 ms로 설정한다. 현재 필수 worker는 UART와 bench clock/time health 두 개다. 둘 다 period 1 ms·deadline 10 ms이며 callback budget은 각각 1000 us·100 us다. 둘의 새 진척 vote가 모두 필요하며 후속 CAN·safety worker도 별도 vote와 budget 검증이 필요하다.
- build mode는 target 전체에 forced include되는 compile-time `CAPTURE_ONLY` 하나로 고정한다. `CANVIEW_STM_ENABLE_BENCH_TX`, `CANVIEW_STM_ENABLE_VEHICLE_TX`와 mode override는 거부하며 control capability/TX permit은 0/false다. target post-build source gate도 FDCAN TX API/register 사용을 거부한다.
- generated board header의 board+pin SHA-256을 `CANVIEW_BOARD_HARDWARE_DIGEST`로 보존하고, protocol/UART schema·profile·mode를 합친 non-cryptographic build contract digest를 diagnostic metadata로 제공한다. digest는 authenticity나 차량 승인 증명이 아니다.
- reserved high-end stack window를 linker symbol과 현재 MSP로 계산해 64-byte guard를 남긴 뒤 static `0xA5/0x5A` checkerboard watermark를 arm한다. sample은 low-address 연속 prefix를 한 번에 256 byte까지만 검사하고, 더 긴 영역은 보수적인 lower bound로만 보고한다. TIM2 `PSC/ARR/DIER` 계약과 4ms/50% 진행 lower bound도 health에서 확인한다. 전체 call-chain/전원 reset/HIL 증명은 아니다.
- protected control-root API는 T-102에서 Flash를 읽거나 쓰지 않는 policy skeleton이다. root shape 또는 authenticity/debug-lock이 불확실하면 control/TX를 닫고, service reset erase는 실제 삭제 대신 pending decision만 만든다.
- reset reason/build digest/profile/stack 상태를 pointer 없는 version 2 40-byte little-endian diagnostic record로 encode한다. byte 5는 분류된 reset reason이다. 실제 UART 송신과 최종 UART ABI 연결은 T-104 범위다.
- `canview_auto_sport.c`는 기본 70 km/h 진입·55 km/h 복귀, 중속 1.4 m/s² 급가속, 2.5초/0.8초 진입 dwell, 8초 release와 15초 최소 SPORT 유지시간을 구현한다.
- SPORT 진입 전 mode를 snapshot하고 해제 시 `NORMAL` 고정값이 아닌 그 mode의 feedback까지 확인한다. 물리 mode 조작은 `MANUAL_HOLD`로 우선한다.

상세 설계는 다음 문서를 따른다.

- [Communicator hardware](../../../docs/hardware/communicator.md)
- [Communicator UART protocol](../../../docs/architecture/protocols/communicator-uart.md)
- [자동 제어 로직](../../../docs/architecture/automation.md)
- [장치별 toolchain](../../../docs/development/toolchains.md)

## 현재 기반 범위 안내

현재 STM32는 공용 safe-idle startup 대신 자체 app/boot와 SDK 독립 scheduler·queue를 사용한다. BSP/platform 경계와 나머지 ESP32의 공용 startup은 유지한다. T-103 source에는 profile 검증·FDCAN monitor adapter·capture ring이 있지만 기본 app은 CAN worker를 시작하지 않으며 UART와 health worker를 시작한다. CAN TX request·ARM·WDI 출력은 안전 비활성 상태이고 실제 FDCAN receive 전환은 G2 전기 시험 전까지 `NOT_RUN`이다. HSE/PLL·timer·IWDG·T-102 diagnostic source와 T-103 host/target compile은 실제 보드 flash/clock/HIL과 구분한다. linker는 전체 Flash를 사용하는 bench 전용으로 제품 OTA loader/slot image와 다르다. 세부 범위와 현재 gate는 [T-102](../../../docs/tasks/T-102-stm32-platform.md), [T-103](../../../docs/tasks/T-103-stm32-fdcan-capture.md), [core-bench](docs/core-bench.md), [FDCAN capture](docs/fdcan-capture.md)를 따른다.
