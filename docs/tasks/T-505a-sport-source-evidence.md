# T-505a SPORT와 이전 mode의 read-only 근거 검증

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G3 read-only`
- 선행: `T-501`, `T-403`, `T-500`

## 목표

SPORT 제어 전 physical button·mode feedback·수동 override·안전 입력의 수신 근거를 확보한다. T-106이 필요로 하는 source evidence를 후속 T-505 송신 시험과 분리한다.

## 고정 결정

[자동화 SPORT](../architecture/automation.md), [차량 신호 목록](../vehicle/signal-catalog.md)과 [통합 설계 §9–10](../architecture/implementation-readiness.md)을 따른다. pulse로 원하는 mode에 도달한다는 추정이나 DBC 이름만으로 builder를 VERIFIED 처리하지 않는다.

## 구현 범위

- 실차 물리 버튼 조작의 frame owner/counter/checksum/bit mask·mode 전이·feedback 조사
- SPORT 진입 전 NORMAL/ECO 등 실제 enum과 이전 mode 복귀 가능성·수동 override 분류
- local safety 입력 speed/gear/reverse/brake/ESC/torque/전원 및 freshness의 실제 가용성 조사
- T-106/T-505가 사용할 승인된 evidence manifest·offline vectors·HIL 기대 시나리오

## 범위 밖

CAN 명령 송신, 자동 SPORT arm, HIL/G5 최종 승인(T-505), raw replay.

## 예상 변경 파일

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
tests/sport/analyze_capture.py
tests/sport/fixtures/
vehicle-profiles/tucson-tl-2017/
docs/vehicle/signal-catalog.md
```

## 수용 기준

- [ ] 반복 ignition cycle·positive/negative control·물리 조작 annotation으로 mode/owner/counter/checksum을 검증하고 승인 서명 없이는 candidate로 유지한다.
- [ ] 실제로 관측한 이전 mode만 저장하며 unknown/stale/미관측 enum에서 복귀 pulse를 추측하지 않는다.
- [ ] 각 safety 입력은 AVAILABLE/미지원/미확인과 unit/freshness를 기록한다. 필수 입력 부재는 기능 disable 조건이지 가짜 기본값의 근거가 아니다.
- [ ] generated profile용 source evidence와 실제 주입 frame의 G4/G5 검증은 분리한다. 이 task에서는 CAN TX0이다.
- [ ] 버튼 one-shot인지 periodic owner인지, pulse 횟수/최소 간격/feedback deadline/중단 시 상태를 근거로 정하고 실패 시 T-505를 차단한다.

## 검증 계획

이 task에서 offline analysis와 signed evidence validation fixture를 만들고 TX gate off인 수신 capture만 분석한다. `tests/sport/analyze_capture.py --no-transmit` 경로를 구현하고 analyzer의 TX0 증거를 별도로 남긴다.

## evidence와 rollback

capture digest·차량/ignition 조건·source approval·전이/안전 입력 표를 남긴다. 근거가 부족하면 후보를 보존하고 release gate를 닫는다.
