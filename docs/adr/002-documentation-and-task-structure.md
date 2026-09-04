# ADR-002: 문서·task·ADR·runbook 정본 구조

- 상태: superseded by ADR-004
- 날짜: 2026-09-05

## 컨텍스트

CANView 설계 문서는 기능별로 존재하지만, 에이전트가 진입할 정본 위치와 현재 진척·작업·결정의 관계가 분산되어 있었다. 상세 task를 유지하면서도 전체 backlog를 한 곳에서 확인할 수 있어야 한다.

## 결정

kor-travel-geo의 문서 구조를 적용한다.

- AGENTS.md와 SKILL.md는 저장소 작업 규칙의 정본이다.
- docs/architecture/는 상위 구조를 둔다.
- docs/adr/와 docs/decisions.md는 결정 기록을 둔다.
- docs/runbooks/는 반복 실행 절차를 둔다.
- docs/resume.md와 docs/journal.md는 현재 진척과 작업 이력을 둔다.
- 열린 task 요약은 docs/tasks.md 하나에 둔다.
- 각 task의 상세 내용은 docs/tasks/T-NNN-*.md 파일로 분리한다.
- 완료 task는 docs/tasks-done.md로 이동한다.

## 결과

task 본문이 길어져도 요약 인덱스가 안정적으로 유지되고, 새 에이전트가 같은 순서로 작업을 시작할 수 있다. 기존 docs/tasks/README.md 링크는 호환용 포인터로 남긴다.

## 대체

문서 분류와 agent 읽기 정책을 더 명확히 한 [ADR-004](004-layered-documentation-and-review-archive.md)가 이 결정을 대체한다. 상세 task 분리 원칙은 ADR-004에서도 유지한다.
