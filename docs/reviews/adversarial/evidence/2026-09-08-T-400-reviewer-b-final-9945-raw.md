# CANView Diagnostic Bridge PR #28 — Final Independent Hostile Review

Execution ID: `codex-thread-review-B-20260908-9945f90`
Start: `2026-09-08T11:21:25.3686849+09:00`
End: `2026-09-08T11:36:58.8837364+09:00`
Candidate: `9945f902e4279b53bc2fb69cdb76ac970058ddce`
Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`

Isolation: candidate/base Git objects only. Exact candidate archive was extracted to a temporary directory. Working tree, untracked files, and `docs/reviews/**` contents were not inspected. No repository edits, commits, pushes, or agents.

검증 결과:

- Python unit tests: 48/48 PASS.
- Host Debug build: 99/99 objects PASS.
- CTest: tests 1–75 and 77–112 PASS; test 76 (`uart-fault-stream`, configured for 86,400 virtual seconds) did not produce completion evidence and is INCOMPLETE. A 1-second reduced run passed.
- ESP32 coverage build/gate: PASS, but scope is portable core/SDK fixture, not physical/HIL.
- Board generation, generated-artifact check, SDK negative mutations, canonical gzip, and `.gitattributes` EOL reproducibility: PASS.
- `max_open_sockets = 1U`: PASS.
- `session_close_pending` set/check/clear path: present.
- DNS bounded query slice and cleanup retry state: statically consistent.
- Target ESP-IDF build/artifacts: NOT_RUN locally; `idf.py` unavailable.
- Browser/live HTTP/WS security integration: NOT_RUN.
- Physical/HIL: NOT_RUN.
- CAN TX: NO-GO.

Findings

P0: None observed.

P1-01 — Mandatory HTTP security/read-only integration evidence is absent

Files/lines:

- `docs/tasks/T-400-diagnostic-bridge-bootstrap.md:81-89`
- `CMakeLists.txt:175-199`
- `.github/workflows/foundation.yml:18-59`
- `tools/ui/check-browser.cjs:14-23,109-112`
- `tests/ui/diagnostic-browser.cjs:9-18`

Failure scenario: the static asset test and auth unit test pass, while an actual ESP-IDF HTTP/WS implementation has a regression in Origin handling, bearer authentication, session exclusivity, body limits, forced close, DNS interaction, or read-only/no-TX routing.

Impact: the task’s documented `tests/security/bridge_http.py` and `tests/ui/bridge_offline_browser.py` gates do not exist. The registered browser test exercises local `file:` HTML and is not invoked by the foundation workflow. No actual HTTP/WS security boundary is tested.

Recommendation: add a real HTTP/WS integration test against the built Bridge target or an ESP-IDF test fixture; cover Origin, Authorization, session expiry, forced-close failure, malformed/oversized bodies, DNS lifecycle, one-socket enforcement, and assertions that no CAN-TX/control-lease/raw-replay route exists. Register it in CMake and CI. Update the stale task commands.

P1-02 — Cleanup can discard live HTTP state after `httpd_stop()` failure

Files/lines:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:403-410`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:449-468`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1818-1835`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1907-1914`

Failure scenario: session expiry or startup cleanup calls `httpd_stop()`, which returns an error while the HTTP task/callback is still live. The code records the error but sets `web_state.server = NULL`, clears lock handles, deletes the locks, and zeroes state. The retry from `app_main.c` can no longer retry the HTTP server using its handle.

Impact: callbacks may access cleared state or deleted locks; the server may remain live after authentication/session invalidation. This weakens fail-closed cleanup and can cause use-after-delete behavior under an injected cleanup failure.

Recommendation: preserve the server handle and synchronization objects until shutdown is confirmed. Do not delete locks or zero state while HTTP callbacks may remain active. Route startup-failure cleanup through the same checked cleanup state machine and retain retryable failure state.

P2-01 — Documented 5-minute idle timeout is not implemented

Files/lines:

- `docs/architecture/diagnostic-bridge.md:170`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:42-75`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1470-1507`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1795-1805`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1849-1917`

Failure scenario: an authenticated WebSocket remains connected but idle. The only relevant values are 5-second per-I/O waits and the 10-minute service-window expiry; there is no `last_activity` state or idle-close logic.

Impact: the single permitted socket can remain occupied longer than the documented resource boundary, delaying legitimate access and weakening resource-exhaustion guarantees.

Recommendation: track successful request/WS activity and close idle sessions at 5 minutes, or revise the architecture contract and test the actual intended policy.

P2-02 — ESP32 Bridge coverage gate covers generic app code, not the Bridge web/bootstrap composition

Files/lines:

- `CMakeLists.txt:51-73,78-91`
- `tools/check_esp32_core_coverage.py:20-35,64-70`

Failure scenario: CI reports Bridge app coverage while exercising `firmware/app/esp_core.c`. The actual Bridge integration target contains `bridge_bootstrap.c` and Bridge BSP sources, but those are not part of the coverage groups; `app_main.c`, authentication, web, DNS, and HTTP callbacks are also excluded.

Impact: coverage evidence is materially narrower than its Bridge label implies.

Recommendation: add an explicit Bridge composition coverage target or rename the current gate to reflect its generic scope. Include Bridge bootstrap/BSP and separately test web/auth/DNS boundaries.

P2-03 — Target resource-exhaustion budgets are not fail-closed in CI

Files/lines:

- `.github/workflows/foundation.yml:210-223`
- `tools/check_budgets.py`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1795-1805`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:11-18,164-175`

Failure scenario: target build succeeds while heap/PSRAM usage, HTTP task stack, cJSON allocation pressure, DNS flood behavior, or long-lived WebSocket/resource exhaustion regresses.

Impact: current target CI runs build and `size-components`, but no target-specific threshold parser or HTTP/DNS flood gate fails the build.

Recommendation: add target map/size parsing with explicit heap, PSRAM, task-stack, socket, body/frame, and DNS throughput thresholds; add dynamic stress tests where hardware or an ESP-IDF fixture is available.

P3-01 — Target artifact manifest is not source-revision bound

Files/lines:

- `.github/workflows/foundation.yml:250-264`

Failure scenario: an artifact manifest is later separated from its CI run metadata.

Impact: the manifest records toolchain provenance and artifact hashes but not candidate commit, SDK/configuration digest, or generated-source digest, weakening standalone auditability.

Recommendation: include `HEAD`, build configuration, SDK/config digest, generator digests, and CI run identity in `target-artifacts.json`.

P3-02 — Authentication API comment contradicts the implementation

Files/lines:

- `firmware/diagnostic-bridge/components/canview_bridge_auth/include/canview_bridge_auth.h:43-48`
- `firmware/diagnostic-bridge/components/canview_bridge_auth/canview_bridge_auth.c:138-153`

The header says initialization clears supplied storage, but the supplied PIN digest is `const` and is copied, not cleared. Correct the API documentation.

Unreviewed scope

- Actual ESP-IDF 6.0.3 Bridge target build and target artifact hashes.
- Live HTTP/WS, DNS, browser, Wi-Fi, and malformed-network testing.
- Physical pin/package validation, flash, rail/reset/brownout, watchdog soak, PSRAM stress, and HIL.
- Vehicle CAN capture/evidence and all CAN-TX release gates.
- Full 86,400-second `uart-fault-stream`; only its reduced run completed.
- Hosted CI execution and uploaded artifact verification.

CAN TX remains NO-GO.

BLOCK
