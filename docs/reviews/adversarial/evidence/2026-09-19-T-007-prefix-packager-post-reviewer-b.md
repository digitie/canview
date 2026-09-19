# T-007 prefix·packager post-fix reviewer B 원본

- Subagent ID: `01a0b784-5e5f-7d42-8953-968159b85914`

## 전달 요청 원문

```text
T-007 prefix·packager post-fix 재검토. 같은 저장소 object-only. base 63c87272fe9f2b00b76893055bfcc8a9ca71cf26 -> candidate ec44647e4674cc382d655bb668e07254b2612fca. 실행코드 delta는 tools/ota/container.py의 RAM 설명 docstring 2줄뿐, C/tests/CMC 변경 없음. docs/resume.md 및 T-007의 이전140/140을 PR35 source12100ac에 귀속하고 이번63c8727 Debug/Release142/142·Linux independent checkout ASan137/137을 별도기록. 본인 finding와 전체delta회귀(보고서원문은감사보존용이며 peerfinding독립공유금지)를 재확인. 이미두원본저장됨. B-01/B-02 수정후 test_container.py 4groups/90CNG, document links337docs/1377targets0errors, plan49PASS. CI35416056122는63c8727의 host5jobsPASS/targetpending. ec44647 새CI진행예정. 원본evidence의Markdownhardbreak/마지막빈줄은원문보존했으며 그두파일만 whitespace blank-at-eol/eof예외로검사, 다른파일기본검사PASS. gitshow/diff기준선확인, 새 실행식별/시각/범위/실제명령/한계/P0-P3/verdict포함한한국어완전raw반환. 전체T007·merge승인아님;physical/HIL NOT_RUN. 보고서를수정하지말고새결과본문제출.
```

## 반환 원문

# T-007 prefix·packager post-fix 재검토 — Reviewer B

판정: **PASS — B-01/B-02 수정 및 이번 delta의 정적 재검토 범위에 한정**

B-01과 B-02는 모두 **FIXED**로 확인했다. 새 P0/P1/P2/P3 finding은 없다. 전체 T-007 완료·merge 승인이나 candidate의 CI 완료 판정은 아니다.

## 실행 식별과 기준선

| 항목 | 실제 확인값 |
|---|---|
| Execution ID | `c9e9c6bc-2ff0-4f48-8b99-cdb80f461104` |
| 시작 UTC | `2026-09-19T02:44:21.3354825Z` |
| 종료 UTC | `2026-09-19T02:45:05.8866372Z` |
| 저장소 | `F:/dev/canview-wt/t007-ota-container` |
| Base | `63c87272fe9f2b00b76893055bfcc8a9ca71cf26` |
| Candidate | `ec44647e4674cc382d655bb668e07254b2612fca` |
| 전문범위 | Host packager 보안·리소스 계약, 검증 결과의 source 귀속, 기존 B finding closure와 delta 회귀 |

두 commit의 존재와 전체 hash를 시작·종료 시 확인했다. 소스는 지정 commit의 `git show`와 두 commit 사이 `git diff`로만 읽었다. 이동 중인 checkout 내용과 로컬 build 로그는 사용하지 않았다.

파일 수정·commit·worktree 생성·peer 탐색·공유는 수행하지 않았다. 보존된 reviewer A/B 원문과 통합 보고서 본문은 열지 않았다. 필수 task와 journal에 포함된 리뷰 상태 요약은 읽었지만 다른 reviewer의 finding을 판정 근거로 사용하지 않았다.

## 기존 finding 재판정

### B-01 — P3 — 이전 전체시험 결과의 현재 source 귀속

상태: **FIXED**

수정 위치:

- `docs/resume.md:56`
- `docs/resume.md:57`
- `docs/resume.md:58`
- `docs/tasks/T-007-ota-container.md:30`
- `docs/tasks/T-007-ota-container.md:31`
- `docs/tasks/T-007-ota-container.md:76`
- `docs/tasks/T-007-ota-container.md:77`

확인 근거:

이전 `140/140` 결과는 PR35 최종 source `12100ac`에 명시적으로 귀속됐다. 새 Debug/Release `142/142`와 독립 Linux checkout ASan/UBSan `137/137`은 checkpoint `63c8727`의 결과로 분리됐다.

`docs/resume.md:58`은 CI `35416056122`를 host 5개 job 통과·target 진행 중으로 기록한다. 이를 새 candidate `ec44647`의 전체 CI 통과로 표현하지 않는다.

기존 실패 시나리오였던 “이전 140/140을 새 collector/container까지 포함한 전체시험 결과로 오인”하는 문구는 제거됐다. 수정의 충분성은 직접 확인했으며, 기재된 시험 로그 자체를 독립 검증한 것은 아니다.

### B-02 — P3 — 입력 길이 상한을 host RAM 상한으로 표현

상태: **FIXED**

수정 위치:

- `tools/ota/container.py:42`
- `tools/ota/container.py:43`

확인 근거:

설명은 `MAX_BUNDLE`을 “단일 입력 bytes 길이 상한”으로 한정한다. 임시 복사·decoded 객체·암호 provider를 포함하는 peak RAM 보장이 아니라는 점도 명시했다.

이는 `tools/ota/container.py:47`의 실제 길이 검사와 일치한다. 설명 수정 과정에서 입력 상한이나 검증 동작을 완화하지 않았다. 기존 오해 가능성은 해소됐으며 peak RAM 측정은 이번 closure에 포함하지 않는다.

## 전체 delta 회귀 검토

전체 변경 목록은 8개 파일이다.

- `tools/ota/container.py`: docstring 한 줄을 두 줄로 교체. 인증·hash·padding·identity·bounded read·출력 생성·오류 처리 로직 변경 없음.
- `docs/resume.md`, 상세 T-007 task: 시험 source 귀속 수정과 checkpoint별 검증 기록 추가.
- `docs/journal.md`: checkpoint 실행·실패 원인·재실행·미완료 gate 기록 추가.
- 리뷰 index·통합 보고서·A/B 원문: 감사 보존 산출물. 독립성을 위해 본문 의미 재검토에서 제외하고 변경 목록·whitespace 검사 범위로만 확인.

회귀 공격 관점과 결과:

| 관점 | 정적 확인 결과 |
|---|---|
| 설명 수정에 숨은 검사 완화 | container.py 전체를 줄 단위로 다시 읽고 diff를 대조했다. docstring 외 변경 없음 |
| C collector·시험·fixture 변경 | `shared`, `tests`의 base/candidate diff가 비어 있음 |
| CTest·CI 등록 변경 | root CMake, presets, foundation workflow의 diff가 비어 있음 |
| 다른 실행 파일의 추가 변경 | `docs/**`와 container.py를 제외한 전체 diff가 비어 있음 |
| 이전 결과를 candidate 결과로 확대 | `12100ac`와 `63c8727`을 구분하며 `ec44647`의 새 실행 완료를 주장하지 않음 |
| Coverage의 과장 | collector 함수별 수치이며 전체 envelope.c·target/HIL·MCU timing 결과가 아니라고 한정 |
| 실패 이력 은폐 | journal에 최초 Debug fixture mismatch, Release 실행 중 HEAD 변경, WSL Git 경로 실패와 후속 재실행을 구분해 기록 |
| Partial checkpoint의 완료 확대 | T-007 `IN_PROGRESS`, native/정상 owner 연결 미완료, physical/HIL `NOT_RUN`, 차량 TX `NO-GO` 유지 |
| Whitespace 예외 확대 | 두 raw evidence 파일에만 `blank-at-eol`·`blank-at-eof` 예외 적용. 나머지 delta는 기본 검사 |

새 finding: **P0 0건 / P1 0건 / P2 0건 / P3 0건**.

## 실제 읽은 파일과 적용 문맥

Candidate에서 전체를 직접 읽음:

- `AGENTS.md`
- `docs/README.md`
- `docs/resume.md`
- `docs/tasks/T-007-ota-container.md`
- `tools/ota/container.py`

변경 부분을 직접 읽음:

- 위 resume/task/container의 base→candidate diff
- `docs/journal.md`의 새 checkpoint 항목

변경 없음 확인:

- `shared`
- `tests`
- `CMakeLists.txt`
- `CMakePresets.json`
- `.github/workflows/foundation.yml`
- `AGENTS.md`
- `docs/README.md`
- `docs/runbooks/agent-workflow.md`

직전 리뷰에서 직접 읽은 `agent-workflow.md`와 `embedded-cstyle`, `embedded-architecture` skill의 검토 문맥을 유지했다. workflow는 이번 diff에서 불변임을 확인했다. C 변경이 없으므로 기존 C 전체를 다시 읽거나 실행하지 않았다.

## 실제 실행 명령

아래에서 `B`와 `C`는 각각 위 표의 전체 base/candidate hash를 뜻한다.

```text
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('o')

git cat-file -e '<B>^{commit}'
git cat-file -e '<C>^{commit}'
git rev-parse '<B>^{commit}'
git rev-parse '<C>^{commit}'

git diff --find-renames --stat <B> <C>
git diff --find-renames --name-status <B> <C>

git diff --find-renames <B> <C> -- tools/ota/container.py docs/resume.md docs/tasks/T-007-ota-container.md docs/journal.md

git show <C>:AGENTS.md
git show <C>:docs/README.md
git show <C>:docs/resume.md
git show <C>:docs/tasks/T-007-ota-container.md
git show <C>:tools/ota/container.py

git diff --find-renames --exit-code <B> <C> -- shared tests CMakeLists.txt CMakePresets.json .github/workflows/foundation.yml AGENTS.md docs/README.md docs/runbooks/agent-workflow.md

git diff --find-renames --exit-code <B> <C> -- . ':(exclude)docs/**' ':(exclude)tools/ota/container.py'
```

`git show` 출력에는 PowerShell `ForEach-Object`로 줄 번호를 붙였다.

실제 whitespace 검사:

```text
git diff --find-renames --check <B> <C> -- . ':(exclude)docs/reviews/adversarial/evidence/2026-09-19-T-007-prefix-packager-reviewer-a.md' ':(exclude)docs/reviews/adversarial/evidence/2026-09-19-T-007-prefix-packager-reviewer-b.md'

git -c core.whitespace=-blank-at-eol,-blank-at-eof diff --find-renames --check <B> <C> -- docs/reviews/adversarial/evidence/2026-09-19-T-007-prefix-packager-reviewer-a.md docs/reviews/adversarial/evidence/2026-09-19-T-007-prefix-packager-reviewer-b.md
```

실제 결과:

- 실행·시험·규칙 경로 불변 검사: exit `0`
- 기타 파일 불변 검사: exit `0`
- 원문 2개 제외 기본 whitespace 검사: exit `0`
- 원문 2개 한정 whitespace 예외 검사: exit `0`

원문 파일을 자동 정리하거나 수정하지 않았다. Whitespace 검사 성공은 원문 byte 보존의 독립 대조 결과를 뜻하지 않는다.

## 실행 검증과 한계

이번 리뷰에서 compile, Python 시험, CNG, CTest, sanitizer, coverage, document links, plan validator를 재실행하지 않았다.

사용자가 제공한 다음 결과는 전달받은 검증 정보이며, 본 리뷰의 직접 실행 PASS로 집계하지 않는다.

- 수정 후 `test_container.py` 4 groups / 90 CNG
- Document links 337 docs / 1377 targets / 0 errors
- Plan 49 PASS
- `63c8727` Debug/Release 142/142, Linux ASan/UBSan 137/137
- CI `35416056122`: `63c8727`의 host 5 jobs PASS / target pending
- `ec44647`의 새 CI는 진행 예정

Remote CI 상태·artifact·로컬 로그 진위는 조회하지 않았다. Native packager/full golden, 정상 target·Flash owner 연결, 실제 메모리·시간 측정, Flash/power/vehicle TX 검증은 범위 밖이다.

**Physical/HIL: NOT_RUN.**

최종 판정은 **B-01 FIXED, B-02 FIXED, 새 finding 없음, post-fix 정적 PASS**다. 전체 T-007 완료와 merge 승인은 포함하지 않는다.
