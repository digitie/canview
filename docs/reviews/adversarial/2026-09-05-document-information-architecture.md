# 2026-09-05 문서 정보구조와 리뷰 gate 적대적 리뷰

- Review ID: `ADV-2026-09-05-DOC-IA-01`
- 종류: 전문 리뷰어 서브에이전트 2인 독립 적대적 리뷰
- Review candidate commit: `b6f523fe1d5fdaa01b337facbbda127cc384cda5`
- Base/parent commit: `018d9ecc4d1651a5d00f640011ee861f164d7d87`
- Post-fix commit: `ab613c8f70d6bb5658c20b662e152e61d421c239`
- 대상 범위: AGENTS·문서 정보구조·Windows/worktree·CodeGraph·2인 review gate·대규모 문서 이동
- 범위 밖: firmware runtime, 실제 hardware/HIL/차량, 외부 link 내용 검증
- 관련 요청: 문서 개괄→상세 재구성, 상세 문서 하위 분류, 실행별 review 누적, 전문 reviewer 2인의 결과 반영
- Coordinator: Codex 주 작업 에이전트
- 리뷰 시작·종료: `2026-09-05T06:50:52+09:00` ~ `2026-09-05T09:29:58+09:00`
- 상태: `COMPLETE`

## 1. 공통 manifest와 독립성

두 reviewer를 별도 subagent thread로 동시에 시작하고 같은 candidate, base, 사용자 요구와 공통 finding 형식을 전달했다. 각자 final output을 확정하기 전에는 상대 thread나 결과를 제공하지 않았다. 두 reviewer 모두 commit object-only 방식으로 `git show`와 `git diff`를 사용했고 작업 트리를 수정하지 않았다.

| 항목 | Reviewer A | Reviewer B |
|---|---|---|
| 전문 영역 | 문서 IA·인지부하·Windows/worktree·탐색성 | 품질 gate·감사성·disposition·Git/PR 회귀 |
| Subagent thread ID | `01a06e67-2e3f-7343-b605-e72ad615a94b` | `01a06e67-3b80-77f1-80b0-e6616ec4e4eb` |
| Turn ID | `01a06e67-3329-7021-b05c-fedf06b8480a` | `01a06e67-4129-7202-9b67-94deee8b614b` |
| 실행 시각 | 06:50:52~06:59:20 KST | 06:50:56~06:58:22 KST |
| 격리 방식 | commit object-only | commit object-only |
| 실제 관찰 commit | `b6f523fe1d5fdaa01b337facbbda127cc384cda5` | `b6f523fe1d5fdaa01b337facbbda127cc384cda5` |
| 작업 트리 변경 | 없음 | 없음 |
| 원본 evidence | [Reviewer A](evidence/2026-09-05-document-information-architecture-reviewer-a.md) | [Reviewer B](evidence/2026-09-05-document-information-architecture-reviewer-b.md) |
| 최초 verdict | `BLOCK` | `BLOCK` |

Reviewer A는 83개 Markdown의 local link 547개, Reviewer B는 별도 parser로 local link 459개를 검사했다. parser의 포함 범위는 달랐지만 둘 다 미해결 target 0개, task index/detail 34/34와 `git diff --check` 통과를 확인했다.

## 2. Reviewer A findings

| ID | 심각도 | 근거·위치 | 재현·실패 형태 | 영향 | 권고 |
|---|---|---|---|---|---|
| A-P1-01 | P1 | workflow §5, review tree | 새 정책 commit에 그 정책이 요구한 2인 report가 없음 | gate closure 감사 불가 | 동일 candidate의 독립 결과·report·재검증 추가 |
| A-P2-01 | P2 | `SKILL.md`, `docs/README.md` 시작 규칙 | task 없는 문서·review 작업도 상세 task가 필수인 것으로 읽힘 | 무관한 task 선택·토큰 낭비 | task가 있을 때만 읽도록 조건화 |
| A-P3-01 | P3 | automation·ESP-NOW·Controller hardware link label | target은 새 경로지만 label은 옛 basename | 정본 검색 혼선 | 역할 기반 label로 교체 |

추가 공격에서 Windows worktree 명령 중복과 parent directory 생성 누락도 확인됐다.

## 3. Reviewer B findings

| ID | 심각도 | 근거·위치 | 재현·실패 형태 | 영향 | 권고 |
|---|---|---|---|---|---|
| B-P1-01 | P1 | workflow review 준비 | 이동 가능한 branch를 read-only로 가정하고 hash·dirty 검증 없음 | 다른 tree를 검토할 수 있음 | object-only 또는 detached 검증 명령 고정 |
| B-P1-02 | P1 | workflow·template | reviewer 실행 ID·입력·원본 artifact가 없음 | 한 명 실행·finding 누락을 감사할 수 없음 | reviewer별 evidence와 metadata 보존 |
| B-P1-03 | P1 | workflow·review archive | P0/P1 release 차단 closure와 merge 금지가 충돌 | risk acceptance로 merge 우회 | disposition과 등급별 허용 상태 고정 |
| B-P1-04 | P1 | workflow 면제 | 작성자 자기신고만으로 link/서식 면제 가능 | normative gate를 면제로 약화 | 비면제 경로·좁은 조건·비작성자 승인 |
| B-P1-05 | P1 | T-001 대 Windows 정본 | P0 task가 Linux를 기준환경으로 선언 | local·CI 재현성 분기 | Windows 필수, Linux portability 역할 분리 |
| B-P2-01 | P2 | review template | workflow가 요구한 impact 열이 없음 | 심각도 근거 누락 | impact·재현 열 추가 |
| B-P2-02 | P2 | workflow·archive | P0~P3 정의가 없음 | reviewer별 gate 판정 불일치 | 등급과 merge 효과 정의 |
| B-P3-01 | P3 | `docs/README.md` UI 안내 | 존재하지 않는 prototype README를 약속 | 탐색 중단 | README 추가 또는 안내 제거 |

## 4. 교차 확인과 최초 판정

두 reviewer는 공통으로 이 변경 자체의 2인 review evidence 부재를 merge 차단 사유로 보았다. Reviewer A는 진입 규칙·stale label을, Reviewer B는 review 제도의 위조·우회 가능성과 T-001 환경 충돌을 독립적으로 추가 발견했다. 직접적인 차량 송신·runtime `P0`는 없었지만 총 6개 `P1`, 3개 `P2`, 2개 `P3`와 worktree 중복 관찰사항 때문에 최초 통합 verdict는 `BLOCK`이다.

## 5. 반영 결과

| Finding | 상태 | 변경 또는 근거 | Owner·task·gate·기한 | 재검증 | 원 reviewer 재확인 |
|---|---|---|---|---|---|
| A-P1-01 | `FIXED` | 이 report, reviewer별 최초·post-fix evidence, archive index를 추가 | Coordinator, 현재 PR | report metadata·link 검사 | `FIXED*`; closure 기록 조건 충족 |
| A-P2-01 | `FIXED` | SKILL·문서 지도·workflow에 “task가 있는 경우”와 docs/review 진입점 명시 | Coordinator, 현재 PR | 세 정본 문구 교차 검사 | `FIXED` |
| A-P3-01 | `FIXED` | 4개 옛 basename label을 역할 label로 교체 | Coordinator, 현재 PR | stale basename 검색 0건 | `FIXED` |
| A 추가 관찰 | `FIXED` | Windows 문서에서 worktree 명령을 제거해 workflow 한 곳으로 위임하고 두 worktree 절차에 parent 생성 추가 | Coordinator, 현재 PR | 두 문서와 명령 교차 검사 | `FIXED` |
| B-P1-01 | `FIXED` | object-only와 detached 방식, hash·clean 검증 명령 추가 | Coordinator, 현재 PR | post-fix object-only 검사 | `FIXED` |
| B-P1-02 | `FIXED` | execution/time/manifest/hash/evidence를 template과 archive에 필수화하고 reviewer별 원본 보존 | Coordinator, 현재 PR | evidence metadata 교차 확인 | `FIXED` |
| B-P1-03 | `FIXED` | disposition 4종 정의, P0/P1은 FIXED 또는 evidence 기각+원 reviewer 확인만 허용 | Coordinator, 현재 PR | 상충 문구 검색 0건 | `FIXED` |
| B-P1-04 | `FIXED` | normative 경로 비면제, 좁은 면제 조건·비작성자 승인·closure artifact 경계 명시 | Coordinator, 현재 PR | 면제 문구 교차 검사 | `FIXED` |
| B-P1-05 | `FIXED` | T-001을 Windows 필수 기준으로 바꾸고 Linux는 portability/sanitizer matrix로 분리 | Coordinator, T-001/G0 | Windows/Linux 기준 교차 검사 | `FIXED` |
| B-P2-01 | `FIXED` | template에 재현·실패 형태와 영향 열 추가 | Coordinator, 현재 PR | template 필드 검사 | `FIXED` |
| B-P2-02 | `FIXED` | workflow에 P0~P3 기준과 merge 효과 추가 | Coordinator, 현재 PR | 등급 정의 검사 | `FIXED` |
| B-P3-01 | `FIXED` | `ui/prototype/README.md`를 추가하고 문서 지도에서 직접 링크 | Coordinator, 현재 PR | snapshot local link 검사 | `FIXED` |

두 원 reviewer는 `ab613c8`에서 자신의 finding 전부가 해소됐고 전체 delta에 신규 `P0`/`P1` 회귀가 없음을 확인했다. 두 재검토의 `CONDITIONAL` 조건은 이 절의 disposition과 아래 post-fix 결과를 closure artifact에 기록하는 것뿐이며, 규범·제품 문서의 추가 변경은 요구하지 않았다.

## 6. Post-fix 재검토

| Reviewer | 확인 commit | 확인 범위 | verdict | 잔여 위험 |
|---|---|---|---|---|
| A | `ab613c8f70d6bb5658c20b662e152e61d421c239` | A findings·추가 관찰 + 전체 delta | `CONDITIONAL`; 모든 항목 `FIXED`, closure 기록만 요구 | Windows PowerShell·CodeGraph·SDK/HIL 미실행; [post-fix evidence](evidence/2026-09-05-document-information-architecture-post-fix-reviewer-a.md) |
| B | `ab613c8f70d6bb5658c20b662e152e61d421c239` | B findings + 전체 delta | `CONDITIONAL`; 모든 항목 `FIXED`, closure 기록만 요구 | runtime·hardware·외부 link 내용은 범위 밖; [post-fix evidence](evidence/2026-09-05-document-information-architecture-post-fix-reviewer-b.md) |

## 7. 최종 판정

- 최종 verdict: `PASS`
- 열린 finding: 0개. 최초 11개 finding과 추가 관찰 1개를 모두 `FIXED`로 닫았다.
- 판정 근거: 두 원 reviewer가 자신의 모든 finding 해소와 신규 `P0`/`P1` 회귀 없음에 동의했다. 두 `CONDITIONAL` verdict가 요구한 post-fix hash, disposition, 두 결과와 검증을 이 closure artifact에 기록했으므로 조건이 충족됐다.
- Coordinator 최종 검증: closure 포함 Markdown 89개 기준 local link 476개·fragment 11개 오류 0, 상세 task/요약 link 34/34, 이동 전 경로 잔존 0개, Windows Node `prototype.js --check`, Windows Git `diff --check` 통과.
- 미실행 검증: 실제 CodeGraph, SDK build, firmware runtime, CI, HIL·hardware·차량과 외부 link 내용 검증은 문서 전용 변경 범위 밖이다.
- PR 반영 위치: 현재 branch의 본 report와 PR 본문의 리뷰·검증 절

이 report는 이번 실행의 누적 기록이며 2026-09-04 기준선 report에 append하지 않는다.
