# CANView ADR — Architecture Decision Records

CANView의 구조적 결정은 파일당 하나의 ADR로 둔다. 파일명은 NNN-<slug>.md 형식이며, 이 색인은 상태와 제목을 관리한다.

## 규칙

- 핵심 장치 구조, 안전 권한, protocol 계약, 저장소·문서 구조처럼 여러 subsystem에 영향을 주는 결정만 ADR로 기록한다.
- 공통 정책은 `AGENTS.md`, 작업별 문서 선택은 `SKILL.md`, task 규칙과 반복 절차는 `docs/tasks-rule.md`와 `docs/runbooks/`에 둔다.
- 결정을 뒤집을 때는 새 ADR를 만들고 이전 ADR의 상태를 superseded by ADR-XXX로 바꾼다.
- ADR 본문과 코드·문서·테스트는 같은 PR에서 동기화한다.
- 다음 번호는 docs/decisions.md의 최댓값 + 1로 배정한다.

## 목록

| ADR | 제목 | 상태 |
|-----|------|------|
| [ADR-001](001-canview-safety-boundary.md) | Controller·Communicator·Diagnostic Bridge와 차량 송신 안전 경계 | accepted |
| [ADR-002](002-documentation-and-task-structure.md) | 문서·task·ADR·runbook 정본 구조 | superseded by ADR-004 |
| [ADR-003](003-windows-development-and-ephemeral-worktrees.md) | Windows 개발환경과 일회성 worktree | accepted |
| [ADR-004](004-layered-documentation-and-review-archive.md) | 계층형 문서 정보구조와 누적 독립 리뷰 아카이브 | accepted |
| [ADR-005](005-latest-windows-embedded-toolchain.md) | 최신 Windows 임베디드 toolchain baseline | accepted |
