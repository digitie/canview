# T-503 OEM audio command와 feedback 검증

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G3 조사`, `G4 bench TX`
- 선행: `T-501`, `T-403`, `T-500`
- 후속: `T-106`, `T-504`

## 목표

취침 mode, 뒷좌석 강화, 상대 volume offset과 OEM snapshot 복원에 필요한 frame owner, alive counter, checksum, feedback을 검증한다. 실제 head unit/bus를 교란하지 않고 bench에서 먼저 재현한다.

## 조사 항목

- physical head-unit 조작 전후 TX owner와 arbitration ID
- volume/fader/balance/main mute/rear mute/SDVC signal 방향과 enum/range
- alive counter/checksum 계산과 다른 field 보존
- command event인지 periodic owner frame인지
- 적용 feedback, OEM manual override, source/phone/reverse 안내와 충돌
- ignition cycle/reboot 때 OEM default와 snapshot validity

## 고정 기능 경계

- Controller는 raw fader/balance UI를 제공하지 않는다.
- semantic profile은 필요한 scope 전체가 있을 때만 atomic하게 적용한다.
- periodic owner spoof는 owner arbitration과 충돌 해소 근거 없이 금지한다.
- audio snapshot이 stale/unknown이면 restore를 실행하지 않고 기능을 disable한다.

## 수용 기준

- [ ] 각 field/counter/checksum/feedback에 반복 capture와 negative control이 있다.
- [ ] physical operation과 generated bench frame의 analyzer diff가 허용 mask 안이다.
- [ ] unrelated payload bit가 바뀌는 frame이 0개다.
- [ ] partial profile failure와 reverse/call/manual override에서 복원 matrix가 통과한다.
- [ ] snapshot revision/TTL과 ignition invalidation 규칙이 test로 고정된다.
- [ ] vehicle profile generator가 VERIFIED evidence 없이는 audio command를 생성하지 않는다.

## 검증

```bash
python tests/audio/analyze_owner_counter.py private/evidence/audio/*.cvtrace
python tests/hil/run_audio_profile_matrix.py --bench-only
python tests/hil/compare_tx_allowlist.py evidence/latest/audio-tx.log
```

## 차량 송신 금지

이 task의 조사 단계는 read-only다. G4에서 ECU/head-unit simulator bench가 통과하기 전 실제 차량에 frame을 송신하지 않는다.
