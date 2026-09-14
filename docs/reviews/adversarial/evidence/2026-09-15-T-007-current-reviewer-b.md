# 2026-09-15 T-007 현재 구현분 Reviewer B 실행 기록

- execution ID: `01a0a229-545f-7fc0-a90a-240470bb1206`
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

Python/schema/cryptographic wire/native metadata, malformed/resource exhaustion/generator drift, CMake/CI/security/evidence/reproducibility와 요구사항 정합성.

## 원본 결과

# PR #35 독립 적대적 리뷰 — Reviewer B

## 실행 식별과 판정

| 항목 | 기록 |
|---|---|
| 실제 execution ID | `01a0a229-545f-7fc0-a90a-240470bb1206` |
| 시작 UTC | `2026-09-14T23:04:06Z` — 보존된 최초 격리 확인 시각 |
| 종료 UTC | `2026-09-14T23:09:59Z` |
| 전문영역 | Python/schema, 암호 wire 계약, native metadata, malformed/resource 제한, generator, CMake/CI, evidence·요구사항 정합성 |
| candidate | `21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962` |
| base | `d229772de77a48ae197e2ff1b4e55b6cef9a88ed` |
| 변경 규모 | 82파일, `+7465/-152` |
| merge verdict | **CONDITIONAL** |

검토한 범위에서 **P0/P1은 확인하지 못했다. P2 1건, P3 1건은 OPEN**이다. 현재 구현분의 제한된 merge 위험 평가이며, 전체 T-007 완료·배포·설치 승인이 아니다.

조건은 다음과 같다.

- 아래 finding을 수정하거나, coordinator가 owner·상세 task·gate·목표 시점을 갖춰 disposition한다.
- candidate에 해당하는 필수 CI와 target artifact 감사를 완료한다.
- 독립 Reviewer A 결과와 함께 최종 판단한다.
- 현재 구현분 merge 후 일시중지 범위를 coordinator가 확정하고, T-007 미완료 상태와 남은 gate를 유지한다. 전달받은 범위 질문은 아직 답변 대기 상태다.

## 격리와 독립성

시작·종료에 `git cat-file -e`와 `git rev-parse`로 candidate/base 객체를 확인했다. 저장소 내용은 `git show <candidate>:<path>`, 변경은 `git diff --find-renames <base> <candidate>`로만 읽었다.

Python 실행도 candidate의 Git 객체를 메모리에서 로드했다. 저장소 데이터 읽기는 `git show`로 연결했으며 checkout의 이동 가능한 파일을 실행 기준으로 사용하지 않았다. 생성물과 변이 입력은 메모리에만 존재했다.

파일 작성, 코드 수정, commit, push, merge, artifact 다운로드는 수행하지 않았다. Object-only 방식이므로 worktree clean 상태를 보증하거나 기준으로 사용하지 않았다. 현재 Reviewer A 결과에 접근하지 않았다.

지정된 embedded-cstyle·embedded-architecture·embedded-documentation 원문을 읽고 적용했다. 스킬의 자동 수정 지침 대신 사용자의 읽기 전용 요청을 따랐다.

## Findings

모든 위치는 위 candidate 기준이다.

### B-01 — P2 / OPEN: generator가 신규 staging 64KiB 정렬 계약을 검증하지 않는다

**위치**

- `tools/generate_boards.py:77`, `:84`, `:179`
- `tests/foundation/test_generators.py:27`
- 대조: `firmware/platform/esp32s3/ota_image.c:40`

**근거와 실제 재현**

generator는 `data_offset`을 4KiB 단위로 검사하고 그 값을 새 BSP staging 상수와 CSV에 사용한다. 그러나 native adapter는 staging partition 시작에 64KiB 정렬을 요구한다.

candidate의 `boards.json`을 메모리에서 다음처럼 변경했다.

```text
Communicator ota.data_offset += 4096
Communicator ota.data_size   -= 4096
```

전체 Flash 끝과 기존 크기·겹침 제약을 보존한 이 입력에 대해 실제 `outputs()`가 성공하며 13개 결과를 반환했다.

```text
bundle_stage,0x40,3,0xA41000,0x480000,encrypted
0xA41000 % 65536 == 4096
```

이 주소는 `ota_image.c:40`의 검사에서 거절된다. generator 수용은 실행으로 확인했고, adapter 거절은 해당 C 조건의 정적 판단이다. C/SDK 장치 실행으로 재현한 것은 아니다.

**실패 시나리오와 영향**

후속 partition 편집에서 sector 단위로 staging을 이동하면 generator는 유효한 설정으로 생성하지만 native image 검증은 사용할 수 없게 된다. 추가된 시험은 현재 정상 설정의 상수·CSV 일치만 확인한다.

**현재 candidate의 실제 staging 주소 `0xA40000`은 정상**이고 생성물 13개도 원본과 일치한다. 현재 보드의 OTA 동작 실패나 인증 우회로 확대 해석하지 않는다. 발견 사항은 현재 generator의 입력 검증 결함이다.

**권고**

staging이 있는 board에는 `data_offset % 65536 == 0`을 검증하고, 위 4KiB 이동 입력을 거절하는 회귀시험을 추가한다.

### B-02 — P3 / OPEN: 모듈 README에 이전 구현 단계의 미구현 설명이 남아 있다

**위치**

- `shared/ota/README.md:63`
- `shared/ota/README.md:100`
- `shared/ota/README.md:165`
- `shared/ota/README.md:237`

**근거**

서명·manifest·streaming lifecycle을 후속 구현으로 설명하고, ESP native 검사를 미구현이라고 단정한다. 같은 문서의 `:113`, `:174`, `:311` 이후에는 실제 SDK adapter, body와 공통 metadata 구현이 설명되어 있다.

**실패 시나리오와 영향**

후속 작업자가 모듈 계약을 읽고 이미 구현된 기능을 다시 구현하거나, 남은 정상 owner/target 연결과 검사기 자체의 부재를 혼동할 수 있다. 현재 인증 경계가 열리는 결함은 아니다.

**권고**

해당 문구를 현재 primitive의 구현 상태와 미완료 integration 범위로 갱신한다. SDK compile fixture와 실제 RSA/장치 실행은 계속 구분한다.

## 실행·검토 evidence

### 이 execution에서 직접 실행

| 검사 | 결과와 한계 |
|---|---|
| candidate/base 객체 확인 | 시작·종료 모두 일치 |
| Python schema/wire drift 시험 | 지정 unittest 1개 통과 |
| Python malformed JSON·교차 필드 시험 | 지정 unittest 1개 통과 |
| Python CBOR corpus | 11,989건 평가, 990건 수용. C differential 재실행 아님 |
| Python typed manifest | 1,437건 평가, 명시적 기대 상태 1,340건 일치 |
| JSON→CBOR 역할/u64 왕복 | 역할 3종 × 경계 6종, 18건 통과 |
| board generator | 현재 생성물 13개와 candidate byte열 일치 |
| staging 정렬 음성 입력 | 위 B-01 재현: 부적합 입력 수용 |
| 코드·현재 문서 범위 `diff --check` | exit 0 |

전체 `diff --check`에는 과거 review Markdown의 trailing whitespace 4곳이 출력됐다. 코드 결함으로 분류하지 않았으며, 해당 과거 결과를 현재 verdict의 근거로 사용하지 않았다.

실행 명령의 핵심은 다음과 같다. 메모리 loader와 재현식 전체는 이 execution의 도구 기록에 남아 있다.

```powershell
git cat-file -e '<candidate>^{commit}'
git rev-parse '<candidate>^{commit}'
git rev-parse '<base>^{commit}'
git diff --find-renames --stat <base> <candidate>
git diff --find-renames <base> <candidate> -- <검토 경로>
git show <candidate>:<path>
python -B -   # Git 객체 전용 메모리 loader 및 경계·generator 검사
```

### coordinator가 전달한 별도 evidence

이 reviewer가 다운로드하거나 재실행한 결과는 아니다.

- run `34906746695`, candidate `21909e5`: `windows-c99` success.
- Debug/Release 로그 각각 `140 Passed / 0 Failed`.
- host simulation 12 PASS, hardware NOT_RUN.
- Node deprecation·annotated tag clone warning·예상된 argparse 음성시험 error와 runner annotation이 있으므로 **전체 CI warning0를 주장하지 않는다**.
- 전달 시점에 compiler/linker/CMake warning은 발견되지 않았고, target job 및 artifact 감사는 진행 중이었다.

## T-007 acceptance와 미구현 gate

상세 task의 **수용 기준 5개를 완료로 변경할 근거는 없다.**

| 수용 기준 | 확인한 구현 | 남은 범위 |
|---|---|---|
| 잘못된 identity/key/signature·CBOR 거절 | prefix·typed parser와 관련 음성시험 | 완성된 signed package의 통합 검증 |
| C/Python 길이·정수·target·header 정합성 | typed 구조, u64, 정렬·길이 계약 | 전체 package 검사기·packager와 signed golden |
| 검증 전 erase 금지, 전체 검증 전 PREPARED 금지 | 검사 primitive에 writer/selector 권한 없음 | 정상 OTA owner, 실제 writer와 상태 전이 통합 |
| signed sequence 보존과 native metadata 대조 | u64·공통 metadata·STM 검사 | 실제 descriptor 생성 및 ESP/STM 정상 target 연결 |
| 공식 도구 pin과 golden digest·clean host 재현 | wheel hash lock, MCUboot commit/clean 검사 코드 | 고정 signed golden과 완성 package 재현 evidence |

추가로 다음은 미완료다.

- prefix 부분 수신 조립.
- 정상 OTA task/body→BSP orchestration, Flash 단일 owner와 검증 중 불변 보장.
- 영속 policy A/B, CONFIRM_INTENT 조정, 실제 설치 상태 provider.
- 실제 STM provider/bootloader와 Controller·Bridge 통합.
- target에서 실제 native signature 실행, 전체 call-chain stack·시간·resource 측정.

이들은 현재 부분 구현의 자동 P1 판정으로 바꾸지 않았지만, 전체 T-007 PASS로 숨길 수도 없다.

## 공격 관점과 미검토 범위

검토한 공격 관점은 duplicate/unknown field, CBOR 비최소 인코딩·중첩·item 제한, JSON float/NaN·u64 손실, unsigned header 변조, target 중복·역할 혼합, 정렬 padding과 길이 경계, 서명·hash·metadata 결합, 잘못된 native TLV/DER, provider 실패, 생성물 drift와 모형시험의 과대 표시다.

C counterpart에서 signed manifest와 unsigned lengths의 대조, native metadata의 role/target/board/layout/epoch/ABI/sequence 결합, 전체 image hash와 native 서명의 분리를 확인했다. 직접 암호 연산을 구현하지 않고 provider/SDK에 위임하는 경계도 확인했다.

다음은 이 execution의 검증 범위 밖이다.

- C 전체 host suite, ASan/UBSan, coverage와 target build 재실행.
- C parser/runtime의 완전한 전문 리뷰 및 모든 C probe 내부의 줄 단위 검토.
- 실제 SDK RSA 실행, 장치 Flash 불변성·암호화 reader 동작.
- CI artifact의 ELF/MAP/BIN·hash·source provenance 감사.
- 전체 과거 journal/review와 전체 backlog 감사.

**Physical HIL: NOT_RUN. 차량 CAN TX: NO-GO.**

## 실제 읽은 파일 범위

- 필수·설계 문서: `AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-007-ota-container.md`, `docs/runbooks/agent-workflow.md`, `docs/architecture/README.md`, `docs/architecture/ota.md` §7–9, `docs/adr/009-ota-native-image-alignment.md`, `shared/ota/README.md`.
- Python/schema 전체: `tools/ota/{cbor,envelope,manifest,manifest_json}.py`, `schema/cvota-v2.schema.json`, `protocol/schema/ota-container-v2.yaml`.
- C counterpart 전체: `shared/ota/src/{envelope,manifest,body,floor,native_metadata,native_stm}.c`, `manifest.h`, `native_metadata.h`, `native_stm.h`, `firmware/platform/esp32s3/ota_image.{c,h}`, `firmware/communicator/esp32/bsp/ota.{c,h}`.
- 시험 전체: `tests/ota/test_{body,cbor_document,envelope_signature,manifest,manifest_json,native_stm}.py`, `cng_provider.{c,h}`, `native_stm_probe.c`, `envelope_crypto_probe.c`, `test_esp_image_sdk.c`, `test_native_metadata.c`, `test_comm_ota.c`, `esp_image_sdk_fixture.h`.
- build/config 변경: root·shared OTA·ESP OTA component CMake, `tests/fixtures/idf-ota-image/`의 CMake·main·sdkconfig·README, `.github/workflows/foundation.yml`의 변경부와 관련 실행/provenance 구간, `tools/requirements-ota.lock`, `tools/toolchain-versions.json`.
- generator: `tools/generate_boards.py:1–275`, `tests/foundation/test_generators.py` 변경부, board 입력 4파일과 생성물 13개를 Git 객체로 읽어 메모리 대조.
- 기타 delta: ADR/decision/task 색인, `docs/development/toolchains.md`, T-104 상태 추가분, T-103 합성 fixture 변경과 helper `:1–65`.

**최종 판정: CONDITIONAL — 현재 구현분의 제한된 merge 평가. B-01/B-02 disposition, 독립 리뷰와 candidate CI/artifact gate가 남아 있다. T-007 전체 완료와 배포는 승인하지 않는다.**
