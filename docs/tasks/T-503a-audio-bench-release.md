# T-503a OEM audio bench TX와 기능별 폐쇄시험

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G4 bench TX → G5 기능별 폐쇄시험`
- 선행: `T-503`, `T-106`, `T-305`, `T-500`, `T-508`
- 외부 선행: head-unit/ECU simulator와 승인된 폐쇄시험 환경·운전자

## 목표

T-503의 read-only source evidence로 구현한 audio executor를 실제 feedback·partial apply·restore·manual override까지 검증한다. 순수 수신 조사와 송신 release를 분리해 실행 순환을 제거한다.

## 고정 결정

[통합 설계 §10·12](../architecture/implementation-readiness.md)와 [자동화](../architecture/automation.md)를 따른다. volume/fader/balance/mute/rear mute/허용 SDVC·restore의 의미 범위만 허용하며 임의 raw TX·미확인 periodic owner spoof는 금지한다.

## 구현 범위

- T-503 source manifest→generated builder→STM local safety→head-unit feedback end-to-end
- 취침/뒷좌석 강화·baseline/offset·SDVC·source/call/reverse·manual override matrix
- partial state/복원 snapshot TTL·revision·ignition·reboot/power-cut·terminal result audit
- 기능별 bench 합격 후 opt-in 폐쇄시험과 정상/중단/미지원 UX

## 범위 밖

음량 적응 알고리즘 release(T-504), 미확인 audio owner injection, 일반도로 실험.

## 예상 변경 파일

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
tests/hil/run_audio_profile_matrix.py
tests/hil/compare_tx_allowlist.py
tests/audio/
vehicle-profiles/tucson-tl-2017/
```

## 수용 기준

- [ ] G4 시작 전 generated VERIFIED manifest·BENCH_TX build·hardware gate·lease·local snapshot을 확인하고 각 frame 직전 재검사한다.
- [ ] 허용 mask 밖 bit/ID, duplicate execution, partial failure의 silent success가 0건이다.
- [ ] call/reverse/manual override·ACK/RESULT 유실·stale feedback·snapshot 소실·각 step 단전에서 원상복원 가능/불가능을 정직하게 보고한다.
- [ ] 기능별 G4 evidence가 없는 항목은 G5에서 실행하지 않는다. T-507 보안 provisioning 미완료 차량 TX를 금지한다.
- [ ] G5는 실제 물리 button 동작과 비교하며 이전 audio 상태 복원·운전자 override·feature opt-in을 개별 확인한다.
- [ ] audio G5 통과를 adaptive volume(T-504)·SPORT(T-505)·제품 전체 완료로 확대하지 않는다.

## 검증 계획

이 task에서 위 HIL scripts를 생성하고 negative allowlist fixture를 먼저 실행한 뒤 bench matrix→기능별 폐쇄시험을 수행한다. public fixture는 합성/승인 익명화본만 사용한다.

## evidence와 rollback

profile/evidence/build digest·TX/feedback trace·복원 matrix·feature별 gate를 기록한다. 실패 시 기능 capability를 off로 두고 검증된 read-only firmware로 되돌린다.
