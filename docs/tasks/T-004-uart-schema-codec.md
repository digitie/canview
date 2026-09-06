# T-004 Communicator UART v1.0 schema와 codec

- 상태: `IN_PROGRESS`
- branch: `agent/codex-t004-uart-schema-codec`
- PR: [#20](https://github.com/digitie/canview/pull/20)
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

- [x] message ID마다 exact/bounded payload schema가 있다.
- [x] 0, max, max+1 packet과 COBS `0x00` 포함 payload가 round-trip한다.
- [x] random insert/delete/flip 뒤 다음 정상 delimiter packet을 복구한다.
- [x] observer plan의 missing/duplicate/out-of-order chunk가 active plan을 바꾸지 않는다.
- [x] 같은 marker가 capture-control과 marker message 두 경로로 중복 기록될 수 없다.
- [x] result-before-ACK와 duplicate command가 재실행되지 않는다.
- [x] 4 Mbps 상당 host stream 24시간 simulation에서 parser leak/overflow가 없다.

## 계획 보완 수용 기준

- [x] UART 1.1 CLOCK_ANCHOR QUERY/REPLY는 navigation companion schema에서 생성하며 1.0 peer에는 보내지 않는다. recovery UART는 T-108의 별도 schema/CRC/baud로 분리한다.

## 검증 명령

```bash
python tools/generate_protocol.py --schema protocol/schema/uart-v1.0.yaml --check
cmake --build --preset host-sanitize
ctest --preset host-sanitize -R uart --output-on-failure
python tests/protocol/uart_fault_stream.py --seed 1 --duration-seconds 86400
cmake --build --preset host-release
./build/host-release/canview-uart-tests.exe soak-24h
```

## 안전·rollback

UART error가 vehicle command retry로 직접 변환되면 안 된다. link resync와 current snapshot 전까지 STM32 command admission은 닫힌다.


## 산출물·범위 경계

- 예상 산출물은 `protocol/schema/uart-v1.0.yaml`, generated UART header/codec와 이 문서의 `tests/protocol/` UART fixture다. UART DMA/실물 flow control(T-104/T-202)과 OTA recovery(T-108)는 범위 밖이다.
- wire/generator digest와 malformed/resync 결과를 보존하며 실제 target 4 Mbps 검증은 후속 task에 남긴다.

## 현재 검증 범위

- UART 1.1 CLOCK_ANCHOR는 기존 `navigation-v1.json` companion과 schema 기반 host codec에서만 직렬화한다. `uart-schema`가 1.0 peer/capability 누락 거부와 message ID 분리를, C `uart-limits`가 1.0 catalog의 미지원 처리를 검사한다. 실제 1.1 target transport는 T-100b/T-202에 남긴다.
- 가상시간 Python 시험은 방향당 초당 10개 primary frame의 결함 복구 시험이다. 이것만으로 4 Mbps 24시간 바이트 부하를 증명하지 않는다. 별도 C `soak-24h`는 8-N-1의 10 bit/byte를 적용해 방향당 최소 34,560,000,000 byte를 실제 production decoder에 전달하고 계수한다.
- command enqueue는 반환 전 bounded queue의 소유 버퍼로 header와 payload를 복사한다. parser를 다음 command로 덮어쓴 뒤 queue-full·reset에도 보관 command가 유지되는 `uart-payload-lifetime` 회귀시험으로 확인한다.
- link/plan/cache/replay는 단일 worker가 소유한다. ISR/DMA와 다른 task는 event를 전달하고 같은 context를 직접 변경하지 않는다. T-104/T-202가 실제 queue flush·DMA/ISR, T-105/T-106이 local auth/safety와 TX 직전 검사, T-203이 observer reserve·실제 예산을 소유한다. 이 task의 codec 성공은 차량 admission 승인이 아니다.

## 검증 evidence

- [2인 적대적 리뷰·각 finding disposition](../reviews/adversarial/2026-09-07-T-004.md)
- [최종 68개 host 검증·coverage·target binary SHA-256](../reviews/adversarial/evidence/2026-09-07-T-004-validation-final.md)
- [방향당 34.56GB 실제 C parser soak](../reviews/adversarial/evidence/2026-09-07-T-004-soak-final.md)

수용 기준 시험은 완료했으며 reviewer 최종 판정·원격 CI·PR merge 완료 전까지 상태는 IN_PROGRESS다. physical/HIL gate는 위 범위 경계를 따른다.
