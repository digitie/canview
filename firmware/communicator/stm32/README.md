# Communicator STM32 firmware

`STM32G474CEU6`용 C99 최소 boot/fault bench firmware다. safe-state 뒤 bounded HSE/PLL·IWDG·TIM2/SysTick과 cooperative health scheduler를 실행한다. UART/FDCAN 송수신·SPORT·OTA는 활성화하지 않는다. [core 구조·API 소유권·ISR·시험·미실행 gate](docs/core-bench.md)가 현재 구현 계약이다. 기존 자동 SPORT prototype은 별도 host 회귀에서만 유지한다.

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
- UART 4 Mbps와 FDCAN은 후속 task에서 clock/fault 계측과 함께 추가한다. 현재 모든 PHY enable GPIO는 부팅 뒤에도 안전 비활성 상태를 유지한다.
- IWDG는 명목 375 ms로 설정한다. 현재 필수 worker는 bench clock/time health 하나이며, 후속 CAN·UART·safety worker가 연결되면 각자의 실제 진척 vote가 필요하다.
- `canview_auto_sport.c`는 기본 70 km/h 진입·55 km/h 복귀, 중속 1.4 m/s² 급가속, 2.5초/0.8초 진입 dwell, 8초 release와 15초 최소 SPORT 유지시간을 구현한다.
- SPORT 진입 전 mode를 snapshot하고 해제 시 `NORMAL` 고정값이 아닌 그 mode의 feedback까지 확인한다. 물리 mode 조작은 `MANUAL_HOLD`로 우선한다.

상세 설계는 다음 문서를 따른다.

- [Communicator hardware](../../../docs/hardware/communicator.md)
- [Communicator UART protocol](../../../docs/architecture/protocols/communicator-uart.md)
- [자동 제어 로직](../../../docs/architecture/automation.md)
- [장치별 toolchain](../../../docs/development/toolchains.md)

## 현재 기반 범위 안내

현재 STM32는 공용 safe-idle startup 대신 자체 app/boot와 SDK 독립 scheduler·queue를 사용한다. BSP/platform 경계와 나머지 ESP32의 공용 startup은 유지한다. CAN PHY 요청·ARM·WDI 출력은 안전 비활성 상태이고 UART/CAN은 시작하지 않는다. HSE/PLL·timer·IWDG source 검증과 실제 보드 flash/clock/HIL을 구분한다. linker는 전체 Flash를 사용하는 bench 전용으로 제품 OTA loader/slot image와 다르다. 최종 검증 상태는 [T-102a](../../../docs/tasks/T-102a-stm32-core-bench.md)를 따른다.
