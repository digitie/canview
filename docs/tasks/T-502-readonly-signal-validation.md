# T-502 4WD, TPMS, DPF, 연비와 주행 신호 read-only 승격

- 상태: `BLOCKED`
- 우선순위: `P1`
- Gate: `G3`
- 선행: `T-501`, `T-301`, `T-403`
- 병렬 가능: `T-503`

## 목표

운전자 기본 화면에 필요한 신호를 실차 evidence로 VERIFIED 승격하고 정상·stale·fault UI를 완성한다.

## 우선순위

1. speed, RPM, battery voltage, transmission clutch lock, transmission temperature, engine temperature
2. 4WD rear coupling/clutch torque 후보와 바퀴별 drive index
3. TPMS FL/FR/RL/RR와 warning mapping
4. instant fuel economy
5. DPF lamp; soot/load/regeneration은 별도 진단 근거가 있을 때만
6. speed limit, tail/headlamp, dimmer, drive mode feedback

## 검증 방법

- 최소 3 ignition cycle, independent action repetition, negative control
- OEM cluster/scanner/physical gauge와 같은 시간축 비교
- unit, sign, byte order, wheel order, invalid raw, range, period, stale threshold 확인
- derived 4WD index는 실제 wheel torque percent로 이름 붙이지 않음
- DPF lamp off를 regeneration complete로 해석하지 않음

## 수용 기준

- [ ] 각 승격 signal에 evidence ID, raw vector, unit/range/freshness가 있다.
- [ ] TPMS wheel order는 한 바퀴씩 압력 변화로 확인된다.
- [ ] 4WD 표시가 검증된 물리량과 추정값을 명확히 구분한다.
- [ ] DPF unavailable field가 demo 숫자를 표시하지 않는다.
- [ ] stale/fault/unavailable에서 마지막 숫자를 고정하지 않는다.
- [ ] 경고 없는 기본 화면과 각 fault/과속/야간 screenshot이 있다.
- [ ] candidate 값은 운전자 화면 output에서 차단된다.

## 계획 보완 수용 기준

- [ ] 미션 lock과 미션 온도는 엔진 온도/4WD LOCK과 분리해 OEM scanner·실측에 대조한다. direct lock 신호 부재 시 RPM/slip 추정을 확정 lock으로 승격하지 않는다.
- [ ] 순간연비의 km/L·L/100 km·단위 enum·공회전·연료차단·0·invalid/max sentinel을 대조하며 injector 추정으로 빈칸을 채우지 않는다.
- [ ] SPORT용 gear/brake/ABS/TCS/ESC/longitudinal acceleration/drive mode의 valid·status·freshness·manual override 근거도 T-505a에 제공한다. GNSS/INS를 CAN safety 대신 쓰지 않는다.
- [ ] 정상 lamp off는 검증된 lamp 의미만 나타내며 soot/load/regen 또는 모든 DPF 정상을 보장하지 않는다. 범위 내 미지원 항목은 이유·추가 evidence gate를 남긴다.

## 검증

```bash
python tools/validate_cvtrace.py private/evidence/<capture>.cvtrace
python tools/generate_vehicle_profile.py vehicle-profiles/tucson-tl-2017/profile.yaml --check
python tests/profile/cross_decode.py vehicle-profiles/tucson-tl-2017/profile.yaml
python tests/ui/capture_screens.py --profile tucson-tl-2017 --all-quality-states
```

## 결과 문서

`docs/vehicle/signal-catalog.md`, `docs/vehicle/target-2017-tucson.md`, profile report와 UI screenshot을 함께 갱신한다.

## 산출물·범위 경계

- 위 신호별 read-only 검증 matrix가 구현 범위다. 산출물은 `tests/vehicle/` 분석/부정 fixture, profile/evidence manifest와 `docs/vehicle/signal-catalog.md`다. OBD/UDS query·DPF 강제 재생·candidate 정상 UI는 범위 밖이다.
- 미증명 신호는 미확인/미지원으로 남기고 해당 기능 gate를 닫는다. 미지원 사유는 release 제한으로 추적한다.
