# T-205 OTA-06 호환성·영속 정책과 설정 migration

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2 / OTA-06`
- 선행: `T-306`, `T-108`, `T-304`, `T-105`

## 목표

두 MCU 비원자 업데이트와 세 ESP 설정 보존을 실제 정책으로 구현한다. 승인·확정·downgrade floor·rollback을 하나의 성공 flag로 합치지 않는다.

## 고정 결정

[OTA §7.1·8.1·9·11](../architecture/ota.md)이 저장/설치 정책 정본이다. config A/B는 권위 snapshot이고 normal NVS는 비권위 cache다. STM MCUboot journal은 별도다. control lease와 SPORT arm은 reboot 뒤 자동 복원하지 않는다.

## 구현 범위

- host-testable ota_manager 상태기계, role별 flash/crypto/reset adapter와 single journal writer
- policy-v1 A/B generation/CRC/commit marker·3072B record limit·CONFIRM_INTENT와 floor 조정
- immutable activation plan·4가지 ESP/STM 조합·중간 호환 release 경로
- old/new config snapshot 선택·pairing/권한 보존·storage endurance·REPAIR/RECOVERY_LOCKED

## 범위 밖

hardware monotonic anti-rollback·OTA eFuse epoch 변경·private signing key 관리 실행·차량 기능 승인.

## 예상 변경 파일

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
shared/ota/
config/schema/ota-policy-v1.yaml
config/schema/
firmware/*/components/canview_ota_manager/
firmware/communicator/stm32/
tests/ota/test_policy.py
tests/ota/test_compatibility.py
```

## 수용 기준

- [ ] old/old·new/old·old/new·new/new ESP/STM 조합과 Controller/Bridge old/new에서 관리/recovery ABI·capability 교집합을 검사한다.
- [ ] activation intent write/readback/commit 이전에 boot 변경0이며 torn intent·cancel/activate 동시 요청·ACK 유실·재부팅에서 승인 plan이 바뀌지 않는다.
- [ ] CONFIRM_INTENT와 실제 image confirmation 사이 모든 cut point에서 floor를 안전하게 완성한다. reconciliation 이전 update API/service exit를 열지 않는다.
- [ ] floor 미만 및 같은 sequence 다른 digest를 거절한다. policy만 정상이고 앱이 손상되면 ALREADY_INSTALLED가 아니라 같은 정식 digest의 REPAIR와 새 승인/시험부팅을 요구한다.
- [ ] 한 policy copy 손상은 이전 valid generation, 양쪽 손상은 RECOVERY_LOCKED로 수렴하고 floor0/키 초기화/자동 차량 제어로 탈출하지 않는다.
- [ ] 후보 확정 전 원래 config schema를 파괴하지 않고 rollback이 이전 snapshot을 읽는다. NVS init 실패에 전체 erase를 호출하지 않는다.
- [ ] 설정 owner/revision·pair root·calibration migration과 16 KiB snapshot/쓰기 빈도 budget을 검사한다. 넘치면 정본 결정 없이 영역을 확대하지 않는다.
- [ ] 두 MCU의 실제 confirmed digest가 일치할 때만 bundle 완료로 기록하며 한쪽 성공을 임의 rollback 표시하지 않는다.

## 검증 계획

이 task에서 policy/compatibility/config state-machine tests를 만들고 모든 영속 write/readback/commit 경계의 deterministic power-cut fixture를 실행한다. 실제 Flash/circuit cut point 최종 증거는 T-508이다.

## evidence와 rollback

전이 coverage·각 journal byte fixture·config 보존 digest·실패 결과를 남긴다. 불명확한 상태는 recovery 격리하며 공장 reset을 성공 복구로 대체하지 않는다.
