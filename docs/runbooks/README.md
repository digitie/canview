# CANView 에이전트 운영 Runbook

이 디렉터리에는 반복 실행 절차만 둔다. 시스템·protocol 설계는 [architecture](../architecture/README.md), 작업 정본은 [tasks](../tasks.md)와 `docs/tasks/`, 전체 문서 선택은 [문서 지도](../README.md)를 따른다.

모든 runbook을 작업 시작 때 읽지 않는다. 현재 상황에 해당하는 문서만 연다.

| 문서 | 읽는 시점 | 책임 |
|---|---|---|
| [agent workflow](agent-workflow.md) | branch 생성, 구현 검증, 2인 리뷰, PR, merge를 수행할 때 | Windows branch/worktree, CodeGraph, gate, 독립 적대적 리뷰, 보안 감사, 정리 |
| [documentation maintenance](documentation-maintenance.md) | 문서·ADR·task·review를 만들거나 이동할 때 | 정본 관계, 갱신 조건, 누적 기록, link 검증 |
| [failure patterns](agent-failure-patterns.md) | 실제 실패를 분류하거나 반복을 막을 때 | 환경·protocol·hardware·merge 실패의 진단과 복구 |

새 반복 절차를 추가할 때 제품 설계나 일회성 작업 로그를 이 디렉터리에 넣지 않는다. 설계 결정은 ADR, 현재 구조는 architecture, 일회성 결과는 task·review·journal에 둔다.
