# STM32 FDCAN capture-only 모듈

이 문서는 T-103의 현재 C 구현인 `canview_stm_fdcan_capture`와 STM32G474
CMSIS adapter의 책임·수명·실패 경계를 정의한다. 이 구현은 세 CAN bus를
관찰하고 classic CAN record를 만드는 기능만 제공한다. CAN TX descriptor,
TX queue, control lease, raw replay API와 차량 송신 경로는 존재하지 않는다.

## 구조와 소유권

```text
FDCAN1/2/3 RX FIFO
        │ IRQ: register snapshot 4 words + TIM2 timestamp
        ▼
platform raw ring (채널별 16 slot, bounded)
        │ service() worker: decode + callback
        ├── frame_sink → module capture_ingest()
        └── drop_sink  → module capture_record_drops()
                         │
                         ▼
                 채널별 64 slot static ring
                         │ build_batch() 단일 worker
                         ├── classic CAN wire batch
                         └── generic ID inventory / observer filter
```

| 계층 | 파일 | 소유 책임 |
|---|---|---|
| interface | `interface/canview_stm_fdcan_capture.h` | vendor 독립 profile, frame, timestamp, ring, batch와 inventory 계약 |
| module | `module/fdcan_capture.c` | timing/profile 검증, ID/DLC/padding 검증, timestamp 확장, ring/drop, batch와 inventory |
| platform | `platform/stm32g474/fdcan_capture.c` | CMSIS register, GPIO alternate function, FDCAN monitor mode, FIFO snapshot, IRQ와 PSR/ECR 읽기 |
| app/BSP | `app/main.c`, `bsp/core.c`, `bsp/board.c` | lifecycle과 safe output 소유. 현재 composition root는 이 adapter를 시작하지 않음 |

module은 `stm32g474xx.h`, CMSIS register, FreeRTOS와 HAL을 include하지 않는다.
platform은 module의 public contract만 호출한다. caller는 capture와 callback
context를 zero-init하고 정적 수명으로 보유해야 한다. `frame_sink`와
`drop_sink`는 `service()`에서만 호출되며 재진입·blocking·heap 사용이 없다.

## Board profile과 PHY 계약

`canview_stm_fdcan_channel_profile_validate()`는 공통 timing 검증에 더해
board별 transceiver와 bitrate를 검사한다. enabled profile은 bitrate와 PHY
종류를 모두 알고 있어야 하며, 알 수 없는 profile은 초기화에서 거부된다.
disabled profile은 모든 timing/bitrate/PHY field가 zero/unknown이어야 하고
해당 PHY는 standby로 남는다.

| Channel | FDCAN RX | TX request GPIO | PHY | 허용 nominal bitrate |
|---|---|---|---|---:|
| 0 | PA11, AF9 | PA12, high 고정 | TCAN1046 | profile timing table |
| 1 | PB12, AF9 | PB13, high 고정 | TCAN1046 | profile timing table |
| 2 | PA8, AF11 | PA15, high 고정 | MAX3055 | 125000 bit/s만 |

FDCAN은 `MON`과 `DAR`를 사용하고 RX FIFO0만 설정한다. 모든 표준/확장
frame을 FIFO0으로 받지만, module은 FD/BRS/DLC>8을 classic record로 내보내지
않는다. start 전에는 TCAN `STB=high`, MAX3055 `FT_EN=low`, 세 TX request
출력을 high로 설정하고, validated enabled profile만 receive mode로 전환한다.
외부 pull/gate가 reset·brownout 동안 같은 안전값을 보장해야 한다.

## Message RAM과 IRQ 경계

STM32CubeG4 v1.6.3의 STM32G4 FDCAN instance layout을 기준으로 instance마다
848 byte, RX FIFO0 offset 176 byte, element 72 byte, FIFO depth 3을 사용한다.
source에는 instance/channel 수와 FIFO 범위를 compile-time assertion으로
고정했다. FIFO fill level이나 get index가 이 계약을 벗어나면 element를
읽거나 callback을 호출하지 않고 FIFO loss를 기록한다.

FDCAN IRQ handler의 책임은 다음으로 제한된다.

- FIFO status를 읽고 최대 3개 element를 처리한다.
- element의 W1..W4와 TIM2 1 MHz counter snapshot을 raw ring에 복사한다.
- raw ring이 가득 차면 element를 버리고 saturating raw-drop counter를 증가시킨다.
- ISR 진입 시점의 RX/non-RX interrupt snapshot을 FIFO drain 전에 acknowledge한다.
  drain 중 새로 올라온 RX flag를 마지막에 다시 지우지 않아 다음 IRQ로 남긴다.
- pending interrupt와 FIFO loss/raw-ring overflow latch는 worker가 critical section에서
  snapshot/clear하며, stop/start 때 raw index·pending·loss 상태를 새 session으로
  초기화한다. singleton owner가 아닌 context의 stop은 GPIO나 peripheral을 건드리지 않는다.

IRQ에서는 ID/DLC/data decode, callback, `printf`, malloc, blocking과 batch
생성을 하지 않는다. worker `service()`가 raw snapshot을 `decode_element()`로
변환하고 `frame_sink`를 호출한다. raw ring drop은 `drop_sink`를 통해 module
drop counter에 합산하고, module ring이 가득 찬 경우에는 `frame_sink`가
`CANVIEW_RESOURCE_BUSY`를 반환하면서 module이 직접 drop을 센다.

## Timestamp와 상태

timestamp source는 FDCAN 내부 SOF timestamp가 아니라 platform IRQ에서 읽은
TIM2 1 MHz hardware counter다. 따라서 이 값은 RX interrupt sample 시각이며
bus wire의 SOF 시각이나 physical latency 측정값으로 해석하지 않는다. start는
HSE/PLL ready, SYSCLK PLL, TIM2 enabled, `PSC=159`, `ARR=UINT32_MAX`를 확인한
뒤에만 성공한다.

module은 u32 source counter를 다음 규칙으로 확장한다.

- 처음 받은 accepted frame만 timestamp high-water를 초기화한다.
- source distance가 half-range보다 크고 낮아진 경우에만 2^32 wrap으로 본다.
- 같은 channel의 extended timestamp가 역행하면 frame을 거부하고 `FAULT`를
  latch한다.
- channel마다 source timestamp/epoch를 따로 보유한다. 다른 channel이 뒤늦게
  시작하면 global high-water에 가장 가까운 epoch를 선택하므로, 한 channel의
  wrap 뒤 다른 channel의 이전 epoch frame도 정상적인 순서로 보존한다.
- 다른 channel의 작은 역순 도착은 multi-channel scheduling 지연으로 허용하지만
  global high-water는 낮추지 않는다. PSR/ECR 상태 snapshot은 수신 frame이 아니므로
  channel의 마지막 frame timestamp를 덮어쓰거나 오래된 frame을 fresh로 만들지 않는다.
- malformed, unsupported, ring full frame은 timestamp state를 변경하지 않는다.
- batch의 base timestamp와 record 사이 delta가 `UINT16_MAX`를 넘으면 다음
  batch로 남긴다.

bus state는 `UNKNOWN_BITRATE`, `NO_DATA`, `ERROR_ACTIVE`, `ERROR_PASSIVE`,
`BUS_OFF`, `FAULT`를 구분한다. `BUS_OFF`, `ERROR_PASSIVE`, `FAULT`는
`observe()`가 `NO_DATA`나 active로 덮어쓰지 않는다. `last_error`는 PSR LEC와
활성 interrupt snapshot을 보존하며, bus-off 복구나 송신 재개를 수행하지
않는다.

## Batch와 inventory

채널별 static ring은 64 record다. producer는 validated frame을 bounded copy한
뒤 ownership을 넘기고, 단일 worker가 세 ring의 pending record를 timestamp와
channel index tie-break로 merge한다. `CANVIEW_WIRE_CAN_MAX_RECORDS`까지만
한 번의 callback/commit transaction에서 처리한다. observer filter가 거부한
record도 bounded item으로 소비해 queue를 막지 않고 `filtered_frames`와 inventory에
반영한다. callback reentry는 `CANVIEW_RESOURCE_BUSY`로 종료하고 pending record를
보존한다. callback이 실행되는 동안 ring index를 바꾸지 않으며, callback 뒤 ring
front가 snapshot과 같은지 확인한 뒤에만 일괄 commit한다. producer race나 변조가
감지되면 `MALFORMED`를 반환하고 ring을 소비하지 않는다.

inventory는 `(bus_id, wire flags, DLC, CAN ID)`별 frame count, data byte 변화
mask, 최초/최종 timestamp, 최대 8개 period sample, p50/p95와 제한된 rate만
제공한다. 이는 generic ID inventory일 뿐 DBC signal decode, scale 추정,
candidate 승격 또는 control permission의 근거가 아니다. 64개 entry가 차면
새 ID만 `inventory_dropped`로 세고 capture/batch는 계속한다.

## 검증 경계

다음은 현재 source/host/target에서 확인하는 항목이다.

- strict C99 host unit: profile/PHY, raw decode, standard/extended/RTR, FD/BRS,
  malformed padding, three-channel ordering, channel별 wrap/delta overflow, no-data/
  passive/bus-off, callback reentry/transaction commit, ring/raw drop와 inventory 포화
- CMSIS fake-register host adapter: clock/profile/output rollback, owner/session reset,
  FIFO fill/index/loss, raw-ring 포화, IRQ wrapper, PSR/ECR 상태, sink timeout과
  callback 계약. 이 시험은 register model일 뿐 실제 STM32 peripheral/HIL이 아니다.
- STM32G474 Arm GNU 15.3.Rel1 target Debug/Release: CMSIS compile, link,
  ELF/MAP/HEX/BIN, size/stack/금지 TX source·symbol gate
- generated board/config, 전체 host CTest, sanitizer와 coverage는 immutable
  candidate에서 다시 실행해 기록한다. 현재 host coverage gate는 module
  function 100%/line 99.1%/branch 94.1%, fake adapter function 100%/line
  98.6%/branch 93.2%이다.

다음 항목은 이 문서나 target build가 성공해도 닫히지 않는다.

- 실제 STM32 board flash, ST-LINK/serial 연결
- FDCAN clock/bitrate, GPIO alternate function, PHY standby/receive 전기 측정
- IRQ latency, raw/module ring saturation under real bus load
- reset/brownout rail gate와 CAN analyzer의 ACK/data TX 0건
- 차량 connector, bitrate, CAN capture evidence와 vehicle profile 승격

위 physical/HIL gate는 장비가 연결될 때까지 `NOT_RUN`이며 차량 CAN TX는
계속 `NO-GO`다. 기본 `app/main.c`가 adapter를 시작하지 않는 것은 T-103
source/host/target bench 구현과 실제 G2 wiring을 분리하기 위한 의도적인
경계다. UART 전달과 실제 task orchestration은 T-104에서 이 capture contract를
소비한 뒤 연결한다.
