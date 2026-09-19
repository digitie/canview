# T-007 signed golden post-fix 재검토

- Candidate: `262bf092b8516286552e5e01d881009b95551247`
- Base: `bd9a1a6212413674bf35078016356fb90f9f1728`
- 범위: [최초 리뷰](2026-09-19-T-007-signed-golden.md)의 동일 P2와9파일 delta
- 판정: 원 A/B 정적 `PASS`, A-SG-01/B-P2-01 `FIXED`, 신규 P0/P1/P2/P3 없음

## 원문과 실행 범위

| 항목 | A | B |
|---|---|---|
| 전문 영역 | 암호·CNG 경계 | SDK·CI·근거 |
| Subagent | `01a0b784-5d6a-7540-97e8-ac159eb3a828` | `01a0b784-5e5f-7d42-8953-968159b85914` |
| Execution UUID | `819f0d36-1440-4715-a7d1-427601f44a23` | `0530b7af-7f8d-44f8-b658-5165130e604c` |
| 시작 UTC | `2026-09-19T03:29:17.2480032Z` | `2026-09-19T03:27:07.4516392Z` |
| 종료 UTC | `2026-09-19T03:30:45.0137103Z` | `2026-09-19T03:28:46.7488231Z` |
| 격리 | commit object-only, hash 재확인 | commit object-only, hash 재확인 |
| 원문 | [A](evidence/2026-09-19-T-007-signed-golden-post-reviewer-a.md) | [B](evidence/2026-09-19-T-007-signed-golden-post-reviewer-b.md) |

두 reviewer는 서로의 새 결과를 공유하지 않고 수정 시험 전체와 C verifier/probe 경로를
직접 읽었다. payload·native SHA256 TLV·whole hash를 갱신하고 원본 서명을 유지한
네 번째 사례가 실제 verify까지 도달하며 status와 호출 수로 조기 거절·서명 결과 무시를
구별함을 확인했다. README의 검사 범위 구분도 일치한다.

A는 보존 Git blob을 메모리에서 변이하여 TLV offset/hash·서명 보존을 독립 확인했다.
B는 기존 body probe 숫자 출력과 공통 helper의 호환성도 대조했다. 둘 모두 compiler·
CNG·SDK·mutant를 실행하지 않았고 실제 실행 결과는 coordinator 근거로 구분했다.
raw 결과와 전달 요청은 그대로 보존한다.

## Disposition과 한계

A-SG-01과 B-P2-01은 같은 P2이며 원 reviewer 모두 `FIXED`로 닫았다.
로컬 post-fix Debug/Release143/143 및 실제 C mutant 검출 근거는 최초 통합 기록에 있다.
새 CI35418535641과 target artifact 확인은 진행 중이다. 이후 PSA adapter 추가분이나
전체 T-007·정상 OTA owner/target·제품 signing 구현의 리뷰를 대신하지 않는다.
PR36 Draft, physical/HIL·장치 native 실행·Flash `NOT_RUN`, 차량 TX `NO-GO`를 유지한다.
