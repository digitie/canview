# 2026-09-19 T-007 receiver oracle post-fix 리뷰

- Review ID: T-007-receiver-post
- Base: `0acdc453cd8503631ee179c3bd8b49a1e30fce1f`
- Candidate: `c7f5780b48810b3e2f1ba1bf4fc0792c013b0d62`
- 범위: B-RX-01 P2 수정·C mutant·CTest·문서 delta
- 범위 밖: 이후 native CLI, 전체 T-007 완료, SDK device/Flash/HIL
- Coordinator: Codex
- 상태: A/B 정적 PASS, B-RX-01 FIXED

## 독립 실행

| 항목 | 원 A / Maxwell | 원 B / Huygens |
|---|---|---|
| Agent | 01a0b784-5d6a-7540-97e8-ac159eb3a828 | 01a0b784-5e5f-7d42-8953-968159b85914 |
| Execution UUID | 88ac26fc-3a43-4868-93f5-3f03e5d6a374 | 622b300d-d9a2-4df8-b5dc-ca489228dca1 |
| 시작 UTC | 2026-09-19T04:47:48.3049052Z | 2026-09-19T04:47:48.7336574Z |
| 종료 UTC | 2026-09-19T04:49:03.9761866Z | 2026-09-19T04:49:47.3540213Z |
| 전문 | C runtime·cleanup·false PASS | oracle·CTest/CI·근거 |
| 원문 | [A](evidence/2026-09-19-T-007-receiver-post-a.md) | [B](evidence/2026-09-19-T-007-receiver-post-b.md) |
| Verdict | PASS | PASS |

양쪽은 시작·종료 commit object를 확인하고 git show/diff만 읽었다. 서로의 새 finding을
보지 않았고 원문을 각각 보존한 뒤 비교했다. 실제 읽은 파일·명령·미검토 범위는 원문에 있다.
독립 compile/test/CI artifact 확인은 NOT_RUN이며 작성자 실행과 구분한다.

## 동일 request

```text
T-007 receiver B-RX-01 P2 post-fix 원 A/B 재검토. 이전 양쪽 raw는 그대로 보존 완료. Repo F:/dev/canview-wt/t007-ota-container; base 0acdc453cd8503631ee179c3bd8b49a1e30fce1f; candidate c7f5780b48810b3e2f1ba1bf4fc0792c013b0d62. 새executionUUID/시작종료시각, object-only cat-file/rev-parse gitshow/diff. 서로새finding공유금지. 파일수정/compile/다른agent금지.
대상 receiver.c의body_tamper_rejected 및offset==size, tests/ota/check_receiver_oracle.py actualmutant, rootCTest등록, 관련README/resume/task/journal. A runtime/cleanup/falsePASS, B oracle/CI/근거. B-RX-01은 조기body_open/hash AUTH_FAILED를 마지막byte변이성공으로오인하는 문제. 수정은변이feed자체AUTH_FAILED와최종offset을요구한다. 새로운독립C컴파일3개: early-open→exit1,early-feed→exit1,같은earlyfeed+이전status-onlyoracle→exit0대조. compileexception/timeoutexit을PASS로받지않음.
작성자실행: WindowsDebugRelease146/146; CNG변이3개expectedexit; SDK실제ELF/MAP/BIN경고0 metadata168변이4절단; receiverframe624B. SDKdevice/nativeFlash/총stackheap/timingNOT_RUN. CI0ac는Doxygen다운로드실패, c7f신규CI진행. 모형을actualdevice로주장않음.
핵심scope전체읽고 B-RX-01 FIXED여부 및delta회귀 file:line evidence P0..P3 failure/impact/recommendation, 실제읽은filescommands/unread범위/verdictraw필수. 전체acceptanceBLOCK은유지하며이제한수정검토로전체taskPASS하지말것. newframework추가없음.
```

## Disposition

B-RX-01 P2는 원 B와 A가 모두 FIXED로 확인했다. 마지막 변이 feed의 AUTH_FAILED와
최종 offset을 요구해 조기 open/feed 오류를 성공으로 집계하지 않는다. 지역 flag 초기화,
cleanup 순서·static context 수명도 유지한다. 신규 P0/P1/P2/P3는 양쪽 모두 없다.

작성자 Debug/Release146/146과 실제 CNG mutant3건이 통과했다. 조기 open/feed는 exit1,
같은 early-feed를 이전 status-only oracle로 검사하면 exit0다. SDK 재빌드/metadata172개
음성 사례/경고0, receiver frame624B를 확인했다. Mutant는 caller status 주입이며
PSA 내부 고장 주입이나 장치 실행 증거가 아니다.

- SDK fixture BIN SHA256: `f861ce08c6df92fdef7325e46991193734eba8839021236fafa72b15f3e04344`
- Oracle 실행 로그 SHA256: `ae7dc95c5773c5f75eb6c9acc3ae18b77c971065cc2ee3c984b9ac8dc01ddc85`

전체 acceptance BLOCK, PR36 Draft, physical/HIL·실제 PSA·총 stack/heap/timing·Flash
NOT_RUN, 차량 TX NO-GO를 유지한다. 이후 native CLI나 전체 Task/merge 승인이 아니다.
