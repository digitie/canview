# CANView 리뷰 아카이브

이 디렉터리는 특정 commit 또는 diff를 대상으로 수행한 리뷰를 누적 보존한다. review report는 역사적 evidence이며 현재 설계의 정본은 아니다. finding을 반영한 결과는 architecture, ADR, task와 코드에 남긴다.

## 기록 규칙

1. 적대적 리뷰 요청마다 `adversarial/YYYY-MM-DD-<scope>.md` 파일을 새로 만든다. 같은 날 같은 범위가 반복되면 `-02`, `-03` suffix를 붙인다.
2. 과거 report에 새 review 결과를 덧붙이지 않는다. 오탈자나 깨진 링크를 고친 경우에만 correction note를 남긴다.
3. report는 review ID, 종류, 기준 commit과 base, 범위 밖, reviewer 전문 영역, 검증 방법, 시작·종료 시각을 기록한다.
4. 전문 리뷰어 서브에이전트 2명은 같은 manifest와 immutable 기준선을 서로 독립적으로 검토한다. 두 원본 결과가 확정되기 전에는 어느 reviewer에게도 상대 결과를 보여 주지 않는다.
5. reviewer별 실행 ID, 전달 입력, 실제 관찰 hash, 격리·clean 검증, 원본 결과는 `adversarial/evidence/YYYY-MM-DD-<scope>-reviewer-{a,b}.md`에 따로 보존하고 통합 report에서 연결한다.
6. finding ID는 report 안에서 고유하게 유지하고 심각도 `P0`~`P3`, 근거·위치, 재현 또는 실패 형태, 영향, 권고와 disposition을 포함한다.
7. 심각도와 disposition 상태의 정본은 [agent workflow §5](../runbooks/agent-workflow.md#5-전문-리뷰어-서브에이전트-2인-적대적-리뷰)다. `P0`/`P1`은 단순 risk acceptance나 release 차단으로 닫지 않는다.
8. review 반영 뒤 관련 검증을 다시 실행하고 post-fix commit을 두 reviewer가 재검토한다. report와 PR에 두 verdict를 기록한다.
9. review 원본·통합 disposition·재검증 결과와 archive index만 기록하는 closure commit은 같은 review를 재귀적으로 시작하지 않는다. 규범 문구를 함께 바꾸면 새 기준선 리뷰가 필요하다.

## 리뷰 목록

최신 항목을 위에 추가한다.

| 날짜 | 종류 | 기준선·범위 | 리뷰어 | 결과 |
|---|---|---|---|---|
| 2026-09-05 | 최신 toolchain bootstrap | `bfdd2c2`, target 환경·SDK pin·ESP-IDF/STM32 CMake | embedded safety·target build / protocol·재현성·문서 | [report](adversarial/2026-09-05-latest-toolchain-bootstrap.md) |
| 2026-09-05 | 문서 정보구조·review gate | `b6f523f`, 문서 구조·운영 정책 | 문서 IA·agent 실행성 / 품질 gate·감사성 | [report](adversarial/2026-09-05-document-information-architecture.md) |
| 2026-09-04 | 독립 적대적 설계 리뷰 | `50b5e733`, 전체 구현 준비성 | 전원·안전·protocol / build·통합·시험 | [report](adversarial/2026-09-04-baseline-design.md) |

## 새 리뷰 시작

[적대적 리뷰 템플릿](adversarial/TEMPLATE.md)을 복사해 새 파일을 만들고, [agent workflow](../runbooks/agent-workflow.md)의 2인 리뷰 gate를 따른다. 일반 작업 시작 시 이 아카이브 전체를 읽지 않는다.
