# Independent Hostile Review Report

- Execution ID: `codex-thread-review-A-20260908-8ce8392`
- Started: `2026-09-08T04:25:40.4682334Z`
- Ended: `2026-09-08T04:28:44.2693220Z`
- Candidate: `8ce8392a0dfdde389f570af56013be802bfb87a8`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Isolation: immutable Git objects only; no files, commits, branches, reports, or prior review contents modified/read.
- Worktree status: clean.

## Findings

### P0

없음.

### P1

없음.

Post-fix auth lifecycle 검토 결과, token expiry/reconcile 및 invalid request의 pre-auth re-arm 경로는 연결되어 있습니다:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:352-362`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:721-797`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:2074-2161`
- `firmware/diagnostic-bridge/components/canview_bridge_auth/canview_bridge_auth.c:138-157`

### P2-001 — HTTP/DNS worker watchdog coverage 부재

- 위치:
  - `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:131-213`
  - `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:215-230`
  - `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:2036`
  - `firmware/platform/esp32s3/core_runtime.c:69-100`
- 문제: 확인된 watchdog 등록은 core owner task 중심이며, HTTPD worker와 static DNS worker에 worker-specific watchdog subscription 또는 supervisor heartbeat가 없습니다.
- 실패 시나리오: main service loop는 계속 정상적으로 watchdog을 feed하지만 HTTP/DNS task가 교착·무응답 상태가 되어 진단 웹 인터페이스가 영구 정지할 수 있습니다.
- 영향: 진단 availability 및 운영자 관찰 기능 상실. 차량 CAN TX 권한을 직접 확장하지는 않지만 안전 진단 표면의 stale 상태를 숨길 수 있습니다.
- 권고: HTTPD/DNS worker heartbeat 또는 개별 TWDT subscription을 추가하고, deadline 초과 시 서버 teardown/restart 및 fail-closed 상태 전환을 supervisor가 수행하도록 하십시오.

### P3-001 — pre-auth hard deadline이 `recv_wait_timeout`만큼 초과 가능

- 위치:
  - `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1794-1816`
  - `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1812-1834`
  - `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:2032`
- 문제: deadline은 blocking `recv()` 전에 한 번만 검사되며, HTTPD receive timeout은 `5U`입니다.
- 실패 시나리오: deadline 직전에 `recv()`가 시작되면 연결 종료가 최대 약 5초 늦어질 수 있습니다.
- 권고: 남은 deadline을 socket receive timeout에 반영하거나 non-blocking/poll 방식으로 deadline을 강제하십시오.

## 수정되어 결함으로 승격하지 않은 항목

- DNS socket descriptor ownership/reuse race: task-local descriptor와 bounded receive timeout으로 수정됨.
- HTTP close callback double-close 의심: 현재 callback이 close ownership을 명시적으로 보유함.
- startup/teardown cleanup: bounded retry 및 restart fail-closed 경로 확인.
- Diagnostic Bridge raw CAN TX/control lease/replay: 검토 범위에서 활성화 경로 확인되지 않음.

## 실행한 명령

- candidate/base object 검증 및 commit metadata 확인
- `git status --porcelain=v1 --untracked-files=all`
- `git diff-tree --name-status`
- base→candidate 및 parent→candidate `git diff`
- `git diff --check`
- candidate source에 대한 `git grep` 및 `git show`
- watchdog, socket ownership, pre-auth deadline, HTTPD/DNS startup 경로 검색

## 읽은 주요 파일

`docs/README.md`, `docs/resume.md`, `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`

Diagnostic Bridge:

- `canview_bridge_auth.c/.h`
- `canview_bridge_web.c/.h`
- `canview_bridge_web_session.c/.h`
- `dns_server.c`
- `app_main.c`
- `bridge_bootstrap.c/.h`
- `bsp/runtime.c`
- `bsp/board.c/.h/.pins.h`
- `bridge_button.c/.h`
- 관련 `CMakeLists.txt`, `sdkconfig.defaults`

Runtime/platform:

- `firmware/app/esp_core.c`
- `firmware/module/esp_core/health.c`
- `firmware/module/esp_core/pool.c`
- `firmware/platform/esp32s3/core_runtime.c`
- `firmware/platform/esp32s3/canview_gpio.c`
- 관련 interface/header 파일

Communicator:

- `firmware/communicator/esp32/bsp/board.c`
- `runtime.c`
- `board_pins.h`

Tests:

- `tests/bridge_web/test_auth.c`
- `tests/bridge_web/test_session.c`
- `tests/test_bridge_web_assets.py`
- `tests/security/bridge_http.py`
- `tests/esp_core/test_bridge_integration.c`

## 검증 한계

- Candidate를 현재 worktree에 materialize하지 않았으므로 candidate 대상 build/test는 실행하지 않았습니다.
- Physical/HIL: `NOT_RUN`
- 실제 GPIO external pull, brownout/reset, watchdog recovery, network fault: `NOT_RUN`
- Vehicle CAN TX: `NO-GO`
- `docs/reviews/**`의 기존 보고서 내용은 읽지 않았습니다.
- 전체 저장소의 비관련 UI·문서·tooling과 실제 차량 evidence는 이 검토 범위에 포함되지 않았습니다.

## VERDICT

`BLOCK`
