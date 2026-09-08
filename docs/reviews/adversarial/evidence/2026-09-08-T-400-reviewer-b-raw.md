Execution ID: `codex-thread-review-B-20260908-b0f2356`
Start: `2026-09-08T09:10:35.2439993+09:00`
End: `2026-09-08T09:41:12.4294859+09:00`
Candidate: `b0f23561aeb74eac9e32eaafa02298fd9bfdac64`
Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`

## Scope

SDK/config, board profile and generator consistency, CMake/component dependencies, auth/web/DNS security, warning and reproducibility controls, mutation-test coverage, resource exhaustion, CI/evidence integrity, generated-artifact drift, and read-only isolation.

All source evidence came from immutable objects in `F:/dev/canview`. No edits, commits, pushes, builds, tests, agents, or prior reviewer evidence were used. Reviewer A/B evidence files were not read or used.

## Commands run

- `Get-Date -Format o` at start and end.
- Candidate/base `git rev-parse`, `git rev-parse --verify`, `git cat-file -e`, and `git show --no-patch --format=fuller`.
- `git diff --find-renames <base> <candidate> --name-status`.
- `git diff --find-renames <base> <candidate> -- tools/sdkconfig-allowlist/esp32s3-idf-6.0.3.keys`.
- `git ls-tree` for ESP32 test and firmware paths.
- Object-tree `git grep` for `cJSON`.
- Repeated `git show <candidate>:<path>` commands, piped only through PowerShell line-number formatting and bounded output selection.

The first unquoted `git cat-file` attempt was rejected by PowerShell parsing; the quoted verification subsequently succeeded.

## Files read

`AGENTS.md`; `docs/README.md`; `docs/resume.md`; `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`; `docs/architecture/diagnostic-bridge.md`; `docs/architecture/firmware-foundation.md`; `docs/reviews/README.md`; `docs/reviews/adversarial/2026-09-08-T-400.md`; `.github/workflows/foundation.yml`.

`CMakeLists.txt`; `firmware/boards/boards.json`; `hardware/bridge/pinmap.csv`; Diagnostic Bridge CMake, main, BSP, auth, web, DNS, SDK, partition, dependency-lock, README, and web-shell files; `firmware/app/esp_core.c`; `firmware/platform/esp32s3/core_runtime.c`; common ESP32 component CMake files; Communicator and Controller relevant CMake/BSP files.

`tools/check_sdkconfig.py`; `tools/generate_boards.py`; `tools/check_esp32_core_coverage.py`; `tools/check_budgets.py`; `tools/check_generated.py`; `tools/check_negative_fixtures.py`; `tools/generate_bridge_web_assets.py`; `tools/validate_document_links.py`; `tools/sdkconfig-allowlist/esp32s3-idf-6.0.3.keys`; foundation budget files.

`tests/foundation/CMakeLists.txt`; `tests/foundation/test_sdkconfig.py`; `tests/foundation/test_generators.py`; `tests/bridge_web/test_auth.c`; `tests/test_bridge_web_assets.py`; all `tests/esp_core/*`; `ui/diagnostic-web/bridge-shell.html`.

## Findings

### P0

None found in source inspection. This does not waive physical or target gates.

### P1-01 — Bridge integration and coverage do not exercise the submitted Bridge app

Evidence: `CMakeLists.txt:75-90` calls the fixture a real Bridge composition but links `firmware/app/esp_core.c:5-60`, not `firmware/diagnostic-bridge/main/app_main.c`. Coverage repeats the same scope at `tools/check_esp32_core_coverage.py:20-35`. The actual IDF composition is only listed in `firmware/diagnostic-bridge/main/CMakeLists.txt:2-6`.

Failure scenario: regressions in credential loading, web startup ordering, service authentication, or terminal-fault handling in `app_main.c` leave host CTest and coverage unchanged.

Impact: the claimed integration/security evidence can remain green while the actual Diagnostic Bridge boot path is broken.

Recommendation: extract a host-testable Bridge composition function and link it into the fixture, or clearly downgrade the fixture to portable-core coverage and add separate app behavior tests.

### P1-02 — Slow HTTP body can block the owner loop and trigger watchdog failure

Evidence: `canview_bridge_web.c:289-317` takes the global mutex with `portMAX_DELAY`; `:571-600` performs repeated network reads; `:741-766` calls that function before authentication completes; `:1455-1466` permits five-second receive/send waits. The main loop is `app_main.c:190-205`; watchdog subscription/reset is `core_runtime.c:187-200`, with a two-second watchdog at `sdkconfig.defaults:18-23`.

Failure scenario: an AP client sends `POST /api/v1/session` with `Content-Length: 8192` and drips or withholds the body. The HTTP handler holds `web_state.lock`; `canview_bridge_web_poll()` blocks; no core step or watchdog reset occurs.

Impact: unauthenticated network access can cause watchdog reset or terminal timeout, violating bounded-request and flood-preservation requirements.

Recommendation: never hold the owner mutex across socket I/O; use per-request bounded state, finite lock deadlines, and explicit request-duration limits.

### P1-03 — Held/stuck service button reopens the service window indefinitely

Evidence: `canview_bridge_web.c:1516-1533` opens the window while the button remains pressed. Expiry at `:1539-1543` clears only `service_window_open`; it does not clear `button_down` or `button_started_ms`.

Failure scenario: GPIO4 stays low. After the initial three-second hold, the window opens. At ten minutes it expires, then the next 100 ms poll sees the still-held button and opens it again without a release or new hold.

Impact: the physical authorization window can remain permanently open, contrary to `docs/architecture/diagnostic-bridge.md:141` and `:169`.

Recommendation: require a release edge after the hold, re-arm only after release, and clear the hold state on expiry. Add held-button and stuck-input tests.

### P1-04 — WebSocket authentication callbacks are optional but not required by the SDK gate

Evidence: defaults enable all three flags at `sdkconfig.defaults:35-37`, but the URI callbacks are conditional at `canview_bridge_web.c:1385-1398`. `handle_live()` performs its auth check only at `:1171-1188`. The validator’s required set at `tools/check_sdkconfig.py:8-34` omits all three flags, and validation at `:121-135` does not require them. Tests only inspect generated defaults at `tests/foundation/test_sdkconfig.py:225-260`.

Failure scenario: an actual generated config with WebSocket support enabled but pre/post-handshake callbacks disabled passes `check_sdkconfig.py`. The source then has no handshake authentication callback, allowing a client to establish an unauthenticated WebSocket and occupy the single client slot.

Impact: the WebSocket service/PIN boundary is not fail-closed.

Recommendation: require all three flags for `bridge-r1-n8r2` and add negative mutations for each flag being unset.

### P2-01 — Bridge SDK allowlist admits routing-related configuration without forbidding it

Evidence: the candidate allowlist includes `CONFIG_ESP_NETIF_BRIDGE_EN`, `CONFIG_ESP_NETIF_L2_TAP`, `CONFIG_LWIP_FORCE_ROUTER_FORWARDING`, `CONFIG_LWIP_IPV6_FORWARD`, and `CONFIG_LWIP_IP_FORWARD` at `tools/sdkconfig-allowlist/esp32s3-idf-6.0.3.keys:1140,1142,1279,1313,1322`. `tools/check_sdkconfig.py:94-96` admits allowlisted keys, while `:133-135` has no Bridge-specific prohibition for them.

Failure scenario: a config mutant enabling an IP-forwarding option passes the validator despite the no-external-route/NAPT contract.

Impact: configuration drift can evade the fail-closed network policy. The current defaults do not activate these options, but the gate does not prevent activation.

Recommendation: add Bridge-specific forbidden routing/bridge/tap keys and mutation tests.

### P2-02 — CI does not enforce Bridge heap/PSRAM or web-task resource budgets

Evidence: the target job only runs build and artifact checks at `.github/workflows/foundation.yml:210-228`. The budget checker at `tools/check_budgets.py:89-153` validates only the foundation manifest, whose evidence is explicitly synthetic at `config/budgets/foundation.md:3`. The T-400 acceptance requires heap/PSRAM and flood budgets at `docs/tasks/T-400-diagnostic-bridge-bootstrap.md:64-66`.

Failure scenario: HTTP stack, DNS task, JSON arena, buffers, or Wi-Fi allocations exceed runtime margins while the image still fits its flash partition.

Impact: required resource and flood acceptance remains unverified.

Recommendation: run `idf.py size-components`, record target map/stack/heap/PSRAM evidence, and add authenticated/unauthenticated HTTP-DNS stress gates.

### P2-03 — Wi-Fi, HTTP, and DNS lifetime/cleanup is incomplete

Evidence: Wi-Fi starts at `canview_bridge_web.c:1246-1315`; expiry at `:1539-1542` closes only auth state. Startup cleanup at `:342-363` stops HTTP and deletes mutexes but does not stop Wi-Fi or DNS. DNS has only a start path at `dns_server.c:193-208`. The app enters terminal idle at `app_main.c:190-218` without a web stop path. The missing shutdown is also documented at `firmware/diagnostic-bridge/docs/web-shell.md:18`.

Failure scenario: service expiry, URI-registration failure, DNS-start failure, or a later core fault leaves AP/DNS/server resources active or leaves an AP running after partial startup.

Impact: stale attack surface, resource leakage, and no explicit clean recovery state.

Recommendation: add an idempotent stop path covering HTTP clients, DNS task/socket, Wi-Fi, netifs, auth, and state; invoke it on expiry and terminal fault.

### P2-04 — DNS task has no rate or CPU budget

Evidence: `dns_server.c:11-17` sets a priority-4 task; valid packets are processed in the tight loop at `:128-190` without a quota or yield; the task is started permanently at `:193-208`.

Failure scenario: one connected station floods valid DNS queries.

Impact: DNS processing and UDP responses can consume unbounded service time and compete with the owner loop, HTTP server, and watchdog-sensitive work. No flood evidence was executed.

Recommendation: add response rate limiting, bounded per-period work, explicit yielding, and stress tests under watchdog/heartbeat observation.

### P2-05 — Web asset reproducibility is only tested within one interpreter

Evidence: `tools/generate_bridge_web_assets.py:21-23` delegates gzip encoding to the runtime’s gzip/zlib implementation. `tests/test_bridge_web_assets.py:31-41` compares two outputs from the same interpreter only. The Linux portability job at `.github/workflows/foundation.yml:71-87` does not run this cross-platform asset check.

Failure scenario: Python/zlib implementation differences alter gzip header or compressed bytes while the source HTML is unchanged.

Impact: generated asset hashes are not proven reproducible across build environments.

Recommendation: canonicalize the complete gzip representation or compare against a pinned golden digest in Windows and Linux CI.

### P3

None.

## Unreviewed and unrun scope

- Actual ESP-IDF configure/build and target artifact verification.
- Android Chrome/iOS Safari behavior.
- Physical button, AP association, captive DNS, power/reset/brownout, PSRAM, clock, watchdog soak, and channel stability.
- ESP-NOW, capture, throughput pause, authenticated flood, and heartbeat/control-deadline preservation.
- Production provisioning, encryption, vehicle integration, and all physical CAN-path evidence.
- Dynamic HTTP/WS concurrency and malformed/flood mutation execution.

Physical/HIL: `NOT_RUN`
CAN TX: `NO-GO`

Verdict: BLOCK
