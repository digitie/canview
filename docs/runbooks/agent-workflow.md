# CANView 에이전트 개발 워크플로

## 1. 작업 시작

1. README.md, AGENTS.md, SKILL.md, docs/architecture/architecture.md, docs/resume.md, docs/tasks.md를 읽는다.
2. docs/tasks-rule.md와 관련 상세 task에서 scope·선행·gate를 확인한다.
3. Linux worktree에서 최신 main을 가져와 새 branch를 만든다.

    cd /mnt/f/dev/canview
    git fetch origin main
    git switch -c agent/<agent>-<task> origin/main

4. CodeGraph가 연결되어 있으면 sync 후 status를 실행한다.

## 2. 작업과 검증

고정 worktree에서는 편집·commit·push를 하고, WSL ext4 미러에서는 의존성 설치·build·test를 한다.

검증 순서는 protocol/schema → host test → target build → KiCad 또는 UI 검증 → HIL/fault → 관련 evidence다. 실제 차량이 필요한 task는 차량 gate를 낮추어 대체하지 않는다.

실패하거나 미실행된 검증은 숨기지 않고 task·journal·PR에 명시한다.

## 3. 기록

작업 종료 전 docs/journal.md에 역시간순 항목을 추가하고 docs/resume.md의 다음 한 작업을 갱신한다. 설계 결정이 생기면 ADR를 추가하며, 열린 task 상태는 docs/tasks.md와 상세 문서에서 함께 바꾼다.

## 4. PR

변경 파일을 명시적으로 stage하고 diff와 secret scan을 확인한다. PR 본문에는 다음을 포함한다.

- Task와 변경 목적
- 통과한 Gate와 실행 명령
- 남은 위험과 미실행 검증
- evidence 위치와 digest
- rollback 방법

CI와 필요한 리뷰가 완료되기 전 main에 직접 push하지 않는다.

## 5. 머지 후

main과 origin/main을 확인하고, branch 전환 뒤 CodeGraph sync/status를 실행한다. 머지 결과와 다음 작업을 journal/resume에 반영한다.
