# Raw hostile review report

- Execution ID: `codex-thread-review-A-20260908-bf7a11c`
- Specialty: embedded runtime, RTOS ownership, watchdog, reset/brownout, memory bounds, GPIO/electrical, callback/ISR/task lifetime, fail-safe behavior, Communicator/Bridge isolation
- Candidate: `bf7a11cd1acc86bc89a644c2855a9838caddf8da`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Candidate parent: `19efeedd895e1885a52ab9023bf6e5095c358cb3`
- Review window: `2026-09-08T03:09:03.6071787Z`–`2026-09-08T03:14:04.7738224Z`
- Isolation: immutable Git objects only; no working-tree inspection, no `docs/reviews/**`, no edits, commits, pushes, agents, builds, or tests.

## Findings

### P1-001 — Startup failure can leave HTTP/AP resources running without watchdog or cleanup retry

Files:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:420-429`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1870-1883`
- `firmware/diagnostic-bridge/main/app_main.c:168-174,214-222`

Failure scenario:

1. URI registration or DNS startup fails.
2. `discard_start_state()` calls `httpd_stop()`.
3. If `httpd_stop()` fails, cleanup returns early while retaining the server, locks, Wi-Fi, and auth state.
4. `canview_bridge_web_start()` returns failure.
5. `app_main()` leaves `web_started == false`, so its cleanup retry is skipped.

Impact: the Bridge can remain network-active after boot failure, while the application never arms its watchdog. This violates fail-closed startup ownership and can leave unrecoverable HTTP/AP resources.

Recommendation: retry cleanup on every failed `canview_bridge_web_start()` path, or make `canview_bridge_web_start()` own a bounded cleanup retry. Set the caller’s ownership flag as soon as resources are allocated.

### P1-002 — DNS task may outlive network teardown after stop timeout

Files:

- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:240-271`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:132-218`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:404-489`

Failure scenario:

1. `canview_bridge_dns_stop()` times out while the DNS task remains active.
2. `discard_start_state()` records the error but continues.
3. Wi-Fi, netifs, event loop, and synchronization objects are stopped/deleted while `dns_task()` may still execute socket operations.

Impact: a live task can access networking resources after their teardown, creating a task-lifetime race, crash, or undefined reset behavior.

Recommendation: if DNS stop does not confirm `stopped`, return immediately and preserve all dependent network resources. Retry DNS shutdown before Wi-Fi/netif/event-loop destruction.

### P2-001 — Authentication validation and session activity recording are split across a revocation race

Files:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:370-387`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:672-683`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1191-1199,1243-1250`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1941-1960`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1706-1718`

`authenticated()` releases `state_lock` before `record_authenticated_request()` reacquires it. The poll/close path does not serialize with `request_lock`.

Possible interleaving: service-window expiry logs out and closes a client between the token check and activity recording; after the close callback clears the session state, `record_authenticated_request()` can reclaim the same descriptor without revalidating the token.

Impact: an in-flight protected request may continue after forced logout, and stale client ownership can be re-established.

Recommendation: combine token validation and activity recording under one state-lock critical section, or add a session generation value that every handler must revalidate before producing a protected response.

## P0

No P0 finding.

## Attacked scenarios that passed source review

- Wrong-board preflight occurs before Bridge safe-state/runtime setup: `bridge_bootstrap.c:14-29`.
- Runtime callbacks reject ISR, wrong owner, and callback reentry: `core_runtime.c`.
- Communicator drives `RUN_OK` low first and keeps Bridge read-only: `communicator/esp32/bsp/board.c:10-22`.
- Diagnostic Bridge contains no raw replay, control lease, or vehicle CAN TX path.
- JSON, WebSocket, HTTP header, and DNS buffers have explicit bounds.
- Pre-auth requests do not refresh authenticated session activity.
- Candidate/base diff passed `git diff --check`.

## Files read

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web_session.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/include/canview_bridge_web_session.h`
- `firmware/diagnostic-bridge/components/canview_bridge_web/include/canview_bridge_web.h`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.h`
- `firmware/diagnostic-bridge/components/canview_bridge_web/CMakeLists.txt`
- `firmware/diagnostic-bridge/components/canview_bridge_auth/canview_bridge_auth.c`
- `firmware/diagnostic-bridge/components/canview_bridge_auth/include/canview_bridge_auth.h`
- `firmware/diagnostic-bridge/components/canview_bridge_auth/CMakeLists.txt`
- `firmware/diagnostic-bridge/main/app_main.c`
- `firmware/diagnostic-bridge/main/bridge_bootstrap.c`
- `firmware/diagnostic-bridge/bsp/runtime.c`
- `firmware/diagnostic-bridge/bsp/board.c`
- `firmware/diagnostic-bridge/bsp/board_pins.h`
- `firmware/diagnostic-bridge/bsp/bridge_button.c`
- `firmware/diagnostic-bridge/bsp/bridge_button.h`
- `firmware/diagnostic-bridge/sdkconfig.defaults`
- `firmware/platform/esp32s3/core_runtime.c`
- `firmware/platform/esp32s3/canview_gpio.c`
- `firmware/module/esp_core/health.c`
- `firmware/module/esp_core/pool.c`
- `firmware/communicator/esp32/bsp/board.c`
- `firmware/communicator/esp32/bsp/runtime.c`
- `firmware/communicator/esp32/bsp/board_pins.h`
- relevant ESP interface headers
- `tests/bridge_web/test_session.c`
- `tests/bridge_web/test_auth.c`
- `tests/esp_core/test_bridge_integration.c`
- `tests/security/bridge_http.py`
- `tests/test_bridge_web_assets.py`

## Commands run

- `git cat-file -e '<candidate>^{commit}'`
- `git cat-file -e '<base>^{commit}'`
- `git show -s --format=... <candidate>`
- `git diff --find-renames --name-status <base> <candidate>`
- `git diff --find-renames --check <base> <candidate>`
- `git diff --find-renames --unified=60 <parent> <candidate> -- <review files>`
- `git show <candidate>:<file>`
- `git grep -n -E '...' <candidate> -- firmware ':!docs/reviews/**'`
- `Get-Date -AsUTC -Format o`

## Unreviewed / not run

No builds, host tests, live HTTP/WebSocket probes, board flash, AP association, watchdog-reset tests, heap/PSRAM soak, GPIO/electrical measurement, reset/brownout HIL, or vehicle integration were run.

Physical/HIL: `NOT_RUN`  
Vehicle CAN TX: `NO-GO`

## Verdict

**BLOCK**
