# T-007 prefix·packager checkpoint 독립 적대적 리뷰

- Review ID: `T007-PREFIX-PACKAGER-20260919`
- 종류: 전문 리뷰어 2인 독립 object-only 정적 리뷰
- Base: `6cf1b8e57840a27b83c407d1325a92f869cf2f5d`
- Candidate: `63c87272fe9f2b00b76893055bfcc8a9ca71cf26`
- 범위: C 고정 prefix 조립, host detached P256 컨테이너 조립·검사, 관련 시험·문서
- 범위 밖: native packager/golden, 정상 target·Flash owner, hardware/HIL·차량 TX
- Task: [T-007](../../tasks/T-007-ota-container.md), 전체 `IN_PROGRESS`
- Coordinator: Codex
- 상태: 최초 리뷰 완료, P3 표현 수정·원 reviewer post-fix 확인 전

## 1. 동일 manifest와 독립 원본

두 reviewer에게 같은 base/candidate/scope/수용 기준과 실행 가능한 검증을 전달했다.
전체 요청은 각각의 evidence에 보존했다. 소스는 `git show <candidate>:<path>`와
`git diff <base> <candidate>`로만 읽었다. 두 원본 저장 전 finding을 공유하지 않았다.
리뷰어들은 compile/test를 실행하지 않았고 checkout clean을 주장하지 않았다.

| 항목 | A | B |
|---|---|---|
| 전문 범위 | C bounds·ownership·reset·인증 분리 | 암호·입력·출력·CI·근거 정합성 |
| Subagent ID | `01a0b784-5d6a-7540-97e8-ac159eb3a828` | `01a0b784-5e5f-7d42-8953-968159b85914` |
| 자체 발급 execution ID | `7d17f5f8-9f65-4f26-96e8-ea6a410ccc6f` | `13439260-779e-4310-b42e-df5ac25969c8` |
| 시작 UTC | 2026-09-19T02:35:17.157Z | 2026-09-19T02:35:30.9295493Z |
| 종료 UTC | 2026-09-19T02:37:58.420Z | 2026-09-19T02:38:37.4372250Z |
| 격리·hash | object-only, 위 base/candidate 확인 | object-only, 위 base/candidate 확인 |
| 원본 | [A](evidence/2026-09-19-T-007-prefix-packager-reviewer-a.md) | [B](evidence/2026-09-19-T-007-prefix-packager-reviewer-b.md) |
| 최초 verdict | 정적 PASS | CONDITIONAL |

## 2. Finding과 disposition

A는 확정 finding이 없었다. B의 finding은 다음 두 건이며 심각도를 변경하지 않았다.

| ID | 심각도 | Candidate 위치 | 실패·영향 | 권고와 반영 | 상태 |
|---|---|---|---|---|---|
| B-01 | P3 | `docs/resume.md:56`, `docs/tasks/T-007-ota-container.md:30` | 이전140/140을 현재 source 결과로 오인 | PR35 source12100ac의 과거 결과로 귀속하고 이번63c8727 결과142/142·137/137을 별도 기록 | 수정, 원 reviewer 확인 전 |
| B-02 | P3 | `tools/ota/container.py:42` | 입력 길이 제한을 전체 host RAM 보장으로 오인 | 단일 bytes 입력 상한으로 수정, 임시 복사·decoded 객체·provider의 peak RAM은 미보장 명시 | 수정, 원 reviewer 확인 전 |

신규 P0/P1/P2는 두 reviewer 모두 보고하지 않았다. 미실행 native/target/HIL을 PASS로
해석하지 않는다. 90개 CNG 사례 중45개만 collector 경로이며 독립 보안 속성90개가 아니다.

## 3. Coordinator 실행 검증

- Windows Debug142/142·Release142/142, 실제 P256/SHA-256 CNG 교차90개 통과.
- Linux native Git checkout `63c8727` ASan/UBSan137/137 통과.
- 새 prefix_feed coverage 행93.33%·분기96%, init/finish 행·분기100%.
  전체 envelope.c coverage 또는 MCU WCET·stack 실측이 아니다.
- 합성 T103 fixture의 source digest를 직접 재산출해5행·helper에 반영했다. 값은
  `8e128a30cfe2b153bcd0e1f17267078fe22dc12c2c4cbe374c85018bab325fe1`이다.
- 실패했던 최초 fixture mismatch, 실행 중 HEAD 변경, WSL Windows `.git` 경로 문제와
  재실행은 [journal](../../journal.md)에 기록했다. 실패를 PASS로 바꾸지 않았다.
- CI35416056122 host5개 job PASS, target job 진행 중. Artifact/warning 최종 감사 전.
- 표현 수정 후 `test_container.py`4개 그룹/90교차, 문서 링크336개 문서·1372target,
  plan49개 검사 통과. 기존 암호/C 실행 코드는 변경하지 않았다.

## 4. 최초 판정과 후속

현재 구현분 판정은 A 정적 PASS/B CONDITIONAL을 그대로 유지한다. P3 두 건의 수정
commit을 같은 reviewer에게 재검토시킨 뒤 새 post-fix report를 만든다. 전체 T-007의
native signing/golden·정상 target 연결 및 최종 리뷰·CI gate는 아직 남아 있다.
Physical/HIL `NOT_RUN`, 차량 CAN TX `NO-GO`, PR36 `DRAFT`를 유지한다.
