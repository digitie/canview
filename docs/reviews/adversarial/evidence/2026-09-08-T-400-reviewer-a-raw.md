# Independent hostile review report

- Execution ID: `codex-thread-review-A-20260908-b0f2356`
- Start: `2026-09-08T00:09:23.4347043Z`
- End: `2026-09-08T00:31:01.2422692Z`
- Candidate: `b0f23561aeb74eac9e32eaafa02298fd9bfdac64`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Candidate parent: `3eb3647776215e472467bedbcdda9858fdbeb52f`
- Isolation: Git-object reads from `F:\dev\canview` only. No edits, commits, pushes, worktree reads, tests, agents, or reviewer evidence used.

## Scope

Reviewed embedded runtime, RTOS ownership, watchdog, reset behavior, memory bounds, GPIO/board contracts, callback/ISR/task lifetime and reentry, Diagnostic Bridge auth/web/DNS, CMake composition, Communicator/Bridge isolation, and related tests/docs.

Explicitly read:

- `firmware/app/esp_core.c`
- `firmware/module/esp_core/health.c`
- `firmware/module/esp_core/pool.c`
- `firmware/interface/canview_esp_core.h`
- `firmware/interface/canview_esp_pool.h`
- `firmware/interface/canview_esp_runtime.h`
- `firmware/interface/canview_board.h`
- `firmware/platform/esp32s3/core_runtime.c`
- `firmware/platform/esp32s3/canview_gpio.c`
- `firmware/communicator/esp32/bsp/{board.c,board_pins.h,runtime.c}`
- `firmware/communicator/esp32/main/CMakeLists.txt`
- `firmware/diagnostic-bridge/bsp/{board.c,board_pins.h,runtime.c,bridge_button.c,bridge_button.h}`
- Diagnostic Bridge app, auth, web, DNS, CMake, sdkconfig, partitions, and dependencies
- `shared/interface/{canview_platform_port.h,canview_status.h}`
- `CMakeLists.txt`
- board manifest/pinmap and generators
- relevant auth/runtime tests and web asset
- `docs/README.md`, `docs/resume.md`, task, architecture, and web-shell documents

`firmware/interface/canview_platform_port.h` was attempted but does not exist; the actual file is under `shared/interface/`.

## Findings

### P1-001 — HTTP network I/O can starve the main-task watchdog

References:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:296-317`
- `.../canview_bridge_web.c:571-600`
- `.../canview_bridge_web.c:238-274`
- `.../canview_bridge_web.c:1455-1466`
- `firmware/diagnostic-bridge/main/app_main.c:190-205`
- `firmware/diagnostic-bridge/sdkconfig.defaults:18-21`

`enter_request()` holds `web_state.lock` across body reception and response transmission. `receive_json_body()` can block in `httpd_req_recv()`, and JSON responses can block in `httpd_resp_send()`. The main owner calls `canview_bridge_web_poll()`, which waits on the same mutex before reaching `canview_esp_core_step()` and the TWDT feed.

Failure scenario: an AP client sends a request body slowly or stops reading a response. The HTTP task holds the mutex for longer than the configured 2-second watchdog timeout. `app_main()` blocks in `web_poll()`, never reaches `core_step()`, and the bridge resets.

Impact: a local AP client with the WPA2 credential can repeatedly reboot the Diagnostic Bridge and deny service. This violates bounded request/flood behavior and watchdog ownership.

Recommendation: never hold the service-state mutex across `httpd_req_recv()`, `httpd_resp_send()`, or equivalent network I/O. Use per-request bounded storage, short lock sections, bounded lock acquisition, and explicit slow-client tests.

### P1-002 — TWDT is armed before unbounded web startup work

References:

- `firmware/module/esp_core/health.c:64-69`
- `firmware/diagnostic-bridge/main/app_main.c:154-188`
- `firmware/diagnostic-bridge/main/app_main.c:190-205`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1246-1315`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1455-1500`
- `firmware/diagnostic-bridge/sdkconfig.defaults:18-21`

The main task is subscribed to TWDT during `canview_esp_core_boot()`. After that, credential loading, Wi-Fi initialization/start, HTTP server startup, URI registration, DNS task creation, and logging occur before the first feed. No startup deadline or staged feed exists.

Failure scenario: NVS recovery, flash access, Wi-Fi startup, HTTP startup, or logging exceeds two seconds. The legitimate boot path resets before entering the health loop.

Impact: boot liveness depends on an unverified timing assumption; the target watchdog gate is not closed.

Recommendation: define a measured startup budget and enforce it. Either defer TWDT subscription until bounded startup completes or add explicitly bounded startup stages with safe timeout handling. Do not add blind watchdog feeds.

### P2-001 — Active client/session ownership is not forcibly released on service expiry

References:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:307-315`
- `.../canview_bridge_web.c:1171-1207`
- `.../canview_bridge_web.c:1324-1336`
- `.../canview_bridge_web.c:1539-1543`

`active_client_fd` is cleared only by `close_session()`. Service-window expiry clears authentication state but does not close the active HTTP/WebSocket session or clear the client reservation.

Failure scenario: a client keeps the WebSocket or HTTP connection alive while the service window expires. The token is invalidated, but the client reservation remains until transport closure. A new phone/session can receive `one client only`.

Impact: recovery from an expired or abandoned session is transport-dependent and can deny the next legitimate service session.

Recommendation: on service expiry, invalidate the session generation and explicitly close the active socket, or clear the reservation only after confirming the transport is closed. Add idle and expiry tests.

### P2-002 — Failed web startup leaves Wi-Fi state without transactional teardown

References:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:342-363`
- `.../canview_bridge_web.c:1246-1315`
- `.../canview_bridge_web.c:1448-1495`

`start_wifi()` can start Wi-Fi before a later configuration step fails. `discard_start_state()` stops HTTP only; it does not stop Wi-Fi or destroy created netifs, and `httpd_stop()` results are ignored.

Failure scenario: Wi-Fi starts but channel configuration, HTTP registration, or DNS startup fails. The function returns failure, but the AP or allocated network state can remain until watchdog reset.

Impact: partial service state survives a failed startup and restart/cleanup behavior is not deterministic.

Recommendation: make startup transactional with explicit Wi-Fi/netif/DNS/HTTP teardown and checked stop results before returning failure.

### P2-003 — JSON byte limit does not bound parser depth or stack/CPU cost

References:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:23-24`
- `.../canview_bridge_web.c:571-600`
- `.../canview_bridge_web.c:603-651`
- `.../canview_bridge_web.c:768-797`
- `.../canview_bridge_web.c:1455-1457`

An 8 KiB body limit is enforced, but the body is passed directly to cJSON before schema validation. No maximum nesting/depth or parser-complexity bound is enforced; the HTTP task stack is configured to 6144.

Failure scenario: a client submits deeply nested or allocation-heavy JSON within the 8 KiB limit. The parser can consume the fixed arena and substantial task stack/CPU while the web mutex remains held.

Impact: malformed input can cause parser failure, stack exhaustion, or watchdog starvation.

Recommendation: pre-validate the shallow session grammar, enforce a low depth limit, and fuzz nested/allocation-heavy inputs on the target.

## Positive isolation observations

- Diagnostic Bridge CMake does not require CAN, protocol, TWAI, raw replay, control lease, or vehicle-TX components.
- Web capability fields remain hard-coded to `control_scope=0` and `vehicle_tx=false` at:
  - `canview_bridge_web.c:721-722`
  - `canview_bridge_web.c:820`
  - `canview_bridge_web.c:907`
  - `canview_bridge_web.c:943-944`
  - `canview_bridge_web.c:1079-1080`
- Board profile preflight occurs before safe GPIO in `firmware/diagnostic-bridge/main/app_main.c:140-149`.
- No P0 finding observed. No P3 finding observed.

## Verification status

- `git diff --check`: no output.
- Host tests: NOT_RUN.
- ESP-IDF target build: NOT_RUN.
- Slow-client, malformed JSON, WebSocket expiry, Wi-Fi failure cleanup, and watchdog timing tests: NOT_RUN.
- Physical/HIL: `NOT_RUN` — board flash, AP association, phone browser, power/reset/brownout, PSRAM/clock/WDT soak, ESP-NOW, capture, and provisioning.
- CAN TX: `NO-GO`.

## Commands run

All source commands were Git-object reads from `F:\dev\canview`.

```powershell
Get-Date -AsUTC -Format o
git cat-file -e 'b0f23561aeb74eac9e32eaafa02298fd9bfdac64^{commit}'
git cat-file -e '9fe46c753be151e6aa23f0fdc95fd527f86cf82d^{commit}'
git rev-parse --verify 'b0f23561aeb74eac9e32eaafa02298fd9bfdac64^{commit}'
git rev-parse --verify '9fe46c753be151e6aa23f0fdc95fd527f86cf82d^{commit}'
git diff --find-renames --name-status 9fe46c753be151e6aa23f0fdc95fd527f86cf82d b0f23561aeb74eac9e32eaafa02298fd9bfdac64
git diff --find-renames --stat 9fe46c753be151e6aa23f0fdc95fd527f86cf82d b0f23561aeb74eac9e32eaafa02298fd9bfdac64
git diff --find-renames --unified=80 9fe46c753be151e6aa23f0fdc95fd527f86cf82d b0f23561aeb74eac9e32eaafa02298fd9bfdac64 -- <runtime/header paths>
git diff --check 9fe46c753be151e6aa23f0fdc95fd527f86cf82d b0f23561aeb74eac9e32eaafa02298fd9bfdac64
git show -s --format='candidate=%H%nparent=%P%nauthor=%aI%ncommitter=%cI%nsubject=%s' b0f23561aeb74eac9e32eaafa02298fd9bfdac64
git show -s --format='base=%H%nparent=%P%nauthor=%aI%ncommitter=%cI%nsubject=%s' 9fe46c753be151e6aa23f0fdc95fd527f86cf82d
git ls-tree -r --name-only b0f23561aeb74eac9e32eaafa02298fd9bfdac64 ...
git grep -n -E 'cJSON|cjson' b0f23561aeb74eac9e32eaafa02298fd9bfdac64 -- '*.c' '*.h'
git grep -n -E 'canview_can|CAN|can_|raw|replay|control_lease|vehicle_tx|vehicle TX|TX' b0f23561aeb74eac9e32eaafa02298fd9bfdac64 -- firmware/diagnostic-bridge firmware/components
git grep -n -E 'canview_can|canview_protocol|esp_can|twai|esp_now_send|vehicle_tx|control_scope|raw replay|control lease' b0f23561aeb74eac9e32eaafa02298fd9bfdac64 -- firmware/diagnostic-bridge firmware/diagnostic-bridge/CMakeLists.txt
```

Line-numbered `git show` was run against each explicitly listed source file, with range variants for the large web/runtime test files. No filesystem-writing command was run.

Verdict: BLOCK
