# T-203 peer별 subscription, quota와 observer union

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2`
- 선행: `T-201`, `T-202`, `T-103`, `T-204`
- 후속: `T-301`, `T-401`, `T-501`

## 목표

Controller와 Diagnostic Bridge가 필요한 CAN record만 각각 받도록 per-peer subscription을 구현하고, STM32에는 bounded union plan을 원자적으로 적용한다.

## 고정 model

- Communicator store key: `(peer_device_id, subscription_revision)`
- Controller local allow-list와 Communicator subscription은 별도 store다.
- peer당 32 filters, 설정 batch 8, STM union 64 entries
- aggregate 20 kB/s sustained, 32 kB burst
- P0/P1 2 kB/s, Controller runtime 최소 8 kB/s reserve
- stats/raw capture는 P4와 남은 token만 사용

## 구현 범위

- ADD/REPLACE/DELETE/CLEAR optimistic transaction
- desired/effective revision과 exact accepted quota response
- union normalize/deduplicate, fastest period 선택
- UART observer plan BEGIN/CHUNK/COMMIT/ABORT
- incoming raw record의 peer별 재필터와 token bucket
- persistent Controller default subscription, Bridge lease-bound temporary subscription
- status/drop/high-water counters
- filter snapshot ID/revision/part index/count/total count와 all-parts commit
- 설치 전체 scheduler와 P0/P1 2 kB/s·Primary 8 kB/s non-borrowable reserve

## 안전 규칙

- 한 peer의 wide mask가 다른 peer의 effective allow-list를 넓히지 않는다.
- Bridge filter는 diagnostic lease 만료 시 제거한다.
- Controller subscription은 대역폭 최적화일 뿐 Controller local trust boundary를 대체하지 않는다.
- union commit 실패 시 이전 plan을 유지한다.

## 수용 기준

- [ ] 두 peer의 겹침/비겹침 filter property test에서 정보 누출이 없다.
- [ ] 64 entry 초과 요청이 partial apply 없이 거부된다.
- [ ] missing/reordered chunk가 active union revision을 바꾸지 않는다.
- [ ] Bridge flood 중 Controller 8 kB/s와 P0/P1 reserve가 유지된다.
- [ ] effective quota가 요청값과 다르면 response/UI에 정확히 드러난다.
- [ ] reboot 뒤 stale revision과 lease-bound filter가 복원되지 않는다.
- [ ] filter mask/DLC/bus/flags fuzz가 out-of-bounds를 만들지 않는다.
- [ ] snapshot fragment loss/reorder/duplicate/동시 revision 변경에서 hybrid snapshot이 commit되지 않는다.
- [ ] read-only+Bridge flood에서도 설치 합계 20 kB/s/32 kB burst와 reserved traffic이 동일하게 적용된다.

## 계획 보완 수용 기준

- [ ] CRUD의 존재/비존재 ID·빈 CLEAR·혼합 invalid batch·expected revision 0·revision 소진과 중복 요청을 schema가 정한 결과로 검사한다. GET은 변경하지 않는다.
- [ ] 20 ms/60 s·1/32 record 경계, 중첩 filter 중복 delivery/count 차감, stream-period/burst/초당 byte budget을 독립적으로 시험하고 requested/effective/count/drop을 관찰한다.
- [ ] 영속 default는 configuration만 복구한다. session/lease/token bucket·소모 count·effective plan은 snapshot 재협상 후 생성하며 Bridge 임시 filter는 부팅 후 비어 있다.
- [ ] T-204의 실제 config A/B layout과 bounded storage I/O를 사용한다. peer default serializer는 이 task, Controller filter/config serializer는 T-301/T-304가 소유하며 후속 UI/migration 완료를 이 task의 선행으로 요구하지 않는다.
- [ ] 센서 8,192 B/s와 raw/ACK/retry/remote config를 설치 합계에 넣어 T-100b 통합 시 예약 대역폭을 초과하지 않는다.

## 검증

```bash
ctest --preset host-sanitize -R 'filter|subscription|quota|observer-plan' --output-on-failure
python tests/protocol/property_peer_isolation.py --seed 1 --cases 10000
python tests/hil/run_peer_fairness.py --controller-rate 8000 --bridge-flood
```

## rollback

subscription state가 불명확하면 모든 peer raw stream을 default-deny하고 HELLO/snapshot부터 다시 협상한다. 오래된 union plan을 임의 추정해 복원하지 않는다.


## 산출물·범위 경계

- 예상 산출물은 Communicator subscription store/union scheduler, STM observer-plan adapter와 이 문서의 property/HIL scripts다. Controller local allow-list를 제거하거나 observer에 control 예산을 빌려주는 기능은 범위 밖이다.
- namespace별 requested/effective revision·quota·drop trace를 evidence로 남긴다. 센서/일반 raw를 합친 부하 fixture도 별도 등록한다.
