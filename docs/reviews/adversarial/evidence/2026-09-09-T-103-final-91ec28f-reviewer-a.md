## T-103 적대적 재검토 원시 보고서

- 실행 ID: `CV-HOSTILE-20260909-T103-91EC-03`
- 후보: `91ec28f550f18575f22a205155cffe2cbf18422d`
- 기준: `6db3998092812354afe3b3918892b682022990b4`
- 시작: `2026-09-09T00:33:33.7579608Z` / KST `2026-09-09T09:33:33.7579608`
- 종료: `2026-09-09T00:38:52.1513162Z` / KST `2026-09-09T09:38:52.1513162`
- 객체 검증: PASS
- diff: 49 files, `+7048/-48`
- `git diff --check`: PASS
- 작업 트리: 사용자 변경 포함 dirty 상태 보존, 수정 없음

검토 결과:

- P0: 없음
- P1: 없음
- P2: 없음
- P3: 없음

확인된 후보 수정:

- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:491-531`
  - `RESOURCE_BUSY`와 `TIMEOUT`만 raw slot을 보류
  - 그 외 sink 오류는 terminal 소비 후 ring 진행
  - `tests/stm32/test_fdcan_platform.c:560-595` 회귀 테스트 확인
- HIL provenance 고정:
  - `tests/hil/run_can_capture.py:27-89`
  - 후보 commit, firmware SHA-256, harness SHA-256를 모두 요구·검증
  - `tests/test_t103_capture_helpers.py:45-63,231-255` 실패 경로 확인
- timestamp half-range, stale status, sticky fault/reset, STM32G4 Message RAM layout 관련 기존 문제는 후보 코드와 테스트에서 보강됨.
- STM32 production source에서 CAN TX primitive는 확인되지 않음. 검색 결과는 테스트 fixture·validator·protocol audit 명칭뿐임.

차단 사유:

- 지정된 artifact가 후보 객체에 없습니다.

  `docs/reviews/adversarial/evidence/2026-09-09-T-103-target-91ec28f.md:0` — object에 없음

- 존재하는 `2026-09-09-T-103-target.md`는 지시에 따라 대체해서 읽지 않았습니다.
- 지정된 `firmware/communicator/stm32/bsp/runtime.c`도 없으며, 실제 후보 경로는 `firmware/communicator/stm32/app/boot.c`입니다.
- 대상 후보를 checkout하지 않았으므로 host CTest와 target build는 재실행하지 않았습니다. 정적 object 검사만 수행했습니다.

물리/HIL gate: `NOT_RUN`  
CAN TX: `NO-GO`

최종 판정: **BLOCK / review incomplete**

코드상 P0/P1은 발견하지 못했지만, 요청된 target evidence 파일이 없어 지시된 기준상 PASS 또는 완료 판정을 낼 수 없습니다.
