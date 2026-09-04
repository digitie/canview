# CANView 리뷰 아카이브

이 디렉터리는 특정 commit 또는 diff를 대상으로 수행한 리뷰를 누적 보존한다. review report는 역사적 evidence이며 현재 설계의 정본은 아니다. finding을 반영한 결과는 architecture, ADR, task와 코드에 남긴다.

## 기록 규칙

1. 적대적 리뷰 요청마다 `adversarial/YYYY-MM-DD-<scope>.md` 파일을 새로 만든다. 같은 날 같은 범위가 반복되면 `-02`, `-03` suffix를 붙인다.
2. 과거 report에 새 review 결과를 덧붙이지 않는다. 오탈자나 깨진 링크를 고친 경우에만 correction note를 남긴다.
3. report는 review 종류, 기준 commit/diff, 범위 밖, reviewer 전문 영역, 검증 방법과 작성 시각을 기록한다.
4. 전문 리뷰어 서브에이전트 2명은 같은 immutable 기준선을 서로 독립적으로 검토한다. 두 번째 reviewer에게 첫 report를 보여 주지 않는다.
5. finding ID는 report 안에서 고유하게 유지하고 심각도 `P0`~`P3`, 근거, 실패 형태, 권고, disposition을 포함한다.
6. `P0`/`P1`은 수정·명시적 설계 결정·release 차단 중 하나로 닫기 전 merge하지 않는다. `P2`/`P3`를 미루면 owner와 상세 task를 연결한다.
7. review 반영 뒤 관련 검증을 다시 실행하고 report의 disposition table과 PR 본문에 결과를 기록한다.

## 리뷰 목록

최신 항목을 위에 추가한다.

| 날짜 | 종류 | 기준선·범위 | 리뷰어 | 결과 |
|---|---|---|---|---|
| 2026-09-04 | 독립 적대적 설계 리뷰 | `50b5e733`, 전체 구현 준비성 | 전원·안전·protocol / build·통합·시험 | [report](adversarial/2026-09-04-baseline-design.md) |

## 새 리뷰 시작

[적대적 리뷰 템플릿](adversarial/TEMPLATE.md)을 복사해 새 파일을 만들고, [agent workflow](../runbooks/agent-workflow.md)의 2인 리뷰 gate를 따른다. 일반 작업 시작 시 이 아카이브 전체를 읽지 않는다.
