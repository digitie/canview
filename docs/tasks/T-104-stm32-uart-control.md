# T-104 STM32 UART DMA, link state와 idempotency

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2`
- 선행: `T-004`, `T-102`
- 병렬 가능: `T-103`

## 목표

4 Mbps UART를 non-blocking DMA/ring으로 구현하고 ESP32 link 상태, priority queue, command duplicate cache를 STM32 안전 경계에 연결한다.

## 구현 범위

- USART2 PA0/1/2/3 AF7, RTS/CTS, RX circular DMA와 idle/delimiter 처리
- generated UART codec adapter
- bidirectional HELLO/boot ID/version/capability state machine
- P0/P1, state, raw telemetry 세 TX queue
- flow-stall/offline timer와 counters
- 256-entry/60초 request token+digest result cache
- control/diagnostic lease mirror와 snapshot serialization
- Controller origin device/boot/session/control generation/scope와 end-to-end control tag를 보존하는 opaque envelope
- Controller↔STM `CONTROL_TIME_SYNC` mapping, uncertainty와 generation
- ESP boot change 시 pending/lease 폐기

## 고정 동작

- CTS 100 ms block: new command admission stop
- CTS 1초 block 또는 heartbeat 1초: UART offline, lease 폐기
- safety/control queue full: raw queue를 비워도 공간이 안 나면 fail-closed
- UART ACK는 vehicle feedback 완료가 아님
- ESP reboot 뒤 이전 pending command 자동 실행 금지

## 수용 기준

- [ ] 4 Mbps PRBS/packet 24시간에 unaccounted byte loss가 없다.
- [ ] insert/delete/flip 뒤 다음 delimiter에서 resync한다.
- [ ] telemetry 포화 중 P0/P1 p99 latency가 budget 안이다.
- [ ] duplicate same digest가 재실행 0회, cached result를 반환한다.
- [ ] same token/different digest가 fault로 거부된다.
- [ ] ESP boot 변경, heartbeat loss, CTS stall에서 lease와 pending이 폐기된다.
- [ ] idempotency cache memory가 STM SRAM budget 안이다.
- [ ] Communicator ESP 변조 또는 raw UART injection은 유효한 새 lease/command/result tag를 만들지 못한다.
- [ ] 256 cache entry가 모두 live면 기존 token을 축출하지 않고 새 command를 pre-ACK `BUSY`로 거부한다.
- [ ] clock drift/asymmetric delay/wrap와 양 MCU reboot에서 stale generation command TX가 0건이다.

## 검증

```bash
ctest --preset host-sanitize -R 'uart|idempotency|lease' --output-on-failure
python tests/hil/run_uart_soak.py --baud 4000000 --hours 24
python tests/hil/inject_uart_faults.py --seed 1
```

## 주의

256개 전체 result payload가 SRAM budget을 넘으면 임의로 entry 수를 줄이지 않는다. compact result record와 60초 time bucket을 설계하고 budget 변경 근거를 이 task에 남긴다.


## 산출물·범위 경계

- 예상 산출물은 STM UART DMA adapter·link/cache component와 이 문서의 `tests/hil/` soak/fault scripts다. vehicle frame builder와 recovery UART는 범위 밖이다.
- DMA buffer 수명·wrap/overrun·parser view copy·queue 취소·reset 후 callback을 검증한다. command cache의 live entry를 줄여 SRAM 실패를 감추지 않고 CONTROL scope를 닫는다.
