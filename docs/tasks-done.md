# CANView 완료 task archive

완료·종료 task를 newest-first로 이동해 기록한다. 설계 감사와 문서 구조 정리는 구현 task 완료 이력과 분리해 PR·journal에 기록한다.

## 2026-09-06

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-001](tasks/T-001-host-toolchain-ci.md) | DONE | P0 | 재현 가능한 host toolchain과 CI | 없음 |

T-001은 acceptance·evidence·2인 적대적 리뷰·원격 target CI 통과 후 PR #17이 `74d43ff`로 main에 merge됐다. 실제 보드 flash·HIL·차량 CAN TX·production OTA는 별도 gate로 남겼다.

## 2026-09-07

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-002](tasks/T-002-espnow-schema-v1.3.md) | DONE | P0 | ESP-NOW v1.3 schema와 생성 header 동결 | T-001 |

T-002는 acceptance·host/target CI·2인 적대적 리뷰를 통과한 뒤 PR #18이 merge commit `c18a8a5`로 `main`에 통합됐다. 생성 header/golden/malformed/compatibility 계약은 유지하며, 보드 flash·RF/HIL·실차·production OTA signing은 별도 NOT_RUN gate다.
