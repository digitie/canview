# T-004 Communicator UART v1.0 schema와 codec

- 상태: `IN_PROGRESS`
- branch: `agent/codex-t004-uart-schema-codec`
- PR: draft (생성 후 링크 기록)
- 우선순위: `P0`
- Gate: `G0`
- 선행: `T-001`, `T-002`
- 병렬 가능: `T-003`, `T-005`

## 목표

ESP32–STM32 내부 UART의 모든 payload ABI와 COBS/CRC parser를 구현한다. ESP-NOW packet tunnel 없이 semantic message만 전달한다.

## 고정 결정

- UART protocol version은 ESP-NOW와 독립 `1.0`이다.
- 4,000,000 baud, 8-N-1, RTS/CTS, COBS + `0x00`, CRC-32/ISO-HDLC다.
- decoded packet 최대 1,024 byte, encoded 수집 최대값은 generator가 COBS overhead를 계산한다.
- `CAN_ID_STATS`, `CAN_OBSERVER_PLAN`, `CAN_CAPTURE_CONTROL`, `CAN_CAPTURE_STATUS`, 별도 `CAN_EVENT_MARKER`를 첫 1.0에 포함한다.
- marker는 `CAN_EVENT_MARKER`로만 전달하고 capture control에는 MARK action을 두지 않는다.
- UART HELLO의 STM boot ID가 바뀌면 ESP-NOW session도 교체한다.
- command token과 canonical argument digest를 end-to-end로 보존한다.

## 구현 범위

- `protocol/schema/uart-v1.0.yaml`과 generated C header
- LINK_HELLO/ACK, HEARTBEAT, ACK/ERROR payload
- CAN batch/bus status/safety snapshot/TX audit payload
- command/config/control lease mirror payload
- observer plan BEGIN/CHUNK/COMMIT/ABORT transaction
- streaming COBS decoder, oversize delimiter resync, CRC
- fixed-buffer encoder, host duplex fault simulator

## parser 규칙

delimiter까지 buffer가 넘치면 다음 `0x00`까지 버리고 한 번만 oversize counter를 올린다. 빈 packet은 무시한다. CRC·length 오류 뒤 byte 단위 추정 복구를 하지 않고 delimiter 경계에서 재시작한다. flow stall 100 ms에는 command admission을 중단하고 1초에는 link offline으로 전환한다.

## 수용 기준

- [ ] message ID마다 exact/bounded payload schema가 있다.
- [ ] 0, max, max+1 packet과 COBS `0x00` 포함 payload가 round-trip한다.
- [ ] random insert/delete/flip 뒤 다음 정상 delimiter packet을 복구한다.
- [ ] observer plan의 missing/duplicate/out-of-order chunk가 active plan을 바꾸지 않는다.
- [ ] 같은 marker가 capture-control과 marker message 두 경로로 중복 기록될 수 없다.
- [ ] result-before-ACK와 duplicate command가 재실행되지 않는다.
- [ ] 4 Mbps 상당 host stream 24시간 simulation에서 parser leak/overflow가 없다.

## 계획 보완 수용 기준

- [ ] UART 1.1 CLOCK_ANCHOR QUERY/REPLY는 navigation companion schema에서 생성하며 1.0 peer에는 보내지 않는다. recovery UART는 T-108의 별도 schema/CRC/baud로 분리한다.

## 검증 명령

```bash
python tools/generate_protocol.py --schema protocol/schema/uart-v1.0.yaml --check
cmake --build --preset host-sanitize
ctest --preset host-sanitize -R uart --output-on-failure
python tests/protocol/uart_fault_stream.py --seed 1 --duration-seconds 3600
```

## 안전·rollback

UART error가 vehicle command retry로 직접 변환되면 안 된다. link resync와 current snapshot 전까지 STM32 command admission은 닫힌다.


## 산출물·범위 경계

- 예상 산출물은 `protocol/schema/uart-v1.0.yaml`, generated UART header/codec와 이 문서의 `tests/protocol/` UART fixture다. UART DMA/실물 flow control(T-104/T-202)과 OTA recovery(T-108)는 범위 밖이다.
- wire/generator digest와 malformed/resync 결과를 보존하며 실제 target 4 Mbps 검증은 후속 task에 남긴다.
