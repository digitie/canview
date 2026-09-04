# CANView 에이전트 작업·문서화 가이드

이 문서는 AGENTS.md와 SKILL.md를 실제 작업 순서로 연결한다. 환경·실패 복구 절차는 [runbooks](runbooks/README.md), task 규약은 [tasks-rule.md](tasks-rule.md)가 정본이다.

## 1. 첫 진입 순서

1. README.md
2. AGENTS.md
3. SKILL.md
4. docs/architecture/architecture.md
5. docs/resume.md
6. docs/journal.md 최신 항목
7. docs/tasks-rule.md와 docs/tasks.md
8. 관련 ADR
9. 변경 대상 subsystem 문서
10. docs/runbooks/agent-workflow.md와 agent-failure-patterns.md

## 2. 기록 문서 5종

| 문서 | 역할 |
|------|------|
| docs/decisions.md | ADR 색인과 구조적 결정 |
| docs/resume.md | 현재 진척과 다음 한 작업 |
| docs/journal.md | 역시간순 작업 로그 |
| docs/tasks.md + docs/tasks-done.md | 열린 task와 완료 이력 |
| CHANGELOG.md | 사용자 가시 변경의 릴리스 기록 |

코드·protocol·회로·UI를 바꾸었는데 관련 기록이 하나도 갱신되지 않았다면 작업은 불완전한 것으로 본다.

## 3. journal 형식

가장 최근 항목을 위에 추가한다. 각 항목은 가능한 한 작업, 변경 파일, 결정, 발견, 다음을 포함한다. 기존 항목은 임의로 고치지 않는다.

## 4. resume 형식

현재 상태, 다음 한 작업, 시작 파일, 검증 방법, 알려진 함정과 차단 조건을 짧게 유지한다. 상세한 설계나 과거 로그를 resume에 복제하지 않는다.

## 5. ADR 형식

ADR는 파일당 하나로 docs/adr/ 아래에 두고, docs/decisions.md에서 색인한다. 결정이 뒤집히면 옛 문서를 삭제하지 않고 새 ADR를 추가해 superseded 상태를 기록한다.

## 6. 변경 분류별 확인

- protocol: schema → generated ABI → codec → golden vector → malformed input
- firmware: host test → target compile → reset/fault test → evidence
- hardware: exact MPN/package → schematic → ERC → BOM/netlist → power/SI calculation
- DBC/profile: upstream digest → candidate evidence → generated catalog/safety profile
- UI: model freshness/pending → LVGL adapter → static prototype → device test
- vehicle: CAPTURE_ONLY → read-only promotion → HIL → BENCH_TX → closed-course gate

## 7. PR

main에 직접 push하지 않는다. PR 본문에는 Task, Gate, Risk, Tests, Evidence, Rollback을 포함한다. 차량 제어와 인증 변경은 해당 gate가 닫히기 전까지 release 가능으로 표시하지 않는다.
