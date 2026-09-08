# T-103 Reviewer A Raw Report

- Execution ID: `CV-HOSTILE-20260909-T103-A-03`
- Start: `2026-09-08T21:27:28.1379402Z` / `2026-09-09T06:27:28.1379402+09:00`
- End: `2026-09-08T21:33:53.0404789Z` / `2026-09-09T06:33:53.0404789+09:00`
- Scope: STM32G474 three-channel FDCAN capture-only module, CMSIS adapter, ISR/RTOS ownership, lifecycle, FIFO/raw-ring loss, timestamps, board/electrical contract, no-TX boundary.
- Out of scope: real board flash, power measurements, vehicle CAN evidence, provisioning.

## Candidate and isolation

- Candidate: `a8d515849d98b89bfc7904356cf3ad8c5a2334bd`
- Base: `6db3998092812354afe3b3918892b682022990b4`
- Candidate parent: `3e13b2ca6e72a3aec5a32a6357285c614bc191f9`
- Both objects verified as commits.
- `git diff --check`: clean.
- Review was object-only; no files modified, staged, committed, or pushed.
- Worktree was already dirty with unrelated untracked files; preserved.

## Findings

### P1-01 — RX FIFO/message-RAM configuration is missing

Location: `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:187-210`

`configure_instance()` configures `RXGFC`, interrupts, and nominal timing, but never configures the RX FIFO base/depth or element size registers (`RXF0C`, `RXESC`, and related message-RAM configuration). The reader nevertheless assumes hard-coded offsets and 72-byte elements at lines 15-19 and 391-420.

Failure scenario:

1. Hardware starts with reset/default FIFO configuration.
2. A valid frame arrives.
3. FIFO0 is disabled, incorrectly located, or uses a different element stride.
4. Capture receives no frames or reads the wrong message-RAM locations.

Impact: target capture can be nonfunctional or decode incorrect memory. The fake register model omits these registers, so host adapter tests cannot detect this.

Recommendation: configure and read back the complete FIFO/message-RAM layout, derive or validate offsets against the pinned STM32CubeG4 CMSIS definitions, and extend the fake-register model and tests.

### P1-02 — Hardware FIFO loss and message-RAM errors do not force a fault or counted drop

Locations:

- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:403-415`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:532-558`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:629-647`

`fifo_loss_unknown` is latched only for invalid FIFO metadata or software raw-ring overflow. A real hardware `RF0L` event is merely acknowledged and copied into `pending_interrupts`. At service time, only `fifo_loss_unknown` changes the state to `BUS_FAULT`; `RF0L` and `MRAF` otherwise appear only in `last_error`.

Failure scenario: the hardware FIFO overflows while the software raw ring still has space, or a message-RAM access error occurs.

Impact: frames are lost without an exact drop count and the reported state may remain `ERROR_ACTIVE`. Downstream code can treat incomplete capture as healthy.

Recommendation: latch hardware `RF0L` and `MRAF` explicitly, report loss as unknown/exact where appropriate, force `BUS_FAULT`, and test actual interrupt-bit paths separately from malformed FIFO metadata.

### P2-01 — Platform sink callback reentry is not guarded

Locations:

- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.h:32-48`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:469-504`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:589-650`

The contract informally requires callbacks not to reenter, but `service()` has no reentry guard. A `frame_sink` or `drop_sink` that calls `platform_service()` can recursively drain the raw ring and clear pending status while the outer service is still active.

Impact: recursive stack growth, reordered callbacks, and lost/deferred status reporting.

Recommendation: add an explicit service-owner/reentry guard, or make callback delivery deferred through a bounded queue with a fail-closed `BUSY` result.

### P2-02 — Failed frame sinks lose already-popped raw elements

Locations:

- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:455-465`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:491-503`

The raw-ring read index advances before `frame_sink()` is called. If the callback returns `CANVIEW_TIMEOUT` or another error, the element is discarded. Only an internal `sink_failures` counter changes; the module drop counter and `drop_sink` are not notified.

Impact: transient callback failure causes unreported capture loss.

Recommendation: retain the failed element for bounded retry, or convert it into an explicit drop/loss event before advancing ownership.

### P2-03 — Partial safe-output failure has no complete rollback

Locations:

- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:103-124`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:289-292`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:364-385`

`set_capture_outputs()` stops on the first GPIO error. `start()` returns immediately, and `stop()` records the error but continues without verifying that every PHY standby and TX-request output reached its safe value.

Failure scenario: an invalid generated pin contract or future GPIO-layer failure occurs after only part of the safe-output sequence.

Impact: lifecycle failure can leave PHY enable/standby or gate-request outputs in an unknown state.

Recommendation: validate all board pins before mutation, attempt all safe writes, latch failure, verify gate sense where available, and refuse further lifecycle progress unless the complete safe state is confirmed.

### P2-04 — Module capture session reset is not defined

Locations:

- `firmware/communicator/stm32/interface/canview_stm_fdcan_capture.h:223-255`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:347-385`

The platform adapter resets its raw session state on stop/start, but the module context has no reset/deinitialization API. Its queued frames, timestamp epoch, inventory, and counters persist across adapter restart.

Impact: a future app integration can deliver pre-restart frames or stale timestamps into a new capture session.

Recommendation: add an explicit session-reset API or define and test persistent-history semantics with a session identifier.

## Previous Reviewer A findings rechecked

Fixed at the candidate:

- Batch callback reentry prefix loss: transactional commit now occurs after callbacks.
- Cross-channel timestamp-wrap rejection: `timestamp_near_anchor()` added.
- Status timestamp corrupting frame freshness: frame and status timestamps are separated.
- FIFO acknowledge ordering: RX flags are acknowledged before draining.
- Weak no-TX evidence validation: schema, sequence, three-channel summary, gate, completion, ACK, and TX checks are now fail-closed.
- Stop/start raw-ring leakage: runtime indices and loss state are reset.
- Non-owner stop mutating hardware: active-owner check added.
- Filter-reject processing budget: `item_count` now bounds consumed records.

Still open:

- Hardware FIFO-loss semantics remain incomplete, covered by P1-02.

## Reviewed without finding

- Module batch transactional commit and mutation detection.
- Module filter reentry handling.
- Timestamp wrap and cross-channel ordering logic.
- Module ring-full behavior and saturating counters.
- Raw-ring SPSC ownership and bounded ISR copying.
- Standard/extended/RTR/FD/BRS/DLC/padding validation.
- Profile and PHY validation.
- Stop/start raw-ring reset and non-owner stop protection.
- Watchdog feed is not performed from the capture ISR.
- No FDCAN TX descriptor, TX buffer, replay, or control API was found.
- `MON` and `DAR` are enabled in the FDCAN configuration.
- Repository TX search found no FDCAN TX-register writes in the reviewed path.

## Commands and files actually read

Object verification and repository checks:

```text
git cat-file -e "a8d515849d98b89bfc7904356cf3ad8c5a2334bd^{commit}"
git rev-parse "a8d515849d98b89bfc7904356cf3ad8c5a2334bd^{commit}"
git diff --find-renames 6db3998092812354afe3b3918892b682022990b4 a8d515849d98b89bfc7904356cf3ad8c5a2334bd
git -c safe.directory=F:/dev/canview cat-file -t ...
git -c safe.directory=F:/dev/canview rev-parse ...
git -c safe.directory=F:/dev/canview diff --check ...
git -c safe.directory=F:/dev/canview diff --name-status --find-renames ...
git -c safe.directory=F:/dev/canview status --short --branch
git -c safe.directory=F:/dev/canview ls-tree -r --name-only ...
git -c safe.directory=F:/dev/canview grep -n -E 'TXBAR|TXBUF|TXFQS|TXBC|TXESC|TXBRP|Transmit|CAN_TX|vehicle_tx' ...
```

The first three commands failed only because Git’s dubious-ownership guard was active; they were rerun with a per-command `safe.directory` override. No global Git configuration was changed.

Candidate files read with `git show`:

```text
docs/README.md
docs/resume.md
docs/tasks/T-103-stm32-fdcan-capture.md
docs/reviews/adversarial/evidence/2026-09-09-T-103-reviewer-a.md
CMakeLists.txt
firmware/communicator/stm32/CMakeLists.txt
firmware/communicator/stm32/module/CMakeLists.txt
firmware/communicator/stm32/interface/canview_stm_fdcan_capture.h
firmware/communicator/stm32/module/fdcan_capture.c
firmware/communicator/stm32/platform/stm32g474/fdcan_capture.h
firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c
firmware/communicator/stm32/platform/stm32g474/core_hw.c
firmware/communicator/stm32/platform/stm32g474/safe_gpio.c
firmware/communicator/stm32/bsp/board_pins.h
firmware/communicator/stm32/docs/fdcan-capture.md
tests/stm32/test_fdcan_capture.c
tests/stm32/test_fdcan_platform.c
tests/stm32/fake_stm32/fake_hardware.c
tests/stm32/fake_stm32/stm32g474xx.h
tests/stm32/fake_stm32/core_hw.h
tests/stm32/fake_stm32/safe_gpio.h
tests/hil/assert_no_tx.py
tests/hil/run_can_capture.py
tests/hil/fixtures/t103-capture-only.jsonl
tests/hil/run.py
```

Requested path `firmware/communicator/stm32/platform/stm32g474/board_pins.h` does not exist in the candidate; the actual generated board contract is `firmware/communicator/stm32/bsp/board_pins.h`.

## Verification limits

- Host CTest: `NOT_RUN`
- Sanitizer and coverage: `NOT_RUN`
- STM32 target build: `NOT_RUN`
- Board flash/ST-LINK: `NOT_RUN`
- Actual CMSIS register behavior and STM32 reference-manual confirmation: `NOT_RUN`
- FDCAN electrical/bitrate/IRQ-latency testing: `NOT_RUN`
- Reset/brownout/power-rail gate: `NOT_RUN`
- CAN analyzer ACK/data-TX measurement: `NOT_RUN`
- Vehicle capture and vehicle CAN TX: `NOT_RUN`; vehicle CAN TX remains `NO-GO`.

## Verdict

BLOCK
