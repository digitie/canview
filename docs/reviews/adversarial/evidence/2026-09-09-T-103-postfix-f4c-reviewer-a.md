CV-HOSTILE-20260909-T103-F4C4-02

검토 시작: `2026-09-08T23:54:07.1726101Z` / `2026-09-09T08:54:07.1726101 KST`  
검토 종료: `2026-09-08T23:59:50.2206886Z` / `2026-09-09T08:59:50.2206886 KST`  
Candidate: `f4c413a153dcf2ff5c771c457275e3b4af25f385`  
Base: `6db3998092812354afe3b3918892b682022990b4`

전문 범위: STM32 FDCAN capture-only runtime, ISR/worker ownership·reentry, RTOS 경계, watchdog/reset/brownout, bounds·Message RAM, GPIO/electrical contract, fail-safe, Communicator/Bridge isolation, no-TX evidence provenance.

격리: candidate/base를 Git object 명령으로만 읽었다. 현재 작업 트리 HEAD는 `10716b12a19b7244982d6e1572f1fad81057d348`이며 dirty 상태였지만, 코드 검토에는 사용하지 않았다. 수정·commit·push·worktree 생성 없음. 다른 reviewer report는 읽지 않았다.

읽은 파일:

- `AGENTS.md`, `docs/README.md`, `docs/resume.md`
- `docs/tasks/T-103-stm32-fdcan-capture.md`
- `docs/architecture/README.md`, `docs/architecture/firmware-foundation.md`
- `docs/hardware/r1/firmware-pinmap.md`
- `firmware/communicator/stm32/README.md`
- `firmware/communicator/stm32/docs/fdcan-capture.md`
- `firmware/communicator/stm32/module/fdcan_capture.c`
- `firmware/communicator/stm32/interface/canview_stm_fdcan_capture.h`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.h`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_message_ram.h`
- `firmware/communicator/stm32/platform/stm32g474/{core_hw.c,core_hw.h,safe_gpio.c,safe_gpio.h}`
- `firmware/communicator/stm32/bsp/board_pins.h`
- STM32/module CMake 파일
- `tests/stm32/test_fdcan_capture.c`
- `tests/stm32/test_fdcan_platform.c`
- `tests/test_hil_runner.py`
- `tests/test_t103_capture_helpers.py`
- `tests/test_stm32_fdcan_layout.py`
- `tests/hil/{run.py,events.py,assert_no_tx.py,validate_evidence.py,run_can_capture.py}`
- `tools/check_stm32_fdcan_layout.py`

실행 명령:

- `git -c safe.directory=F:/dev/canview cat-file -t`
- `git ... rev-parse`
- `git ... show --no-patch`
- `git ... diff --stat`, `--name-status`, `--unified`
- `git ... show` line-numbered source extraction
- `git ... grep` for timestamp, sticky flags, TX symbols, build mode, identity
- `git ... status --short --branch`
- `git ... diff --check`
- PowerShell timestamp 및 Message RAM byte 계산
- ST 공식 RM0440 검색: [RM0440 공식 문서](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- 한 번의 PowerShell line-filter 명령은 구문 오류로 실패했으며 파일 변경은 없었다.

제공된 target build evidence는 STM32CubeG4 v1.6.3, Debug/Release, layout checker, warning/error scan 통과로 기록되어 있으나 이 실행에서 재실행하지 않았다.

P0 findings

없음.

P1 findings

P1-01 — unsupported CAN FD frame이 raw slot을 영구 점유함

위치: `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:491-528`; `firmware/communicator/stm32/module/fdcan_capture.c:198-205,607-615`

`decode_element()`은 FD frame을 정상 decode한 뒤 `frame_sink()`를 호출한다. module의 `capture_ingest()`는 `CANVIEW_UNSUPPORTED_MESSAGE`를 반환하지만 platform은 모든 non-OK sink 결과에서 raw slot을 release하지 않고 즉시 반환한다.

재현:

1. 첫 번째 FIFO element가 FDF frame.
2. `frame_sink()`가 `CANVIEW_UNSUPPORTED_MESSAGE` 반환.
3. `raw_read_index`가 유지됨.
4. 다음 `service()`도 동일 frame을 재처리.
5. 이후 frame이 채널 raw ring에 누적되어 overflow/drop.

영향: 실제 CAN FD frame 하나만으로 해당 채널 worker가 영구 정지한다.

권고: `UNSUPPORTED_MESSAGE`와 malformed frame은 terminal-consumed 결과로 분류해 release하고, retry는 명시적 transient/resource 오류에만 허용할 것. 현재 platform test에는 FD frame을 실제 adapter 경로로 주입하는 회귀시험이 없다.

P1-02 — T-103 capture wrapper가 firmware identity는 고정하지만 harness identity는 candidate에 고정하지 않음

위치: `tests/hil/run_can_capture.py:28-31,60-79`; `tests/hil/run.py:84-89`; `tests/hil/validate_evidence.py:191-202`

`run_can_capture.py`는 expected commit와 firmware source SHA를 검증한다. 그러나 HIL runner 자체의 `harness.source_sha256`는 현재 작업 트리에서 계산되며, expected harness digest와 비교되지 않는다. `validate_evidence.py`도 현재 checkout의 harness hash를 다시 계산할 뿐이다.

재현: candidate firmware source는 유지한 채 `tests/hil` runner를 수정하고, 수정된 runner가 자체 harness digest를 report에 기록하면 현재 validator는 내부적으로 일치하는 evidence로 수용할 수 있다.

영향: no-TX evidence가 검토 대상 candidate의 동일한 HIL 실행기에서 생성되었다는 provenance가 보장되지 않는다.

권고: `run_can_capture.py`에 expected harness SHA를 필수화하고, candidate Git object 또는 pinned checkout에서 firmware·harness digest를 함께 계산해 EventLog/report/validator가 모두 비교하도록 할 것.

P2 findings

P2-01 — 공용 `assert_no_tx`/EventLog identity가 SHA-256 형식을 자체 강제하지 않음

위치: `tests/hil/assert_no_tx.py:210-216`; `tests/hil/events.py:75-83`

직접 CLI/helper 호출에서는 임의의 non-empty bounded string을 firmware identity로 사용할 수 있다. T-103 wrapper가 64-hex firmware SHA를 별도로 검사하므로 wrapper 경로의 위험은 완화됐지만, helper 자체는 source SHA-bound contract를 보장하지 않는다.

권고: 공용 identity 검증에도 64-hex SHA-256 형식과 필요 시 expected candidate commit 관계를 강제할 것.

이전 finding 재평가

- Message RAM에 `RXF0C` 미설정: CLOSED. STM32G474는 고정 Message RAM integration이며 CMSIS에 `RXF0C/RXESC`가 없다. `fdcan_message_ram.h:5-9,23-36`과 layout checker가 vendor HAL 매크로를 비교한다.
- Message RAM word/byte 주소 오류: CLOSED. 후보는 STM32CubeG4 고정 layout의 byte 값 `instance=848`, `FIFO0=176`, `element=72`를 사용한다. RM0440은 hardware address가 32-bit word 기반임을 설명하지만 HAL 상수는 byte size/offset으로 변환된 값이다.
- stale status 역행: CLOSED. `module/fdcan_capture.c:1057-1073`에서 stale status를 `CANVIEW_STALE`로 거부하고 최신 BUS_OFF 상태를 보존한다. 회귀시험 `tests/stm32/test_fdcan_capture.c:840-860` 확인.
- timestamp half-range ambiguity: CLOSED. `module/fdcan_capture.c:245-247,294-315`에서 정확히 `0x80000000`을 거부하고, 회귀시험 `tests/stm32/test_fdcan_capture.c:490-508` 확인.
- raw-slot release: PARTIAL. decode 오류는 `platform/.../fdcan_capture.c:505-512`에서 release하지만, sink가 반환하는 deterministic unsupported 오류는 아직 release하지 않는다.
- sticky/reset: CLOSED BY SOURCE AND TESTS. `platform/.../fdcan_capture.c:74-95`와 `652-666`에서 FIFO loss/raw overflow/Message RAM fault를 session sticky로 누적·reset한다. `tests/stm32/test_fdcan_platform.c:456-484,503-526`에서 sticky와 stop reset을 확인한다. module reset도 `module/fdcan_capture.c:546-590` 및 `tests/stm32/test_fdcan_capture.c:1020-1081`에서 확인한다.
- firmware EventLog identity: T-103 wrapper 기준 PARTIAL/CLOSED. `run_can_capture.py`가 expected commit와 firmware SHA를 검증하지만, 위 P1-02처럼 harness identity가 남은 gap이다.

Reviewed-not-found

- 검토 범위에 CAN TX register/API, TX callback, transmit queue 또는 raw replay 경로 없음.
- FDCAN `MON`/`DAR`, TX request GPIO high-safe 경로 확인.
- ISR는 최대 3개 FIFO element만 다루고 callback, decode, printf, malloc, blocking을 수행하지 않음.
- worker는 bounded raw-ring service 및 module batch/reentry guard를 사용함.
- board pin source contract와 문서의 CAN1/2/3 pin·AF 매핑은 일치함.
- session fault flags는 worker critical section에서 snapshot/clear되고 session flag는 stop/start까지 유지됨.
- 실제 FreeRTOS task wiring은 아직 T-104 범위이며, 현재 기본 STM32 app은 adapter를 시작하지 않음.
- watchdog, reset, brownout external pull/gate, PHY electrical behavior의 source 계약은 있으나 physical proof는 없음.
- Diagnostic Bridge에 control lease/raw replay/CAN TX 권한을 추가한 변경은 검토 범위에서 발견하지 못함.

물리/HIL gate

`NOT_RUN`:

- STM32 flash/ST-LINK
- 실제 FDCAN clock/bitrate와 GPIO AF
- TCAN/MAX3055 standby/receive 전기 상태
- reset/brownout/rail gate
- IRQ latency 및 실제 bus-load saturation
- CAN analyzer의 ACK/data TX 0건
- 차량 connector/profile/evidence

최종 verdict: BLOCK

P1-01은 단일 unsupported FD frame으로 capture worker를 영구 정지시킬 수 있다. P1-02는 candidate-bound no-TX evidence provenance를 완전히 보장하지 않는다. 두 finding 수정 후 unsupported consume 회귀시험, harness identity pinning, target build 재검증 및 physical/HIL gate를 수행해야 한다.
