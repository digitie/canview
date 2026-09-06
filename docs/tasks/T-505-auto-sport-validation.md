# T-505 자동 SPORT monitor, HIL과 폐쇄시험

- 상태: `BLOCKED`
- 우선순위: `P2`
- Gate: `G4/G5`
- 선행: `T-106`, `T-305`, `T-505a`, `T-500`, `T-508`
- 병렬 가능: `T-504`

## 목표

속도 또는 중속 이상 급가속을 감지해 SPORT로 전환하고 속도가 충분히 내려가면 이전 mode로 복귀하는 기능을 monitor-only부터 단계적으로 검증한다.

## 고정 로직

- entry speed 선택 60/70/80 km/h; exit는 entry−15 km/h
- speed entry confirmation, exit confirmation, minimum SPORT hold
- acceleration은 중속 이상에서 median-filtered longitudinal value와 hysteresis 사용
- brake, reverse/non-forward, ABS/TCS/ESC, stale signal, mode fault, link/lease 문제에서 inhibit
- 실제 운전자 mode 변경은 manual hold로 우선
- previous mode raw 값을 직접 ECU에 쓰지 않고 검증된 button event를 feedback마다 한 번씩 사용
- safety input freshness ceiling 300 ms, tick gap 250 ms 초과 시 dwell reset과 pulse 금지
- `DISABLED/MONITOR_ONLY/ARMED_TX/PENDING/ACTIVE/INHIBITED` 분리, reboot 기본 MONITOR_ONLY
- entry setter는 wire 600/700/800만 받아들이고 그 밖의 값을 clamp하지 않고 무변경 거부
- feedback timeout 1,500 ms는 first physical TX-complete 기준

## 단계

1. capture replay와 pure C state machine
2. 실제 주행 `MONITOR_ONLY`: proposed action만 log, TX 0
3. HIL에서 button/feedback/timeout/fault
4. 정차 bench의 한 pulse
5. 폐쇄시험장에서 기능별 opt-in

## 수용 기준

- [ ] MONITOR_ONLY analyzer에서 TX 0건이다.
- [ ] threshold 주변 속도/가속도 noise에서 chatter가 없다.
- [ ] SPORT 진입 전 previous mode를 한 번 snapshot하고 external change에서 ownership을 놓는다.
- [ ] exit는 minimum hold와 confirm을 모두 만족해야 한다.
- [ ] feedback 없는 반복 pulse가 command limit을 넘지 않는다.
- [ ] 모든 inhibit/fault에서 새 pulse 0건이다.
- [ ] T-505a에서 해당 차량에 실제 확인한 mode만 이전 mode로 복귀하고 미관측 ECO/SMART/COMFORT 등을 있다고 가정하지 않는다. unknown은 자동화 disable이다.
- [ ] stale signal이나 scheduler stall이 entry/exit confirmation을 충족시키거나 복귀 pulse를 만들지 않는다.
- [ ] enable만으로는 action/TX 0건이고 explicit arm+G5 capability 뒤에만 pulse가 가능하다.
- [ ] 590/650/730/750/810은 거부·state 불변, 600/700/800만 성공한다.

## 검증

```bash
ctest --preset host-sanitize -R auto-sport --output-on-failure
python tests/vehicle/replay_sport_monitor.py private/evidence/drive/*.cvtrace
python tests/hil/run_auto_sport_matrix.py --bench-only
```

## release

기본 off, opt-in이다. public road에서 첫 TX 시험을 하지 않는다. 폐쇄시험 evidence와 rollback 확인 전 G5를 통과시키지 않는다.

## 산출물·범위 경계

- 위 단계와 inhibit/복귀 matrix가 구현 범위다. 산출물은 STM SPORT·Controller arm 통합·replay/HIL scripts·feature report다. 다른 ECU 제어·미확인 mode·일반도로 첫 TX는 범위 밖이다.
- T-508 포함 작업대/보안 gate 뒤에만 새 OTA firmware로 차량 시험한다. 실패 시 disarm/monitor-only로 남고 previous mode feedback 없이 복귀 완료로 표시하지 않는다.
