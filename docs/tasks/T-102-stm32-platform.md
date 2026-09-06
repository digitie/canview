# T-102 STM32 platform, clock, watchdog와 cooperative scheduler

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G1/G2`
- 선행: `T-001`
- 병렬 가능: `T-100`, `T-200`, `T-300`

## 목표

현재 safe GPIO 후 `__WFI()`만 하는 scaffold를 production firmware 기반으로 확장하되 CAN TX는 열지 않는다.

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

- [ ] HSE 정상/실패 fixture에서 상태 전이가 결정적이다.
- [ ] safe GPIO write가 clock/peripheral init보다 먼저 실행된다.
- [ ] one worker가 progress하지 않으면 IWDG가 reset하고 PHY default로 돌아간다.
- [ ] `.data+.bss`, stack, map report가 통합 설계 budget 안이다.
- [ ] `CAPTURE_ONLY`가 default이며 command TX symbol을 link하지 않는다.
- [ ] reset reason과 build digest를 UART diagnostic으로 읽을 수 있다.
- [ ] boot authenticity 또는 production debug lock이 불확실하면 reported control capability와 TX permit이 0이다.

## 계획 보완 수용 기준

- [ ] [현행 R1 clock](../hardware/r1/firmware-pinmap.md)의 PLL/BRR/AF·PG10-NRST를 target map과 계측으로 확인하고 UART 4 Mbps를 170 MHz 가정으로 계산하지 않는다.
- [ ] module→interface→BSP/platform 의존, ISR ring 소유권·수명, task별 주기/우선순위/stack/WCET/overflow/progress-vote를 해당 firmware README와 public header에 기록한다.
- [ ] T-101이 사용할 최소 boot/fault image를 먼저 제공한다. T-102의 전체 완료를 T-101의 시작 조건으로 오해하지 않고 실물 미확인 항목은 열린 상태로 유지한다.
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


## 산출물·범위 경계

- FDCAN capture·command executor·OTA bootloader는 범위 밖이다. boot/clock/watchdog 검증 실패 시 모든 capability와 TX gate를 닫고 이전 검증된 capture-only scaffold로 제한한다.
