# YYYY-MM-DD <범위> 적대적 리뷰

- 종류: 적대적 설계·코드 리뷰
- 기준 commit/diff:
- 대상 범위:
- 범위 밖:
- 리뷰 요청자:
- Reviewer A 전문 영역:
- Reviewer B 전문 영역:
- 상태: 진행 중

## 1. 검토 방법

두 reviewer가 같은 immutable 기준선을 독립적으로 검토한 방법, 실행한 명령과 확인하지 못한 gate를 기록한다. 서로의 finding은 각 report가 완성된 뒤에만 비교한다.

## 2. Reviewer A findings

| ID | 심각도 | 근거·위치 | 실패 형태 | 권고 |
|---|---|---|---|---|
| A-P0-01 | P0 | | | |

## 3. Reviewer B findings

| ID | 심각도 | 근거·위치 | 실패 형태 | 권고 |
|---|---|---|---|---|
| B-P0-01 | P0 | | | |

## 4. 교차 확인과 최종 판정

중복 finding, reviewer 간 불일치, GO/NO-GO 범위와 남은 불확실성을 기록한다.

## 5. 반영 결과

| Finding | Disposition | 변경 또는 근거 | 연결 ADR/task | 재검증 |
|---|---|---|---|---|
| | 수정 / 결정 / 연기 / 기각 | | | |

`P0`/`P1`이 열려 있으면 상태를 완료로 바꾸지 않는다. 연기한 finding은 owner와 상세 task가 없으면 닫힌 것으로 간주하지 않는다.
