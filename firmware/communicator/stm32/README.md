# Communicator STM32 firmware

`STM32G474CEU6`용 CMake project 골격이다. 현재 구현은 reset 직후 transceiver safe-state만 설정하며 UART/FDCAN 송수신은 의도적으로 활성화하지 않는다.

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
- PA6/PA7을 low로 설정해 MAX3055가 normal operating mode로 들어가지 않게 한다.
- 외부 pull resistor가 MCU reset부터 같은 safe state를 보장해야 한다.
- UART 4 Mbps와 FDCAN은 회로/ERC, clock tree, fault-injection 검토 후 추가한다.

상세 설계는 다음 문서를 따른다.

- [`../../../docs/communicator-hardware.md`](../../../docs/communicator-hardware.md)
- [`../../../docs/communicator-uart-protocol.md`](../../../docs/communicator-uart-protocol.md)
- [`../../../docs/development-environments.md`](../../../docs/development-environments.md)
