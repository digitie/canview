# Communicator STM32 firmware

`STM32G474CEU6`용 C99 CMake project 골격이다. reset 직후 transceiver safe-state만 실행하며 UART/FDCAN 송수신과 SPORT는 활성화하지 않는다. 기존 자동 SPORT prototype은 별도 host 회귀에서만 유지한다.

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
- UART 4 Mbps와 FDCAN은 회로/ERC, clock tree, fault-injection 검토 후 추가한다. HSE/FDCAN 초기화가 완료되기 전에는 PHY enable GPIO를 바꾸지 않는다.
- IWDG 목표 timeout은 250–500 ms이며 CAN·UART·safety task가 모두 정상일 때만 refresh한다.
- `canview_auto_sport.c`는 기본 70 km/h 진입·55 km/h 복귀, 중속 1.4 m/s² 급가속, 2.5초/0.8초 진입 dwell, 8초 release와 15초 최소 SPORT 유지시간을 구현한다.
- SPORT 진입 전 mode를 snapshot하고 해제 시 `NORMAL` 고정값이 아닌 그 mode의 feedback까지 확인한다. 물리 mode 조작은 `MANUAL_HOLD`로 우선한다.

상세 설계는 다음 문서를 따른다.

- [Communicator hardware](../../../docs/hardware/communicator.md)
- [Communicator UART protocol](../../../docs/architecture/protocols/communicator-uart.md)
- [자동 제어 로직](../../../docs/architecture/automation.md)
- [장치별 toolchain](../../../docs/development/toolchains.md)

## 현재 기반 범위 안내

현재 프로젝트는 [C99 기반 구조](../../../docs/architecture/firmware-foundation.md)의 공용 startup/app과 분리된 BSP/platform을 사용한다. CAN PHY 요청·ARM·WD 출력은 안전 비활성 상태로 유지하고 UART/CAN/HSE는 시작하지 않는다. Arm GNU `15.3.Rel1`로 debug/release ELF·HEX·BIN·MAP 생성을 확인했고 컴파일·링커 warning/error scan도 통과했다. 실제 보드 flash·clock·HIL은 미실행이다. linker는 전체 Flash를 사용하는 bench 전용으로 제품 OTA loader/slot image와 다르다.
