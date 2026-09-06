# T-501 2017 Tucson TL bus, bitrate와 connector discovery

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G3`
- 선행: `T-101`, `T-103`, `T-203`, `T-401`, `T-500`
- 후속: `T-502`, `T-503`, `T-505`

## 목표

대상 차량의 실제 논리 CAN1/2/3 매핑, 물리 bus type, bitrate, connector pin과 주요 ID 존재를 capture-only로 확정한다.

## 대상 식별

- 2017 Hyundai Tucson TL
- 2.0 diesel, 4WD, BlueLink
- 차량 고유 식별자는 private evidence에만 저장하고 public report에는 익명 vehicle ID를 사용한다.

## 안전 준비

- G2 pass, Communicator `CAPTURE_ONLY`, hard gate off
- termination DNP와 harness continuity/ground 확인
- current-limited fused power
- OEM service information 또는 실제 측정으로 connector 후보 확인
- 두 사람 check 또는 사진 증거로 CANH/CANL/BATT/GND mapping 확인

## capture matrix

ignition off, accessory, ignition on engine off, idle, 정차 switch 동작을 서로 다른 cycle에서 반복한다. 주행 capture는 정차 자료로 bus가 확정된 뒤 별도 승인한다. 각 capture는 bitrate scan을 active probing하지 않고 listen-only controller 설정으로 순차 확인한다.

## 수용 기준

- [ ] 각 논리 bus의 connector pin, transceiver type, bitrate와 termination 측정 근거가 있다.
- [ ] MAX3055 channel이 실제 125 kbps fault-tolerant bus임이 확인되지 않으면 disabled로 남는다.
- [ ] analyzer가 전체 과정에서 CAN TX/ACK 0건을 확인한다.
- [ ] 적어도 3개 ignition cycle에서 ID/rate inventory가 재현된다.
- [ ] opendbc 후보와 일치/불일치 ID가 모두 report에 있다.
- [ ] drop/gap/time uncertainty가 승격 기준 안이다.
- [ ] GPS 좌표/시간이 없다는 기존 결론을 새 관찰로 재검토하되 opaque field를 임의 decode하지 않는다.

## 검증

```bash
python tests/hil/run_can_capture.py --mode capture-only --channels 3 --vehicle-profile tucson-tl-2017
python tests/hil/assert_no_tx.py evidence/latest/can-analyzer.log
python tools/validate_cvtrace.py private/evidence/tucson-tl-2017/*.cvtrace
python tests/vehicle/compare_inventory.py --profile tucson-tl-2017
```

## 증거

`.cvtrace`, harness 사진/diagram, analyzer no-TX log, multimeter/scope 결과, bus mapping report를 private evidence bundle에 둔다. public profile에는 opaque evidence ID와 digest만 넣는다.

## rollback

bus type/bitrate/pin 중 하나라도 모순되면 해당 channel을 `UNKNOWN`/standby로 되돌린다. 다른 Hyundai 모델의 명칭으로 보완 추정하지 않는다.

## 산출물·범위 경계

- 위 capture matrix와 bus/harness 근거 수집이 구현 범위다. 산출물은 vehicle analysis scripts, 비공개 capture와 `docs/vehicle/target-2017-tucson.md`의 승인 요약이다. CAN TX·DBC 기반 bus 자동확정은 범위 밖이다.
