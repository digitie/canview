# T-301 Controller local allow-list, catalog와 freshness pipeline

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2/G3`
- 선행: `T-003`, `T-006`, `T-201`, `T-300`
- 병렬 가능: Diagnostic Bridge shell

## 목표

ESP-NOW CAN batch를 secure session 뒤에도 local default-deny allow-list로 제한하고 generated catalog로 decode해 품질·freshness를 계산한다. 새 signal은 Communicator firmware 수정 없이 profile/catalog만으로 추가한다.

## 구현 범위

- ESP-NOW decoded frame→local admission queue
- local desired/effective allow-list와 NVS transaction
- upstream peer subscription sync adapter
- generated descriptor catalog load/digest/revision
- Intel/Motorola/signed/range decoder integration
- decoder API의 source timestamp/STM boot/catalog revision과 saturating age 계산
- RX DATA-only 기본 frame-kind admission; RTR/error/TX echo는 일반 signal에서 제외
- signal store, sample time, age, stale/unavailable transitions
- derived signals와 quality/evidence propagation
- session/profile/catalog change atomic flush
- display model producer API

## 고정 변경 순서

- filter 축소/삭제: local deny를 먼저 적용
- 확대/추가: local staging 후 upstream 수락과 matching catalog digest가 있어야 effective
- session/STM boot/catalog revision 변경: 이전 decoded state를 새 model과 섞지 않음

## 수용 기준

- [ ] empty/corrupt filter store가 default-deny다.
- [ ] peer subscription을 우회해 보낸 frame도 local filter에서 거부된다.
- [ ] 32 slots, batch 8, period/count/byte budget이 모두 적용된다.
- [ ] generated raw vectors가 reference decoder와 일치한다.
- [ ] candidate/observed signal이 운전자 output topic에 나오지 않는다.
- [ ] speed/RPM 포함 모든 signal이 stale threshold 뒤 `—` model이 된다.
- [ ] session/profile/catalog change 도중 mixed revision snapshot이 0개다.
- [ ] delayed/reordered/replayed/이전 STM boot record가 age 0 또는 fresh로 나오지 않는다.
- [ ] 같은 ID의 RTR/error/TX echo가 일반 signal 값·freshness를 갱신하지 않는다.
- [ ] local filter batch 한 entry 오류·power cut·revision 경쟁에서 old/new 전체 상태 중 하나만 보인다.

## 검증

```bash
ctest --preset host-sanitize -R 'controller-can|catalog|freshness' --output-on-failure
python tests/profile/cross_decode.py vehicle-profiles/tucson-tl-2017/profile.yaml
python tests/protocol/property_local_allowlist.py --seed 1 --cases 10000
```

## rollback

NVS transaction 실패나 catalog digest mismatch에서는 이전 wider allow-list로 복귀하지 않고 effective store를 default-deny로 내린다. UI는 link가 있어도 데이터 unavailable로 표시한다.
