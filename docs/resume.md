# resume.md

## 현재 작업

2026-09-15, [T-007 OTA-01](tasks/T-007-ota-container.md)은 `IN_PROGRESS`다.

사용자는 현재 작업을 완료·merge한 뒤 일시중지를 요청했다. 다음 task를 시작하지 않는다.
T-007 전체 완성인지 현재 구현분의 review/CI closure인지 확인 질문은 답변 대기 중이다.
추가 기능 구현과 merge는 보류하고, 어느 범위에도 필요한 현재 candidate의 독립 리뷰·CI를 확인한다.

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
target 연결은 아직 없다. ESP native의 read-only SDK adapter는 실제 ESP-IDF
서명 설정으로 compile/link했다. 반환 metadata의 role/board/layout/epoch/ABI/u64
sequence와 ESP version 대조는 기존 STM 검사와 공통화했다. Communicator BSP는 고정
bundle_stage와 generated board 계약을 검사하고 SDK→metadata를 연결한다.
정상 OTA task/body와 Flash 단일 owner 연결은 남아 있다.

body 시작 전에 신뢰된 로컬 snapshot으로 구·신 ABI 네 조합, MCU별 boot/recovery,
hardware capability와 config 읽기 범위를 검사한다. 미확인 snapshot은 거부하며
Controller/Bridge의 존재 여부를 Communicator 복구 조건으로 추가하지 않는다.

로컬 version floor 검사도 body 시작 전 필수로 연결했다. 낮은 sequence와 같은
sequence/다른 hash를 거부하고, 실제 설치 증거로 ALREADY_INSTALLED/REPAIR_REQUIRED를
구분한다. 영속 copy/CONFIRM_INTENT 조정·실제 설치 검사 provider는 아직 없다.

[ADR-009](adr/009-ota-native-image-alignment.md)에 따라 미배포 컨테이너를
revision2로 구분했다. C/Python이 image 앞64KiB 정렬을 계산하고 C body가0 padding을
chunk로 검사한다. prefix buffer와 native image byte열은 그대로이며 v1은 거절한다.

현재 body/호환성/floor 시험은 모형1702건, 실제 P256+SHA-2561708건이다.
모형 ASan/UBSan과 body.c 함수·행·분기100%를 확인했다. typed manifest 교차1437/1440건,
서명 prefix232건도 통과했다. manifest.c의 이번 모형 coverage는 함수100%·행93.78%·
분기90.00%이며 예전 실행의 더 높은 수치를 새 source에 적용하지 않는다.
floor C3847건과 STM native 시험도 유지한다. 앞선 STM native의 비암호 모형
ASan/UBSan2284건과 함수100%·행98.14%·분기94.74%는 해당 이전 source의 기록이다.
실제 CNG native 시험은 ECDSA DER 길이에 따라 건수가 달라진다.
현재 source의 Windows Host Debug/Release는 각각140/140을 통과했다.
schema field/enum/limit drift와 JSON→CBOR→C typed parser 대조, 서명 전 CLI의
bounded 입력·기존 출력 보존 시험을 추가했다. 이 출력은 완전한 package가 아니다.
BSP 연결 모형 ASan/UBSan과 함수1/1·행29/29·분기32/32, 실제 SDK fixture
ELF/MAP/BIN 경고0을 확인했다. 이는 장치에서 native 서명을 실행한 결과가 아니다.
공통 metadata의 모형 ASan/UBSan, 함수6/6·행60/60·분기76/76도 통과했다.
기존 공식 imgtool/CNG STM 회귀를 재실행했고, SDK fixture에서 공통 함수까지
실제 ESP32-S3 compile/link와 경고0을 확인했다. BSP 연결도 SDK fixture에 추가했지만
정상 OTA app 경로는 아직 아니다.
새 ESP SDK adapter의 signed 모형 ASan/UBSan과 함수1/1·행54/54·분기70/70을 확인했다.
실제 SDK fixture ELF/MAP/BIN도 생성했고 경고0을 확인했다. 이것은 SDK 연결/compile
증거이며 장치에서 RSA를 실행하거나 정상 firmware 설치를 검증한 결과는 아니다.
Cortex-M4 body object compile은 통과했지만 OTA target 통합의 증거는 아니다.
`21909e5`의 CI `34906746695`는6개 job이 성공했다. 이후 수정 source에 적용하지 않는다.
내려받은 Host Debug/Release 로그는 각각
140/140이며 ASan/UBSan job은136/136이다. target artifact/hash 대조는 아직 하지 않았다.
전체 job 로그에는 Node/Git 경고와 예상된 argparse 음성 시험 출력이 있으므로 CI 전체
warning0으로 표시하지 않는다. 자세한 분류는 [journal](journal.md)에 기록했다.

[CBOR checkpoint 리뷰](reviews/adversarial/2026-09-13-T-007-cbor.md)의 A/B static
PASS는 `6d83962` 범위에만 해당한다. 이후 서명/manifest 구현과 전체 T-007의
최종 독립 2인 리뷰는 남아 있다. 두 기존 reviewer는 완료 결과를 보존한 뒤 종료했다.

현재 구현분의 새 object-only 리뷰는 `21909e5`/base`d229772`에서 A/B CONDITIONAL로 끝났다.
A `01a0a229-5363-7f61-a607-ecae1f0a4ff8`, B `01a0a229-545f-7fc0-a90a-240470bb1206`.
요청 원문은 [A](reviews/adversarial/evidence/2026-09-15-T-007-current-reviewer-a.md)와
[B](reviews/adversarial/evidence/2026-09-15-T-007-current-reviewer-b.md)에 있다.
원본은 보존했고 P0/P1 없음, P2 두 건과 공통 문서 P3를 수정했다.
[통합 기록](reviews/adversarial/2026-09-15-T-007-current.md)의 원 reviewer 재확인과
post-fix CI/target artifact 감사를 마치기 전 merge하지 않는다. 전체 T-007 미완료는 유지한다.

## 다음 한 작업

schema와 JSON→CBOR 작성 도구를 native image signing·검증 및 전체 `.cvota` packager에
연결한다. C/Python의 기존 byte 계약은 유지했다. 작성 도구의 UNSIGNED_MANIFEST 출력은
서명된 package가 아니며 최종 signed golden/독립 리뷰도 아직 없다.
그 뒤 Communicator BSP 검증을 body 완료 뒤의 단일 OTA owner 경로에 연결하고
Flash 불변 보장·서명된 실제 descriptor 생성과 provider 연결을 확인한다.
staging 위치/크기와 암호화 flag는 generator로 연결했고 공식 SDK partition parser로
세 보드 template을 검사했다. [SDK fixture 계약](../tests/fixtures/idf-ota-image/README.md)을 따른다.
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
