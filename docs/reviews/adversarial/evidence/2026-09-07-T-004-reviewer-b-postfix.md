# Reviewer B 수정 후 독립 적대적 재리뷰 원문

- Execution ID: `B-POSTFIX-REREVIEW-0e1a09f-20260907-0422`
- 대상 commit: `0e1a09fc7a8825f1fc46b9c177d69983c8d5c06e`
- Branch: `agent/codex-t004-uart-schema-codec`
- Parent: `fa6749a5a92042b7895575b25273ddd4256da274`
- 종료: `2026-09-07 04:30:22 KST`
- detached worktree clean, 원본 workspace clean, target HEAD 일치, review worktree 제거
- 다른 reviewer 보고서 및 `docs/reviews/` 결과는 읽지 않음
- 소스 수정 및 commit 없음

## 실행 검증

- generator `--check`, diff check, host Debug/Release build와 전체 CTest 각 65/65 PASS
- UART CTest 각 16/16 PASS
- Windows sanitizer는 project guard로 거부됨(PASS 아님)
- target toolchain/STM32Cube/ESP-IDF, target compile, HIL/실차/physical UART는 이 reviewer 환경에서 미실행

## 이전 finding 중 해결 확인

plan staging timeout, quota, revision/order, atomic commit, reserved bytes, CAN TX flags width, plan digest comparison, full command cache identity, terminal result overwrite, generated TTL bounds는 현재 코드와 경계시험에서 해결됐다. HELLO/ACK/CTS는 일부 해결됐으나 freshness와 dispatch 연결은 남았다.

## 최초 verdict

**BLOCK**

## Findings

### B-P1-01 — Command TTL이 실제 ACK/result 경로에 적용되지 않음

`COMMAND_REQUEST` TTL 범위만 검사되고 cache entry에 request deadline이 저장되지 않는다. `mark_ack()`는 `now_ms`를 받지 않고, `record_result()`와 `lookup()`은 고정 60초 retention만 검사한다. TTL 500 ms request의 501 ms ACK/result가 허용될 수 있다.

권고: entry/handle에 request deadline을 저장하고 `admit`, `mark_ack(now_ms)`, `record_result(now_ms)`, `lookup(now_ms)` 모두 검사하며 TTL 경계·TTL+1·retention+1 negative test를 추가한다.

### B-P1-02 — Direction metadata가 mandatory enforcement로 연결되지 않음

`message_direction_allowed()` matrix는 있으나 `message_validate()`/`message_encode()`는 endpoint/flow를 받지 않는다. 따라서 valid `COMMAND_REQUEST`가 반대 방향에서 validate/encode될 수 있다. helper-only test는 실제 dispatch enforcement를 증명하지 않는다.

권고: endpoint/flow를 요구하는 non-bypassable validate/dispatch API 또는 STM/ESP adapter의 강제 check를 만들고, 모든 policy 양방향 negative integration test와 target dispatcher 호출 evidence를 추가한다.

### B-P1-03 — CRC-valid request가 authentication/control-tag 검증 없이 admission context와 분리됨

`validate_command_request()`는 metadata/TTL 구조만 확인하고 control tag 또는 canonical digest의 인증·대조를 하지 않는다. `command_admission_allowed()`는 decoded request가 아닌 caller boolean만 받는다. CRC는 authentication이 아니다.

권고: exact canonical bytes와 identity/session/generation/token/command/TTL/state revision에 묶인 proof를 STM32 executor가 검증하도록 API를 분리하고 wrong tag/digest, expired, stale generation/revision negative test를 추가한다.

### B-P1-04 — Safety snapshot이 freshness/revision에 묶이지 않음

`note_safety_snapshot()`이 timestamp, safety revision, vehicle-state digest를 저장하지 않고 valid latch만 설정한다. heartbeat가 계속되면 stale snapshot이 current로 남는다.

권고: timestamp/peer boot/revision/digest를 저장하고 최대 age 및 command revision/digest를 admission/dequeue/TX 직전에 검사하며 revision change/reconnect/delayed snapshot 시험을 추가한다.

### B-P1-05 — 필수 build/target/HIL evidence 미완료

C direct long-stream은 72,000 frame이고 CMake Python fault stream은 3600 virtual seconds(1시간)이다. task gate의 24시간 equivalent, C implementation soak, target warning-free build, physical RTS/CTS와 HIL은 증명되지 않았다. Windows sanitizer는 project guard로 실행되지 않았다.

권고: C implementation deterministic 24-hour equivalent soak, pinned target warning-free build와 artifact, physical/HIL gate를 별도 evidence로 보존하고 미실행 gate는 `UNRUN`으로 유지한다.

### B-P2-01 — 동일 peer boot의 duplicate HELLO가 cache와 active plan을 삭제함

`session_note_hello()`가 같은 peer boot ID의 duplicate HELLO인지와 관계없이 cache와 plan을 reset한다. 이는 duplicate command 재실행 가능성과 availability 저하를 만들 수 있다.

권고: peer boot/session이 실제 변경된 경우에만 cache/plan을 reset하고 동일 boot HELLO 재전송 회귀시험을 추가한다.

### B-P3-01 — CMake 설명이 UART schema version과 불일치

CMake 주석이 protocol ABI를 `v1.3 schema`에서 생성한다고 쓰지만 UART schema는 `v1.0`이다. 서로 다른 protocol schema를 구분하도록 주석을 수정한다.

## 현재 정상 확인된 항목

schema/generated drift, packed ABI C11/C99, plan bounds/reserved/revision/digest, 256-slot cache quota와 terminal result immutability는 PASS였다. 이는 host evidence이며 target ABI/HIL evidence로 승격할 수 없다.

## 잔여 불확실성

target dispatcher가 direction/auth/safety 검사를 수행하는지, physical UART flow-control timing과 target compiler/ABI가 어떤지 확인되지 않았다. SHA-256 plan digest는 integrity/identity일 뿐 authentication/authorization이 아니다.
