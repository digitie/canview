# T-102 STM32 platform, clock, watchdog와 cooperative scheduler

- 상태: `IN_PROGRESS`
- 우선순위: `P0`
- Gate: `G1/G2`
- 선행: `T-001`, `T-102a`
- 병렬 가능: `T-100`, `T-200`, `T-300`

## 2026-09-08 source-only 시작

`T-001`과 `T-102a`가 main에 merge되어 source 선행이 준비됐다. 사용자가 `G1 이전 fw 구현 허용`과 `C로 작성`을 명시했으므로 승인 PCB·실물 clock/reset/HIL 없이 이 task의 C99 platform, clock plan, watchdog vote와 cooperative scheduler source 및 target compile을 진행한다. 이 예외는 physical acceptance를 닫지 않으며, FDCAN/UART 송수신과 차량 CAN TX는 이 task 범위에서 활성화하지 않는다.

## 2026-09-08 C source 구현

T-102a의 safe GPIO·HSE/PLL·TIM2/SysTick·IWDG·cooperative scheduler를 유지하면서 다음 C99 계약을 추가했다.

- `canview_build_mode.h`에서 STM32 image를 `CAPTURE_ONLY` 하나로 고정하고 TX mode override macro를 compile error로 거부했다.
- target의 모든 C translation unit에 `CAPTURE_ONLY` header와 composition token을 forced include하고, BSP link anchor·source/API/register gate·실 compiler negative fixture로 mode 우회를 검사한다.
- build metadata assembly를 module에서 BSP provider target으로 이동해 module이 generated board/protocol header에 직접 의존하지 않게 했다.
- generated `board_pins.h`에 board+pin input SHA-256 기반 `CANVIEW_BOARD_HARDWARE_DIGEST`를 추가했다. protocol/UART schema digest·profile·hardware digest·mode를 포함한 non-cryptographic build contract digest와 target symbol을 제공한다.
- linker의 실제 high-end reserved stack window를 `__stack_limit`으로 export하고 current MSP에서 64-byte guard를 제외한 static `0xA5/0x5A` checkerboard watermark를 arm한다. sample은 low-address 연속 prefix를 256 byte bounded scan하고 더 긴 영역은 보수적인 lower bound로만 보고하며, 128-byte 미만 free watermark는 health fault다. TIM2 `PSC/ARR/DIER` invariant와 4ms/50% 진행 lower bound도 health에서 검사한다.
- RCC reset flags를 단일/복합 원인으로 fail-closed 분류하고, protected root의 RAM shadow와 service-reset erase pending decision skeleton을 추가했다. Flash write/erase, authenticity 증명과 debug lock 변경은 없다.
- reset/build/profile/stack/capability를 version 2, reset reason byte를 포함한 pointer 없는 40-byte little-endian diagnostic record로 encode하고 boot에서 metadata·stack·CAPTURE_ONLY invariants를 확인한다. 초기화 전 HardFault는 명시적 system reset을 요청하며 clock health fault는 terminal latch한다. 실제 UART 송신은 T-104다.

변경된 public/module/test 경계와 owner·수명·bounded 조건은 [STM32 core-bench 계약](../../firmware/communicator/stm32/docs/core-bench.md)에 기록했다.

## 목표

현재 safe GPIO 후 `__WFI()`만 하는 scaffold를 production firmware 기반으로 확장하되 CAN TX는 열지 않는다.

최소 boot/fault image·host fixture·target compile은 [T-102a](T-102a-stm32-core-bench.md)로 먼저 구현한다. 이 하위 task의 완료만으로 아래 G1/G2·실제 clock/reset/UART diagnostic·watermark 수용 기준을 완료 처리하지 않는다.

## 구현 범위

- vector/startup와 R1 HSE 16 MHz → SYSCLK 160 MHz, APB/USART2/FDCAN 80 MHz
- 선택 HSE와 FDCAN/USART kernel clock compile-time calculation
- SysTick 또는 monotonic hardware timer 1 ms, microsecond capture timer
- IWDG 250–500 ms와 progress-vote watchdog
- reset reason·clock failure·stack watermark diagnostic
- static queue/ring primitives와 cooperative scheduler
- board pin initialization을 generated pin header와 대조
- build mode `CAPTURE_ONLY` 기본값
- protected control-root page, boot authenticity/debug-lock status와 erase-on-service-reset policy skeleton

## 고정 규칙

- output latch safe level을 mode 변경 전에 쓴다.
- HSE/PLL 검증 실패 시 PHY command TX를 열지 않고 bounded retry 후 watchdog reset한다.
- 초기화 후 dynamic allocation을 금지한다.
- watchdog은 main loop 한 곳에서 무조건 refresh하지 않는다.
- firmware build metadata에 protocol/profile/hardware digest를 넣는다.

## 예상 파일

```text
firmware/communicator/stm32/src/platform/*
firmware/communicator/stm32/src/scheduler/*
firmware/communicator/stm32/include/canview_build_mode.h
firmware/communicator/stm32/tests/*
```

## 수용 기준

- [x] HSE 정상/실패 fixture에서 상태 전이가 결정적이다. (host named-register model)
- [x] safe GPIO write가 clock/peripheral init보다 먼저 실행된다. (기존 T-102a source/host fixture)
- [x] one worker가 progress하지 않으면 IWDG가 reset하고 PHY default로 돌아간다. (cooperative vote/health fixture; physical reset은 미실행)
- [x] `.data+.bss`, stack, map report가 통합 설계 budget 안이다. (target ELF/linker/stack-usage gate)
- [x] `CAPTURE_ONLY`가 default이며 command TX symbol을 link하지 않는다. (forced compile-time contract, BSP link anchor, source/API/register gate와 target symbol scan)
- [ ] reset reason과 build digest를 UART diagnostic으로 읽을 수 있다. (40-byte source record는 준비했지만 UART transport는 T-104)
- [x] boot authenticity 또는 production debug lock이 불확실하면 reported control capability와 TX permit이 0이다. (root/policy C test와 target default)

## 계획 보완 수용 기준

- [ ] [현행 R1 clock](../hardware/r1/firmware-pinmap.md)의 PLL/BRR/AF·PG10-NRST를 target map과 계측으로 확인하고 UART 4 Mbps를 170 MHz 가정으로 계산하지 않는다. (source compile 완료, physical 계측 `NOT_RUN`)
- [x] module→interface→BSP/platform 의존, ISR ring 소유권·수명, task별 주기/우선순위/stack/WCET/overflow/progress-vote를 해당 firmware README와 public header에 기록한다.
- [x] T-101이 사용할 최소 boot/fault image를 먼저 제공한다. T-102의 전체 완료를 T-101의 시작 조건으로 오해하지 않고 실물 미확인 항목은 열린 상태로 유지한다. (target Debug/Release source image)
- [ ] STM Flash 보호 root/config와 T-107 부트로더 map의 배치가 겹치지 않는다. T-107 이전 전체 Flash scaffold는 OTA 지원 image가 아니다.

## 검증 명령

```powershell
. .\tools\environment\setup-windows.ps1
Push-Location firmware/communicator/stm32
cmake --preset debug
cmake --build --preset debug
arm-none-eabi-size build/debug/canview-communicator-stm32.elf
Pop-Location
```

host `ctest`는 별도의 T-001 root test preset이 추가된 뒤 저장소 루트에서 수행한다.

## evidence

map/size/stack-usage, HSE failure scope, IWDG reset log를 남긴다. hardware 미도착 시 host test까지 진행하되 task 상태는 G1 evidence 전까지 완료하지 않는다.

2026-09-08 검증 결과: pinned Windows Clang 23.1.0/CMake 4.4.3/Ninja 1.13.2에서 Host Debug/Release 전체 CTest가 각각 116/116 통과했고, 공용 core·ESP32 core·STM32 platform/register coverage gate를 통과했다. `stm32-build-mode-negative` 실 compiler fixture에서 forced include·mode/TX override 거부를 확인했고, `generate_boards --check`, document link, plan, sdkconfig/generator negative·mutation test, Doxygen 32 API/Sphinx strict도 통과했다. pinned Arm GNU 15.3.Rel1와 STM32CubeG4 1.6.3에서 Debug/Release target ELF/MAP/BIN/HEX가 생성되고 post-build text/data/bss·stack/symbol/source TX gate 및 compiler/linker/CMake warning/error scan이 0건이었다. physical board/HIL·UART 실제 송신·clock/reset/rail 계측·Flash root 배치·FDCAN은 `NOT_RUN`이며 post-fix 독립 reviewer/CI는 다음 gate다.


## 산출물·범위 경계

- FDCAN capture·command executor·OTA bootloader는 범위 밖이다. boot/clock/watchdog 검증 실패 시 모든 capability와 TX gate를 닫고 이전 검증된 capture-only scaffold로 제한한다.
