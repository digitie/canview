# CANView 에이전트 개발 워크플로

이 문서는 branch, 검증, 독립 적대적 리뷰, PR, merge 정리의 실행 절차다. 제품 설계는 [architecture](../architecture/README.md), 문서 역할과 갱신 규칙은 [documentation maintenance](documentation-maintenance.md)가 정본이다.

## 1. 범위와 기준선 확정

작업 시작 시 모든 문서를 통독하지 않는다. 다음 순서로 최소 문맥만 확인한다.

1. `AGENTS.md`, [문서 지도](../README.md), [현재 상태](../resume.md)를 읽는다.
2. 해당 상세 task가 있으면 그 파일 한 개에서 scope, 선행 조건, acceptance, gate를 확인한다. task를 고르는 경우에만 [tasks](../tasks.md)를 연다. 문서·review 전용 요청처럼 task가 없으면 관련 runbook 또는 template을 작업 기준으로 삼는다.
3. 변경 분야의 architecture 또는 상세 문서 한두 개와 관련 ADR만 읽는다.
4. `git status`, 현재 branch, `origin/main`과의 차이를 확인하고 사용자 변경을 구분한다.
5. 검증할 수 없는 하드웨어·차량·network gate와 중요한 가정을 작업 전에 드러낸다.

반복 실패가 실제로 발생했을 때만 [failure patterns](agent-failure-patterns.md)를 읽는다.

## 2. Windows branch와 임시 worktree

정본 개발환경은 Windows PowerShell과 Windows native Git이다. 일반적인 단일 작업은 `F:/dev/canview`에서 새 branch를 만들어 수행한다.

```powershell
Set-Location F:/dev/canview
git fetch origin main
git switch -c agent/<agent>-<task> origin/main
```

다음 중 하나가 실제로 필요할 때만 임시 worktree를 만든다.

- 서로 다른 변경을 병렬 구현할 때
- dirty checkout과 완전히 격리해야 할 때
- 같은 immutable review 기준선을 전문 리뷰어 서브에이전트가 독립적으로 검사할 때
- 장시간 target build나 실험 때문에 기본 checkout을 점유하면 안 될 때

```powershell
New-Item -ItemType Directory -Force F:/dev/canview-wt | Out-Null
git -C F:/dev/canview worktree add `
  -b agent/<agent>-<task> `
  F:/dev/canview-wt/<agent>-<task> `
  origin/main
Set-Location F:/dev/canview-wt/<agent>-<task>
```

- 같은 branch를 여러 worktree에서 checkout하지 않는다.
- 사용자 변경이 있는 checkout을 정리하거나 덮어쓰지 않는다.
- worktree는 상시 자원이 아니다. merge 또는 abandon 뒤 활성 process와 미커밋 변경이 없는지 확인한 다음 제거한다.
- 임시 worktree 삭제 전 필요한 commit이 다른 ref 또는 remote에 도달했는지 확인한다. 강제 삭제는 복구 계획 없이 사용하지 않는다.

```powershell
git -C F:/dev/canview worktree remove F:/dev/canview-wt/<agent>-<task>
git -C F:/dev/canview worktree prune
```

## 3. CodeGraph 수명주기

CodeGraph가 설치되어 있고 변경 범위 분석에 유용할 때만 사용한다.

1. 새 임시 worktree에서는 최초 한 번 `codegraph init -i`를 실행한다.
2. branch 전환, pull, merge, 대규모 이동 뒤에는 `codegraph sync`를 실행한다.
3. 분석 전후 `codegraph status`로 현재 worktree와 index가 일치하는지 확인한다.
4. 공용 UI, protocol, 안전 경계처럼 호출자 영향이 넓은 변경은 편집 전에 `explore` 또는 `impact`로 범위를 확인한다.

도구가 없으면 설치된 것처럼 기록하지 않는다. `rg`, compiler dependency output, test와 수동 추적으로 대체하고 한계를 남긴다.

## 4. 구현과 검증

구현은 선택한 task의 acceptance를 만족하는 최소 변경으로 제한한다. 검증은 변경 범위에 맞춰 아래 순서에서 필요한 계층만 실행한다.

```text
schema/protocol
  -> generator·codec·golden vector·malformed input
  -> host unit/integration test
  -> target compile
  -> KiCad ERC 또는 UI static/device test
  -> HIL·fault injection
  -> bench/vehicle evidence
```

- schema로 생성되는 ABI와 generated file을 함께 검증한다.
- 차량이나 bench가 필요한 gate를 host simulation으로 대체해 통과 처리하지 않는다.
- 실패·미실행 gate는 영향, 이유, 재현 명령과 후속 task를 기록한다.
- 리뷰 finding을 고친 뒤에는 영향받은 검증을 다시 실행한다.

## 5. 전문 리뷰어 서브에이전트 2인 적대적 리뷰

비단순 변경은 PR 전에 전문 영역이 서로 다른 리뷰어 서브에이전트 2명이 독립적으로 적대적 리뷰한다.

다음 변경은 면제하지 않는다.

- architecture, protocol wire/ABI, 권한·인증·암호화
- 차량 송신, safety gate, watchdog·brownout·boot state
- 회로, pinmap, 전원, CAN PHY, 제작 산출물
- ISR·RTOS·동시성, memory·대역폭·타이밍 budget
- DBC signal 승격, 자동화 판정, 운전자 경고
- 공용 UI state model 또는 넓은 refactor
- 작업 정책, 품질 gate, 문서 정본 관계의 변경
- `AGENTS.md`, `SKILL.md`, `docs/README.md`, architecture, ADR, runbook, task/review 규칙·index·template 변경

일반 면제는 다음을 모두 만족할 때만 가능하다.

- 오탈자·공백 또는 의미와 정본 역할이 같은 link의 label/target correction뿐이다.
- source code, schema, 생성물, build/config, hardware, 차량 evidence와 위 비면제 문서를 바꾸지 않는다.
- 동작, 권한, acceptance, gate, 심각도, disposition과 정본 우선순위를 바꾸지 않는다.
- 작성자가 아닌 maintainer 또는 PR reviewer가 diff를 확인하고 면제 근거를 PR에 승인한다.

리뷰 완료 뒤 원본 결과를 그대로 보존하고 통합 report의 disposition·재검증 결과와 archive index 한 행만 갱신하는 **review closure artifact**는 같은 리뷰를 재귀적으로 다시 시작하지 않는다. 이 예외로 규범 문구를 바꾸면 안 되며, 바꾸면 새 기준선으로 2인 리뷰를 다시 수행한다.

### 5.1 Review manifest와 immutable 기준선

1. 구현과 1차 검증을 마친 review candidate를 commit하고 commit hash를 기록한다.
2. 두 reviewer에게 같은 manifest를 제공한다. manifest에는 기준 commit과 parent/base, task 또는 요청, scope·범위 밖, 관련 architecture·acceptance, 실행 가능한 검증을 넣는다.
3. reviewer 실행 ID·역할·시작 시각과 전달한 review request 원문을 reviewer별 evidence 파일에 기록한다.
4. 각 reviewer는 아래 두 격리 방식 중 하나를 사용하고 실제로 확인한 hash와 clean 상태를 결과에 남긴다. 이동 가능한 branch를 read-only 기준선으로 부르지 않는다.

**Commit object-only 방식**은 compile이 필요 없는 문서·diff 리뷰에 쓴다. 모든 내용은 plain worktree가 아니라 `git show`와 `git diff`로 읽는다.

```powershell
git -C F:/dev/canview cat-file -e "<commit>^{commit}"
git -C F:/dev/canview rev-parse "<commit>^{commit}"
git -C F:/dev/canview diff --find-renames <base> <commit>
git -C F:/dev/canview show <commit>:<path>
```

**Detached worktree 방식**은 compiler·test·filesystem tool이 필요할 때 쓴다.

```powershell
New-Item -ItemType Directory -Force F:/dev/canview-wt | Out-Null
git -C F:/dev/canview worktree add --detach F:/dev/canview-wt/review-<id> <commit>
git -C F:/dev/canview-wt/review-<id> rev-parse HEAD
git -C F:/dev/canview-wt/review-<id> status --porcelain=v1
```

`HEAD`는 manifest의 commit과 같고 review 시작·종료의 `status --porcelain=v1` 출력은 비어 있어야 한다. 종료 뒤 [worktree 정리 절차](#8-merge-또는-abandon-뒤-정리)로 제거한다.

5. 두 reviewer가 각자의 원본 결과를 확정해 저장하기 전에는 상대방의 report나 finding을 보여 주지 않는다.

### 5.2 전문 영역 배정

변경에 맞게 서로 다른 실패 관점을 배정한다. 예시는 다음과 같다.

| Reviewer | 우선 전문 영역 | 필수 공격 관점 |
|---|---|---|
| A | 차량 안전·hardware·embedded runtime | reset/brownout, bus 방해, fail-open, timing, electrical assumption |
| B | protocol·security·build·integration | malformed/replay, version skew, resource exhaustion, generated artifact drift, 검증 누락 |

UI·문서 구조 변경이라면 A는 운전자 안전·상태 오해·접근성, B는 정보 정본·링크·agent 실행 가능성·회귀 검증처럼 영역을 다시 배정한다. 두 reviewer 모두 정상 경로 확인에 그치지 않고 숨은 가정, 경계값, 고장 주입, 복구 불가 상태와 다른 subsystem의 부작용을 찾는다.

### 5.3 독립 산출물과 심각도

심각도는 다음처럼 판정한다.

| 등급 | 기준 | merge 효과 |
|---|---|---|
| `P0` | 즉각적인 인명·차량 bus·비밀 유출·복구 불가 손상 또는 release 경계 붕괴 | 즉시 `BLOCK` |
| `P1` | 정상적인 실패 조건에서 안전·보안·정합성·필수 gate·감사성을 깨뜨림 | `BLOCK` |
| `P2` | 기능·검증·유지보수에 실질적 결함이지만 현재 merge 안전 경계를 직접 깨지 않음 | 수정 권고, 조건부 연기 가능 |
| `P3` | 작은 탐색성·표현·비기능 품질 문제 | 수정 또는 명시적 추적 |

각 reviewer는 최소한 다음을 제출한다.

- reviewer execution ID, 시작·종료 시각, 전문 영역
- 전달받은 manifest와 실제 검토한 commit hash, 격리 방식, 파일 범위
- 실행하거나 읽은 검증과 검토하지 못한 범위
- `P0`~`P3` finding: 위치, 근거, 재현 또는 실패 시나리오, 영향, 권고
- finding이 없더라도 공격한 시나리오와 남은 불확실성
- merge verdict: `BLOCK`, `CONDITIONAL`, `PASS`

주 작업 에이전트는 교차 비교 전에 두 원본 결과를 `docs/reviews/adversarial/evidence/YYYY-MM-DD-<scope>-reviewer-{a,b}.md`에 각각 보존한다. 요약하면서 finding을 삭제하거나 심각도를 낮추지 않는다. 근거 없는 취향 차이는 finding으로 만들지 않지만, 시험하지 않은 안전 가정을 단순히 “문제 없음”으로 닫지도 않는다.

### 5.4 결과 반영과 gate

두 독립 리뷰가 끝난 뒤에만 주 작업 에이전트가 결과를 교차 검토한다.

1. 매 리뷰 실행마다 [review template](../reviews/adversarial/TEMPLATE.md)으로 `docs/reviews/adversarial/YYYY-MM-DD-<scope>.md`를 새로 만든다.
2. 통합 report에서 두 원본 evidence를 연결하고 finding을 누락 없이 옮긴다. 중복은 연결하되 원래 ID, 심각도와 관점을 보존한다.
3. 각 finding은 아래 상태 중 하나를 사용한다.

| 상태 | 의미 | 허용 등급 |
|---|---|---|
| `OPEN` | 아직 조치·판정되지 않음 | 전체 |
| `FIXED` | 코드·문서·시험을 수정하고 검증함 | 전체 |
| `REJECTED_WITH_EVIDENCE` | finding 전제가 틀렸음을 근거와 원 reviewer 재확인으로 입증 | 전체 |
| `DEFERRED` | owner·상세 task·gate·기한을 연결해 후속 차단 | `P2`/`P3`만 |

4. `P0`/`P1`은 `FIXED` 또는 `REJECTED_WITH_EVIDENCE`로 원 reviewer가 재확인하기 전 merge하지 않는다. 단순 risk acceptance나 “release 차단” 표시는 P0/P1 closure가 아니다.
5. `P2`/`P3`의 `DEFERRED`는 owner, 상세 task, 적용 gate와 목표 시점이 모두 있어야 하며 열린 위험으로 표시한다.
6. 수정 위치, commit 또는 evidence, 재검증 명령과 결과를 각 finding에 연결한다.
7. 규범·코드 수정 결과를 새 commit으로 만들고 두 reviewer가 그 post-fix commit에서 자신의 finding과 전체 delta의 회귀를 재검토한다.
8. 두 재검토 verdict와 남은 위험을 통합 report, 관련 task와 PR 본문에 반영한다.

과거 review report에 새 실행 결과를 덧붙이지 않는다. 리뷰 파일 명명, correction, archive index 규칙은 [review archive](../reviews/README.md)를 따른다.

## 6. 기록 갱신

변경과 직접 관련된 기록만 갱신한다.

- 현재 상태나 다음 한 작업이 바뀌면 `docs/resume.md`
- task 상태·acceptance·evidence가 바뀌면 `docs/tasks.md`와 해당 상세 task
- 구조적 결정이 생기면 새 ADR와 `docs/adr/README.md`, `docs/decisions.md`
- 작업 재현 정보가 필요하면 `docs/journal.md` 최상단
- 사용자 가시 변경이면 `CHANGELOG.md`
- 적대적 리뷰를 실행했다면 새 review report와 `docs/reviews/README.md`

상세 형식과 문서 이동 절차는 [documentation maintenance](documentation-maintenance.md)를 따른다.

## 7. Stage, 보안 감사와 PR

파일을 경로별로 명시해 stage한다. `git add .`과 `git add -A`는 사용하지 않는다.

1. `git status`로 사용자 파일과 로컬 파일이 섞이지 않았는지 확인한다.
2. `git diff --staged` 전체를 직접 읽는다.
3. secret, private key, provisioning material, Wi-Fi credential, VIN, raw 위치정보, 개인 capture, 내부 주소가 없는지 검사한다.
4. 생성물·문서 링크·task index가 source와 일치하는지 확인한다.
5. branch를 push하고 main 대상 PR을 만든다. main에 직접 push하지 않는다.

PR 본문에는 다음을 포함한다.

- task와 변경 목적
- 통과한 gate와 정확한 실행 명령
- 전문 리뷰어 2명의 영역, report 링크와 최종 disposition
- 실패 또는 미실행 검증과 남은 위험
- evidence 위치와 digest
- rollback 방법

CI, 필수 reviewer gate와 미해결 `P0`/`P1` 확인이 끝나기 전 merge하지 않는다.

## 8. Merge 또는 abandon 뒤 정리

1. remote의 PR merge 결과와 `origin/main` commit을 확인한다.
2. 기본 checkout을 `main` 또는 다음 작업 branch로 전환하고 최신 상태로 맞춘다.
3. CodeGraph를 사용했다면 `sync` 후 `status`를 확인한다.
4. 임시 구현·리뷰 worktree마다 process, status, 보존할 commit을 확인하고 제거한 뒤 `git worktree prune`을 실행한다.
5. 완료 task, review disposition, 다음 작업을 필요한 기록 문서에 반영한다.

abandon할 때도 유용한 finding과 재현 근거가 있으면 상세 task나 새 review report에 남기고, 브랜치와 worktree를 제거하기 전에 복구 가능한 remote/ref를 확보한다.
