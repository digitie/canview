# STM32 UART DMA·link 모듈

이 문서는 T-104의 `canview_stm_uart` runtime과 STM32G474 USART2 adapter의
소유권·실패 경계를 정의한다. 구현은 C99이며 `CANVIEW_STM_CAPTURE_ONLY_CONTRACT`
에서 동작한다. 이 task의 UART 경로는 raw CAN frame, raw replay, control lease 발급
또는 차량 CAN TX를 제공하지 않는다.

## 계층과 데이터 흐름

```text
USART2 PA0/PA1/PA2/PA3
  ├─ DMA1 channel 1 circular RX (2048 byte)
  │    └─ IRQ: wrap/error/event만 기록
  └─ DMA1 channel 2 normal TX
       └─ IRQ: done/error만 기록
                    │
                    ▼
app/main.c UART worker
  └─ platform_service()
       ├─ DMA producer 위치를 bounded하게 snapshot
       ├─ RX byte budget만큼 worker에서 codec feed
       ├─ CTS sample·timeout·heartbeat·cache maintenance
       └─ priority TX queue를 DMA로 하나씩 전송
                    │
                    ▼
module/uart_link.c
  ├─ generated UART ABI validation
  ├─ HELLO/heartbeat/snapshot/time-sync session
  ├─ P0/P1/state/raw fixed queue
  ├─ 256-entry idempotency cache와 8-slot pending
  └─ local authorizer 이후에도 CAPTURE_ONLY로 control/TX 차단
```

| 계층 | 파일 | 소유 책임 |
|---|---|---|
| interface | `interface/canview_stm_uart.h` | 고정 메모리 runtime·queue·pending·mapping API |
| module | `module/uart_link.c` | protocol/session/cache/lease mirror와 command admission |
| platform | `platform/stm32g474/uart_dma.c` | CMSIS register, USART2, DMA/DMAMUX, IRQ, CTS |
| app | `app/main.c` | boot 뒤 UART worker와 watchdog scheduler 연결 |

module은 CMSIS, HAL, DMA, FreeRTOS를 include하지 않는다. 모든 payload는 queue에
동기 복사하며 codec view나 caller buffer의 수명을 연장하지 않는다. runtime의
`servicing` guard는 callback·worker 재진입을 `CANVIEW_RESOURCE_BUSY`로 닫는다.

## 보드·전송 계약

| STM32 signal | pin | ESP32 signal | 의미 |
|---|---|---|---|
| USART2_CTS | PA0 | GPIO15 RTS | active-low 입력, 외부 10 kΩ pull-up |
| USART2_RTS | PA1 | GPIO16 CTS | active-low 출력, 외부 10 kΩ pull-up |
| USART2_TX | PA2 | GPIO18 RX | 4 Mbps 8N1 |
| USART2_RX | PA3 | GPIO17 TX | 4 Mbps 8N1 |

PA0–PA3의 AF7과 PCLK1 80 MHz/BRR 20은 generated board pin·clock contract와
compile-time 검사로 묶인다. RX circular DMA의 CNDTR는 2048이며 4-byte aligned
caller buffer만 허용한다. IDLE interrupt는 byte parsing을 하지 않고 worker가
CNDTR와 wrap counter를 읽게 한다. 32-bit DMA 위치와 별도로 software byte
cursor는 64-bit라서 4 Mbps 24시간 window에서 누적 cursor가 wrap하지 않는다.
UART envelope의 `sender_time_us`와 deadline은 `canview_stm_now_us64()`를 사용한다.
이 API는 TIM2 low-word를 PRIMASK critical section에서 읽고 wrap epoch를 확장하며,
1 ms UART worker가 counter wrap보다 자주 호출하므로 24시간 software window에서
timestamp가 역행하지 않는다. 기존 `canview_stm_now_us()` 32-bit API는 FDCAN
capture compatibility 용도로만 남긴다.

## IRQ와 worker 경계

IRQ는 다음 정보만 기록한다.

- RX DMA half/full transfer와 USART parity/framing/noise/overrun: event와 error latch
- TX DMA transfer-complete/error: event와 error latch
- USART IDLE/CTS: event bit

IRQ에서 COBS/CRC/parser, `memcpy` payload 처리, malloc, printf, callback, blocking과
runtime 호출은 하지 않는다. worker는 호출마다 RX budget을 제한하고 DMA producer가
reader보다 한 바퀴 이상 앞서면 RX를 버린 뒤 runtime·DMA를 함께 reset한다. DMA
wrap IRQ가 늦게 도착하는 경계에서는 producer가 reader보다 한 바퀴 앞선 경우를
보정하며, ISR wrap counter overflow와 모순된 CNDTR는 복구 실패로 처리한다.

TX는 `P0 → P1 → STATE → RAW` 순서다. P0/P1 full은 새 safety/control 입력을
거부하고 safety inhibit를 세운다. STATE는 `(message_type, correlation_id)`로
coalesce하고 RAW는 가장 오래된 항목부터 drop한다. CTS가 100 ms 이상 막히면 새
command admission을 멈추며 1초 이상이면 session과 pending/lease를 폐기하고
HELLO부터 다시 시작한다.

## session·idempotency·시간

양방향 `LINK_HELLO`/`LINK_HELLO_ACK`와 최근 heartbeat, CTS sample, local safety
snapshot이 모두 준비되어야 command admission이 가능하다. `HEARTBEAT`의
`state_revision`은 T-105 generated safety state와 비교하며, 일치하는 경우에만
STM local `SAFETY_SNAPSHOT`을 state queue에 coalesce한다. snapshot은 현재 build의
`CAPTURE_ONLY`, closed TX gate와 inhibit reason을 명시한다.

`CONTROL_TIME_SYNC`는 `(controller_boot_id, stm_boot_id, generation)`에 묶인
4-timestamp mapping을 만들고 uncertainty가 50 ms를 넘거나 30초가 지나면
무효화한다. command의 origin·session·generation·token·digest·control tag는
ESP32에서 재작성하지 않고 pending/cache에 copy한다.

동일한 cache key와 digest는 terminal result를 재전송하고 executor를 다시 호출하지
않는다. 같은 token identity에 다른 digest가 오면 conflict로 거부한다. 256개 entry가
모두 live인 경우에는 entry를 축출하지 않고 pre-ACK `BUSY`로 끝낸다. ESP boot 변경,
heartbeat/CTS offline, runtime reset은 pending·mapping·lease를 모두 폐기한다.

## 검증과 미실행 gate

host에서는 `stm32-uart-link`와 `uart-*` protocol test가 handshake, malformed
resync, cache full, duplicate/conflict, queue 포화, callback reentry, boot 변경,
CTS offline을 검증한다. target은 실제 STM32CubeG4 CMSIS와 Arm GCC로 USART2/DMA/
DMAMUX object 및 최종 ELF/HEX/BIN/MAP을 빌드한다.

아래는 장비가 없는 현재 software gate에서 `NOT_RUN`이다.

- 실제 보드 flash·UART 전기 level·RTS/CTS eye와 24시간 4 Mbps PRBS
- DMA overrun/USART noise·framing의 실제 주입과 reset/brownout rail
- ST-LINK/USB 연결, watchdog 장시간 측정, 차량 CAN 연결·vehicle evidence

이 문서의 host/target 결과는 위 physical/HIL 근거를 대체하지 않는다.
