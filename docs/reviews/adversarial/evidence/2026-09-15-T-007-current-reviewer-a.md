# 2026-09-15 T-007 현재 구현분 Reviewer A 실행 기록

- execution ID: `01a0a229-5363-7f61-a607-ecae1f0a4ff8`
- 실행 요청 시작: 2026-09-14 23:03:32 UTC
- candidate: `21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962`
- base: `d229772de77a48ae197e2ff1b4e55b6cef9a88ed`
- 격리 요청: commit object-only
- 상태: 원본 수신, CONDITIONAL. 전체 T-007 PASS가 아니다.
- 실제 검토 시작/종료·읽은 파일·명령은 아래 원본 보고서에 보존한다.

## 공통 요청 원문

CANView PR35의 현재 구현분 독립 적대적 리뷰. 전체 T-007 완료 승인이 아니라 현재 코드 merge 위험 평가이며, 누락 acceptance/기능도 별도 명시. 사용자 최신 지시: 지금 작업 완료/merge 후 일시중지; 전체 T007 완성 vs 현재 구현분 merge 범위 질문은 답변 대기 중. 구현/merge는 coordinator만, 읽기 전용 리뷰 요청.
Repository F:/dev/canview-wt/t007-ota-container. Immutable candidate 21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962, base d229772de77a48ae197e2ff1b4e55b6cef9a88ed. Object-only isolation: git cat-file -e <candidate>^{commit}, git rev-parse <candidate>^{commit}, 모든 source는 git show <candidate>:<path>, diff는 git diff --find-renames <base> <candidate>; moving worktree source를 기준선으로 쓰지 말 것. 전체 diff82files +7465/-152. 두 reviewer에 동일 manifest; 상대 결과 접근 금지.
먼저 candidate AGENTS.md, docs/README.md, docs/resume.md, docs/tasks/T-007-ota-container.md, docs/runbooks/agent-workflow.md, docs/architecture/README.md와 관련 OTA §7-9/ADR009 읽기. 적용 embedded-cstyle/architecture/documentation skills는 원본을 직접 읽고 사용자 정본 우선. 기록만인 과거 journal/review는 필요만 읽기.
공통 scope: shared/ota/src/* (C99 CBOR/envelope/typed manifest/body/floor/native STM/metadata), firmware/platform/esp32s3/ota_image.*, firmware/communicator/esp32/bsp/ota.*, tools/ota/*, schema/cvota-v2.schema.json, protocol/schema/ota-container-v2.yaml, tests/ota/*, root/shared/component/fixture CMake, .github/workflows/foundation.yml, board generator/partitions, changed requirements/docs. 이미지 컨테이너2+64KiB 정렬은 acceptedADR009. PortableCheap/vendorleak없음, SDK서명재사용. SDKfixture는unsigned/NULLnegative, actual deviceRSA/HIL NOT_RUN, CAN TX NO-GO. 현재 sourceHost140/140DebugRelease. CI34906746695 live, artifacts 별도 coordinator 검증. 전체 T007packager/signedgolden/normalOTAowner/policy/targetintegration 미완성.
코드 변경 금지. raw 최종 보고서를 한국어 markdown으로 답변(직접 파일 쓰지 말기): 실제executionID, 시작/종료UTC, 전문영역, candidate/base, 격리검증, 실제 읽은파일/명령, 정확한file:line P0/P1/P2/P3finding(실패scenario/영향/권고), 공격시나리오/미검토범위, physicalHILNOT_RUN, verdict BLOCK/CONDITIONAL/PASS. 미완료를 PASS로 꾸미지 말고 확인한 범위를 명확히 적기. 관련 코드의 linelevel검토 필요. 실제 현재 오류와 향후미구현gate를 구분하되 누락을 숨기지 말 것. 결과 완료 후 재검토필요시같은execution 재사용 예정.

## 전문 배정

C memory/bounds/integer, callback lifetime/reentry/ownership/partial reset/failclosed, floor/ABI preconditions, STM native parser, SDK/BSP trust boundary.

## 원본 결과

# PR #35 독립 적대적 리뷰 — Reviewer A

## 판정

`CONDITIONAL` — 지정된 현재 구현분의 merge 위험 평가다. 전체 T-007 완료·배포·설치 승인이 아니다.

- 확인한 범위에서 P0/P1 finding은 없다.
- P2 1건, P3 1건이 `OPEN`이다.
- 정상 OTA owner·영속 정책·signed packager·target 통합은 미완성이다.
- physical/HIL: `NOT_RUN`. 차량 CAN TX: `NO-GO`.
- 구현분 merge 범위에 관한 사용자 답변 대기는 coordinator가 처리해야 한다. 본 리뷰가 그 답변을 대신하지 않는다.

## 실행 및 격리 증거

| 항목 | 실제 값 |
|---|---|
| Reviewer execution ID | `CODEX_THREAD_ID=01a0a229-5363-7f61-a607-ecae1f0a4ff8` |
| Runtime session ID | `CODEX_SESSION_ID=01a07aaf-b78b-7010-962e-8de2706b60ea` |
| 시작 UTC | `2026-09-14T23:03:51.7274104Z` |
| 종료 UTC | `2026-09-14T23:09:17.3284043Z` |
| 전문영역 | C memory/bounds/integer, callback·ownership·reset·fail-closed, floor/ABI, STM native parser, ESP SDK/BSP 신뢰 경계 |
| Repository | `F:/dev/canview-wt/t007-ota-container` |
| Candidate | `21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962` |
| Base | `d229772de77a48ae197e2ff1b4e55b6cef9a88ed` |
| 격리 | Commit object-only |

시작·종료 시 candidate 존재와 해석된 전체 hash를 확인했다. Base도 전체 hash가 일치했다. 저장소 내용은 지정 객체의 `git show`, 차이는 고정 base/candidate의 `git diff --find-renames`로 읽었다. Moving worktree source나 기존 build 산출물을 기준선으로 사용하지 않았다.

전체 변경량 `82 files, +7465/-152`를 확인했다. 아래의 A 전문영역 소스는 전량 검토했으며, 82개 파일 전체의 모든 행을 검토했다는 주장은 하지 않는다. 상대 reviewer 결과와 과거 reviewer 원본은 열지 않았다. 코드·문서·Git 상태를 변경하지 않았고 build/test를 실행하지 않았다. Object-only 방식이므로 checkout clean 여부는 격리 근거로 사용하지 않았다.

## Findings

### A-01 — P2 / OPEN: cleanup 재시도 실패가 최초 오류를 덮어쓴다

**위치:** [shared/ota/src/body.c:250](https://github.com/digitie/canview/blob/21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962/shared/ota/src/body.c#L250), 같은 파일 `:24`, `:243`.

**실패 시나리오 — 소스 경로 추적으로 확인, 실행 재현은 NOT_RUN:**

1. 유효한 prefix로 body를 열고 변조된 image를 수신한다.
2. Hash 불일치가 `CANVIEW_AUTH_FAILED`를 발생시킨다.
3. 자동 cleanup의 provider `reset()`이 `CANVIEW_RESOURCE_BUSY`를 반환한다.
4. 이때 `body.error=AUTH_FAILED`, `cleanup_error=RESOURCE_BUSY`, `hash_live=true`로 최초 오류와 cleanup 오류가 구분된다.
5. Caller가 `body_reset()`을 재시도하고 provider가 다시 실패하면, `body_mark_failed(body, status)`가 `body.error`도 `RESOURCE_BUSY`로 바꾼다.
6. 이후 `body_finish()`/`body_feed()`는 최초 인증 실패 대신 cleanup 오류를 반환한다.

**영향:** 자원 소유 상태와 FAILED 상태는 유지되어 fail-open은 확인되지 않았다. 그러나 오류 후 cleanup 재시도라는 정상적인 장애 처리 과정에서 최초 실패 원인을 잃는다. `shared/ota/README.md:192`의 “원 오류와 cleanup 오류는 별도” 계약과 맞지 않으며, 상태 보고·원인 분석을 잘못할 수 있다.

**시험 공백:** `tests/ota/body_probe.c:89`의 `!probe->fired` 조건은 선택한 오류를 한 번만 발생시킨다. `tests/ota/test_body.py:213`의 복합 오류 시험도 반복 cleanup 실패를 검증하지 않는다.

**권고:** 이미 FAILED인 객체에서는 최초 `body.error`를 보존하고, reset 반환값과 `cleanup_error`로 cleanup 실패를 전달한다. 최초 오류 뒤 reset이 두 번 이상 실패하고 최종 성공하는 회귀시험을 추가한다.

### A-02 — P3 / OPEN: 모듈 README의 구현 상태가 서로 모순된다

**위치:** [shared/ota/README.md:63](https://github.com/digitie/canview/blob/21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962/shared/ota/README.md#L63), 같은 파일 `:165`, `:238`.

**근거:**

- `:63`은 manifest 서명 검사와 streaming lifecycle을 후속 구현으로 서술한다.
- `:165`, `:238`은 ESP native 검사가 남아 있거나 미구현이라고 서술한다.
- 같은 candidate에는 manifest/body 구현과 ESP SDK adapter·BSP 연결이 있으며, README 후반부와 상세 task도 이를 설명한다.

**영향:** 기존 구현과 남은 정상 owner/target 연결을 혼동해 후속 범위·검증 계획을 잘못 잡을 수 있다. 코드의 인증 우회 문제는 아니다.

**권고:** “검사기/adapter 구현됨, 정상 owner·장치 실행·target 통합 미완료”로 구분한다. 이전 checkpoint 설명이면 시점과 적용 범위를 명시한다.

## 공격 관점별 확인 결과

| 공격·실패 관점 | 소스 및 대응 시험 검토 결과 |
|---|---|
| CBOR 절단·길이·깊이·item 폭주 | 입력 상한, 최소 인코딩, 고정 stack, 문자열 잔여 길이 검사를 확인했다. |
| Duplicate/unknown key·잘못된 role/target | 구조 검사와 고정 필드 parser, 역할별 image 수·target·signature 조합 검사를 확인했다. |
| 정렬·정수 overflow·본문 추가 입력 | ADR-009의 64KiB 정렬, 덧셈 전 overflow 검사, zero padding, 순차 offset, 마지막 추가 bytes 거부를 확인했다. |
| Callback 재진입·입력 수명 | body busy 방어와 descriptor/함수표 복사, chunk 미보존을 확인했다. 동일 객체의 동시 호출은 caller 직렬화 전제다. |
| Partial start/finish/reset | 부분 초기화도 cleanup 대상으로 유지한다. 반복 cleanup 실패의 최초 오류 보존은 A-01이다. |
| Floor·ABI | u64 직접 비교, 같은 sequence/다른 digest 거부, 실제 설치 증거 구분, unknown snapshot 거부, 구·신 네 ABI 조합 검사를 확인했다. |
| STM native parser | Header/TLV 경계, 정규 DER 변환, 전체 image hash와 native signed 영역 hash·서명, protected metadata 대조를 확인했다. |
| SDK/BSP 경계 | 고정 staging 선택, 내부 Flash·주소·크기·정렬·암호화 조건, 서명 설정 제한, 실패 출력 초기화를 확인했다. |

ESP adapter의 DATA 모드 전체 파일 hash와 RSA 검증 후 반환 길이 대조는 고정 SDK 객체의 관련 구현과 대조했다. 장치 실행 검증은 아니다. [고정 SDK hash 구현](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/components/bootloader_support/src/bootloader_common.c), [고정 SDK image verifier](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/components/bootloader_support/src/esp_image_format.c).

공유 core에서 heap·vendor SDK 의존성·Flash writer·boot selector 호출은 발견하지 않았다. 성공 결과를 설치 권한과 분리한 계약도 확인했다. 스킬의 일반적인 구조·스타일 규칙은 저장소 정본과 현재 내부 API 범위에 맞춰 적용했다.

## 미완성 acceptance 및 기능

아래는 현재 코드 결함 finding과 구분한 **전체 T-007의 열린 항목**이다.

| 항목 | 현재 확인한 범위와 남은 작업 |
|---|---|
| Wrong identity/key/signature·malformed 거부 | C 검사기와 대응 시험 구현을 검토했다. 전체 signed container/target 경로의 acceptance 완료는 아니다. |
| 길이·정수 경계·중복 target·C/Python 일치 | Primitive와 body 시험이 존재한다. 최종 packager·signed golden을 통한 전체 경로 검증은 남아 있다. |
| 검증 전 erase 금지·검증 후 제한된 writer 권한 | 현재 parser에는 writer 자체가 없다. 정상 owner, enum→비활성 slot/staging 연결, read-back, PREPARED/activation 분리는 미통합이다. |
| Signed u64 sequence 대조 | Manifest·floor·native metadata 비교는 있다. 실제 descriptor 생성과 target provider 연결은 남아 있다. |
| 공식 signing 도구·golden digest·clean host | MCUboot pin과 실행 중 생성하는 합성 시험은 있다. 최종 서명 package와 고정 signed golden 산출물은 미완성이다. |
| Streaming 전체 수명 | Body streaming은 구현됐다. Prefix 부분 수신 조립과 실제 transport/owner 연결은 남아 있다. |
| 영속 floor 정책 | 비교 함수만 있다. A/B copy, CONFIRM_INTENT 조정, 실제 설치 상태 provider·복구 연결은 없다. |
| ESP/STM target 통합 | SDK fixture와 BSP 연결은 존재한다. 정상 OTA app/body owner 및 STM bootloader/provider 통합은 남아 있다. |
| 실행 자원·실물 | 실제 MCU call-chain stack·최악 실행시간, RSA 장치 실행, reset/brownout·Flash fault/HIL은 확인하지 못했다. |

특히 `tests/fixtures/idf-ota-image/main/main.c:13`과 `:16`은 NULL negative 호출이다. 서명 설정으로 compile/link하는 fixture를 실제 native RSA 성공 실행으로 집계하면 안 된다.

## 실제 읽은 파일 및 명령

### Candidate에서 읽은 파일

- 정본: `AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-007-ota-container.md`, `docs/runbooks/agent-workflow.md`, `docs/architecture/README.md`, `docs/architecture/ota.md` §7–9, `docs/adr/009-ota-native-image-alignment.md`.
- 공유 core 전량: `shared/ota/src/`의 `cbor_head`, `cbor_document`, `envelope`, `manifest`, `body`, `floor`, `native_stm`, `native_metadata` 각각 `.c`와 `.h`.
- SDK/BSP: `firmware/platform/esp32s3/ota_image.{c,h}`, `firmware/communicator/esp32/bsp/ota.{c,h}`.
- 시험 C/헤더: `tests/ota/`의 모든 `.c`와 `.h`.
- 대응 시험 Python: `test_body.py`, `test_floor.c` 대응 경로, `test_native_stm.py`, `test_manifest.py`, `test_cbor_document.py`, `test_envelope_signature.py`.
- 계약: `shared/ota/README.md`, `protocol/schema/ota-container-v2.yaml`, `schema/cvota-v2.schema.json`.
- SDK fixture: README, root/main CMake, `main/main.c`, `sdkconfig.defaults`.
- 빌드 연결: root/shared/component CMake 변경, 세 ESP 정상 app의 main CMake, STM32 CMake.
- 보조 diff: foundation CI, board generator·generated staging 상수, 세 OTA partition template, toolchain pin 및 관련 development 문서.

스킬 원본 세 파일은 `C:/Users/digit/.codex/skills/embedded-{cstyle,architecture,documentation}/SKILL.md`에서 직접 읽었다.

### 명령 계열

```powershell
git cat-file -e '21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962^{commit}'
git rev-parse '21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962^{commit}'
git rev-parse 'd229772de77a48ae197e2ff1b4e55b6cef9a88ed^{commit}'
git ls-tree -r --name-only 21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962 <범위>
git diff --find-renames --stat <base> <candidate>
git diff --find-renames <base> <candidate> -- <검토 경로>
git show <candidate>:<path>
```

`git show` 출력에 PowerShell로 행 번호를 붙였고, 탐색에는 `rg`를 사용했다. SDK는 `C:/cv/esp-idf-6.0.3`에서 고정 commit 존재/hash 확인 후 해당 객체의 `git show`로 관련 구현을 대조했다. SDK cwd에서 프로젝트 component를 읽으려던 1회 경로 오류는 프로젝트를 명시한 `git -C`로 재조회했다.

## 검증 한계와 merge 조건

- 본 execution의 CTest·sanitizer·target build·실제 암호 실행: `NOT_RUN`.
- 전달받은 Host Debug/Release `140/140`은 reviewer가 재실행하거나 산출물을 감사한 결과가 아니다.
- CI `34906746695` 및 artifact/source hash 감사는 coordinator 범위이며 여기서 통과로 판정하지 않았다.
- Python 도구·schema·CI 전체 심층 검토는 Reviewer B 주영역이다. 상대 결과는 확인하지 않았다.
- physical/HIL·장치 Flash·실제 RSA·차량 시험: `NOT_RUN`.

현재 구현분 merge에는 A-01/A-02의 수정 또는 규칙에 맞는 disposition, 독립 B 리뷰 및 coordinator의 동일 candidate 검증·artifact 감사가 필요하다. P2/P3를 연기한다면 owner·상세 task·적용 gate·목표 시점을 명시해야 한다.

**최종 verdict: `CONDITIONAL`. 전체 T-007은 미완료이며, 구현·merge·일시중지 처리는 coordinator만 수행한다.**
