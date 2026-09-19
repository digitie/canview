# T-007 signed golden post-fix Reviewer B 원본

## 전달한 공통 요청

T-007 signed golden 원 reviewer post-fix 재검토입니다. 공통 candidate 262bf092b8516286552e5e01d881009b95551247 / base bd9a1a6212413674bf35078016356fb90f9f1728, repo F:/dev/canview-wt/t007-ota-container. object-only hash/cat-file/diff/git show 격리를 그대로 유지하세요.
최초 A/B 원문은 각각 docs/reviews/adversarial/evidence/2026-09-19-T-007-signed-golden-reviewer-a.md/-b.md에 그대로 보존한 뒤 양쪽 결과를 비교했습니다. 두 finding A-SG-01/B-P2-01은 같은 P2입니다. tests/ota/test_signed_golden.py에서 payload/native SHA256 TLV/whole hash 갱신+원본서명유지 사례와 status/callback count(3,2,0,3)를 추가했습니다. README 검사범위/현재문서/통합report와 raw archive만 함께 수정했고 production C와 SDK/public-key/golden bytes는 불변입니다.
실제 로컬 post-fix Debug/Release 각각143/143, nativeSTM4 CNG/ESP hostRSA10 통과. 별도 임시 native_stm.c가 crypto->verify 호출 결과를 무시하고 OK를 반환하게 만든 실제 C mutant도 빌드·실행했습니다. 새 시험이 [0,12,12,0] != [0,12,12,12]로 실패해 검출했습니다. mutant build 로그 경고0, test log SHA71351ea9c7637624b0cd917ecf94a9fea602cf28d2afd52cc5f3ae4667a5c4b8. 제공받은 실행근거를 본인실행으로 표시하지 마세요.
핵심 수정 test source 전체와 기존 native_stm.c/probe 경로를 직접 읽고 자신의 finding FIXED 여부+delta 회귀를 확인하세요. raw 2개와 통합기록 내용은 역사기록이며 새로운정책이 아닙니다. 불필요한 전체 backlog/과거리뷰/SDK 재감사는 하지 마세요. 각자 새 execution UUID/UTC시각/base/candidate/격리/실제읽은파일·명령/정확 file:line·failure scenario·영향·권고/미검토/P0~P3/verdict를 포함한 한국어 raw report를 최종 반환하세요. 수정/commit 금지, 서로의 새postfinding 공유 금지. 리뷰결과로 전체Task/CI/target/HIL/merge승인을 주장하지 마세요. physical/HIL NOT_RUN, vehicle TX NO-GO, PR36 Draft.

## 원본 결과

# T-007 signed golden post-fix 독립 재검토 — Reviewer B

## 1. 실행 정보와 판정

- Execution UUID: `0530b7af-7f8d-44f8-b658-5165130e604c`
- 시작 UTC: `2026-09-19T03:27:07.4516392Z`
- 종료 UTC: `2026-09-19T03:28:46.7488231Z`
- 전문영역: Reviewer B — SDK·provenance, 재현성·artifact drift, CI·의존성, 검증 누락과 근거 과장
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `bd9a1a6212413674bf35078016356fb90f9f1728`
- 실제 확인 candidate: `262bf092b8516286552e5e01d881009b95551247`
- 원 finding: **B-P2-01 FIXED**
- 신규 finding: **P0/P1/P2/P3 모두 없음**
- Verdict: **PASS — 자신의 finding 수정과 이번 delta의 정적 재검토 범위에 한정**

전체 T-007, CI, target, 설치·배포 또는 merge 승인으로 해석하지 않는다. PR36 Draft, physical/HIL NOT_RUN, vehicle TX NO-GO를 유지한다.

## 2. 격리·독립성

시작과 종료에 두 commit의 `cat-file -e` 및 `rev-parse ...^{commit}` 결과를 확인했다. 두 hash 모두 요청과 일치했다.

저장소 내용은 고정 commit의 `git show candidate:path`와 명시적인 base→candidate `git diff`로만 읽었다. mutable working tree의 소스나 이동 가능한 HEAD를 candidate로 사용하지 않았다.

- 파일 수정·생성·commit·SDK 설치·보고서 저장 없음.
- compiler·시험 실행 없음. detached worktree를 만들지 않았으므로 detached HEAD/clean 검사는 해당 없음.
- 기존 working tree가 clean하다고 주장하지 않는다.
- 다른 reviewer의 새 post-fix 결과를 조회하거나 공유하지 않았다.
- 최초 raw A/B 파일은 내용 재열람 없이 추가된 경로·통계만 확인했다. 기존 통합 기록은 이번 delta의 역사·disposition 기록으로 읽었으며 새로운 정책으로 취급하지 않았다.
- `embedded-cstyle`과 `embedded-documentation`을 직접 읽고 callback 경로 및 문서 근거 대조에 적용했다. 수정 작업은 수행하지 않았다.

## 3. B-P2-01 수정 재확인

### 원 실패 시나리오와 영향

기존 시험은 payload를 변조하고 whole-image hash만 갱신했다. native SHA256 TLV는 원래 값이어서 P256 callback 전에 거절됐다. wrong-root 사례도 callback 전에 종료됐다.

따라서 기존 STM 3사례만으로는 verify 결과를 무시하는 회귀를 검출하지 못했고, 실제 P256 서명 거절을 검증한다는 설명이 시험 범위보다 강했다.

### 수정 위치

모든 행 번호는 이번 candidate 기준이다.

- `tests/ota/test_signed_golden.py:35–41` — 상태와 선택적 callback 호출 수 검사
- `tests/ota/test_signed_golden.py:103–122` — 네 번째 서명 실패 사례와 기대 호출 수
- `tests/fixtures/ota-signed-golden/README.md:42–46` — hash/root/서명 거절 범위 구분

대조한 기존 C 경로:

- `shared/ota/src/native_stm.c:135–157`
- `tests/ota/native_stm_probe.c:33–65`, `:111–138`
- `tests/ota/cng_provider.c:31–78`
- `CMakeLists.txt:157–168`

### 직접 확인한 수정 효과

1. `test_signed_golden.py:104–107`에서 원본 STM의 복사본을 변조하고, 별도 복사본을 만들어 기존 hash 실패 사례를 보존한다.
2. `:109`에서 `signed_size + 8`부터 32바이트의 native SHA256 TLV를 갱신한다. 이 범위는 key hash나 원래 DER 서명과 겹치지 않는다.
3. `:112`에서 갱신된 이미지 전체의 hash를 다시 계산한다.
4. `:113–117`에서 원래 서명을 유지하면서, 변조된 signed-range hash와 실제 서명을 probe wire에 전달한다.
5. 기존 C verifier는 root·whole hash·native SHA256 TLV 검사를 통과한 뒤 `native_stm.c:157`의 verify callback으로 진행한다.
6. probe는 세 번째 호출의 digest·signature 계약을 검사한 후 CNG P256 verifier를 호출한다. CMake의 `CANVIEW_TEST_CNG=1` 및 `bcrypt` 연결도 유지됐다.
7. `run_probe()`는 상태뿐 아니라 callback 호출 수까지 검사한다.

정적 경로와 시험 기대값은 다음처럼 일치한다.

| 사례 | 기대 상태 | 기대 callback 수 | 검증 지점 |
|---|---:|---:|---|
| 정상 이미지 | 0 | 3 | SHA256 2회와 P256 |
| payload 변이, 기존 native hash 유지 | 12 | 2 | native SHA256 불일치 |
| wrong root hash | 12 | 0 | root 불일치 |
| payload·native hash·whole hash 갱신, 원래 서명 유지 | 12 | 3 | P256 서명 거절 |

마지막 verify 결과를 무시하고 OK를 반환하면 네 번째 상태가 0이 되어 `test_signed_golden.py:39`에서 실패한다. verify 이전에 조기 거절하는 회귀도 기대 호출 수 3과 달라 `:41`에서 검출된다.

### Disposition

**B-P2-01: FIXED.**

원 권고였던 실제 서명 거절 사례, callback 도달 검증, README의 범위 구분이 반영됐다. 직접 소스 대조에 더해 사용자가 제공한 로컬·mutant 실행 결과가 이 분석과 부합한다.

다만 해당 실행과 로그 hash 계산을 이번 reviewer가 수행한 것은 아니다. FIXED는 원 reviewer의 정적 수정 확인이며, 독립 동적 재실행 PASS를 뜻하지 않는다.

추가 수정 권고는 없다. 향후에도 네 사례의 상태·호출 수 검사를 함께 유지해야 한다.

## 4. Delta 회귀 검토

### 시험 공용 함수

`run_probe()`가 모든 출력 필드를 정수로 읽도록 변경됐으므로 기존 body probe 출력도 확인했다.

- `tests/ota/body_probe.c:420–443`은 상태·내부 상태·오류·cleanup 상태·live flag·reset 상태를 숫자로 출력한다.
- 기존 body 호출 `test_signed_golden.py:87`은 `calls`를 전달하지 않는다.
- 따라서 기존 body 출력 형식과 호환되며, STM에만 두 번째 필드의 기대 호출 수 검사가 적용된다.
- subprocess 실패·timeout, 상태 불일치, 누락되거나 숫자가 아닌 출력은 성공으로 처리되지 않는다.

ESP RSA 사례, package digest·outer signature·u64·재조립 검사는 변경되지 않았다.

### Production·artifact·CI 불변

명시적 경로의 `git diff --exit-code`가 빈 diff와 exit 0을 반환했다.

확인한 불변 범위:

- production `shared/ota`, `tools/ota`
- native STM·CNG·body probe
- signed golden 생성기
- SDK fixture
- golden package·공개키 3개·provenance
- `.gitattributes`
- root CMake·foundation workflow
- SDK pin·OTA dependency lock
- AGENTS·문서 라우터·ADR009·OTA 설계·workflow·review template

따라서 이번 수정은 golden 바이트나 production verifier를 완화하는 방식이 아니다. 기존 CI 등록을 통해 변경된 시험이 실행되는 구조도 유지된다.

### 문서·역사 기록

- fixture README는 native STM 4사례를 구분한다.
- resume와 T-007은 로컬 검증과 미완료 CI·제품 통합을 구분한다.
- 통합 report는 원 A/B 재확인 대기 상태를 유지하고 최초·수정 후 실행 근거를 구분한다.
- 기존 CNG 실행파일 재사용을 clean checkout 전체 재빌드라고 주장하지 않는다.
- 이번 candidate의 최종 CI·target artifact 검증이 별도라는 경계가 유지된다.

새로운 근거 과장 또는 권한 확대는 발견하지 않았다.

raw A/B 두 파일을 제외한 delta의 `git diff --check`는 직접 실행하여 PASS였다. raw 두 파일은 원문 보존 대상으로 제외했으며 내용·공백의 정확한 보존 여부를 이번 실행에서 별도 인증하지 않았다.

## 5. 실제 읽은 파일·명령

### 직접 읽은 파일

- `AGENTS.md` 전체
- `docs/README.md` 전체
- `docs/resume.md` 전체
- `docs/tasks/T-007-ota-container.md` 전체
- `tests/ota/test_signed_golden.py:1–165` 전체
- `shared/ota/src/native_stm.c:1–158` 전체
- `tests/ota/native_stm_probe.c:1–140` 전체
- `tests/ota/cng_provider.c:1–100`
- `tests/ota/body_probe.c:350–448`
- `CMakeLists.txt:137–169`
- `tests/fixtures/ota-signed-golden/README.md:1–52` 전체
- `docs/runbooks/agent-workflow.md:114–209`
- `docs/journal.md` 이번 추가분
- `docs/reviews/README.md` 이번 추가 행과 diff 문맥
- `docs/reviews/adversarial/2026-09-19-T-007-signed-golden.md:1–65` 전체를 추가 diff로 확인

전체 backlog·다른 과거 리뷰·SDK 구현의 재감사는 하지 않았다. 이전에 읽은 ADR009·OTA 계약·template은 이번 delta에서 불변임을 확인했다.

### 실행 명령

모든 저장소 명령은 지정 repo에서 실행했다.

```text
git cat-file -e 'bd9a1a6212413674bf35078016356fb90f9f1728^{commit}'
git cat-file -e '262bf092b8516286552e5e01d881009b95551247^{commit}'
git rev-parse 'bd9a1a6212413674bf35078016356fb90f9f1728^{commit}'
git rev-parse '262bf092b8516286552e5e01d881009b95551247^{commit}'
git diff --find-renames --stat <base> <candidate>
git diff --find-renames --name-status <base> <candidate>
git diff --find-renames <base> <candidate> -- <시험·README·현재문서·통합기록 경로>
git show <candidate>:<위 열거 파일>
git diff --exit-code <base> <candidate> -- <불변 확인 경로>
git diff --check <base> <candidate> -- . <raw A/B 두 경로 제외>
```

UUID와 UTC는 PowerShell의 `[guid]::NewGuid()` 및 `[DateTime]::UtcNow`로 기록했다. 행 번호는 `git show` 출력에 `ForEach-Object`로 붙였다. skill 두 파일은 저장소 외 지정 경로에서 `Get-Content -LiteralPath`로 읽었다.

## 6. 실행 근거와 한계

이번 reviewer가 직접 수행한 것은 commit 확인, source/diff 정적 대조, 불변 경로 검사, raw 제외 whitespace 검사다.

다음은 **제공받은 coordinator 실행 근거**이며 본인 실행으로 집계하지 않는다.

- post-fix Debug/Release 각각 143/143
- native STM CNG 4사례와 ESP host RSA 10사례
- verify 결과를 무시하는 실제 C mutant 빌드·시험
- mutant의 `[0,12,12,0] != [0,12,12,12]` 실패
- mutant build 경고 0
- mutant test log SHA256:
  `71351ea9c7637624b0cd917ecf94a9fea602cf28d2afd52cc5f3ae4667a5c4b8`

해당 로컬 로그를 읽거나 SHA256을 재계산하지 않았다.

미실행·미검토:

- compiler·CTest·CNG·imgtool·espsecure 직접 실행
- mutant 독립 재빌드·재현
- 새 CI 상태·로그·target artifact 감사
- SDK·전이 의존성 전체 재감사
- raw 보고서의 원문 byte 보존 인증
- 정상 OTA owner·영속 policy·제품 signing·target 설치 통합
- MCU native execution·Flash·boot·rollback·power-fault 시험

**physical/HIL: NOT_RUN. Vehicle TX: NO-GO.**

## 7. 최종 결과

- 기존 B-P2-01: **FIXED**
- 신규 P0: 없음
- 신규 P1: 없음
- 신규 P2: 없음
- 신규 P3: 없음
- Verdict: **PASS — 이번 post-fix delta의 Reviewer B 정적 재검토 한정**

핵심 수정 source 전체와 기존 verifier/probe 경로를 직접 확인했다. 서명 거절 전에 hash 검사로 종료되던 검증 공백이 닫혔고, 관련 문서도 실제 시험 경로와 일치한다.

전체 T-007·CI·target·HIL·merge 승인으로 확대하지 않는다. PR36 Draft 및 미완료 gate는 그대로 유지한다.

