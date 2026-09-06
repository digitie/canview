# ADR-004: 계층형 문서 정보구조와 누적 독립 리뷰 아카이브

- 상태: accepted
- 날짜: 2026-09-05
- Supersedes: ADR-002

## 문맥

초기 문서 구조는 상세 task 분리에는 성공했지만 `AGENTS.md`, `SKILL.md`, 개발환경, worktree, CodeGraph와 runbook에 같은 진입 절차가 반복됐다. 상세 설계도 `docs/` 루트에 평평하게 놓여 architecture와 참고 자료의 관계가 불명확했다. 에이전트가 매 작업마다 긴 문서와 과거 review를 통독하면 토큰을 낭비하고, 서로 다른 사본 중 무엇이 현재 정본인지 오판할 위험이 있다.

기존 단일 적대적 리뷰 문서는 한 파일에 후속 review를 누적할 가능성이 있어 기준 commit, reviewer 독립성과 finding disposition을 감사하기 어려웠다.

## 결정

1. `AGENTS.md`는 모든 작업에 적용되는 짧은 정책·안전 금지선의 단일 정본으로 둔다. `SKILL.md`는 작업별 상세 문서를 고르는 router이며 정책을 복제하지 않는다.
2. `docs/README.md`를 중앙 문서 지도로 두고 문서를 반드시 참조, 필요할 때 참조, 특수한 경우에만 참조하는 세 단계로 분류한다.
3. 현재 구조와 protocol 설계는 `docs/architecture/`, 장치 회로는 `docs/hardware/`, Windows와 toolchain은 `docs/development/`, 차량 evidence는 `docs/vehicle/`, 화면 설계는 `docs/ui/`에 둔다.
4. architecture는 현재 설계, ADR은 결정 근거, task는 실행 범위·acceptance, runbook은 반복 절차, review와 journal은 역사 evidence라는 책임을 분리한다.
5. 상세 task는 계속 `docs/tasks/T-NNN-*.md`로 나누고 `docs/tasks.md`에는 상태·의존성·요약·링크만 둔다.
6. 적대적 리뷰는 실행마다 immutable 기준선을 대상으로 전문 리뷰어 서브에이전트 2명이 독립적으로 수행한다. 결과는 `docs/reviews/adversarial/YYYY-MM-DD-<scope>.md` 새 파일에 누적 보존하고 모든 finding의 disposition을 기록한다.
7. 과거 review report는 현재 설계 정본으로 사용하거나 새 review 결과를 덧붙이지 않는다. finding 반영 결과는 코드, architecture, ADR, task와 검증 evidence에 남긴다.

## 결과

- 에이전트는 중앙 인덱스에서 작업과 직접 관련된 문서만 점진적으로 읽을 수 있다.
- 상세 경로와 정본 책임이 분리되어 중복 정책과 stale 사본이 줄어든다.
- protocol·hardware·UI 설계 파일은 의미 있는 하위 디렉터리에서 탐색된다.
- 독립 리뷰의 기준선, 두 전문 관점, finding과 조치가 실행별로 감사 가능하다.
- 문서 이동 시 incoming link, plain path와 상위 인덱스를 함께 검증해야 한다.

## 대안 검토

- **평평한 `docs/`와 하나의 장문 AGENTS 유지**: 검색은 단순하지만 정본 경계와 선택적 읽기가 어렵고 중복이 다시 생긴다.
- **모든 정책을 SKILL에 통합**: skill이 없는 도구와 사람에게 필수 안전 규칙이 보이지 않으므로 채택하지 않았다.
- **하나의 review 문서에 계속 append**: 최신 상태를 한 파일에서 볼 수 있지만 서로 다른 기준선과 reviewer 결과가 섞여 evidence 추적성이 낮아진다.

## 적용 위치

- 정책과 읽기 계층: `AGENTS.md`, `SKILL.md`, `docs/README.md`
- 현재 설계: `docs/architecture/`와 분야별 상세 디렉터리
- 작업 절차: `docs/runbooks/agent-workflow.md`
- 리뷰 기록 규칙: `docs/reviews/README.md`
- 문서 유지 규칙: `docs/runbooks/documentation-maintenance.md`
