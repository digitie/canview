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

현재 body 시험은 모형 수명/오류900건, 실제 P256+SHA-256 906건이다. 모형900건은
ASan/UBSan도 통과했고 body.c 함수/행/분기 coverage100%다. 앞선 typed 시험은
구조1422건·P2561425건, manifest.c coverage 함수100%·행97.12%·분기91.85%다.
현재 source의 Windows Host Debug/Release도 각각129/129을 통과했다.
Cortex-M4 freestanding object compile은 통과했지만 OTA target 통합의 증거는 아니다.
직전 source `b25b69a`의 CI `34727655450`는 success이며, 그 결과를 이후
수정 source에 적용하지 않는다. target artifact/hash의 별도 대조도 아직 하지 않았다.

[CBOR checkpoint 리뷰](reviews/adversarial/2026-09-13-T-007-cbor.md)의 A/B static
PASS는 `6d83962` 범위에만 해당한다. 이후 서명/manifest 구현과 전체 T-007의
최종 독립 2인 리뷰는 남아 있다. 두 기존 reviewer는 완료 결과를 보존한 뒤 종료했다.

## 다음 한 작업

현재/후보 ABI·requires와 native image signature·signed metadata 대조를 연결한다.
본문 streaming은 구현했고 prefix 부분 수신 조립, 정식 schema·CLI·signed golden과
실제 STM32/ESP32 provider/target 연결을 완성해야 한다. 내부 key 배정은 미배포 후보다.

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
