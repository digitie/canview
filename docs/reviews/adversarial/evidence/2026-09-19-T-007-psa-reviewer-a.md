# T-007 PSA provider Reviewer A 원본

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

# T-007 PSA provider 독립 적대적 리뷰 — Reviewer A

## 1. 실행 식별과 판정

- Reviewer: A / Maxwell
- 전문 영역: embedded C 수명, cleanup, partial initialization, 재진입, ownership, 메모리 경계
- Execution UUID: `3b58ee3b-3224-4f31-bbd6-8b8f7b6b3e26`
- 시작 UTC: `2026-09-19T03:49:40.8567557Z`
- 종료 UTC: `2026-09-19T03:51:25.5138672Z`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 Base: `6209eac0a12207e2473ef096a79b068bce4024d6`
- 실제 확인 Candidate: `70c7a384f3763b021546fd50f742ea51cf137dc4`
- Verdict: **PASS — 이번 13파일 delta의 정적 검토 범위에 한정**
- Findings: **P0 없음 / P1 없음 / P2 없음 / P3 없음**

핵심 provider와 모형 시험 소스는 전체를 직접 읽었다. 미열람 핵심 소스를 PASS로 추정한 결과가 아니다. 실행 시험 결과를 독립 재현한 판정도 아니다.

## 2. 격리와 독립성

시작 시 base/candidate 각각의 commit 존재를 `git cat-file -e`로 확인하고 `git rev-parse`로 요청 hash와 일치함을 확인했다. 종료 시 두 hash를 다시 확인했다.

저장소 소스와 문서는 고정 candidate의 `git show candidate:path`, 변경 내용은 고정 base/candidate 사이의 `git diff`로만 읽었다.

- Moving worktree의 소스를 읽거나 실행하지 않았다.
- Worktree HEAD 또는 clean 상태를 candidate 근거로 사용하지 않았다.
- Detached worktree 생성, 파일 수정, commit, push를 하지 않았다.
- 다른 에이전트를 소환하거나 reviewer 결과를 조회·공유하지 않았다.
- 과거 raw review와 통합 report 본문을 열지 않았다.
- 사용자 허용 범위에서 별도 설치 SDK의 `psa/crypto.h` 계약을 읽었다. 이를 candidate 제품 실행 증거로 취급하지 않았다.

`embedded-cstyle`, `embedded-architecture` 스킬을 직접 완독하고 반환값 처리·자원 소유권·SDK 경계 검토에 적용했다. 수정 작업은 수행하지 않았다.

## 3. Findings

| 등급 | 결과 |
|---|---|
| P0 | 없음 |
| P1 | 없음 |
| P2 | 없음 |
| P3 | 없음 |

이번 검토에서 구체적으로 입증한 신규 결함이 없으므로 finding별 실패 시나리오·영향·수정 권고는 없다. 아래에는 실제로 공격한 경로와 판정 근거를 기록한다.

## 4. 공격한 경로와 확인 근거

### 4.1 부분 초기화와 공개키 소유권

검토 위치: `firmware/platform/esp32s3/ota_crypto.c:36–63`, `ota_crypto.h:15–38`.

- NULL·context 중첩·주소 덧셈 overflow를 검사한 뒤 공개키 첫 byte를 읽는다.
- 이미 ready/busy/closing이거나 hash/key 자원을 보유한 context의 재초기화를 거절한다.
- SDK 호출 전 `busy`를 설정한다.
- Hash operation은 공식 초기화 함수로 초기화한다.
- 공개키는 P256, `VERIFY_MESSAGE`, ECDSA/SHA256, volatile 속성으로 import한다.
- Import 후 attributes를 정리하고 SDK 성공일 때만 `ready`가 된다.
- 입력 공개키 포인터를 context에 저장하지 않는다.

설치 SDK의 `psa/crypto.h:617–620`은 import 실패 시 출력 key가 0임을 명시한다. 따라서 실패한 import에서 정상 key를 소유한 것처럼 ready로 진행하는 경로를 발견하지 못했다.

Root는 신뢰된 BSP가 공급한다는 요청 계약을 적용했다. 이 adapter가 역할별 root를 직접 선정하지 않는다는 이유로 결함을 만들지 않았다.

### 4.2 Hash setup/update/finish 실패와 cleanup

검토 위치: `ota_crypto.c:107–180`.

- `hash_dirty`를 setup 호출 전에 설정하므로 partial setup도 abort 대상이다.
- Setup 실패 시 `hash_active`는 false이고 reset 전 새 start를 거절한다.
- SDK update 실패 시 active를 해제하면서 dirty 상태를 보존한다.
- Finish 뒤에는 active를 해제한다.
- SDK finish 실패 또는 32B가 아닌 결과는 성공으로 반환하지 않는다. SDK가 일부 출력한 digest도 지운다.
- 정상 finish 뒤에도 dirty 상태를 유지하여 공통 reset 절차를 거친다.
- Abort 성공 때만 dirty 상태를 해제한다.

공식 header의 `psa/crypto.h:967–976`, `1007–1008`, `1036–1038`, `1117–1142`와 대조했다. 특히 정상 finish 이후 abort가 안전한 무효 작업이라는 계약과 맞는다.

Adapter가 직접 거절한 잘못된 인자와 SDK operation 자체의 실패를 구분했다. 모든 인자 오류에서 기존 hash 상태가 반드시 파괴된다고 주장하지 않는다.

### 4.3 Close 실패, 재시도와 수명 종료

검토 위치: `ota_crypto.c:66–82`, `ota_crypto.h:40–44`.

- Close 진입 시 busy/closing을 설정한다.
- Dirty hash의 abort가 실패하면 key destroy로 진행하지 않는다.
- Abort 성공 뒤 hash 상태를 해제하고 key destroy를 수행한다.
- Cleanup 오류가 있으면 context 전체를 지우지 않는다.
- Close가 시작된 context는 이후 verify/start/update/finish/reset 진입에서 차단된다.
- 전체 cleanup 성공 때만 context를 초기 상태로 돌린다.
- 초기 `{0}` 또는 이미 정리된 context에 대한 반복 close는 자원 호출 없이 성공할 수 있다.

검토한 것은 실패 시 소유 상태를 보존하고 재사용을 차단하는 동작이다. 실제 SDK의 모든 영구 오류가 재시도로 복구된다는 보장은 확인하지 않았으며, 그러한 보장으로 해석하지 않는다.

### 4.4 Body와 provider의 종료 순서

검토 위치:

- `shared/ota/src/body.h:11–16,36–39,97–103`
- `shared/ota/src/body.c:6–43,91–116,232–256`
- `ota_crypto.h:40–56`
- `shared/ota/README.md:318–326`

기존 body는 start 전에 `hash_live`를 설정하고, reset 실패 때 provider/context를 계속 보존한다. Image finish 이후에도 hash reset을 수행한 뒤 다음 image로 넘어간다.

따라서 provider의 “정상 finish 후에도 reset 필요” 상태와 기존 body 흐름이 맞는다. 사용 계약은 **body_reset 성공 후 crypto_close**다. 이 순서를 뒤집으면 body가 보존한 provider의 reset을 closing 상태가 차단할 수 있으므로, 문서화된 순서를 전제로 검토했다.

### 4.5 재진입과 단일 task 계약

검토 위치: `ota_crypto.c:27–33,44–62,66–82,99–104,107–180`, `test_esp_ota_crypto.c:20–33`.

SDK 호출 중 init/close/verify/hash callback으로 재진입하는 경로를 추적했다. 자원 사용 전 busy가 설정되고 정상 반환 경로에서 해제된다.

`busy`는 atomic mutex가 아니다. `ota_crypto.h:15–18`과 README는 단일 task 소유 및 ISR/동시 호출 금지를 명시한다. 이번 요청의 single-task 계약과 일치하며, 다중 task 안전성을 주장하지 않는다.

### 4.6 입력·출력 경계와 alias

검토 위치: `ota_crypto.c:19–24,39–46,89–102,124–165`.

- Context와 입력·출력의 겹침을 검사한다.
- Pointer-range 덧셈 overflow를 검사한다.
- Manifest는 1..16KiB로 제한한다.
- Chunk는 0..16KiB이며 NULL은 길이 0일 때만 허용한다.
- Signature는 고정 64B, digest는 고정 32B 계약이다.
- 길이 0 update는 SDK를 호출하지 않는다.
- Root·message·signature·chunk 포인터를 호출 이후 보존하지 않는다.
- Provider 자체에 직접 heap 할당이나 입력 크기 비례 stack buffer가 없다.

이 검사는 C 포인터의 실제 접근 가능 메모리 크기를 증명하는 기능은 아니다. 호출자가 선언된 길이의 유효한 storage를 제공해야 한다. SDK 내부 heap 사용량·stack·실행시간 상한도 이번 정적 검토로 측정하지 않았다.

### 4.7 모형 시험의 강도와 한계

`tests/ota/psa_crypto_fixture.h:1–43`, `test_esp_ota_crypto.c:1–208`을 완독했다.

모형에는 다음 검사가 존재한다.

- Import attributes와 고정 입력 계약 대조
- SDK 호출 중 재진입 시도
- Setup 실패에도 partial operation이 남는 상황
- Finish 실패 시 partial digest 출력
- Abort/destroy 실패 후 context 보존과 후속 차단
- 호출 단계 8개와 오류 5종 조합
- NULL·상한·중첩·주소 overflow·중복 호출·short digest

다만 이 모형은 정상 finish 후에도 operation을 active로 남기는 등 실제 SDK보다 보수적인 일부 상태를 사용한다. 실제 공개키 수학 검증·ECDSA·SHA256 구현을 수행하지 않는다. 테스트 출력과 문서가 이를 모형으로 구분하고 있어 실제 암호 실행 근거로 확대하지 않았다.

## 5. 전체 delta 회귀

변경 목록 13파일을 확인하고 각 변경 내용을 읽었다.

- Root CMake는 실제 adapter 소스를 모형 헤더와 함께 빌드하는 별도 CTest를 등록한다. `CANVIEW_OTA_PSA_TEST=1`은 해당 시험 target의 PRIVATE 정의다.
- `firmware/components/canview_esp32_ota/CMakeLists.txt:1–6`은 SDK component에 adapter 소스와 `mbedtls` dependency를 추가한다. 이 component에 모형 macro를 추가하지 않았다.
- `tests/fixtures/idf-ota-image/main/main.c:13–18`의 새 호출은 NULL negative와 함수표 연결이다. 정상 key import·PSA 양성 검증·장치 실행 증거가 아니다.
- `shared/ota/README.md:310–326`은 SDK provider, 모형, 정상 owner 미연결을 구분한다.
- Resume/task/journal의 새 내용은 모형 coverage, SDK compile/link, 장치 미실행을 구분한다. 이전 golden CI를 이번 PSA candidate의 CI 증거로 판정하지 않았다.
- T103 JSONL의 다섯 행과 helper 기대값은 동일한 synthetic firmware identity로 변경됐다. No-TX 필드·시험 판정 로직을 완화한 delta는 없었다.

Synthetic identity `e46a8647a68968b8273b41d59c548fdc817075078305df453a27840f2a450444`의 전체 source digest는 독립 재계산하지 않았다. 실제 HIL evidence로 취급하지 않았다.

## 6. 실제 읽은 파일·명령

핵심 전체 열람:

- `firmware/platform/esp32s3/ota_crypto.c:1–186`
- `firmware/platform/esp32s3/ota_crypto.h:1–60`
- `tests/ota/psa_crypto_fixture.h:1–43`
- `tests/ota/test_esp_ota_crypto.c:1–208`
- `firmware/components/canview_esp32_ota/CMakeLists.txt`
- `tests/fixtures/idf-ota-image/main/main.c`
- `shared/ota/src/body.h:1–105`

필요 부분·delta 열람:

- Root `CMakeLists.txt` 추가 시험 등록부
- `shared/ota/src/body.c:1–150,180–257` 및 hash/reset 관련 검색 문맥
- `shared/ota/README.md:300–330`
- `tests/hil/fixtures/t103-capture-only.jsonl` 전체 delta
- `tests/test_t103_capture_helpers.py:1–85` 및 delta
- `docs/journal.md` 이번 추가분

정본:

- Candidate의 `AGENTS.md`, `docs/README.md`, `docs/resume.md`, T-007 task
- `docs/architecture/ota.md:179–285` — §7–9
- ADR-009, `docs/runbooks/agent-workflow.md`

SDK 별도 계약:

`C:/cv/esp-idf-6.0.3/components/mbedtls/mbedtls/tf-psa-crypto/include/psa/crypto.h`

- 직접 읽은 구간: 508–665, 950–1145
- SHA256: `b3cfca32e17621d50f096c27dd5325b0dd16d91cac294ba4ae328413d777bf91`
- 설치 경로의 header를 읽은 것이며 SDK checkout 전체의 commit/clean 상태나 실제 실행을 인증한 것은 아니다.

실제 명령 종류:

```powershell
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('o')

git cat-file -e '<base>^{commit}'
git cat-file -e '<candidate>^{commit}'
git rev-parse '<base>^{commit}' '<candidate>^{commit}'

git diff --find-renames --stat <base> <candidate>
git diff --name-only <base> <candidate>
git diff --find-renames <base> <candidate> -- <명시한 검토 파일>
git show <candidate>:<위 열람 파일>
git diff --check <base> <candidate>
```

`<base>`와 `<candidate>`는 §1의 full hash를 사용했다. `git show` 출력에 PowerShell 행 번호·구간 필터를 적용했다. SDK header 탐색에는 `rg --files`, `rg -n`, 읽기에는 `Get-Content`, digest에는 `Get-FileHash`를 사용했다.

`git diff --check`는 출력 없이 종료했다. 컴파일러·CTest·sanitizer·repository 시험 명령은 실행하지 않았다.

## 7. 작성자 제공 근거와 미실행 범위

다음은 작성자 제공 결과이며 reviewer 독립 실행 결과가 아니다.

- Debug/Release 각각 144/144
- ASan/UBSan 모형 PASS
- Adapter 행 149/149, 분기 114/118
- 실제 ESP-IDF6.0.3 fixture ELF/MAP/BIN 경고 0
- PSA symbol의 nm 확인
- Docs/plan/generator/SDK negative 13 PASS

이번 실행에서는 해당 build/test/coverage 로그, ELF/MAP/BIN, nm 출력 및 새 CI artifact를 직접 검증하지 않았다.

미검토·미실행:

- Host 시험·sanitizer·coverage 재실행: `NOT_RUN`
- SDK build 및 실제 PSA 양성 암호 실행: `NOT_RUN`
- SDK 내부 math·driver 구현, 전체 SDK config 조합 감사
- SDK 내부 자원 부족의 실제 fault injection 및 영구 오류 복구
- 정상 OTA owner·trusted root 공급·영속 policy 통합
- Production signing/provisioning, Flash·boot·rollback
- Physical/HIL: `NOT_RUN`
- Vehicle TX: `NO-GO`

## 8. 최종 결론

**이번 PSA provider 13파일 delta의 Reviewer A 정적 verdict는 PASS이며, P0/P1/P2/P3 finding은 없다.**

확인한 범위는 명시된 single-task·trusted BSP root 계약 아래의 자원 수명, 오류 처리, 재진입 차단, 메모리 경계와 관련 delta다. 전체 T-007 완료, 실제 장치 PSA 검증, CI/target/HIL 또는 merge 승인이 아니다. **PR36 Draft, physical/HIL NOT_RUN, vehicle TX NO-GO**를 유지한다.

