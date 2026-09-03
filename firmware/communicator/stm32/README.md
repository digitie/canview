# Communicator STM32 firmware

`STM32G474CEU6`용 CMake project 골격이다. reset 직후 transceiver safe-state와 순수 C 자동 SPORT 상태기계를 포함하며 UART/FDCAN 송수신은 의도적으로 활성화하지 않는다.

## dependency

STM32CubeG4 `v1.6.3`을 repository 밖에 clone한다.

```bash
git clone --recursive --depth 1 --branch v1.6.3 \
  https://github.com/STMicroelectronics/STM32CubeG4.git /opt/STM32CubeG4
export STM32CUBE_G4_ROOT=/opt/STM32CubeG4
```

GNU Arm Embedded compiler, CMake 3.22+, Ninja가 `PATH`에 있어야 한다. STM32CubeCLT를 사용해도 된다.

## build

```bash
cmake --preset debug
cmake --build --preset debug
arm-none-eabi-size build/debug/canview-communicator-stm32.elf
```

release build는 다음과 같다.

```bash
cmake --preset release
cmake --build --preset release
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

- [`../../../docs/communicator-hardware.md`](../../../docs/communicator-hardware.md)
- [`../../../docs/communicator-uart-protocol.md`](../../../docs/communicator-uart-protocol.md)
- [`../../../docs/automation-control.md`](../../../docs/automation-control.md)
- [`../../../docs/development-environments.md`](../../../docs/development-environments.md)
