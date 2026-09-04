# YYYY-MM-DD <범위> 적대적 리뷰

- Review ID:
- 종류: 전문 리뷰어 서브에이전트 2인 독립 적대적 리뷰
- Review candidate commit:
- Base/parent commit:
- Post-fix commit:
- 대상 범위:
- 범위 밖:
- 관련 task 또는 사용자 요청:
- Coordinator:
- 시작·종료 시각:
- 상태: `IN_REVIEW`

## 1. 공통 manifest와 기준선 검증

두 reviewer에게 전달한 동일 manifest, acceptance, review request와 실행 가능한 검증을 기록한다. 기준선은 [agent workflow §5.1](../../runbooks/agent-workflow.md#51-review-manifest와-immutable-기준선)의 object-only 또는 detached worktree 방식으로 확인한다.

| 항목 | Reviewer A | Reviewer B |
|---|---|---|
| 전문 영역 | | |
| subagent/thread 실행 ID | | |
| 시작·종료 시각 | | |
| 격리 방식 | | |
| 실제 관찰 commit | | |
| 시작·종료 clean 검증 | | |
| 원본 evidence | `evidence/YYYY-MM-DD-scope-reviewer-a.md` | `evidence/YYYY-MM-DD-scope-reviewer-b.md` |
| 최초 verdict | | |

두 원본 evidence 파일이 저장되기 전에는 reviewer 사이에 finding을 공유하지 않는다.

## 2. Reviewer A findings

| ID | 심각도 | 근거·위치 | 재현·실패 형태 | 영향 | 권고 |
|---|---|---|---|---|---|
| A-P0-01 | P0 | | | | |

## 3. Reviewer B findings

| ID | 심각도 | 근거·위치 | 재현·실패 형태 | 영향 | 권고 |
|---|---|---|---|---|---|
| B-P0-01 | P0 | | | | |

## 4. 교차 확인과 최초 판정

두 원본이 확정된 뒤 중복 finding, reviewer 간 불일치, GO/NO-GO 범위와 남은 불확실성을 기록한다.

## 5. 반영 결과

| Finding | 상태 | 변경 또는 기각 근거 | Owner·task·gate·기한 | 재검증 | 원 reviewer 재확인 |
|---|---|---|---|---|---|
| | `OPEN` / `FIXED` / `REJECTED_WITH_EVIDENCE` / `DEFERRED` | | | | |

`P0`/`P1`은 `FIXED` 또는 `REJECTED_WITH_EVIDENCE`로 원 reviewer가 재확인하기 전 merge하지 않는다. `DEFERRED`는 `P2`/`P3`에만 허용하며 owner·상세 task·gate·기한이 모두 필요하다.

## 6. Post-fix 재검토

| Reviewer | 확인 commit | 확인 범위 | verdict | 잔여 위험 |
|---|---|---|---|---|
| A | | 자신의 finding + 전체 delta 회귀 | | |
| B | | 자신의 finding + 전체 delta 회귀 | | |

## 7. 최종 판정

- 최종 verdict:
- 열린 finding:
- 미실행 검증:
- PR 반영 위치:

이 report와 reviewer 원본 evidence를 기존 review 파일에 append하지 않는다. 오탈자 correction 외 변경은 correction note를 남긴다.
