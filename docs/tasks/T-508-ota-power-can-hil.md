# T-508 OTA-08 전원 차단·Flash·CAN 통합 HIL

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2 OTA qualification / OTA-08`
- 선행: `T-507`, `T-500`, `T-101`
- 외부 선행: 모든 지원 hardware variant 실물, power-cut rig·scope·CAN analyzer

## 목표

OTA 설계의 복구와 CAN 차단 주장을 실물 고장 주입으로 검증한다. 이 gate는 OTA 후 차량 연결 전 작업대 증거이며 기능별 G4/G5 승인과 별개다.

## 고정 결정

[OTA §12](../architecture/ota.md)의 고장 주입 표 전체가 수용 기준이다. deterministic cut point 전수와 variant별 무작위 1000회 이상을 수행한다. 정적 Boolean/ERC/host fixture는 아날로그·Flash 측정을 대체하지 않는다.

## 구현 범위

- T-500 rig adapter에 OTA cut point·journal/digest 수집·전원/RESET/CAN scope 동기화 추가
- ESP 4 KiB erase/write·otadata, STM 2 KiB erase/8B write·swap/trailer/confirm 전후 시험
- activation/floor/config/policy·same-version REPAIR·PSRAM/normal NVS 손상 복구
- J31/J32·GPIO7/48·stale ARM·독립 rail/USB hot-plug·PHY dominant 측정

## 범위 밖

실제 차량 fault injection·G5 기능 승인·모든 물리 고장에 대한 수학적 보장·무인 fleet OTA.

## 예상 변경 파일

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
tests/hil/run_ota_faults.py
tests/hil/assert_ota_recovery.py
tests/hil/fixtures/ota/
docs/hardware/r1/verification.md
```

## 수용 기준

- [ ] OTA §12 각 행을 fixture ID·variant·cut point·기대 상태·실제 digest·CAN 증거와 1:1 매핑하고 누락 행을 실패로 처리한다.
- [ ] 전원 차단 후 유효한 앱 또는 명시 recovery로 수렴하고 brick/reset loop/ECC/NMI/재업로드 필요 횟수·복구 시간을 숨기지 않는다.
- [ ] GPIO48 고착도 J31 원신호를 역구동하지 않으며 J31 재삽입 뒤 새 ARM 없이는 TX0이다. reset/rail collapse 중 dominant 폭도 실측한다.
- [ ] PREPARED에서 승인 없는 설치0, activation commit 후 같은 plan만 진행, policy 불명확 시 RECOVERY_LOCKED, 동일 digest REPAIR의 새 승인·확정을 확인한다.
- [ ] phone/AP 단절·CTS/UART hang·한쪽 MCU reset·candidate panic에서 실제 조합을 보고하고 미완료를 성공으로 표시하지 않는다.
- [ ] 변형별 deterministic 전수+무작위≥1000회 완료 여부를 보고한다. 일부 variant 미시험이면 그 variant의 OTA/차량 gate는 닫힌다.
- [ ] 일반 앱 두 slot 손상·PSRAM 불량·NVS 손상에서 독립 recovery AP와 CAN TX0을 확인하고 첫 제조/bootloader 물리손상의 보장 제외를 표시한다.

## 검증 계획

이 task에서 두 HIL script와 실패 로그 fixture를 만든 뒤 T-500 rig로 실행한다. raw trace는 private evidence에 보관하고 승인된 요약/digest만 공유한다. 장비 없으면 NOT_RUN과 영향 gate를 기록한다.

## evidence와 rollback

variant별 sample 수·random seed·cut-point coverage·Flash/CAN trace·복구 결과를 release manifest 입력으로 남긴다. 미해결 failure는 OTA 배포와 해당 차량 연결을 차단하며 이전 검증 image/유선 서비스로 제한한다.
