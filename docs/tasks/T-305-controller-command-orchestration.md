# T-305 Controller audio/SPORT command orchestration

- 상태: `BLOCKED`
- 우선순위: `P1`
- Gate: UI mock은 G2, 실제 명령은 G4/G5
- 선행: `T-003`, `T-302`, `T-201`, `T-105`
- 후속: `T-504`, `T-505`

## 목표

UI intent를 owner·scope·lease·revision이 있는 semantic command로 바꾸고 ACK와 feedback completion을 정확히 표시한다. Controller가 raw CAN 값을 만들지 않게 한다.

## 구현 범위

- command owner router와 capability/scope precheck
- secure control lease acquire/renew/release
- 8-slot pending tracker를 session-aware transaction manager로 확장
- audio QUIET/REAR_BOOST/CENTER, restore snapshot intent
- adaptive volume offset intent
- SPORT automation arm/disarm/config intent
- request/accepted/executing/completed/rejected UI model
- external OEM override와 manual hold 처리
- reconnect snapshot reconciliation
- input freshness ceiling, tick discontinuity와 automation별 단일 pending-token barrier
- Primary Controller↔STM pair-specific control tag와 signed terminal result 검증 adapter

## 고정 규칙

- ACK로 UI 상태를 applied로 바꾸지 않는다.
- terminal success는 matching `COMMAND_RESULT(COMPLETED)`와 state/feedback snapshot이 있어야 한다.
- UI에는 volume +/- button과 임의 fader/balance 조절을 두지 않는다.
- 내부 profile은 필요한 fader/balance/mute scope를 모두 요구하고 partial apply하지 않는다.
- Controller reboot/session change에서 command를 자동 재실행하지 않는다.
- applied audio offset/mode는 command를 enqueue할 때가 아니라 terminal result와 matching feedback snapshot에서만 바꾼다.

## 수용 기준

- [ ] scope 하나가 없으면 profile transaction 전체가 거부된다.
- [ ] result-before-ACK, duplicate ACK/result, late result가 terminal state를 되돌리지 않는다.
- [ ] TTL/retry/session change에서 UI와 Communicator pending이 일치한다.
- [ ] read-only/Bridge peer로 command path가 생성되지 않는다.
- [ ] raw CAN ID/DLC/data를 인자로 받는 Controller API가 없다.
- [ ] mock backend로 모든 UI 상태 screenshot test가 가능하다.
- [ ] OEM manual change가 automation을 멈추고 snapshot을 갱신한다.
- [ ] pending feedback 중 두 번째 같은 automation command가 생성되지 않는다.
- [ ] stale speed/FFT/audio/mode 또는 250 ms 초과 tick gap에서 dwell과 command 수가 증가하지 않는다.
- [ ] SPORT entry UI 60/70/80 km/h가 owner registry adapter에서 wire 600/700/800으로만 변환되고 key는 `0x0202`다.

## 검증

```bash
ctest --preset host-sanitize -R 'command|lease|orchestration' --output-on-failure
python tests/protocol/run_command_fault_matrix.py --seed 1
python tests/ui/capture_screens.py --scenario command-lifecycle
```

## release 차단

이 task의 mock 성공은 차량 송신 승인이 아니다. T-503/T-106/G4가 끝날 때까지 command transport는 simulator backend에만 연결한다.
