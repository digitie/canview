# resume.md

## 현재 작업

2026-09-13, [T-007 OTA-01](tasks/T-007-ota-container.md)은 `IN_PROGRESS`다.

- worktree: `F:/dev/canview-wt/t007-ota-container`
- branch: `codex/t007-ota-container`
- [PR #35](https://github.com/digitie/canview/pull/35): Draft
- [단순한 구현 우선](../AGENTS.md#2-작업-원칙): 고정 manifest와 순차 image만 사용한다.
  기존 SDK/부트로더를 재사용하고 범용 package framework를 만들지 않는다.

C99 CBOR·P256 prefix·typed manifest 뒤에 순차 image 길이/SHA-256 검사를
연결했다. offset 중복/누락, partial input/reset, hash/provider/cleanup 실패를
검사한다. body는 입력 chunk를 보존하지 않으며 성공 상태도 `HASHES_MATCHED`다.
native image signature·설치 승인이 아니며 Flash writer를 호출하지 않는다.

별도 STM native 검사기는 공식 MCUboot v2.4.0 image의 전체 hash/P256 서명과
protected metadata(board/role/layout/epoch/ABI/u64 sequence)를 대조한다. SDK/부트로더
target 연결은 아직 없으며 ESP native 검사는 다음 구현이다.

body 시작 전에 신뢰된 로컬 snapshot으로 구·신 ABI 네 조합, MCU별 boot/recovery,
hardware capability와 config 읽기 범위를 검사한다. 미확인 snapshot은 거부하며
Controller/Bridge의 존재 여부를 Communicator 복구 조건으로 추가하지 않는다.

로컬 version floor 검사도 body 시작 전 필수로 연결했다. 낮은 sequence와 같은
sequence/다른 hash를 거부하고, 실제 설치 증거로 ALREADY_INSTALLED/REPAIR_REQUIRED를
구분한다. 영속 copy/CONFIRM_INTENT 조정·실제 설치 검사 provider는 아직 없다.

현재 body/호환성/floor 시험은 모형1480건, 실제 P256+SHA-2561486건이다. floor C
비교3847건과 body 모형1480건은 ASan/UBSan 통과, 두 파일의 함수·행·분기100%다.
앞선 manifest.c coverage는 함수100%·행97.72%·분기93.70%이며 typed P2561425건도 유지한다.
현재 source의 Windows Host Debug/Release는 각각131/131을 통과했다. STM native의
실제 CNG 시험2287건·비암호 모형 ASan/UBSan2284건이 통과했고 native_stm.c coverage는
함수100%·행98.14%·분기94.74%다. DER 길이에 따라 byte 변이/절단 건수는 달라진다.
Cortex-M4 freestanding object compile은 통과했지만 OTA target 통합의 증거는 아니다.
마지막 확인된 성공 CI는 `c0de352`의 `34730820401`이며, 그 결과를 이후
수정 source에 적용하지 않는다. target artifact/hash의 별도 대조도 아직 하지 않았다.

[CBOR checkpoint 리뷰](reviews/adversarial/2026-09-13-T-007-cbor.md)의 A/B static
PASS는 `6d83962` 범위에만 해당한다. 이후 서명/manifest 구현과 전체 T-007의
최종 독립 2인 리뷰는 남아 있다. 두 기존 reviewer는 완료 결과를 보존한 뒤 종료했다.

## 다음 한 작업

ESP native SDK 검증 연결 전, 미배포 컨테이너의 정렬과 포맷 revision을 명시한다.
SDK 전체 verifier는 mapped segment의64KiB 정렬을 요구한다. 작은 prefix 뒤/각 image
앞의 zero padding을 chunk로 검사하면 SDK를 재사용하고 별도 ESP parser를 피할 수 있다.
실제 CI BIN 세 개로4KiB 정렬의 불충분함과64KiB 조건을 재현했다. 상세 근거·공간 계산은
[journal 최신 조사](journal.md)에 있다. 현재 compact parser를 새 규칙으로 바꾼 상태는 아니다.
그 뒤 ESP native image 서명·signed metadata 대조를 연결한다.
본문 streaming은 구현했고 prefix 부분 수신 조립, 정식 schema·CLI·signed golden과
실제 STM32/ESP32 provider/target 연결을 완성해야 한다. 내부 key 배정은 미배포 후보다.
floor 비교 성공은 영속 정책/실제 설치 상태 provider와 복구 통합 완료가 아니다.

시작 파일은 [상세 task](tasks/T-007-ota-container.md)와
[현재 구현 계약](../shared/ota/README.md)이며, 설계 정본은
[OTA §7–9](architecture/ota.md)다. 이미 지정된 task이므로 backlog 전체를 다시 읽지 않는다.

전체 host/sanitizer/coverage·target ELF/MAP/BIN warning0·독립 reviewer2명·
CI/artifact 확인 전에는 T-007을 DONE으로 표시하거나 PR을 ready/merge하지 않는다.
T-007의 선행 T-001과 이전 T-400 구현을 다시 하지 않는다.

## 유지할 안전 경계와 미실행 gate

- hardware 없이 firmware source 구현은 허용됐지만 board flash/HIL, rail/clock,
  reset/brownout, 실제 UART/CAN/무선 장시간 시험과 차량 evidence는 `NOT_RUN`이다.
- 차량 CAN TX는 `NO-GO`다. Diagnostic Bridge는 read-only이며 control lease,
  raw replay, 차량 TX 경로를 추가하지 않는다.
- parser 성공은 erase/write/PREPARED/boot selector 변경 권한이 아니다.
- 실제 eFuse/option-byte 설정, production signing key 생성/배포는 T-007 범위 밖이다.

## 이전 merge와 남은 review debt

T-104 [PR #33](https://github.com/digitie/canview/pull/33)은
`d229772de77a48ae197e2ff1b4e55b6cef9a88ed`로 merge됐고 origin/main ancestry를
확인했다. 그 source의 CI `34335812873`6/6, target 이미지18/18 hash/bytes,
source6/6과 target log21 warning/error0의 상세 근거는
[closure 기록](reviews/adversarial/2026-09-09-T-104-04.md)과 [journal](journal.md)에 있다.

Reviewer A 재검토는 사용자에 의해 **PR #33 한 건만 면제**됐다. 이것은 PASS나
원 P1 closure가 아니며 [이슈 #34](https://github.com/digitie/canview/issues/34)는
OPEN이다. A-10 P2의 owner/시점/gate도 그 이슈와 T-104에 유지한다. 다른 PR에는
면제를 적용하지 않는다. T-104의 physical gate 완료로도 해석하지 않는다.

## 실행 환경과 이력

Windows PowerShell이 정본이며 [개발환경](development/windows.md)의 고정 SDK를
사용한다. 이미 설치한 t104 worktree의 foundation tool 환경은 재사용할 수 있지만
현재 source를 직접 빌드한다. WSL은 보조 sanitizer/coverage 실행에 실제 사용됐다.

기본 `F:/dev/canview`와 다른 worktree의 사용자 변경·로컬 SDK/evidence는 보존한다.
Git/PR 절차는 [workflow](runbooks/agent-workflow.md), 이전 task별 구현·merge·실패
이력은 [journal](journal.md)과 [review archive](reviews/README.md)에 둔다. 이 문서는
현재 상태와 다음 작업만 유지한다.
