# CANView 에이전트 운영 Runbook

이 디렉터리는 반복 작업 절차와 실패 복구 방법을 둔다. 설계 정본은 docs/architecture/와 docs/implementation-readiness.md, task 정본은 docs/tasks.md와 docs/tasks/에 있다.

## 필독

- [agent-workflow.md](agent-workflow.md): branch·미러·검증·PR·문서 갱신 흐름
- [agent-failure-patterns.md](agent-failure-patterns.md): 환경·하드웨어·protocol·merge 실패의 분류와 복구

## 구분

| 문서 | 범위 |
|------|------|
| agent-workflow.md | 모든 agent가 따르는 표준 1-PR 절차 |
| agent-failure-patterns.md | 반복 실패를 재현·분류·기록하는 방법 |
