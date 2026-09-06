# T-006 vehicle profile schema와 분리 generator

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G0`에서 generator, `G3`에서 실제 profile 승인
- 선행: `T-001`, `T-005`
- 병렬 가능: protocol runtime·hardware

## 목표

DBC 원본, 실차 evidence, Controller display catalog와 STM32 safety/command profile 사이를 재현 가능한 generator로 연결한다.

## 고정 결정

- `dbc/opendbc` 원본은 수정하지 않는다.
- profile YAML과 evidence manifest가 사람이 편집하는 정본이다.
- Controller와 STM32 산출물을 분리한다.
- `VERIFIED` 미만 command/feedback signal은 STM32 command artifact로 생성하지 않는다.
- Communicator ESP32에는 DBC bit layout이나 signal 이름을 생성하지 않는다.

## 구현 범위

- `vehicle-profiles/schema/profile.schema.json`
- `vehicle-profiles/tucson-tl-2017/profile.yaml` skeleton
- capture/evidence reference schema
- `tools/generate_vehicle_profile.py`
- Controller descriptor/filter C output
- STM32 safety signal/command builder C output
- profile report와 digest manifest
- same raw vector를 Controller/STM32/reference decoder로 교차 확인하는 fixture

## profile validation

다음을 오류로 처리한다.

- DBC SHA-256 또는 upstream commit 불일치
- duplicate signal/command ID
- bit field가 DLC 밖이거나 서로 모순됨
- non-finite factor/offset/min/max, min>max, invalid endian/value/enum, scale overflow
- unit/range/freshness/evidence 누락
- bus type과 transceiver 불일치
- unverified command, feedback, counter/checksum
- operational/safety signal의 33–64 bit field. v1.3에서는 diagnostic opaque candidate로만 허용
- Controller filter budget 32개 또는 STM observer plan 64개 초과
- derived signal dependency cycle
- operational descriptor가 RX DATA 외 RTR/error/TX echo를 암묵적으로 허용

## 수용 기준

- [ ] 같은 입력에서 byte-identical output이 생성된다.
- [ ] 생성물에 source/generator/profile digest가 있다.
- [ ] 후보-only Tucson skeleton은 Controller diagnostic catalog는 만들되 STM TX command를 0개 만든다.
- [ ] 의도적으로 evidence를 위조/누락한 fixture가 실패한다.
- [ ] Intel/Motorola/signed/range golden vector가 reference와 일치한다.
- [ ] 64-bit DBC candidate가 32-bit operational signal로 잘리지 않고 명시적으로 거부/격리된다.
- [ ] generated diff check가 CI에 연결된다.
- [ ] NaN/Inf, invalid enum, scaling overflow, 33/48/64-bit와 Motorola 경계 fixture가 명시적 reject/diagnostic-only 결과를 낸다.

## 검증 명령

```bash
python tools/generate_vehicle_profile.py vehicle-profiles/tucson-tl-2017/profile.yaml --check
python -m pytest -q tests/profile
cmake --build --preset host-debug
ctest --preset host-debug -R vehicle-profile --output-on-failure
```

## 증거와 rollback

실차 원본 capture 대신 승인된 최소 fixture와 SHA-256 reference만 public repo에 넣는다. 새 profile generation이 실패하면 이전 generated artifact를 수동 유지하지 않고 release를 중단한다.


## 산출물·범위 경계

- 위 구현 범위의 경로와 `tests/profile/` C/Python cross-decode fixture를 생성한다. 실제 capture 수집/승인(T-501/T-502/T-503/T-505a), upstream DBC 편집과 target TX는 범위 밖이다.
