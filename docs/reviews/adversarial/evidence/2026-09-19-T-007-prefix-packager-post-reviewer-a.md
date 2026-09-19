# T-007 prefix·packager post-fix reviewer A 원본

- Subagent ID: `01a0b784-5d6a-7540-97e8-ac159eb3a828`

## 전달 요청 원문

```text
T-007 prefix·packager post-fix 재검토. 같은 저장소 object-only. base 63c87272fe9f2b00b76893055bfcc8a9ca71cf26 -> candidate ec44647e4674cc382d655bb668e07254b2612fca. 실행코드 delta는 tools/ota/container.py의 RAM 설명 docstring 2줄뿐, C/tests/CMC 변경 없음. docs/resume.md 및 T-007의 이전140/140을 PR35 source12100ac에 귀속하고 이번63c8727 Debug/Release142/142·Linux independent checkout ASan137/137을 별도기록. 본인 finding와 전체delta회귀(보고서원문은감사보존용이며 peerfinding독립공유금지)를 재확인. 이미두원본저장됨. B-01/B-02 수정후 test_container.py 4groups/90CNG, document links337docs/1377targets0errors, plan49PASS. CI35416056122는63c8727의 host5jobsPASS/targetpending. ec44647 새CI진행예정. 원본evidence의Markdownhardbreak/마지막빈줄은원문보존했으며 그두파일만 whitespace blank-at-eol/eof예외로검사, 다른파일기본검사PASS. gitshow/diff기준선확인, 새 실행식별/시각/범위/실제명령/한계/P0-P3/verdict포함한한국어완전raw반환. 전체T007·merge승인아님;physical/HIL NOT_RUN. 보고서를수정하지말고새결과본문제출.
```

## 반환 원문

# T-007 prefix·packager post-fix 독립 재검토 — Reviewer A

판정: **PASS — 지정 post-fix delta의 object-only 정적 재검토 한정.**  
신규 P0/P1/P2/P3 finding은 없다. 전체 T-007 완료·merge·배포 승인은 아니다.

## 실행 식별과 기준선

- 새 execution ID: `dc510fa1-ecf5-4782-9e39-7ae62fc523d5`
- 발급 방법: PowerShell `[guid]::NewGuid().ToString()`
- 시작 UTC: `2026-09-19T02:44:18.951Z`
- 종료 UTC: `2026-09-19T02:45:34.694Z`
- 저장소: `F:/dev/canview-wt/t007-ota-container`
- 실제 확인 base: `63c87272fe9f2b00b76893055bfcc8a9ca71cf26`
- 실제 확인 candidate: `ec44647e4674cc382d655bb668e07254b2612fca`
- 전문범위: embedded C runtime, bounds/overflow/alias, partial input/reset/sticky error, caller lifetime, task/stack budget, prefix 성공과 인증·설치 권한의 분리.
- 이전 Reviewer A execution ID: `7d17f5f8-9f65-4f26-96e8-ea6a410ccc6f`

시작에 두 commit 객체의 존재와 해시를 확인하고 종료에 해시를 재확인했다. 저장소 내용은 고정 commit의 `git show`와 `git diff --find-renames`로만 읽었다. 이동 중인 checkout·HEAD·빌드 산출물은 판단 근거로 사용하지 않았다.

파일 편집·커밋·worktree 생성·peer 탐색·공유는 하지 않았다. 기존 보고서에 내용을 추가하거나 수정하지 않았다. Worktree clean 상태는 검사하지 않았으며 clean이라고 주장하지 않는다.

## 전체 delta와 독립성

전체 변경 목록은 8개 파일, 414행 추가·4행 삭제다.

- 동작 관련 소스: `tools/ota/container.py`의 docstring 한 문장을 두 문장으로 교체.
- 검증 설명: `docs/resume.md`, `docs/tasks/T-007-ota-container.md`.
- 작업 기록: `docs/journal.md`.
- 감사 보존물: review index, 통합 report, reviewer A/B 원문 두 파일.

전체 변경 목록과 경로별 diff를 통해 C 구현·header·테스트·root CMake·합성 fixture에 변경이 없음을 확인했다. Python의 분기·호출·상수·입출력 동작에도 변경이 없다. `__doc__` 내용과 이후 소스 행 번호만 달라진다.

감사 보존용 reviewer 원문과 통합 report는 본문을 열람하지 않았다. Task/journal에 포함된 coordinator의 리뷰 상태 요약은 읽었지만 상대 finding의 독립 재현·disposition 근거로 사용하지 않았다. 원문 파일 두 개는 내용을 출력하지 않는 whitespace 검사에만 포함했다.

## 수정 및 회귀 확인

| 대상·위치 | 공격한 실패 시나리오 | 확인 결과 |
|---|---|---|
| `tools/ota/container.py:42` | `MAX_BUNDLE`을 전체 host peak RAM 보장으로 오인하여 입력 복사·decoded 객체·암호 provider 비용을 누락 | 새 문구는 단일 입력 `bytes` 길이 상한으로 한정하고 peak RAM 보장이 아님을 명시한다. `:47`의 길이 검사, `:50`의 prefix slice·manifest 해석과 일치한다. |
| `docs/resume.md:56` | 이전 source의 140/140 결과를 현재 candidate 검증으로 잘못 귀속 | 140/140을 PR35 source `12100ac`에 명시적으로 귀속한다. `:57`부터 142/142·137/137을 checkpoint `63c8727` 결과로 구분한다. |
| `docs/tasks/T-007-ota-container.md:30`, `:76` | Task의 검증 설명이 resume과 다른 source 또는 범위를 주장 | 이전 PR35 결과와 `63c8727` 결과를 동일하게 구분한다. 새 candidate `ec44647`의 실행 결과라고 주장하지 않는다. |
| `docs/resume.md:59`, `docs/tasks/T-007-ota-container.md:77` | 함수별 collector coverage를 전체 `envelope.c`, target/HIL 또는 MCU 시간 보장으로 확대 | 함수별 범위를 명시하고 확대 해석을 배제한다. 부분 coverage를 전체 실행 경로 검증으로 표시하지 않는다. |
| `shared/ota/src/envelope.c:27`, `:63`, `:107` | 문서 수정과 함께 alias 검사·복사 상한·길이 검사·sticky 상태가 조용히 변경 | 해당 파일 diff가 없으며 candidate의 관련 구현도 직접 재열람했다. 기존 경계와 오류 처리에 회귀를 확인하지 못했다. |
| `shared/ota/src/body.c:68`, `:71` | Prefix 조립 성공이 manifest preflight·floor 검사를 우회 | Candidate의 `body_open()`을 직접 재열람했다. 기존 preflight와 floor 검사 후 hash를 시작하는 순서가 유지된다. |
| `tools/ota/container.py:121`, `docs/tasks/T-007-ota-container.md:46` | Packager 성공이나 검증 수치 갱신을 native 검증·Flash 설치 완료로 오인 | 성공 메시지의 native/local policy/install `NOT_VERIFIED`와 정상 target·owner 연결 미완료 설명이 유지된다. |

이전 Reviewer A report에는 finding이 없으므로 본인 finding에 대한 `FIXED` 처리 대상은 없다. 이번 설명 수정은 소스와 정합적이라고 판단하지만, 이를 상대 reviewer finding의 closure 승인으로 대신하지 않는다.

## 실제 읽기·실행 명령

이번에 직접 읽은 candidate 내용:

- `tools/ota/container.py`: 전체 1–126행.
- `docs/resume.md`: 1–75행 및 전체 delta.
- `docs/tasks/T-007-ota-container.md`: 1–56행, 65–90행 및 전체 delta.
- `docs/journal.md`: 이번 추가분 diff.
- `shared/ota/src/envelope.c`: 27–124행.
- `shared/ota/src/body.c`: 46–89행.

이전 실행에서 직접 읽은 `AGENTS.md`, `docs/README.md`, `docs/runbooks/agent-workflow.md`는 이번 diff가 없음을 확인하여 재사용했다. 이전에 읽은 `embedded-cstyle`·`embedded-architecture` skill의 메모리·소유권·권한 경계 점검 기준을 이어 적용했다.

실제 명령은 다음 계열이다. 모든 Git 명령은 지정 저장소에서 실행했다.

```powershell
[guid]::NewGuid().ToString()
[DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')

git cat-file -e '<base>^{commit}'
git cat-file -e '<candidate>^{commit}'
git rev-parse '<base>^{commit}'
git rev-parse '<candidate>^{commit}'

git diff --find-renames --stat <base> <candidate>
git diff --find-renames --name-status <base> <candidate>

git diff --find-renames <base> <candidate> -- `
  tools/ota/container.py docs/resume.md docs/tasks/T-007-ota-container.md

git diff --find-renames <base> <candidate> -- `
  AGENTS.md docs/README.md docs/runbooks/agent-workflow.md `
  shared/ota CMakeLists.txt tests

git diff --find-renames <base> <candidate> -- docs/journal.md
git show <candidate>:<위에 열거한 파일>
```

`git show` 출력은 PowerShell `ForEach-Object`로 행 번호를 부여하고 필요한 구간을 선택했다.

Whitespace 검사는 직접 실행했다.

1. 원문 evidence 두 파일을 제외한 전체 delta에 기본 `git diff --find-renames --check`: **출력 없음, exit 0**.
2. 원문 evidence 두 파일에만 `git -c core.whitespace=-blank-at-eol,-blank-at-eof diff --find-renames --check`: **출력 없음, exit 0**.

예외는 해당 두 경로의 검사 호출에만 적용했다. Git 설정이나 파일은 수정하지 않았다. 이 검사는 원문과 저장본의 byte-for-byte 동일성을 검증한 것은 아니다.

## 실행 검증과 한계

이번 reviewer는 compile·CTest·Python test·sanitizer·coverage·문서 링크 검사·plan 검사를 실행하지 않았다. 해당 항목은 이번 실행 기준 **NOT_RUN**이다.

다음은 사용자/coordinator가 제공한 결과이며 독립 재실행 PASS로 집계하지 않는다.

- `63c8727`: Windows Debug/Release 각각 142/142.
- 같은 candidate의 독립 Linux checkout: ASan/UBSan 137/137.
- 함수별 collector coverage: feed 행 93.33%·분기 96%, init/finish 100%.
- 수정 후 `test_container.py`: 4 groups·90 CNG.
- 문서 링크: 337 docs·1,377 targets·0 errors.
- Plan: 49 PASS.
- CI `35416056122`: `63c8727`의 host 5 jobs PASS·target pending이라는 전달 상태.

외부 CI나 로컬 로그는 조회하지 않았다. 위 상태를 종료 시점의 실시간 CI 상태로 보증하지 않으며 `ec44647`의 새 CI 성공으로 확대하지 않는다.

감사 보존물의 본문·원문 보존 동일성, 실제 host peak RAM, MCU stack/WCET, native packager/full golden, 정상 target·Flash owner 연결은 이번 검토 범위 밖이다. 이 미완료 사항을 이번 docstring·기록 수정의 신규 결함으로 계산하지 않았다.

**Physical/HIL: NOT_RUN. 차량 CAN TX: NO-GO 유지.**

## Finding 및 최종 판정

- P0: 0건
- P1: 0건
- P2: 0건
- P3: 0건

확정 finding이 없으므로 finding별 failure scenario·영향·수정 권고·disposition은 해당 없다. 공격한 시나리오와 잔여 한계는 위에 기록했다.

**Reviewer A: PASS — `63c87272…` → `ec44647e…` post-fix delta의 object-only 정적 재검토 한정. 전체 T-007 완료·merge·배포 승인은 아니다.**
