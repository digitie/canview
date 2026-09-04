# T-504 주행소음 기반 음량 자동화 release

- 상태: `BLOCKED`
- 우선순위: `P2`
- Gate: `G4/G5`
- 선행: `T-303`, `T-305`, `T-503`
- 병렬 가능: `T-505`

## 목표

민감 주파수 대역의 소음이 일정 시간 유지될 때 volume offset을 한 단계씩 올리고, 벗어난 상태가 더 오래 유지될 때 자연스럽게 내리는 기능을 검증한다.

## 고정 control law

- road/balanced/wind band profile과 low/normal/high sensitivity
- attack 기본 5초, release 기본 12초
- offset step 사이 최소 interval과 최대 offset 2–4 step
- minimum speed, confidence, band excess hysteresis
- manual volume 변경 뒤 60초 hold
- command feedback 뒤 microphone evidence freeze
- reverse 안내, call/voice, audio priority, invalid FFT에서 pause/restore
- applied offset은 feedback으로만 갱신
- speed 300 ms, FFT 250 ms, audio feedback 500 ms freshness ceiling과 automation별 pending 1개
- tick gap 250 ms 초과 시 attack/release evidence reset, 해당 tick 명령 금지

## 구현 범위

- FFT feature→existing pure C state machine adapter
- source-volume baseline calibration
- semantic offset command transaction
- setting schema와 UI progress/current volume
- drive scenario log/replay tuning tool
- false positive와 oscillation metrics

## 수용 기준

- [ ] threshold 경계 noise에서 반복 up/down oscillation이 없다.
- [ ] attack 전에는 command 0, release 전에는 down command 0이다.
- [ ] 한 step씩 움직이고 feedback 없는 다음 step을 보내지 않는다.
- [ ] manual change/call/reverse/invalid FFT에서 즉시 pause한다.
- [ ] link loss나 feature off에서 OEM snapshot/offset 정책대로 bounded 복원한다.
- [ ] 음악 bass, blower, window-open, road surface matrix의 false action rate를 보고한다.
- [ ] stale/invalid 입력은 evidence를 누적하지 않고 새 up command를 만들지 않는다.
- [ ] pending terminal result와 matching feedback 전에는 다음 step 또는 applied offset 갱신이 없다.
- [ ] 운전자 평가에서 급격한 volume jump 없이 opt-in/off가 가능하다.

## 검증

```bash
ctest --preset host-sanitize -R adaptive-volume --output-on-failure
python tests/audio/replay_drive_scenarios.py tests/fixtures/audio-scenarios
python tests/hil/run_adaptive_volume.py --bench-only --all-inhibits
```

## release

기본값은 off다. G5 차량 평가 전에는 최대 offset을 2 step으로 제한하고 설정 확장은 별도 evidence로 승인한다.
