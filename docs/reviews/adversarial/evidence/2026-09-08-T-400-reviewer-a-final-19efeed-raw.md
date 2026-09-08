# Raw hostile review report

- Execution ID: `codex-thread-review-A-20260908-19efeed`
- Candidate requested: `19efeed895e1885a52ab9023bf6e5095c358cb3`
- Actual immutable candidate: `19efeedd895e1885a52ab9023bf6e5095c358cb3`
  The supplied hash omitted one `d`; verified via remote branch/PR object.
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Review window: `2026-09-08T02:41:58.9515246Z`–`2026-09-08T02:48:30.8399749Z`
- Isolation: candidate/base Git objects only; no working-tree inspection, no `docs/reviews/**` content, no edits, commits, pushes, agents, builds, or tests.

## Findings

### P2-001 — New idle-session lifecycle has no behavioral regression coverage

Files:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:316-370`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1931-1948`
- `tests/security/bridge_http.py:41-105,129-188`

The new 5-minute idle timeout and forced-close path are validated only by source-string checks and optional unauthenticated live probes. No test drives:

- activity refresh across HTTP/WebSocket requests;
- idle expiration;
- `httpd_sess_trigger_close()` followed by `close_session()`;
- clearing `active_client_fd`, `session_close_pending`, activity state, and auth;
- close-trigger failure followed by service teardown.

A regression could cause false WebSocket/client disconnects or leave the single-client service stuck returning `503`, while the current static checks still pass.

Recommendation: add an injectable-clock/ESP-IDF fixture covering request refresh, WebSocket activity, idle expiry, service-window expiry, callback cleanup, and close-trigger failure.

### P3-001 — Candidate/status documentation is stale

Files:

- `docs/resume.md:5,7,25`
- `docs/tasks/T-400-diagnostic-bridge-bootstrap.md:34,40`

The candidate still identifies `2ecf5b9` and `19a42339`, while the reviewed immutable candidate is `19efeedd895e1885a52ab9023bf6e5095c358cb3`.

Impact: reviewers and release gates can attribute validation and status to the wrong source state.

Recommendation: update the current candidate hashes and review status after this report.

## P0/P1

No P0 or P1 source findings were identified in this isolated review.

The candidate’s `httpd_sess_trigger_close()` failure path now calls `canview_bridge_web_stop()`, and `app_main()` performs cleanup on loop exit.

## Commands used

- `git cat-file -e '19efeedd895e1885a52ab9023bf6e5095c358cb3^{commit}'`
- `git cat-file -e '9fe46c753be151e6aa23f0fdc95fd527f86cf82d^{commit}'`
- `git show -s --format=... 19efeedd895e1885a52ab9023bf6e5095c358cb3`
- `git ls-remote origin refs/heads/codex/t400-bridge-web-bootstrap`
- `git diff --find-renames --name-status 9fe46c753be151e6aa23f0fdc95fd527f86cf82d 19efeedd895e1885a52ab9023bf6e5095c358cb3`
- `git diff --find-renames --check 9fe46c753be151e6aa23f0fdc95fd527f86cf82d 19efeedd895e1885a52ab9023bf6e5095c358cb3`
- `git diff --find-renames --unified=35 9945f902e4279b53bc2fb69cdb76ac970058ddce 19efeedd895e1885a52ab9023bf6e5095c358cb3 -- <web files>`
- `git show <candidate>:<file>`
- `git grep -n -E 'raw replay|control lease|vehicle TX|CAN TX|httpd_sess_trigger_close|last_activity_ms|session_close_pending' <candidate> -- firmware tests ':!docs/reviews/**'`

## Unreviewed / not run

ESP-IDF build, host tests, live HTTP/WebSocket probe, board flash, AP association, browser/phone testing, watchdog reset, heap/PSRAM soak, GPIO/electrical behavior, reset/brownout, and HIL were not run.

Physical/HIL: `NOT_RUN`
Vehicle CAN TX: `NO-GO`

## Verdict

**CONDITIONAL**

No P0/P1 source blocker was found, but P2 behavioral coverage and stale candidate documentation must be addressed before treating the candidate as review-closed.
