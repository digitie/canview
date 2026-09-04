# CANView 결정 색인

구조적 결정은 파일당 하나의 ADR로 docs/adr/ 아래에 기록한다. 이 문서는 목록과 현재 상태만 제공하며 상세 근거는 각 ADR가 정본이다. 다음 후보 번호는 ADR-004이다.

| ADR | 상태 | 제목 | 위치 |
|-----|------|------|------|
| ADR-001 | accepted | Controller·Communicator·Diagnostic Bridge와 차량 송신 안전 경계 | [001-canview-safety-boundary.md](adr/001-canview-safety-boundary.md) |
| ADR-002 | accepted | 문서·task·ADR·runbook 정본 구조 | [002-documentation-and-task-structure.md](adr/002-documentation-and-task-structure.md) |
| ADR-003 | accepted | Windows 개발환경과 일회성 worktree | [003-windows-development-and-ephemeral-worktrees.md](adr/003-windows-development-and-ephemeral-worktrees.md) |

설계 세부는 [implementation-readiness.md](implementation-readiness.md), 기능·protocol·하드웨어의 배경은 관련 subsystem 문서를 함께 참조한다. 결정이 바뀌면 기존 ADR를 삭제하지 않고 새 ADR를 추가한다.
