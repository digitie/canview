# T-105 STM32 generated safety profile runtime

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G3/G4`
- 선행: `T-006`, `T-104`
- 병렬 가능: Controller read-only pipeline

## 목표

vehicle profile generator가 만든 최소 signal decoder와 안전 상태를 STM32에서 실행한다. 화면용 catalog와 분리하고 command 전제조건의 유일한 차량 사실 원천으로 사용한다.

## 구현 범위

- generated profile registration/digest 검증
- speed, gear, brake, ABS/TCS/ESC, drive mode, ignition/awake 등 safety signal store
- signal별 freshness/range/evidence 검사
- `state_revision`과 safety inhibit bitset
- SPORT state machine 입력 adapter
- command별 immutable required precondition known-mask와 recheck phase table
- audio command feedback/snapshot interface skeleton
- UART `SAFETY_SNAPSHOT`과 ESP-NOW state snapshot 자료

## 고정 규칙

- profile digest가 build manifest와 다르면 TX capability는 0이다.
- runtime valid이더라도 evidence가 VERIFIED가 아니면 command 전제조건에 사용할 수 없다.
- stale threshold는 signal profile에 있고 handler가 임의 기본값을 만들지 않는다.
- derived signal은 dependency의 최저 품질/evidence를 넘지 않는다.
- observer/filter queue가 가득 차도 safety decode는 계속된다.

## 수용 기준

- [ ] approved raw fixture에서 reference decoder와 exact value/quality가 일치한다.
- [ ] 각 safety signal stale/out-of-range/fault가 해당 command를 거부한다.
- [ ] profile mismatch/unknown vehicle에서 control scope와 TX가 0이다.
- [ ] state 변경마다 revision이 한 번 증가하고 동일 sample 재처리에는 증가하지 않는다.
- [ ] snapshot이 current STM boot ID와 profile digest를 포함한다.
- [ ] candidate-only Tucson profile build는 command table 0개다.
- [ ] sender의 zero/partial/unknown precondition mask가 generated 필수조건을 생략하지 못한다.
- [ ] admission, dequeue와 pulse 직전 safety 변화가 각각 이후 TX를 막는다.

## 검증

```bash
python tools/generate_vehicle_profile.py vehicle-profiles/tucson-tl-2017/profile.yaml --check
ctest --preset host-sanitize -R 'safety-profile|state-revision' --output-on-failure
```

## evidence

각 inhibit reason의 golden vector와 profile report를 저장한다. 실차 capture가 없으면 runtime framework까지 merge할 수 있지만 Tucson command enable은 별도 blocked 상태로 남긴다.
