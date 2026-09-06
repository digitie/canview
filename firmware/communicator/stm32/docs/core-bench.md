# STM32 최소 core bench 계약

이 구현은 [T-102a](../../../../docs/tasks/T-102a-stm32-core-bench.md)의 최소 boot/fault image다. CAN·UART transport·OTA·Flash 쓰기·보호 설정 변경은 없다. CAPTURE_ONLY만 configure 가능하며 control capability/TX permit은 항상 0이다.

## 구조·소유권

```text
app/main.c + app/boot.c
  → module/scheduler.c, module/queue.c
  → interface/canview_stm_*.h
  → bsp/core.c + bsp/board.c
  → platform/stm32g474/core_hw.c + safe_gpio.c
```

app/module에는 MCU register나 vendor API를 두지 않는다. boot·scheduler·queue context는 caller가 zero-init하고 소유한다. callback context 수명을 caller가 보장한다. descriptor는 init 시 복사한다. queue는 header와 payload를 포함한 최대64byte record 전체를 동기 복사하며 caller view/pointer의 수명을 연장하지 않는다. UART message queue의 크기·수명은 T-104에서 별도 설계하며 이64byte queue에 포인터만 넣어 대체하지 않는다.

queue는 필수 critical port 안에서 단일 core IRQ/main 접근을 직렬화한다. enter의 이전 PRIMASK를 leave가 복원하며 이미 mask된 호출에서 interrupt를 임의로 켜지 않는다. NMI/HardFault·DMA 직접 쓰기는 금지한다. stats 직접 판독도 owner/critical section에서 수행한다. full은 새 record 거부와 saturating drop 증가, empty는 출력 불변이다. 초기화 후 flush/reset API는 없으며 consumer가 drain한다.

## 부팅·고장 상태

UNINITIALIZED → STARTING → READY 순서이며 safe latch → IWDG → HSE/PLL → timer 순으로 호출한다. 각 실패는 FAULT로 고정되고 다음 초기화를 실행하지 않는다. 같은 context 재시작·ISR 실행을 금지한다. 실패 후 IWDG 갱신 없이 reset을 기다리고, IWDG 자체 준비 실패이면 system reset을 요청한다. reset 뒤 이전 권한은 존재하지 않는다.

safe GPIO는 generated pin header를 사용해 latch를 먼저 쓴다. 표준 vendor Reset_Handler/SystemInit의 reset clock 정리 이후 C main에서 실행하며, HSE/PLL과 추가 peripheral 초기화보다 앞선다. 코드 이전의 reset/brownout 안전은 외부 pull/gate에 의존하고 실물 근거가 필요하다.

## 스케줄·watchdog

| 항목 | 계약 | 현재 연결 |
|---|---|---|
| 실행 owner | 단일 main, 재진입 금지 | cooperative super-loop |
| worker | 최대8개, descriptor 순서, due당 최대1회 | clock/timer register health 1개 |
| bench health | period1ms·deadline10ms·budget100us | 실제 CAN/UART/safety worker의 대체 아님 |
| 필수 worker | period≤deadline≤20ms | 전체 fresh progress vote 필요 |
| 선택 worker | period/deadline≤1000ms | 현재 없음 |
| IWDG feed | 모든 필수 신규 vote + 최소10ms cadence | 무조건 loop/ISR feed 없음 |
| main gap | u32 wrap 허용, 간격20ms 초과/backward는 fault | RTC 사용 없음 |
| 실행 budget | 각 callback≤1000us·전체≤20ms | TIM2로 검사, 실물 WCET 미계측 |
| BUSY/error | BUSY는 progress 없음, deadline 초과/error/WCET는 fault | latched, 자동 복구 없음 |

호출 자체는 진척이 아니다. worker는 bounded service/check를 완료했을 때만 OK를 반환한다. deadline은 init 시점부터 연속인 microsecond clock으로 이전 완료→현재 시작·완료 간격을 검사하며, 검사 성공 뒤에만 새 완료 timestamp/vote를 기록한다. ms tick은 dispatch/cadence용이고 `last_progress_ms`는 step 시작 시점 진단값일 뿐 freshness 판정에 쓰지 않는다. watchdog 갱신 직전에도 모든 필수 worker의 freshness를 재검사한다. 한 번 갱신에 쓴 vote는 소모한다. 미래 CAN/UART/safety worker를 연결할 때 required mask와 WCET·queue budget을 새로 검증해야 한다. 현재는 PHY standby·ARM LOW·WDI LOW를 계속 유지하므로 외부 watchdog을 건강하다고 허위 pulse하지 않는다.

IWDG PR32·reload374는 nominal32kHz에서375ms다. DS12288 Rev6의 LSI29.5–34kHz 범위로 계산하면 약353–407ms이며 목표250–500ms 안이다. 실제 oscillator·reset·PHY 전환은 미계측이다. 초기 register 반영 feed와 런타임 health feed를 구분한다.

## Clock·ISR

HSE16MHz crystal(non-bypass), M4/N80/R2/Q4로 SYSCLK160MHz·APB1/2=80MHz·FDCAN PLLQ80MHz, USART2=PCLK1·BRR20을 compile-time 검사한다. USART2/FDCAN peripheral 자체는 시작하지 않는다. range1 boost 전 HCLK/2, Flash4WS, PLL 선택 뒤 DWT160cycles 이상 대기 후 HCLK/1로 전환한다. 모든 readiness/settle loop는 최대1,000,000회이며 실패 시 watchdog 경로로 닫힌다. 이 횟수를 실측 시간으로 주장하지 않는다.

| ISR | 책임 | 공유 데이터·처리 |
|---|---|---|
| SysTick | u32 monotonic ms 증가만 | ISR 단일 writer·main 원자 word read, Cortex-M4 |
| NMI/CSS | fault latch·CSS flag clear·system reset | 중단된 watchdog write로 복귀하지 않는 terminal 예외 |
| HardFault | fault latch·feed 없이 대기 | PHY는 원래 안전 출력 고정 |

TIM2는 APB1 timer160MHz /160 =1MHz, u32 약71.6분 wrap이다. scheduler의 모든 시간차는 unsigned subtraction이고 유효 간격을 제한한다. health worker는 SysTick이 진행한 두 sample 사이 TIM2 정지·역행·과대한 경과도 fault로 고정한다. SysTick과 TIM2의 실측 정확도·IRQ latency는 G1/G2에서 확인한다. handler는 parsing/logging/heap/Flash 작업을 하지 않는다.

## 시험·memory·진단

root CTest의 `stm32-core-*`가 boot 단계 실패·clock wrap/backward·worker 정지/재진입/과실행·queue 경계와 실제 backend의 host named-register model을 실행한다. 모델은 전기적 simulator가 아니며 target build가 모델 상수47개와 실제 고정 CMSIS 값을 독립 compile-time 비교한다. test hook은 host에서만 활성화하고 Arm target에서는 compile error로 차단한다.

target linker는 static RAM80KiB·reserved stack24KiB·총 RAM margin24KiB를 강제한다. 현재 stack reserve는8KiB이고 `check_stm32_core.py`는 실제 ELF 크기·필수 core symbol·금지 heap/FDCAN TX symbol·단일 `.su` frame≤2KiB를 검사한다. compile database의 모든 C object에 해당하는 개별 `.su`가 있어야 하며 일부 누락도 실패한다. 빈 파일은 `nm`으로 해당 object에 code symbol이 없음을 확인한 const table 전용 unit만 허용한다. assembly startup과 prebuilt external library는 이 frame 검사에서 제외한다. 단일 frame 상한은 전체 call-chain/IRQ 중첩 stack watermark 증명이 아니다. 전체 Flash bench layout은 MCUboot/OTA layout이 아니며 root/config page에 쓰는 API가 없다.

`canview_stm_board_diagnostic()`은 reset flags·clock 상태·unknown boot/debug 인증·TX0을 caller snapshot으로 제공한다. 실제 UART diagnostic 전송, profile/hardware/protocol digest 포함 production metadata, stack watermark는 T-102/T-104/T-107에서 연결한다. 포인터/host struct를 wire로 memcpy하지 않는다.

## 근거와 미실행

- [RM0440 Rev9](https://www.st.com/resource/en/reference_manual/dm00355726.pdf), §6 voltage scaling, Flash latency, RCC/IWDG/TIM2.
- [DS12288 Rev6](https://www.st.com/resource/en/datasheet/stm32g474vb.pdf), p122 LSI characteristics.
- 고정 STM32CubeG4 v1.6.3 CMSIS와 `NUCLEO-G474RE/Templates_LL/Src/main.c`의 boost 전환 sequence를 대조했다. 버전/commit은 저장소 toolchain manifest를 따른다.

실물 board flash/clock/reset/PHY·UART/HIL·power failure·CAN/RF·production security는 NOT_RUN이다. SWD로 동작을 관찰할 때 DBGMCU watchdog freeze 여부와 PG10-NRST option byte를 별도 기록하며 이 firmware는 해당 보호 설정을 변경하지 않는다. T-101 이전 차량 bus 연결을 허용하지 않는다.
