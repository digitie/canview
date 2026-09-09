Execution ID: **T104-RA-20260909-05**

Start: **2026-09-09 16:20:28.9659917 KST**  
End: **2026-09-09 16:28:52.9506073 KST**  
Timezone: **Asia/Seoul, UTC+09:00**  
Specialty: **embedded runtime/safety — independent reviewer A**

Candidate: `bdc67981a67d059518e9122f2c4c3eef3c5a4825`  
Base: `b17bdfc0bb2a1bfa9d300c1e7662cac05c96df40`

Isolation: `F:/dev/canview-wt/2026-09-09-t104-postfix-review-a`. HEAD matched at initial and final verification. Status reported no changes; Git warned that the user’s global ignore file was inaccessible. No repository files were modified, committed or pushed. Scratch builds and probes were confined to this task’s `work/` directory. No subagents were used. Reviewer B’s report was not separately opened or used. No further tooling was performed after the stop instruction.

**Unresolved findings: P0: 0; P1: 1; P2: 2; P3: 0.**

**RA-01 — P1: Previous DMA completion can complete a replacement transfer**

Primary location: [uart_dma.c:291](F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/platform/stm32g474/uart_dma.c:291).

Related exact lines in that file:

- 592: worker snapshots and clears pending events.
- 678: RX processing can trigger session recovery.
- 831: completion ISR sets `tx_done_pending`.
- 291–294: abort clears hardware flags and `tx_in_flight`, but not the software completion latch.
- 709: worker starts a replacement transfer.
- 661–667: a later worker applies the stale completion to the replacement transfer.

Failure scenario: after the worker takes events, the previous TX completes during RX processing immediately before a HELLO-triggered reset. Its ISR sets `tx_done_pending`. The reset hook quiesces DMA but leaves that latch set. The same service invocation starts the replacement HELLO. The next service invocation treats the old completion as successful completion of that new transfer.

The subsequent queue selection can encode another message into `tx_serial` before `start_tx_dma()` aborts the still-active replacement channel.

Impact: premature success accounting, truncated/corrupted UART frames and violation of DMA buffer ownership during session recovery. This finding does not establish vehicle CAN transmission.

Independent reproduction: a scratch harness wrapped the real reset hook, injected the old transfer’s completion ISR immediately before invoking it, and delivered an encoded HELLO through the RX ring. Repository source was unchanged.

```text
after_reset new_tx=1 stale_done=1 remaining=106
no_new_irq completed_delta=1 new_type=2 remaining=74
```

Recommendation: atomically quiesce TX and invalidate its hardware interrupt state and software completion/error state before starting another transfer. Bind events to a transfer generation or equivalent ownership identity. Add the demonstrated interleaving as a regression test.

Disposition: **unresolved**.

**RA-02 — P2: Expired time-sync COMMIT is accepted before tick expires it**

Primary location: [uart_link.c:1059](F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/module/uart_link.c:1059).

Related locations:

- `uart_link.c:2097`: pending-sync expiry occurs in maintenance.
- `uart_dma.c:678`: RX processing occurs first.
- `uart_dma.c:701`: maintenance follows RX processing.

Failure scenario: request starts at 40 ms. Tick at 1039 ms leaves it pending. At 1040 ms, COMMIT arrives before the next tick. `handle_time_sync()` checks identity and timestamp binding but does not check pending age. It accepts the exchange at its one-second expiration boundary and records a fresh mapping update.

The probe maintained a live heartbeat and supplied exact matching t1/t2/t3:

```text
expired_commit status=0 mapping_valid=1 age_ms=1000
```

Impact: the pending lifetime depends on caller ordering; an expired exchange can establish a mapping. The current production NULL authorizer prevents this probe from admitting vehicle commands.

Recommendation: enforce `age >= CANVIEW_STM_UART_TIME_SYNC_PENDING_TIMEOUT_MS` within REQUEST/COMMIT handling before acceptance or refresh. Keep tick expiry for cleanup. Test COMMIT-before-tick ordering at the boundary.

Disposition: **unresolved**.

**RA-03 — P2: RX error recovery omits known discarded bytes**

Primary location: [uart_dma.c:609](F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/platform/stm32g474/uart_dma.c:609).

Related exact lines: 343 records the supplied discard count; 366 clears the RX buffer; 374 resets the reader cursor.

Failure scenario: the producer snapshot is valid and identifies unread bytes, but a DMA/USART error is also pending. Recovery is always passed a zero discard count. It clears the buffer and resets the cursors, losing those known bytes without counting them.

```text
rx_error buffered=100 discarded=0 recoveries=1
```

Impact: recovery occurrence and reason are visible while known byte loss remains unaccounted. Repeated errors can invalidate the no-unaccounted-loss acceptance claim.

Recommendation: count known unread bytes whenever the producer snapshot is valid. Record an explicit unknown-loss condition when loss cannot be determined. Add an error-plus-buffered-data regression.

Disposition: **unresolved**.

**Prior closure dispositions**

| Requested check | Disposition |
|---|---|
| CTS high/pull-up flow-stop polarity | Source and host checks satisfied. High input means blocked; existing test exercises high and low. Pinmap connects R49/R50, 10 kΩ, to 3V3. Electrical verification remains outstanding. |
| DMA TX buffer lifetime | Reopened by RA-01. |
| Reset hook/quiesce ordering and bounded read-back | Hook precedes runtime TX-buffer clearing; CCR polling is bounded; failed hook returns before clearing. Software completion ownership remains defective. Actual stuck-bit hardware behavior was not exercised. |
| RNG HSI48/CLK48 setup | Source and host checks satisfied. HSI48 enable/readiness precede clock selection and RNG enable. ST LL definitions confirm zero selection means HSI48. Failure returns zero; main enters boot fault. |
| 64-bit monotonic milliseconds | Source and register tests satisfied within the documented sampling contract. Milliseconds derive from the protected extended TIM2 clock. Physical continuous timing remains unmeasured. |
| Cross-clock ordering | Inspected arithmetic compares t3/t2 and t4/t1 within their respective domains. Checked differences/addition and the negative-offset test were verified. |
| Exact t2/t3 binding | Independent mutation of either value returned `CANVIEW_STALE`; mapping remained invalid. |
| Pending sync timeout | Reopened by RA-02. Existing test calls tick before COMMIT and misses production ordering. |
| Safety-inhibit admission | Independently verified with an already valid mapping: rejection, zero authorizer calls, zero pending commands. |
| RX recovery reason/drop counters | Reason/recovery counters and overrun accounting exist. Error-path byte accounting remains defective under RA-03. |
| Reset/CTS offline cancellation | Existing tests passed for pending cancellation and mapping/lease invalidation. RA-01 remains an independent DMA lifecycle defect. |
| Callback reentry | Existing authorizer/public-operation tests passed. Guards require single cooperative ownership and are not thread synchronization. Arbitrary reset-hook reentry was not exhaustively exercised. |
| Queue/cache bounds | Fixed capacities and admission inspected; priority, coalescing, RAW eviction and 256-live-entry cache tests passed. |
| Invalid board contracts | Wrong CTS pin and wrong PCLK1 scratch headers failed target compilation with expected errors. |
| Communicator/Bridge isolation; hidden CAN TX | No added raw-CAN/vehicle-TX implementation found in the inspected composition/searches. Source TX gate and target ELF checks succeeded. Production authorizer is NULL; capture-only lease acquisition is disabled. |

**Actual verification commands and results**

The requested commands were executed exactly:

```powershell
git -c safe.directory=F:/dev/canview-wt/2026-09-09-t104-postfix-review-a -C F:/dev/canview-wt/2026-09-09-t104-postfix-review-a rev-parse HEAD
git -c safe.directory=F:/dev/canview-wt/2026-09-09-t104-postfix-review-a -C F:/dev/canview-wt/2026-09-09-t104-postfix-review-a status --porcelain=v1
git -c safe.directory=F:/dev/canview-wt/2026-09-09-t104-postfix-review-a -C F:/dev/canview-wt/2026-09-09-t104-postfix-review-a diff --find-renames b17bdfc0bb2a1bfa9d300c1e7662cac05c96df40 bdc67981a67d059518e9122f2c4c3eef3c5a4825
```

The full diff exceeded tool output limits and was not completely inspected. Requested runtime files and UART tests were subsequently read directly in bounded sections. A separate `git diff --stat` reported 43 changed files. HEAD/status were repeated during verification and at completion.

Other executed command groups:

- `Get-Date -Format o`.
- `Get-Content`, numbered `ForEach-Object`, `Select-Object` ranges and `Select-String` for the inventory below.
- `rg --files` for instructions, CMake, UART, board, memory, tests and toolchain discovery.
- `rg -n` for time-sync/session/CTS/admission, TX registers/APIs, raw TX, build modes, allocation, timer tests and pull-up connectivity.
- `Get-Command cmake,ctest,clang,gcc,python,ninja -ErrorAction SilentlyContinue`.
- Bounded `Get-ChildItem` discovery under the main checkout’s `.tools`, `F:/dev`, the user cache, `C:/cv`, Windows Kits, Visual Studio and CANView toolchain locations. Two guessed installation paths did not exist.
- `vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`.
- Installed `Launch-VsDevShell.ps1 -Arch amd64 -HostArch amd64 -SkipAutomaticLocation`.
- `python -X utf8 -B .../tools/generate_uart_protocol.py --check`: succeeded.
- `python -X utf8 -B .../tools/generate_boards.py --check`: succeeded.
- Python invocation of `check_stm32_core.check_source_safety()` on the STM32 tree: `True`.
- `python -X utf8 -B work/negative_board.py`: wrong-pin and wrong-clock compilations failed with expected errors.
- `& ./work/run_probes.ps1`, twice: first reproduced all three defects; second also verified valid-mapping safety inhibit and t2/t3 mutations. Probe assertions confirmed the reported behavior.

Host configuration used CMake 4.4.3, Ninja 1.13.2, Clang 23.1.0, Debug, the verified worktree as source and `work/host` as build directory. Initial configuration failed because the resource compiler environment was missing. Loading Visual Studio’s development environment resolved that failure.

Built targets:

```text
canview-stm32-uart-link-tests
canview-stm32-uart-platform-tests
canview-stm32-register-tests
canview-stm32-core-tests
canview-uart-tests
```

CTest command:

```text
ctest --test-dir work/host --output-on-failure
-R "^(stm32-uart-.*|stm32-core-.*|uart-(matrix|direction|malformed|stream|long-stream|soak-smoke|payload-lifetime|plan|plan-boundaries|command|command-boundaries|link|link-boundaries|limits|session|replay))$"
--timeout 60
```

**24/24 tests succeeded**, total 6.45 seconds:

```text
stm32-uart-link
stm32-uart-platform
stm32-core-register-model
stm32-core-boot
stm32-core-scheduler-validation
stm32-core-scheduler-healthy
stm32-core-scheduler-faults
stm32-core-queue
uart-matrix
uart-direction
uart-malformed
uart-stream
uart-long-stream
uart-soak-smoke
uart-payload-lifetime
uart-plan
uart-plan-boundaries
uart-command
uart-command-boundaries
uart-link
uart-link-boundaries
uart-limits
uart-session
uart-replay
```

Fresh target Debug configuration/build used:

```text
Source: verified worktree/firmware/communicator/stm32
Build: work/target-debug
Generator: Ninja
Toolchain: repository cmake/arm-none-eabi-gcc.cmake
STM32CUBE_G4_ROOT=C:/cv/STM32CubeG4-1.6.3
Arm GNU compiler: 15.3.1
Build parallelism: -j 4
```

ELF/HEX/BIN/MAP generation and configured objcopy, size, `check_stm32_core.py` and `check_stm32_fdcan_layout.py` completed successfully.

```text
FLASH used: 51092 bytes
RAM used:   79800 / 98304 bytes
CCMRAM:     0 bytes
text:       51088
data:       4
bss + reserved stack: 79792
maximum individual stack frame: 928 bytes
C object stack files checked: 23
CMSIS/model constants checked: 54
```

The individual-frame figure is not complete worst-case call-stack evidence.

**Actual file-read inventory**

The following requested files were read directly:

```text
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/interface/canview_stm_uart.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/module/uart_link.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/platform/stm32g474/uart_dma.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/platform/stm32g474/uart_dma.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/platform/stm32g474/core_hw.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/platform/stm32g474/core_hw.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/app/main.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/docs/uart-dma.md
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/tests/stm32/test_uart_dma_platform.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/tests/stm32/test_uart_link.c
```

Supplemental files read wholly or in focused sections/search excerpts:

```text
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/AGENTS.md
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/CMakeLists.txt
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/cmake/CanviewWarnings.cmake
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/docs/README.md
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/docs/resume.md
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/docs/tasks/T-104-stm32-uart-control.md
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/docs/development/windows.md
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/docs/development/foundation.md
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/CMakeLists.txt
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/module/CMakeLists.txt
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/cmake/arm-none-eabi-gcc.cmake
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/ld/STM32G474CEUx_FLASH.ld
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/bsp/board_pins.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/bsp/board.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/bsp/core.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/platform/stm32g474/safe_gpio.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/module/scheduler.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/interface/canview_build_mode.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/interface/canview_stm_fdcan_capture.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/communicator/stm32/tests/test_registers.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/tests/stm32/fake_stm32/board_pins.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/tests/stm32/fake_stm32/core_hw.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/tests/stm32/fake_stm32/fake_hardware.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/tests/stm32/fake_stm32/stm32g474xx.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/tests/foundation/CMakeLists.txt
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/shared/protocol/CMakeLists.txt
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/shared/protocol/src/canview_uart.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/shared/protocol/include/canview_uart.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/shared/protocol/src/canview_espnow.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/tools/check_stm32_core.py
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/tools/environment/foundation-windows.ps1
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/tools/environment/setup-windows.ps1
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/hardware/communicator/pinmap.csv
F:/dev/canview-wt/2026-09-09-t104-postfix-review-a/hardware/communicator/bom.csv
C:/cv/STM32CubeG4-1.6.3/Drivers/STM32G4xx_HAL_Driver/Inc/stm32g4xx_ll_rcc.h
C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md
C:/Users/digit/.codex/skills/embedded-driver-design/SKILL.md
C:/Users/digit/.codex/skills/embedded-isr-design/SKILL.md
C:/Users/digit/.codex/skills/embedded-rtos-design/SKILL.md
C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md
C:/Users/digit/.codex/skills/embedded-documentation/SKILL.md
C:/Users/digit/Documents/Codex/2026-09-09/t104-postfix-review-a-20260909/work/target-debug/compile_commands.json
```

Compiler/checker transitive reads are build inputs, not independently reviewed source. The scratch probe sources, execution script and negative-board script were created and executed only under the task’s `work/` directory.

**Residual and incomplete scope**

All explicitly named runtime sources and UART test sources were inspected. The requested full diff command executed, but its output was truncated and the complete diff was not inspected.

This execution did not rerun the entire repository suite, target Release build, coverage, sanitizers, API documentation, ESP-IDF builds or 24-hour soak. It did not exhaustively verify every DMA/ISR interleaving, RX wrap timing, register failure, reset-hook reentry or worst-case call-stack path.

End-to-end control authentication, future executors, production provisioning and complete Bridge firmware behavior were outside this bounded runtime review. Current NULL authorization and capture-only composition remain essential boundaries.

Host fake registers do not establish asynchronous DMA timing, real interrupt latency, electrical behavior or complete register side effects. The injected race demonstrates a software state-transition defect; it is not HIL evidence.

| Physical/electrical/HIL/vehicle gate | Status |
|---|---|
| Board flash, ST-LINK and USB attachment | NOT_RUN |
| Actual RTS/CTS voltage, pull-up behavior, routing and signal integrity | NOT_RUN |
| Physical 4 Mbps UART and 24-hour PRBS | NOT_RUN |
| DMA timing, real IRQ latency and injected hardware faults | NOT_RUN |
| Watchdog timing and prolonged operation | NOT_RUN |
| Reset, brownout, rail sequencing and external CAN gate behavior | NOT_RUN |
| CAN analyzer, vehicle connection, capture and vehicle TX | NOT_RUN |

Final verdict: **BLOCK**
