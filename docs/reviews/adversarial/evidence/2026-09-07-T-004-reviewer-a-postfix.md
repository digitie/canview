# Reviewer A 수정 후 독립 적대적 재리뷰 원문

- Execution ID: `reviewer-A-rereview-20260907-0e1a09f`
- 전문 영역: 차량 안전·hardware·embedded runtime
- 대상 branch: `agent/codex-t004-uart-schema-codec`
- 실제 commit: `0e1a09fc7a8825f1fc46b9c177d69983c8d5c06e`
- Parent: `fa6749a5a92042b7895575b25273ddd4256da274`
- 종료: `2026-09-07T04:29:14.0363245+09:00`
- 격리: detached worktree `C:\Users\digit\AppData\Local\Temp\canview-t004-review-A-0e1a09f`
- 시작/종료 clean, 대상 HEAD 일치, `git diff --check` PASS, review worktree 제거 완료
- 다른 reviewer 보고서·finding은 열람하지 않음
- 파일 수정 및 commit 없음

## 실행 검증

- generator `--check`, schema tests, negative fixtures, UART virtual fault stream PASS
- host CMake/CTest, sanitizer, STM32/ESP32 target compile은 이 reviewer 환경에서 미실행
- 실제 4 Mbps, RTS/CTS, DMA/ISR, reset/brownout, hardware TX gate, CAN/HIL 미실행

## 최초 verdict

**BLOCK**

## Findings

### A-P1-01 — periodic `link_tick()` 누락 시 late heartbeat가 offline 경계를 우회

`note_heartbeat()`가 이전 heartbeat의 만료 여부를 검사하기 전에 timestamp를 갱신한다. timer task가 지연되거나 delayed heartbeat가 도착하면 새 HELLO/ACK/safety snapshot 없이 link가 재개될 수 있다. CTS도 fresh heartbeat 상태에서 blocked duration을 놓칠 수 있다.

권고: 갱신 전에 이전 timestamp와 CTS duration을 검사하고 threshold 초과 시 `link_require_hello()`로 닫으며, delayed heartbeat·timer tick 누락·CTS unblock-after-1s 회귀시험을 추가한다.

### A-P1-02 — OFFLINE 전환이 command cache와 pending plan을 원자적으로 폐기하지 않음

`link_require_hello()`는 link flag만 지우며 heartbeat timeout 또는 CTS offline만으로 cache/plan을 폐기하지 않는다. cache lookup과 pending plan이 stale 상태로 남아 재협상 후 재사용될 수 있다.

권고: timeout/CTS offline을 cache, pending plan, lease, command queue와 묶는 `session_tick` 또는 동등한 원자 facade를 추가하고 offline transition과 stale state 폐기를 함께 검증한다.

### A-P1-03 — 인증·local safety·hardware gate가 caller-supplied boolean에 남음

구조 검사는 nonzero metadata와 길이만 확인한다. valid CRC와 nonzero metadata를 가진 request의 zero digest/control tag도 wire validation을 통과할 수 있고, admission helper의 `authenticated`, `control_lease_valid`, `hardware_tx_gate_ready` 등은 caller가 제공한다.

권고: STM32 target dispatcher가 control-root tag, identity, session/generation, lease, TTL, profile, state revision, bus 상태와 command precondition을 직접 검증하고 admission/dequeue/각 CAN TX 직전에 local snapshot을 재검사한다. boolean wrapper와 CRC 성공만으로 closure하지 않는다.

### A-P1-04 — UART sequence anti-replay와 single-owner dispatch가 codec에 연결되지 않음

동일 CRC-valid frame을 같은 sequence로 두 번 feed하면 codec은 두 번 semantic `OK`를 반환한다. `canview_sequence_window_accept()`가 UART session/dispatch에 연결되지 않았고, command token만으로는 control-class 전체 replay를 막지 못한다.

권고: 인증 후 queue admission 전에 session별 sequence window를 단일 worker가 commit하고 duplicate/stale/wrap/half-range 및 ISR/DMA worker ownership 시험을 추가한다.

### A-P1-GATE-01 — physical RTS/CTS, DMA/ISR, hardware TX gate는 NOT_RUN

실제 CTS stuck/RTS reset level, DMA ring·ISR handoff, parser ISR 호출 여부, reset/brownout external gate, rail-good/watchdog/TX_ARM/CAN PHY fault, 4 Mbps deadline과 bus interference를 검증하지 못했다. T-004 범위 밖이며 후속 target/bench/HIL evidence가 필요하다.

### A-P1-GATE-02 — safety snapshot freshness와 state revision이 link state에 묶이지 않음

`safety_snapshot_valid`가 heartbeat 동안 무기한 유지되고 snapshot age/revision mismatch를 검사하지 않는다. safety revision/state가 바뀌어도 caller가 invalidate하지 않으면 stale vehicle state로 admission이 가능하다.

권고: STM32 local owner가 revision/age를 보유하고 변경 시 invalidate하며 admission/dequeue/각 TX 직전에 재확인하고 stale snapshot 회귀시험을 추가한다.

### A-P2-01 — target compiler, RAM/stack, WCET evidence 없음

command cache와 plan/codec/SHA context의 target `sizeof`, linker map, stack watermark, WCET가 검증되지 않았다. target compile 및 map evidence가 필요하다.

### A-P2-02 — quota bounds는 있으나 operational queue budget은 증명되지 않음

schema/codec quota는 고정됐지만 observer queue reserve, token bucket, worker-slot reservation과 queue high-water/control latency evidence는 없다. target router/queue 후속 범위로 남는다.

## 확인된 수정 사항

command-cache generation, plan digest, observer quota bounds, direction policy/helper, new boot reset, plan staging timeout은 현재 코드와 host test에서 개선됐다. 이는 physical target gate 또는 local STM safety executor 통과를 의미하지 않는다.

## 잔여 불확실성

target dispatcher의 auth/lease/profile/local safety, link tick ownership, timeout 시 queue/cache/plan/lease 폐기 caller, target ABI/stack/DMA/ISR/RTS-CTS/external gate는 확인되지 않았다.
