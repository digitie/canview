# tasks-rule.md — task 문서 작성·유지 규칙

이 문서는 docs/tasks.md, docs/tasks-done.md와 docs/tasks/ 상세 파일의 작성 규약 정본이다. PR·리뷰·병렬 실행 절차는 docs/runbooks/agent-workflow.md를 따른다.

## 1. 문서 역할

| 문서 | 역할 |
|------|------|
| docs/tasks.md | 열린 task의 요약 backlog와 critical path |
| docs/tasks-done.md | 완료·종료 task의 newest-first archive |
| docs/tasks/T-NNN-*.md | task 하나의 scope·수용 기준·검증·evidence |
| docs/resume.md | 현재 진척과 다음 한 작업 |

## 2. task ID

- T-001~T-099: 계약·도구·공통 모델
- T-100~T-199: Communicator hardware와 STM32
- T-200~T-299: Communicator ESP32와 peer routing
- T-300~T-399: Controller·LVGL·audio
- T-400~T-499: Diagnostic Bridge·웹·evidence
- T-500~T-599: HIL·차량 검증·release

이미 참조된 ID는 재번호하지 않는다. 하위 작업은 T-NNNa 형식으로 둔다.

## 3. 상태

상세 task 상단의 상태는 READY, BLOCKED, IN_PROGRESS, DONE 중 하나를 사용한다. 완료된 task는 docs/tasks-done.md로 옮긴다. 외부 hardware/evidence 대기는 BLOCKED로 유지한다.

## 4. 요약 entry

docs/tasks.md에는 다음 정도만 둔다.

    - [ ] **T-NNN** — 짧은 제목
      무엇을 완료하고 어느 문서·gate를 갱신하는지 한두 문장.

상세 수용 기준과 명령을 summary에 복제하지 않는다. summary의 상태·제목·선행 링크는 상세 파일과 일치해야 한다.

## 5. 상세 task 필수 항목

각 상세 파일은 목표, 고정 결정, 구현 범위, 범위 밖, 예상 변경 파일, 수용 기준, 검증 명령, evidence, rollback 또는 release 차단 조건을 포함한다.

## 6. 완료 처리

모든 수용 기준과 검증을 확인하고 비단순 변경이면 전문 리뷰어 서브에이전트 2인 gate를 통과한 뒤 docs/tasks-done.md에 날짜와 결과를 newest-first로 추가한다. 현재 상태가 바뀌면 docs/journal.md와 docs/resume.md도 갱신한다. 실패한 검증이나 미해결 P0/P1 finding을 남긴 채 DONE으로 바꾸지 않는다.

리뷰를 수행한 경우 기존 리뷰 문서에 append하지 않고 [review archive](reviews/README.md) 규칙에 따라 새 report를 만든다.
