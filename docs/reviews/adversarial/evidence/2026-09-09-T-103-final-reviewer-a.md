CV-HOSTILE-20260909-T103-04A1-01

검토 시작: 2026-09-08T23:14:14.5117120Z / 2026-09-09T08:14:14.5117120 KST  
검토 종료: 2026-09-08T23:24:06.8018540Z / 2026-09-09T08:24:06.8018540 KST  
Candidate: `04a1e9367138498bb03906befac2f904f2f943dd`  
Base: `6db3998092812354afe3b3918892b682022990b4`  
Isolation: candidate/base Git object만 `git show`, `git diff`, `git cat-file`로 읽음. 작업 트리 수정, commit, push, worktree 생성 없음. 다른 reviewer report는 읽지 않음.

검토 파일: `AGENTS.md`, `docs/README.md`, `docs/resume.md`, T-103 task, architecture/hardware 문서, STM32 FDCAN module/platform/header/source, board pin/safe GPIO/core HW, CMake 파일, STM32 unit/fake tests, HIL runner/evidence/helper/fixture 파일, `tools/check_stm32_coverage.py`, `hardware/communicator/pinmap.csv`.

실행 명령: `git cat-file -t`, `git show`, `git diff --stat`, `git diff --name-status`, `git diff --check`, `git diff --numstat`, `git status --short --branch`, `git rev-parse HEAD`, `git grep` 기반의 읽기 전용 검사와 PowerShell timestamp/주소 산술 검사. 공식 하드웨어 근거 확인을 위해 ST RM0440 공식 PDF를 조회함.

검증 결과:

- `git diff --check`: 통과.
- 빌드/unit test/HIL/실차 검증: 실행하지 않음.
- physical/HIL: `NOT_RUN`.
- 차량 CAN TX: `NO-GO`.

P0 findings

없음.

P1 findings

P1-01 — FDCAN RX FIFO가 실제로 설정되지 않음

위치: `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:192-216`

`configure_instance()`가 `CCCR`, `NBTP`, `DBTP`, `TSCC`, `RXGFC`, `IR`, `IE`, `ILS`, `ILE`만 설정하고 `RXF0C` 및 관련 Message RAM element layout을 설정하지 않는다. FDCAN FIFO element count/base가 활성화되지 않으면 FIFO0 수신이 동작하지 않는다.

영향: capture-only 수신 경로가 무수신 또는 잘못된 FIFO 상태가 된다.

권고: 생성된 profile/layout에서 `RXF0C`, `SIDFC`, `XIDFC` 등 필요한 Message RAM 설정을 `INIT|CCE` 구간에 명시하고, 실제 레지스터 값과 주소 단위에 대한 golden test를 추가할 것.

P1-02 — Message RAM 주소 단위가 word/byte로 혼동됨

위치: `platform/stm32g474/fdcan_capture.c:15-24, 401-468`; `docs/fdcan-capture.md:62-69`

코드는 `848`, `176`, `72`를 byte offset처럼 포인터에 직접 더한다. 그러나 STM32G474 RM0440은 FDCAN Message RAM layout/address를 32-bit word 기준으로 정의한다. 따라서 기대되는 byte 값은 FIFO offset `704`, element stride `288`, instance stride `3392`이다. 현재 구현은 실제 RAM 위치를 잘못 읽고 FDCAN instance 간 영역도 겹칠 수 있다.

근거: [ST RM0440 공식 Reference Manual](https://www.st.com/resource/en/reference_manual/dm00355726.pdf)

영향: 잘못된 frame data/timestamp를 읽거나 채널 간 Message RAM을 오염시킨다. Fake hardware도 동일한 잘못된 byte 계산을 모사하므로 현재 테스트가 이 결함을 검출하지 못한다.

권고: word-addressed `uint32_t*` 접근으로 통일하거나 모든 layout offset을 명시적으로 byte 변환하고, 실제 RM layout 기반 multi-instance address test를 추가할 것.

P1-03 — 지원하지 않는 CAN FD frame이 raw ring head에서 영구 재시도됨

위치: `module/fdcan_capture.c:192-205, 587-592`; `platform/stm32g474/fdcan_capture.c:491-531`

module은 FDF frame에 `CANVIEW_UNSUPPORTED_MESSAGE`를 반환한다. platform service는 OK가 아닌 모든 sink 결과를 retryable로 취급하고 raw element를 release하지 않는다.

재현: 첫 FDF frame 수신 → sink가 `UNSUPPORTED_MESSAGE` 반환 → raw ring head 유지 → 이후 frame이 계속 같은 element에서 막힘.

영향: 해당 채널의 capture worker가 영구 정지하고 raw ring이 채워진 뒤 overflow/drop으로 진행된다.

권고: `UNSUPPORTED_MESSAGE`와 malformed frame은 terminal-consumed 결과로 분류하여 release하고, 실제 재시도 대상은 명시적 transient/resource 오류로 제한할 것.

P1-04 — stale status timestamp가 최신 상태를 역행시킬 수 있음

위치: `module/fdcan_capture.c:1018-1073`; API 계약 `interface/canview_stm_fdcan_capture.h:315-325`

`set_status()`는 timestamp가 이전 값보다 작아도 거부하지 않고 channel state와 error를 덮어쓴다. global extended timestamp만 전진하고 stale status 자체는 적용된다.

재현: `BUS_OFF @ 1000` 이후 지연된 `ERROR_ACTIVE @ 900` 도착.

영향: bus-off/fault 상태가 active 상태로 회귀하여 안전 상태와 진단 통계가 손실된다.

권고: status timestamp를 별도로 monotonic 검증하고 stale snapshot은 거부할 것. terminal state는 더 오래된 status가 덮어쓰지 못하게 할 것.

P1-05 — EventLog/report identity가 candidate Git object에 고정되지 않음

위치: `tests/hil/run.py:39-81`; `tests/hil/events.py:72-87`; `tests/hil/assert_no_tx.py:210-216`; `tests/hil/validate_evidence.py:159-204`

runner는 현재 작업 트리의 `HEAD`와 mutable filesystem hash를 사용한다. dirty tree 여부나 검토 대상 candidate `04a1e936...`와의 일치 여부를 강제하지 않는다. `assert_no_tx.py`도 expected identity가 단순히 비어 있지 않고 길이 제한을 만족하는지만 검사하며 SHA-256 형식을 요구하지 않는다.

영향: 다른 commit 또는 수정된 작업 트리에서 생성한 no-TX evidence가 내부적으로 self-consistent하다는 이유만으로 통과할 수 있다. 현재 `run_can_capture.py:45-66` 검사는 candidate immutability가 아니라 내부 필드 일관성만 확인한다.

권고: runner가 기대 candidate revision/tree hash를 필수 입력으로 받고 Git object 또는 pinned checkout에서 digest를 계산할 것. EventLog/report에 candidate commit과 64-hex `source_sha256`를 기록하고, standalone gate에서도 형식 및 expected candidate 일치를 강제할 것.

P2 findings

P2-01 — timestamp half-range ambiguity 처리 오류

위치: `module/fdcan_capture.c:245-253, 277-305`

주석은 정확히 half-range인 `0x80000000`을 invalid tie로 설명하지만 조건은 `> 0x7fffffff`이다. 그 결과 backward half-range는 wrap으로 수용되고 forward half-range는 다른 경로에서 malformed가 되는 비대칭이 발생한다.

권고: `distance == UINT32_C(0x80000000)`를 wrap 판단 전에 명시적으로 reject할 것. 해당 경계값 golden test를 추가할 것.

P2-02 — sticky/reset 회귀시험이 raw-ring-overflow 및 session reset을 완전히 검증하지 않음

위치: `platform/stm32g474/fdcan_capture.c:77-98, 557-607, 622-698`; `tests/stm32/test_fdcan_platform.c:438-445, 476-507`; `tests/stm32/test_fdcan_capture.c:820-850, 1002-1014`

코드상 `fifo_loss_unknown`, `raw_ring_overflow`, `message_ram_fault`가 `session_fault_flags`로 누적되고 start/stop reset에서 초기화되는 경로는 확인된다. 다만 테스트는 FIFO-loss/MRAF sticky와 일부 module reset을 확인할 뿐, raw-ring-overflow와 platform stop/start 후 모든 sticky flag가 0인지 직접 assertion하지 않는다.

권고: 각 flag별 `set → service → repeated service remains sticky → stop/start reset clears` 시험을 추가할 것.

P3 findings

없음.

Sticky fault 검증

- FIFO loss: IRQ에서 capture하고 service 시 session sticky로 누적됨.
- raw ring overflow: `drain_fifo()`에서 `raw_ring_overflow`와 drop counter를 설정함.
- Message RAM fault: `message_ram_fault`와 MRAF pending을 누적함.
- reset clearing: platform `reset_runtime_state()` 및 module `capture_reset()`에서 관련 flag/counter/ring/timestamp를 초기화함.
- 단, 위 P2처럼 raw-ring-overflow reset의 직접적인 회귀 assertion은 부족함.

Reviewed-not-found

- 검토 범위의 FDCAN 경로에서 raw CAN TX register/API, sender callback, TX queue는 발견하지 못함.
- `CCCR.MON`과 `CCCR.DAR` 설정 및 TX request GPIO safe-high 경로를 확인함.
- ISR는 최대 3개 FIFO element만 drain하고 callback/printf/malloc/blocking을 수행하지 않음.
- service worker와 module batch는 각각 bounded loop와 reentry guard를 가짐.
- board pin source contract는 문서와 candidate header에서 CAN1 `PA11/PA12`, CAN2 `PB12/PB13`, CAN3 `PA8/PA15`와 AF 설정이 일치함.
- 외부 pull/gate 및 watchdog/reset 근거는 문서와 source에 있으나 실제 전원·brownout·reset·PHY 시험은 확인하지 못함.
- Communicator/Diagnostic Bridge에 control lease 또는 raw replay/TX 권한을 추가한 변경은 검토 범위에서 발견하지 못함.
- 실제 FreeRTOS task wiring, ST-LINK flash, clock/bitrate, IRQ latency, PHY, ACK, reset/brownout, vehicle bus isolation은 검증하지 않음.

최종 verdict: BLOCK

P1-01~P1-03은 실제 STM32 수신 경로 자체를 무효화하거나 영구 정지시킬 수 있고, P1-05는 no-TX evidence의 provenance를 candidate에 고정하지 못한다. 이 finding들이 수정되고 golden vector, malformed/unsupported consume, sticky reset, candidate-bound evidence 검증 및 physical/HIL gate가 다시 수행되기 전에는 T-103을 PASS 또는 release 가능으로 표시할 수 없다.
