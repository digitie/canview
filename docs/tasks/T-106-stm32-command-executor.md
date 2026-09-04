# T-106 STM32 command executor와 송신 build gate

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G4`, 차량에서는 `G5`
- 선행: `T-101`, `T-105`, `T-503`, `T-500`
- 후속: `T-505`, `T-506`

## 목표

검증된 semantic command만 compiled builder로 실행하고 feedback까지 확인하는 executor를 만든다. `CAPTURE_ONLY`, `BENCH_TX`, `VEHICLE_TX`를 build와 hardware 모두에서 분리한다.

## 구현 범위

- build mode manifest와 CI authorization
- hard TX gate sense/arm state
- control scope·lease·revision·TTL·precondition admission
- generated fixed frame builder, alive counter/checksum owner 정책
- command-specific pulse/rate/total frame limit
- feedback matcher, timeout, cancel/safe termination
- TX audit record와 terminal result cache
- audio snapshot/atomic profile/restore transaction
- auto SPORT action→bounded button pulse transaction
- Controller↔STM pair-specific control-root tag 검증과 signed terminal result
- `COMPLETED/PARTIAL/ROLLED_BACK/ROLLBACK_FAILED` durable audio transaction recovery
- SPORT feedback deadline 1,500 ms를 첫 successful physical TX-complete에서 시작

## 절대 금지 API

아래 형태의 public/internal API를 만들지 않는다.

```c
send_can(bus, arbitrary_id, arbitrary_length, arbitrary_bytes);
execute_profile_value(raw_fader, raw_balance, raw_mode);
```

FDCAN low-level TX 함수는 generated command executor translation unit에서만 link 가능해야 한다.

## 수용 기준

- [ ] default build는 `CAPTURE_ONLY`이고 TX API symbol이 없다.
- [ ] BENCH/VEHICLE build가 승인 manifest 또는 hard gate 없이 시작되지 않는다.
- [ ] 모든 deny 조건별로 frame TX 0건과 reason result가 있다.
- [ ] duplicate/reorder/reboot/fault에서 vehicle effect 최대 1회다.
- [ ] unexpected feedback, bus error, lease loss에서 bounded 종료한다.
- [ ] audio profile 일부 적용 뒤 실패하면 검증된 snapshot restore 또는 명시적 degraded 상태가 된다.
- [ ] SPORT는 physical button capture와 동일한 bounded event만 만들며 raw mode 값을 쓰지 않는다.
- [ ] analyzer diff에서 allow-listed ID/DLC/data mask 밖 frame이 0건이다.
- [ ] Communicator ESP compromise/UART injection fixture에서 valid control tag 없는 TX가 0건이다.
- [ ] audio 각 step 전후 power cut에서 silent partial success가 없고 reboot 결과가 네 terminal 상태 중 하나다.
- [ ] sender mask와 무관하게 dequeue/각 frame 직전 generated safety 조건을 다시 검사한다.

## 검증

```bash
ctest --preset host-sanitize -R command-executor --output-on-failure
python tests/hil/run_command_matrix.py --mode bench-tx --all-faults
python tests/hil/compare_tx_allowlist.py evidence/latest/can-tx.log
```

## release 차단

G4 evidence가 있어도 실차 자동화는 켜지 않는다. 기능별 G5 폐쇄시험과 사용자의 opt-in 설정이 추가로 필요하다.
