# T-403 Signal Lab, candidate와 evidence export

- 상태: `BLOCKED`
- 우선순위: `P1`
- Gate: `G3`
- 선행: `T-006`, `T-402`
- 후속: `T-501`, `T-502`, `T-503`

## 목표

사용자가 행동 전후 변화에서 CAN signal 후보를 찾고 bit/endian/scale을 실험한 뒤 agent가 그대로 읽을 수 있는 evidence manifest로 export하게 한다. 후보를 자동으로 VERIFIED로 승격하지 않는다.

## 구현 범위

- 행동 template, repeated marker, baseline/action window
- ID/bit change map와 설명 가능한 correlation score
- generic bool/unsigned/signed/Intel/Motorola decoder
- diagnostic 전용 1–64 bit raw candidate decoder; 33 bit 이상은 operational export 금지
- factor/offset/unit/range/freshness candidate editor
- capture/marker/drop/gap linkage
- candidate revision/history/conflict
- profile YAML patch preview와 evidence manifest export
- Controller/reference decoder cross-check fixture 생성

## 고정 evidence 등급

`UNKNOWN → CANDIDATE → OBSERVED → VERIFIED`다. UI는 score만으로 승격하지 않는다. VERIFIED는 code review와 T-501/T-502 또는 T-503의 별도 acceptance가 필요하다.

## 수용 기준

- [ ] 동일 input/marker에서 score와 정렬이 deterministic하다.
- [ ] alive/checksum/background bit가 penalty 없이 상위 후보가 되지 않는다.
- [ ] dropped/gapped/incomplete capture가 VERIFIED export를 만들 수 없다.
- [ ] candidate 변경마다 source capture와 revision이 남는다.
- [ ] raw vector가 Bridge/Controller/reference decoder에서 일치한다.
- [ ] 새 ID/bit/scale이 Communicator firmware 변경을 요구하지 않는다.
- [ ] profile patch가 command section을 자동 생성하지 않는다.
- [ ] maintainer-signed evidence approval 없이 VERIFIED grade를 export하지 않는다.

## 검증

```bash
python -m pytest -q tests/signal_lab
python tests/signal_lab/run_known_fixture.py tests/fixtures/signal-lab
python tools/generate_vehicle_profile.py tests/fixtures/signal-lab/exported-profile.yaml --diagnostic-only
```

## 증거

서로 다른 3회 반복, negative control, expected bit map을 가진 synthetic fixture를 먼저 만든다. 실차 candidate report에는 확정 표현 대신 관찰 근거와 반례를 포함한다.
