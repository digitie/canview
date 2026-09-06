# T-507 OTA-07 제조 provisioning과 유선 복구 검증

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2 보안 / OTA-07`
- 선행: `T-205`, `T-201`, `T-101`
- 외부 선행: 폐기 가능한 샘플, 안정된 작업대 전원·유선 장비, 사용자 승인된 제조 보안 정책

## 목표

Secure Boot/Flash encryption·STM 보호·장치별 key·복구 절차를 실제 샘플에서 입증한다. 보호를 켰다는 설정값만으로 production 또는 모든 유선 복구를 보장하지 않는다.

## 고정 결정

[OTA §4·5·7·11](../architecture/ota.md), [구현 준비 §7](../architecture/implementation-readiness.md)을 따른다. ESP hardware anti-rollback은 n, epoch는 제조 고정이다. role별 signing trust와 pair별 link/control root, device-local PMK를 분리한다.

## 구현 범위

- 최초 wired install manifest·option-byte/eFuse readback·제조 sample 절차와 recovery runbook
- 권위 provisioning A/B·고유 AP label 자격·두 사본 손상 시 통제된 복구 정책 확정
- ESP secure download/JTAG, STM RDP/WRP/DBANK/NRST·ROM J32 허용 조합 시험
- 키 rotation·해제/교체·폐기 정책, secret 없는 audit receipt

## 범위 밖

승인 없는 보안 fuse 쓰기·양산·키 외부 공개·G5 차량 시험. OTA가 bootloader/provisioning을 갱신하는 기능.

## 예상 변경 파일

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
tools/provisioning/
config/provisioning/
docs/runbooks/ota-recovery.md
tests/ota/test_provisioning.py
```

## 수용 기준

- [ ] 실행 전 사용자 승인과 폐기 가능한 샘플을 확인한다. 비가역 eFuse/RDP/WRP 변경을 일반 개발 보드에 자동 실행하지 않는다.
- [ ] device별 AP credential·PMK와 pair별 root를 검사하고 설치 전체 공유 secret 또는 Bridge의 control_root 보유를 거절한다.
- [ ] provisioning 두 사본 손상 시 label secret 입력/작업대 복구 중 실제 채택 정책을 위협 모델·recovery 자원 budget과 함께 확정한다. 무암호 AP 자동 전환은 금지한다.
- [ ] ESP/STM 보안 설정·실제 signed boot·임의 이미지 거절·정상 앱 양쪽 손상 복구를 샘플별로 검증한다.
- [ ] J31/J32/RECOVERY/PAIR/RESET·USB 단독 PHY 무전원·unplug 순서를 사용자 절차로 시험하며 유선 복구가 보호 설정상 불가능하면 교체 조건을 명시한다.
- [ ] 제조 중 단전과 key/record mismatch를 정상 완료로 표시하지 않고 quarantine한다. 로그/라벨 preview/fixture에서 private key·VIN·원본 capture를 제거한다.

## 검증 계획

host policy negative tests와 manifest 검사를 이 task에서 만든 뒤 승인된 sample에서 최초 설치·lock/readback·복구·단전 시험을 실행한다. toolchain과 보호 profile별 회복 가능/불가능 결과를 따로 기록한다.

## evidence와 rollback

샘플 revision·비밀 아닌 key ID·보호 readback·복구 로그를 남긴다. 비가역 보호에는 일반 rollback이 없으며 실패 sample 격리/교체가 rollback 경계다.
