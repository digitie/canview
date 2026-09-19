# T-007 PSA 두 번째 post-fix Reviewer B 원본

## 전달 요청 원문

```text
T-007 PSA 두 번째 post-fix 독립 재검토. 같은 원 A/B 전문범위 유지, 새 executionUUID/UTC시작종료. Repo F:/dev/canview-wt/t007-ota-container, base 6f078ace276eecce4f8ceabf464929de5eb6bfd7, candidate 1b06afd194705f87a0d74d65a2de40daba626211.
이번 변경은 tests/ota/test_esp_ota_crypto.c의 signature 중첩 시험을 실제ready unioncontext로 이동한11행과 리뷰/evidence/currentstate 기록이다. A-PSA-POST-01(P2)/B-PSA-02(P2)를 수정한다. reentry대상바꿈→initOK→verify호출수저장→중첩거절→호출수불변→정상verifyOK/호출수+1→closeOK→기존reentry복원. provider.c/h 불변. B-PSA-01(P3) 원B FIXED 유지.
두 이전raw는보존됐으며 새post결과는 서로공유하지말것. Gitobject-only: 시작종료cat-file/rev-parse, gitshowcandidate:path/diffbasecandidate만읽기. movingworktree/파일수정/commit/compile/다른agent소환금지. 핵심시험 source와provider경로 직접읽고 자신의finding 및전체delta회귀 확인.
작성자 실행과 구분: Debug/Release144/144/경고0; GCC-O3정상모형PASS; signature중첩조건만삭제한실제Cmutant가중첩CHECK(line152)에서실패; oldusage0x1000 header mutant는독립typedef컴파일실패; ASanUBSan모형행149/149분기114/118. target source불변, 최신전체CI진행예정. 실제PSAdevice/Flash/HIL NOT_RUN·TXNO-GO·owner통합미완료·PR36Draft.
최종raw는 반복설명은 줄이되 필수내용누락없이 한국어: executionUUID/시각/전문/basecandidate/격리·실제읽은파일행/명령/원findingFIXED여부/P0~P3신규finding(정확위치·실패시나리오·영향·권고)/미검토/NOT_RUN/BLOCK CONDITIONAL PASS. 판정은이delta정적범위이며wholeTask승인아님. 읽지못하면incompleteBLOCK. 완전한raw최종응답 반환.
```

## 반환 원문

# T-007 PSA 두 번째 post-fix 독립 재검토 — Reviewer B

## 실행 및 기준선

- Execution UUID: `74d41417-6ca7-4cf5-8e98-47e3073cef65`
- 시작 UTC: `2026-09-19T04:10:37.3897193Z`
- 종료 UTC: `2026-09-19T04:11:59.5094736Z`
- 역할: Reviewer B / Huygens
- 전문범위: PSA 상수·SDK 계약, 시험의 false-pass 및 회귀 검출력, CMake/CI 포함 관계, 검증 근거의 source 귀속.
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 Base: `6f078ace276eecce4f8ceabf464929de5eb6bfd7`
- 실제 확인 Candidate: `1b06afd194705f87a0d74d65a2de40daba626211`

판정: **PASS — 이번 delta의 독립 정적 검토 범위에 한정.**

B-PSA-02(P2)는 FIXED이며 B-PSA-01(P3)의 FIXED 상태도 유지된다. 신규 P0/P1/P2/P3 finding은 없다. 전체 T-007·최신 CI·target 실행·merge 승인이 아니다.

## 격리와 실제 수행

시작·종료에 양쪽 commit의 `git cat-file -e '<hash>^{commit}'` 및 `git rev-parse '<hash>^{commit}'`를 수행했다. 객체 존재와 반환 hash가 manifest와 일치했다.

저장소 내용은 고정 hash의 `git show`와 `git diff`로만 읽었다. moving worktree 소스, 로컬 build 로그 및 SDK 설치 파일은 읽지 않았다. detached worktree는 만들지 않았으므로 HEAD/clean 검사는 해당 없다.

파일 수정·commit·compile·test 실행·다른 agent 호출 및 새 리뷰 결과 공유는 하지 않았다. 과거 통합 disposition은 역사 기록으로만 읽었으며, raw A/B evidence 본문과 상대의 새 post-fix 결과는 열지 않았다.

실제 명령군:

```text
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('o')
git cat-file -e '<base/candidate>^{commit}'
git rev-parse '<base/candidate>^{commit}'
git diff --find-renames --stat <base> <candidate>
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames <base> <candidate> -- . ':!docs/reviews/adversarial/evidence/*'
git show <candidate>:<path>
git diff --exit-code <base> <candidate> -- <불변 확인 경로>
git diff --check <base> <candidate> -- . ':!docs/reviews/adversarial/evidence/*'
```

`git show` 출력에 PowerShell 행 번호를 붙이고 필요한 구간은 `Select-Object`·`rg`로 선택했다. 도구 출력이 잘린 구간은 후속 `git show`로 보완했다.

불변 확인 결과는 exit 0이었다. 확인 경로는 `firmware`, `shared`, root CMake, foundation workflow, `tests/fixtures`, PSA fixture header, AGENTS 및 관련 정본·workflow·review template이다. raw evidence를 제외한 delta의 `git diff --check`도 exit 0이었다. raw 두 파일의 원문 보존·whitespace 적합성은 인증하지 않았다.

## 직접 읽은 파일과 범위

행 번호는 candidate 기준이다.

| 파일 | 실제 읽은 범위 |
|---|---|
| `tests/ota/test_esp_ota_crypto.c` | 1–222 전체 |
| `firmware/platform/esp32s3/ota_crypto.c` | 1–186 전체 |
| `firmware/platform/esp32s3/ota_crypto.h` | 1–60 전체 |
| `tests/ota/psa_crypto_fixture.h` | 1–44 전체 |
| `tests/fixtures/idf-ota-image/main/main.c` | 1–32 전체 |
| `CMakeLists.txt` | 55–64 |
| `AGENTS.md`, `docs/README.md` | 전체 |
| `docs/resume.md` | 1–175 전체 |
| `docs/tasks/T-007-ota-container.md` | 1–169 전체 |
| `docs/architecture/ota.md` | 179–285, §7–9 |
| `docs/adr/009-ota-native-image-alignment.md` | 전체 |
| `docs/runbooks/agent-workflow.md` | 관련 제목 검색 및 107–170 |
| `docs/reviews/README.md` | 1–20 및 추가 index 행 |
| `docs/reviews/adversarial/TEMPLATE.md` | 전체 |
| `docs/reviews/adversarial/2026-09-19-T-007-psa-post.md` | 신규 1–60 전체 |
| `docs/journal.md` | 이번 추가분 3–30 |

외부 스킬 `embedded-cstyle/SKILL.md`, `embedded-documentation/SKILL.md`는 전체 읽었다. 각각 C 상태·반환값·경계 검사와 문서의 실행 근거 구분에 적용했으며 자동 수정은 하지 않았다.

## 원 finding 재판정

### B-PSA-02 — P2 — FIXED

기존 실패 시나리오는 signature 중첩 시험의 context가 unready여서, production의 signature 중첩 조건을 제거해도 `crypto_enter()`의 unready 거절로 같은 `INVALID_ARGUMENT`가 반환되는 것이었다. 영향은 현재 production 우회가 아니라 중첩 방어 회귀를 놓치는 시험의 false-pass였다.

수정 근거:

- `tests/ota/test_esp_ota_crypto.c:89`: 65바이트 union backing을 유지한다.
- `:149–150`: reentry 대상을 union context로 바꾸고 실제 초기화의 `CANVIEW_OK`를 요구한다.
- `:151–153`: verify 호출 수를 저장하고 중첩 거절 및 SDK verify 호출 수 불변을 각각 검사한다.
- `:154–155`: 같은 context의 정상 verify 성공과 호출 수 +1을 검사한다.
- `:156–157`: close 성공과 누적 mock 계약 검사를 확인한 뒤 기존 reentry 대상을 복원한다.

Production의 `ota_crypto.c:61`에서 초기화 성공 시 ready가 설정된다. 따라서 `:95`의 signature 중첩 조건만 제거하면 이 시험은 더 이상 `:31`의 unready 거절로 빠질 수 없다. `:101–102`의 mock verify까지 도달하고, 현재 무고장 모형은 성공을 반환하므로 시험 `:152`의 기대 거절과 불일치한다. `:153`의 호출 수 검사도 SDK 진입을 별도로 감시한다.

이는 소스 경로에 대한 정적 판정이다. 작성자가 보고한 실제 mutant 실패를 본인이 실행·재현했다고 주장하지 않는다.

권고했던 정상 초기화된 충분한 backing, 호출 수 확인, 후속 정상 동작 및 cleanup이 반영됐다. 이 finding에 대한 추가 수정 권고는 없다.

### B-PSA-01 — P3 — FIXED 유지

- `tests/ota/psa_crypto_fixture.h:24`: `VERIFY_MESSAGE`는 `0x0800U`.
- `tests/ota/test_esp_ota_crypto.c:10`: 독립 literal `0x00000800`과 비교하는 C99 typedef 검사가 유지된다.
- SDK fixture `main/main.c:10`: 공식 헤더 값을 같은 독립 literal과 비교하는 `_Static_assert`가 유지된다.
- Provider `ota_crypto.c:55`: 해당 usage만 설정하는 코드가 불변이다.

fixture 상수를 다시 `0x1000`으로 변경하면 typedef 배열 크기가 음수가 되는 구조다. 이번 실행에서 공식 SDK header 재감사나 header mutant 컴파일은 하지 않았다.

## 신규 findings 및 공격 범위

- P0: 없음.
- P1: 없음.
- P2: 없음.
- P3: 없음.

다음 시나리오를 직접 추적했다.

- unready·busy·closing 오류가 중첩 검사의 누락을 대신 가리는 경우.
- 거절 status만 맞고 실제 SDK verify가 호출되는 경우.
- 중첩 입력 거절 뒤 context가 손상되거나 busy가 남는 경우.
- reentry 대상 교체 누락으로 다른 context를 재진입 검사하는 경우.
- 추가 init/close가 기존 절대 호출 수 검사 및 후속 실패 주입 loop를 깨뜨리는 경우.
- 65바이트 backing 수정의 역행 또는 기존 음성 시험 삭제.
- 수정 시험의 CTest 포함 관계 상실.
- 과거 golden CI/artifact 결과를 최신 PSA candidate의 검증 결과로 재사용하는 문서 오류.

새 블록은 기존 정상 context가 close된 뒤 실행되고, 기존 destroy/abort 절대 횟수 검사 뒤에 배치된다. 추가 verify는 저장한 횟수 기준으로 비교하며, 이후 실패 주입 전에 reentry가 복원된다. 새 수명·순서 회귀는 발견하지 않았다.

Root CMake `:57–63`은 실제 provider source와 해당 시험을 같은 모형 target으로 빌드하고 CTest에 등록한다. 이 등록 및 workflow는 이번 delta에서 불변이다.

`docs/journal.md:21–27`, `docs/resume.md:33–45`, task `:99–112`는 `262bf09`의 과거 artifact 감사와 새 PSA 수정본의 미완료 reviewer/CI gate를 구분한다. 이번 변경에서 과거 결과를 candidate의 전체 통과로 확대하는 새 주장은 발견하지 않았다.

## 실행 근거와 미검토 범위

본인이 수행한 것은 Git 객체 확인, source/diff 정적 검토, 불변 경로 비교와 raw 제외 whitespace 검사뿐이다.

다음은 작성자 제공 결과이며 독립 실행·로그 인증하지 않았다.

- Debug/Release 각각 144/144 및 경고 0.
- GCC-O3 정상 모형 PASS.
- signature 중첩 조건만 제거한 C mutant의 CHECK 실패.
- `0x1000` header mutant의 typedef 컴파일 실패.
- ASan/UBSan 및 행 149/149·분기 114/118.

본인 compile/test/mutant/sanitizer 실행은 모두 **NOT_RUN**이다. 최신 CI와 artifact, 실제 SDK 런타임 암호, raw evidence의 byte 단위 보존, 과거 golden artifact의 hash·CRLF 대조는 독립 검증하지 않았다.

이번은 기존 finding 수정과 delta 회귀 검토이며 SDK 내부 구현이나 전체 OTA 보안 재감사가 아니다. 정상 owner/root/영속 policy 통합 미완료를 이번 delta의 신규 결함으로 계산하지 않았다.

- 실제 PSA device 실행: **NOT_RUN**
- Flash·physical/HIL: **NOT_RUN**
- Vehicle TX: **NO-GO**
- 정상 OTA owner 통합: 미완료
- PR36: Draft 유지

## 최종 판정

**PASS — `6f078ace…` → `1b06afd1…` delta의 정적 재검토 한정.**

B-PSA-02(P2)는 FIXED, B-PSA-01(P3)는 FIXED 유지이며 신규 finding은 없다. 핵심 시험과 provider 경로를 직접 읽었으므로 incomplete 판정은 아니다. 이 결과는 전체 T-007 완료, 실제 장치 검증, 최신 CI 통과 또는 merge 승인을 뜻하지 않는다.

