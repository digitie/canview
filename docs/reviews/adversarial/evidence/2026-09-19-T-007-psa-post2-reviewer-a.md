# T-007 PSA 두 번째 post-fix Reviewer A 원본

## 전달 요청 원문

```text
T-007 PSA 두 번째 post-fix 독립 재검토. 같은 원 A/B 전문범위 유지, 새 executionUUID/UTC시작종료. Repo F:/dev/canview-wt/t007-ota-container, base 6f078ace276eecce4f8ceabf464929de5eb6bfd7, candidate 1b06afd194705f87a0d74d65a2de40daba626211.
이번 변경은 tests/ota/test_esp_ota_crypto.c의 signature 중첩 시험을 실제ready unioncontext로 이동한11행과 리뷰/evidence/currentstate 기록이다. A-PSA-POST-01(P2)/B-PSA-02(P2)를 수정한다. reentry대상바꿈→initOK→verify호출수저장→중첩거절→호출수불변→정상verifyOK/호출수+1→closeOK→기존reentry복원. provider.c/h 불변. B-PSA-01(P3) 원B FIXED 유지.
두 이전raw는보존됐으며 새post결과는 서로공유하지말것. Gitobject-only: 시작종료cat-file/rev-parse, gitshowcandidate:path/diffbasecandidate만읽기. movingworktree/파일수정/commit/compile/다른agent소환금지. 핵심시험 source와provider경로 직접읽고 자신의finding 및전체delta회귀 확인.
작성자 실행과 구분: Debug/Release144/144/경고0; GCC-O3정상모형PASS; signature중첩조건만삭제한실제Cmutant가중첩CHECK(line152)에서실패; oldusage0x1000 header mutant는독립typedef컴파일실패; ASanUBSan모형행149/149분기114/118. target source불변, 최신전체CI진행예정. 실제PSAdevice/Flash/HIL NOT_RUN·TXNO-GO·owner통합미완료·PR36Draft.
최종raw는 반복설명은 줄이되 필수내용누락없이 한국어: executionUUID/시각/전문/basecandidate/격리·실제읽은파일행/명령/원findingFIXED여부/P0~P3신규finding(정확위치·실패시나리오·영향·권고)/미검토/NOT_RUN/BLOCK CONDITIONAL PASS. 판정은이delta정적범위이며wholeTask승인아님. 읽지못하면incompleteBLOCK. 완전한raw최종응답 반환.
```

## 반환 원문

# T-007 PSA 두 번째 post-fix 재검토 — Reviewer A

## 실행·판정

- Reviewer: A / Maxwell
- 전문: C 수명·cleanup·재진입·ownership·bounds·시험 회귀
- Execution UUID: `d166830a-fa90-4e09-b69b-5c3e6669920b`
- 시작 UTC: `2026-09-19T04:08:14.4488692Z`
- 종료 UTC: `2026-09-19T04:08:47.0848317Z`
- Repo: `F:/dev/canview-wt/t007-ota-container`
- 확인 Base: `6f078ace276eecce4f8ceabf464929de5eb6bfd7`
- 확인 Candidate: `1b06afd194705f87a0d74d65a2de40daba626211`
- **Verdict: PASS — 이번 delta의 정적 검토 범위**
- **A-PSA-POST-01 / P2: FIXED**
- 신규 finding: **P0 없음 / P1 없음 / P2 없음 / P3 없음**

## 격리

시작·종료 모두 두 commit의 `cat-file -e`와 `rev-parse`를 확인했다. 요청 hash와 일치한다.

저장소 내용은 고정 commit의 `git show`와 base/candidate `git diff`로만 읽었다. Moving worktree 소스 열람, 파일 수정, commit, 컴파일, 시험 실행, 다른 agent 소환·결과 공유는 없었다. 이전 raw evidence와 통합 report 본문은 열지 않았다.

`embedded-cstyle`, `embedded-architecture` 스킬을 완독하고 storage·수명·재진입 검토에 적용했다.

## 원 finding 재확인

### A-PSA-POST-01 — P2 — FIXED

기존 실패 시나리오는 signature 중첩 검사가 누락되어도 unready context가 동일한 `INVALID_ARGUMENT`를 반환하여 시험이 통과하는 것이었다. 영향은 중첩 검사 회귀의 false pass였다.

수정 위치: `tests/ota/test_esp_ota_crypto.c:148–157`.

직접 확인한 순서:

1. `:149`에서 재진입 검사 대상을 `overlap.context`로 변경한다.
2. `:150`에서 정상 init 성공을 요구한다.
3. `:151`에서 SDK verify 호출 수를 저장한다.
4. `:152–153`에서 signature 중첩 거절과 SDK 호출 수 불변을 확인한다.
5. `:154–155`에서 같은 context의 정상 verify 성공과 호출 수 증가를 확인한다.
6. `:156`에서 close 성공 및 mock 계약 정상 상태를 확인한다.
7. `:157`에서 기존 context로 재진입 대상을 복원한다.

`firmware/platform/esp32s3/ota_crypto.c:61`은 init 성공 시 ready를 설정한다. 따라서 이번 사례는 `:31`의 unready 거절에 기대지 않는다. Signature 중첩 조건(`:95`)만 제거하면 SDK mock으로 진행하여 정상 반환값을 받으므로 `test_esp_ota_crypto.c:152`의 기대값과 달라진다. 기존 false-pass 원인이 제거됐다.

65B union storage(`test_esp_ota_crypto.c:89`)도 유지된다. 기존 context는 새 사례 전에 close되고, union context 역시 후속 오류 조합 시험 전에 close된다. 새 사례가 기존 자원 수명·재진입 대상·앞선 호출 횟수 검사를 교란하는 경로는 발견하지 못했다.

권고했던 ready 전제, SDK 미호출 확인, 후속 정상 사용 및 cleanup이 반영되어 **FIXED**로 판정한다. 이는 직접 소스 추적에 따른 재확인이며 mutant를 독립 실행한 결과는 아니다.

## Delta 회귀 확인

- 핵심 시험 전체와 provider `.c/.h` 전체를 직접 읽었다.
- Provider, firmware/shared 코드, root CMake, workflow, fixture header 및 target fixture는 base/candidate 간 불변임을 확인했다.
- 변경된 시험은 기존 실패·재진입 조합을 유지하고 별도 ready-context 사례를 추가한다.
- Usage 독립 typedef 검사는 시험 `:10`에 유지된다. B-PSA-01의 기존 원 B closure를 변경하지 않는다.
- Resume/task/journal은 과거 golden artifact 감사와 이번 PSA 수정본의 미완료 CI를 구분한다.
- 실제 장치 PSA·정상 owner·Flash 완료로 확대하는 새 주장은 발견하지 못했다.

공격한 주요 시나리오는 unready에 의한 대체 거절, SDK 호출 후 같은 오류 반환, 거절 과정의 context 손상, 잘못된 재진입 대상, 자원 중복 소유·cleanup 누락, 호출 수 기준 변경에 의한 후속 시험 오염이었다.

## 실제 열람 파일·명령

전체 직접 열람:

- `tests/ota/test_esp_ota_crypto.c:1–222`
- `firmware/platform/esp32s3/ota_crypto.c:1–186`
- `firmware/platform/esp32s3/ota_crypto.h:1–60`
- Candidate의 `AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-007-ota-container.md`

Delta 열람:

- 위 시험 변경 전체
- Resume/task 변경 전체
- `docs/journal.md:3–30` 추가분

이전 직접 열람한 OTA §7–9, ADR-009, agent-workflow는 이번 diff에서 불변임을 확인했다. 전체 변경 목록 8파일 중 review 기록 4파일은 목록만 확인했으며 본문·원문 보존 정확성은 감사하지 않았다.

실제 명령 형식:

```powershell
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('o')
git cat-file -e '<hash>^{commit}'
git rev-parse '<base>^{commit}' '<candidate>^{commit}'
git diff --find-renames --stat <base> <candidate>
git diff --find-renames <base> <candidate> -- <명시한 검토 경로>
git show <candidate>:<열람 파일>
git diff --check <base> <candidate> -- . ':(exclude)docs/reviews'
```

Hash는 위 full base/candidate를 사용했다. `git show` 출력에 PowerShell 행 번호를 부여했다. `diff --check`는 명시한 제외 범위 외에서 출력 없이 종료했다.

## 제공 근거·미실행 범위

다음은 작성자 제공 결과이며 본 reviewer 실행이 아니다.

- Debug/Release 각각 144/144, 경고 0
- GCC `-O3` 정상 모형 PASS
- Signature 중첩 조건만 제거한 실제 C mutant가 152행 CHECK에서 실패
- Usage `0x1000` header mutant가 독립 typedef에서 컴파일 실패
- ASan/UBSan 모형 행 149/149, 분기 114/118

해당 로그·mutant·coverage artifact 및 최신 CI는 독립 검증하지 않았다.

- 컴파일·CTest·mutant·sanitizer: **NOT_RUN**
- 실제 PSA device 실행·Flash·physical/HIL: **NOT_RUN**
- SDK 내부 구현·전체 target/CI artifact 재감사: 미검토
- 정상 owner/root·영속 policy 통합: 범위 밖·미완료
- Vehicle TX: **NO-GO**
- PR36: **Draft**

## 최종 결론

**A-PSA-POST-01 P2는 FIXED이며 신규 P0–P3 finding은 없다. 이번 delta의 정적 verdict는 PASS다.**

전체 T-007 완료, 실제 PSA 장치 검증, CI·target·HIL 또는 merge 승인을 뜻하지 않는다.

