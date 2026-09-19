# T-007 PSA provider Reviewer B 원본

## 전달 요청 원문

```text
T-007 PSA provider 추가분 독립 hostile review. 이전 golden review는 종료됐으며 이번은 새 execution UUID/start/end를 기록하십시오. Reviewer A(Maxwell): embedded C 수명/cleanup/partial init/재진입/ownership/메모리 경계. Reviewer B(Huygens): PSA API 보안/SDK config·CMake 통합/모형 시험의 허점·증거 귀속. 각자 해당 전문분야를 주로 보되 전체 delta 회귀도 검사하십시오. 서로 결과 공유 금지.
공통 immutable manifest:
repo F:/dev/canview-wt/t007-ota-container
base 6209eac0a12207e2473ef096a79b068bce4024d6
candidate 70c7a384f3763b021546fd50f742ea51cf137dc4
scope: git diff --find-renames base candidate의13파일. 특히 firmware/platform/esp32s3/ota_crypto.{c,h}, tests/ota/{psa_crypto_fixture.h,test_esp_ota_crypto.c}, root/component CMake, SDKfixture main, shared/ota/README, synthetic digest/doc delta.
소스는 git cat-file -e, rev-parse 및 git show candidate:path /git diff base candidate로만 읽는 object-only 정적 리뷰. moving worktree source 금지. 격리 방식상 실행하지 않은 test는 NOT_RUN으로 기록. 별도 공식 SDK header 계약을 읽을 수 있으면 C:/cv/esp-idf-6.0.3의 psa/crypto.h 기준임을 기록하되 현재제품실행으로 확대 금지.
정본: AGENTS.md, docs/README.md, docs/tasks/T-007-ota-container.md, docs/architecture/ota.md §7–9, ADR009 및 관련 embedded skills. task전체/Flash/provisioning/root/정상OTAowner는 이번 추가분 범위 밖/미완료이며 이 사유만으로 이 checkpoint 결함을 발명하지 말 것. 입력 root는 신뢰된 BSP role root라는 계약; SDK 내부 math를 자체 구현하지 않음. single-task; busy는 재진입방어 not mutex. 실제SDKbuild만 있고 장치양성PSA실행 NOT_RUN.
작성자 검증(독립실행과 구분): Debug/Release144/144; ASan/UBSan 모형 PASS, adapter149/149lines114/118branches; actual ESP-IDF6.0.3 fixture ELF/MAP/BIN warning0 및 nm PSA symbol; docs/plan/generator/SDKnegative13 PASS. 새PSA추가분의CI/artifact검증은 진행예정.
원문 최종 report 한국어: execution UUID, 시작/종료UTC, 역할, 실제확인base/candidate, isolation, 실제 읽은파일/명령, P0/P1/P2/P3 finding(없으면없음), 정확한 file:line+실패시나리오+영향+권고, 공격한시나리오/미검토범위, physical/HIL NOT_RUN·vehicleTX NO-GO, BLOCK/CONDITIONAL/PASS verdict. 파일수정/commit/push/다른에이전트소환 금지. 완전한 raw를 최종응답에 반환. 읽지못했으면 명시incomplete/BLOCK, PASS추정 금지.
```

## 반환 원문

# T-007 PSA provider 독립 hostile review — Reviewer B

## 1. 실행 정보와 판정

- Execution UUID: `f4442cdd-a0c3-4d3d-8e46-1e32066eeebb`
- 시작 UTC: `2026-09-19T03:49:44.2320761Z`
- 종료 UTC: `2026-09-19T03:53:17.9048533Z`
- 역할: Reviewer B / Huygens
- 전문영역: PSA API 보안 계약, SDK 설정·CMake 통합, 모형 시험의 정확성, 검증 근거 귀속
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `6209eac0a12207e2473ef096a79b068bce4024d6`
- 실제 확인 candidate: `70c7a384f3763b021546fd50f742ea51cf137dc4`
- 검토 범위: 명시된 두 commit 사이 13파일 delta와 필요한 기존 계약
- Findings: **P0 없음 / P1 없음 / P2 없음 / P3 1건 OPEN**
- Verdict: **CONDITIONAL — 이번 provider 추가분의 정적 검토에 한정**

핵심 adapter·header·모형 시험 소스를 모두 직접 읽었다. 핵심 소스 미열람에 따른 incomplete 상태는 아니다. compiler·시험은 실행하지 않았다.

## 2. 격리와 독립성

시작·종료에 두 commit의 존재와 `^{commit}` 해석 결과를 확인했다. 요청된 hash와 모두 일치했다.

저장소 소스는 고정 candidate의 `git show candidate:path`와 base→candidate `git diff`로만 읽었다. moving worktree 내용이나 HEAD를 candidate 소스로 사용하지 않았다.

- 파일 수정·생성·commit·push 없음.
- 다른 에이전트 소환·reviewer 결과 조회·공유 없음.
- compiler·CTest·SDK 실행 없음.
- detached worktree를 만들지 않았다. 따라서 detached HEAD/clean 검사는 해당 없으며, 기존 checkout이 clean하다고 주장하지 않는다.
- `embedded-cstyle`, `embedded-architecture`, `embedded-documentation`을 직접 읽고 경계·실패 처리·문서 근거 검토에 적용했다.

사용자가 허용한 별도 SDK 헤더 확인은 다음 설치본에서 수행했다.

- SDK: `C:/cv/esp-idf-6.0.3`
- SDK HEAD: `76f5dedd9950a3012fee8fb7d5586df21fc67802`
- `tf-psa-crypto` HEAD: `ce3f3485a121c100f58f36d700cb35b060f6e866`
- 읽은 PSA 헤더와 `esp_config.h` 경로에 대한 Git status 출력은 비어 있었다.

이는 해당 로컬 SDK 헤더의 계약 확인이지, 전체 SDK checkout·현재 제품 설정·장치 실행 검증이 아니다.

## 3. Findings

### P0

없음.

### P1

없음.

### P2

없음.

### P3 — B-PSA-01: 시험용 VERIFY_MESSAGE 상수가 공식 PSA 권한값과 불일치

상태: `OPEN`

위치:

- `tests/ota/psa_crypto_fixture.h:23`
- 이를 기대값으로 사용하는 `tests/ota/test_esp_ota_crypto.c:39–41`
- 실제 adapter의 권한 설정: `firmware/platform/esp32s3/ota_crypto.c:55`

공식 SDK 근거:

`C:/cv/esp-idf-6.0.3/components/mbedtls/mbedtls/tf-psa-crypto/include/psa/crypto_values.h`

- `:2648`: `PSA_KEY_USAGE_VERIFY_MESSAGE = 0x00000800`
- `:2658`: `PSA_KEY_USAGE_SIGN_HASH = 0x00001000`

현재 시험 fixture는 `PSA_KEY_USAGE_VERIFY_MESSAGE`를 `0x1000U`로 정의한다. 이는 공식 SDK에서는 다른 권한인 `SIGN_HASH` 값이다.

Failure scenario:

host 모형에서는 adapter와 mock 기대값이 같은 잘못된 매크로를 사용하므로 권한 검사가 계속 성공한다. 예를 들어 권한 설정에 잘못된 literal `0x1000`을 전달하는 회귀도 이 모형의 import 검사는 구분하지 못한다. 공식 SDK에서는 그것이 VERIFY_MESSAGE 권한이 아니므로 같은 동작으로 취급할 수 없다.

영향:

- 시험용 PSA 계약 복제본과 공식 SDK 사이에 권한 상수 drift가 있다.
- 모형에서 확인한 권한값을 실제 PSA 정책값과 동일한 것으로 해석할 수 없다.
- **현재 제품 빌드의 권한 설정 오류는 아니다.** 제품 경로는 `ota_crypto.h:10`의 공식 PSA 헤더와 공식 매크로를 사용한다. 공개키에 실제 signing 권한이 부여됐다는 주장도 아니다.
- 영향이 시험 모형 정확성에 한정되므로 P3로 분류한다.

권고:

- fixture 상수를 공식 값 `0x0800U`로 수정한다.
- 복제한 PSA 상수의 SDK version·근거 위치를 남기거나, 공식 헤더와의 drift 검사로 같은 종류의 오류를 검출한다.
- 수정 뒤 `ota-psa-provider` 모형 시험을 재실행한다.

검증 수준: candidate source와 공식 로컬 SDK 헤더의 직접 대조. 변이 실행이나 SDK 권한 실패를 직접 재현한 것은 아니다.

## 4. API·수명·통합 검토 결과

### PSA 호출 계약

다음 사항은 공식 헤더와 일치하는 것으로 확인했다.

- 공개키는 secp256r1 256-bit, `0x04 || X[32] || Y[32]` 형식으로 import한다.
- key lifetime은 volatile이며 실제 adapter는 VERIFY_MESSAGE 용도로 제한한다.
- `psa_verify_message()`에 원문 manifest와 ECDSA(SHA256), raw `r || s` 64바이트를 전달한다. digest를 message로 전달해 이중 hash하는 경로는 없다.
- import 실패 시 key 출력이 0이라는 공식 계약과 mock의 동작이 일치한다.
- hash operation은 공식 initializer로 초기화한다.
- setup/update/finish의 SDK 실패 후 dirty 상태를 유지하고 reset을 요구한다.
- 성공한 finish 뒤에도 abort를 호출할 수 있다는 공식 계약에 따라 reset 경로가 구성돼 있다.
- finish 실패나 비정상 digest 길이에서는 출력 digest를 지우고 성공을 반환하지 않는다.

### Ownership·실패 경로

직접 공격한 경로:

- NULL, manifest/chunk 상한, context와 입력·출력 중첩, 주소 덧셈 overflow
- 재초기화, 초기화 전 callback, 중복 start, update/finish 순서 오류
- SDK 호출 중 init/close/verify/hash callback 재진입
- setup·update·finish 실패와 partial output
- abort/destroy 실패, closing 상태의 새 작업 차단, 재시도
- 성공·실패 상태와 SDK 결과의 반환 매핑

single-task·직렬 호출 계약을 기준으로 검토했다. `busy`를 mutex로 해석하지 않았다. 호출자에게 금지된 활성 context 복사·memset이나 동시 호출을 정상 사용으로 가정해 결함을 만들지 않았다.

다만 mock의 abort/destroy 실패는 설정된 오류와 자원 상태를 돌려주는 모형이다. 실제 SDK backend의 모든 실패 후 자원 상태·복구 가능성을 입증하는 시험으로 확대할 수 없다.

### CMake·SDK 설정

- root `CMakeLists.txt:57–63`은 실제 adapter와 mock 구현을 별도 시험 target으로 빌드하며, `CANVIEW_OTA_PSA_TEST=1`은 해당 target에 PRIVATE로 적용된다.
- component CMake는 `ota_crypto.c`와 필요한 include 경로, `mbedtls` 의존성을 추가한다.
- portable core에 PSA 헤더를 추가한 delta는 없다.
- 기존 workflow의 host·sanitizer CTest 경로와 SDK fixture build 경로를 확인했다.
- SDK fixture main의 새 호출은 NULL 음성 호출이다. 실제 P256 양성 실행을 확인하는 프로그램으로 취급하지 않았다.
- SDK `esp_config.h`에서 ECDSA·secp256r1·SHA256 지원이 설정에 따라 활성화되는 것을 확인했다. symbol 존재만으로 현재 제품의 알고리즘 활성화나 장치 양성 실행을 입증했다고 판단하지 않았다.

### 문서·synthetic digest

- shared README는 모형 PASS, SDK compile/link, 장치 암호 실행을 구분한다.
- journal의 이전 CI와 새 PSA 추가분 근거가 구분돼 있다.
- T103 fixture 다섯 record와 helper 기대값이 동일한 새 digest로 변경됐다. TX 금지 필드는 유지됐다.
- 해당 digest의 전체 source aggregate를 이번 실행에서 재계산하지는 않았다.
- 기존 signed golden·공개키·provenance, `.gitattributes`, workflow, SDK pin·OTA dependency lock은 이번 delta에서 불변임을 확인했다.
- 전체 delta의 `git diff --check`는 직접 실행하여 PASS였다.

## 5. 실제 읽은 파일

행 번호는 별도 SDK 항목을 제외하고 candidate 기준이다.

핵심 전체 읽기:

- `firmware/platform/esp32s3/ota_crypto.c:1–186`
- `firmware/platform/esp32s3/ota_crypto.h:1–60`
- `tests/ota/psa_crypto_fixture.h:1–43`
- `tests/ota/test_esp_ota_crypto.c:1–208`

변경·통합 경로:

- `CMakeLists.txt:1–72`
- `firmware/components/canview_esp32_ota/CMakeLists.txt` 전체
- `tests/fixtures/idf-ota-image/main/main.c` 전체
- `tests/fixtures/idf-ota-image/main/CMakeLists.txt` 전체
- `tests/fixtures/idf-ota-image/CMakeLists.txt` 전체
- `tests/fixtures/idf-ota-image/sdkconfig.defaults` 전체
- `shared/ota/README.md:303–334`
- `tests/hil/fixtures/t103-capture-only.jsonl` 전체 변경 diff
- `tests/test_t103_capture_helpers.py:1–115` 관련 구간과 변경 diff
- `.github/workflows/foundation.yml`의 CTest·sanitizer·SDK fixture·설정 검사·artifact 경로
- `shared/ota/src/body.h`의 hash provider·ownership 계약

정본·기록:

- `AGENTS.md`, `docs/README.md` 전체
- `docs/resume.md`, `docs/tasks/T-007-ota-container.md` 전체
- `docs/adr/009-ota-native-image-alignment.md` 전체
- `docs/architecture/ota.md:179–284`, §7–9
- `docs/runbooks/agent-workflow.md:149–188`
- `docs/journal.md` 이번 추가분

별도 공식 SDK 헤더:

- `psa/crypto.h`: key import/destroy, 공개키 형식, hash setup/update/finish/abort, verify-message 계약
- `psa/crypto_values.h:1533–1568`, `:2610–2685`: raw ECDSA 서명 형식과 usage flag
- `mbedtls/esp_config.h:536–550`, `:2124–2140`, `:2635–2652`: curve·ECDSA·SHA256 설정 연결

## 6. 실제 명령과 검증 한계

주요 실행 명령은 다음과 같다. 저장소 작업 디렉터리는 지정 repo였다.

```text
git cat-file -e '<base>^{commit}'
git cat-file -e '<candidate>^{commit}'
git rev-parse '<base>^{commit}'
git rev-parse '<candidate>^{commit}'
git diff --find-renames --stat <base> <candidate>
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames <base> <candidate> -- <변경 경로>
git show <candidate>:<위 열거 파일>
git diff --check <base> <candidate>
git diff --exit-code <base> <candidate> -- <불변 확인 경로>
```

추가로:

- PowerShell `[guid]::NewGuid()`, `[DateTime]::UtcNow`로 실행 식별자·시각 기록
- `ForEach-Object`로 source 행 번호 표시
- `rg --files`, `rg -n`으로 SDK 헤더·API·설정 위치 탐색
- 허용된 로컬 SDK 헤더를 `Get-Content -LiteralPath`로 읽음
- SDK와 tf-psa-crypto의 `git rev-parse HEAD`, 읽은 헤더 경로의 `git status --porcelain=v1` 확인

이번 실행에서 compiler·시험·SDK build·nm·coverage·CI 조회는 하지 않았다.

다음은 **작성자가 제공한 결과**이며 reviewer 직접 실행으로 집계하지 않는다.

- Debug/Release 각각 144/144
- ASan/UBSan 모형 PASS
- adapter 149/149 lines, 114/118 branches
- 실제 SDK fixture ELF/MAP/BIN, warning 0, PSA symbol 확인
- docs/plan/generator/SDK negative 13 PASS

미검토·미실행 범위:

- 실제 PSA 양성·음성 암호 실행, SDK backend 내부 구현 전체
- 장치에서의 heap·시간·entropy·hardware failure 동작
- 작성자 로그·coverage 수치·BIN digest의 독립 재계산
- 새 PSA candidate의 CI·artifact 감사
- 정상 OTA owner·BSP role-root 공급·provisioning·영속 정책·Flash 연결
- MCU native execution·boot·rollback·power-fault·physical/HIL

**physical/HIL: NOT_RUN. Vehicle TX: NO-GO.**

## 7. 최종 verdict

**CONDITIONAL**

핵심 4개 신규 source와 13파일 delta의 정적 검토를 완료했다. 제품 경로의 새 P0/P1/P2 결함은 발견하지 않았다. 시험 fixture의 PSA 권한 상수 불일치 B-PSA-01(P3)은 수정 또는 명시적 추적이 필요하다.

본 결과는 전체 T-007 완료, 정상 OTA 설치·제품 root/provisioning, 새 CI/target 검증 또는 merge 승인이 아니다. 장치 PSA 양성 실행과 physical/HIL은 계속 NOT_RUN이다.

