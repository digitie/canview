# T-503 OEM audio command와 feedback 수신 조사

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G3` 수신 조사; bench 송신·복원은 T-503a
- 선행: `T-501`, `T-403`, `T-500`
- 후속: `T-106`, `T-503a`

## 목표

취침 mode, 뒷좌석 강화, 상대 volume offset과 OEM snapshot 복원에 필요한 frame owner, alive counter, checksum, feedback을 수신 조사와 offline 비교로 검증한다. 실제 bench 송신·복원 재현은 T-503a가 담당한다.

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
- [ ] physical operation과 offline generated frame의 byte/mask 비교가 일치한다. 실제 bench TX 비교는 T-503a에서 수행한다.
- [ ] unrelated payload bit가 바뀌는 frame이 0개다.
- [ ] partial profile failure와 reverse/call/manual override의 기대 복원 matrix·timeout·scope를 정의하고 T-503a로 전달한다.
- [ ] snapshot revision/TTL과 ignition invalidation 규칙이 test로 고정된다.
- [ ] vehicle profile generator가 VERIFIED evidence 없이는 audio command를 생성하지 않는다.

## 검증

```bash
python tests/audio/analyze_owner_counter.py private/evidence/audio/*.cvtrace
python tests/audio/check_capture_vectors.py --no-transmit
```

## 차량 송신 금지

이 task 전체는 read-only다. 현재 없는 두 audio 분석 script는 이 task의 산출물이다. 실물 bench 명령·restore 시험은 T-106 이후 T-503a에서 수행하며 조사 완료가 차량 송신 승인이 아니다.

## 결정 변경 기록

2026-09-06 계획 감사: T-106이 필요했던 기존 bench acceptance를 T-503a로 옮겨 조사→executor→bench 순서를 만들었다. wire/profile 권한은 변경하지 않는다.

## 산출물·범위 경계

- 위 조사 항목과 두 audio 분석 script·합성 offline fixture·source manifest가 산출 범위다. CAN bench/차량 TX는 범위 밖이며 비교 실패 시 candidate로 유지한다.
