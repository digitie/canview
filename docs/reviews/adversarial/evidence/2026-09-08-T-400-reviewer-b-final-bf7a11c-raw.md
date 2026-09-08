# CANView PR #28 — fresh independent hostile review

- Execution ID: `codex-thread-review-B-20260908-bf7a11c`
- Candidate: `bf7a11cd1acc86bc89a644c2855a9838caddf8da`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Start: `2026-09-08T12:09:09.7757141+09:00`
- End: `2026-09-08T12:16:03.4795427+09:00`
- 이전 report 및 `docs/reviews/**` 내용은 읽지 않음
- candidate archive/Git object만 사용
- working tree 수정·commit·push·agent 생성 없음
- Base→candidate diff: 47 files, `+5034/-43` excluding review archive

## 검증 결과

- Host Clang Debug configure/build: `103/103 PASS`
- CTest: `113/114 PASS`
- 제외: `uart-fault-stream` 86,400초 시험
- Python tests: `48 PASS`
- Generator mutation tests: `9 PASS`
- sdkconfig negative tests: `13 PASS`
- Bridge asset tests: `5 PASS`
- Generated output check: `15 PASS`
- HTTP static contract: `33 PASS`
- HTTP `--require-live`: `NOT_RUN`, exit `2`
- ESP32 coverage: `PASS`; portable/SDK fixture only
- Browser runner: `NOT_RUN`; `playwright` module unavailable
- `idf.py`: `NOT_FOUND`
- Target ESP-IDF build, physical/HIL, AP association, DNS/ESP-NOW flood: `NOT_RUN`
- Vehicle CAN TX: `NO-GO`

## Findings

### P1-01 — pre-auth slowloris가 유일한 HTTP socket을 영구 점유할 수 있음

`C:/Users/digit/AppData/Local/Temp/canview-review-B-source-exact-bf7a11c-20260908/firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:314-359`

새 후보는 authenticated activity만 session owner로 기록하도록 수정했지만, `enter_request()`는 pre-auth request를 계속 허용합니다. 동시에 1851의 `max_open_sockets = 1U`, 1856의 `recv_wait_timeout = 5U`만 존재하며 pre-auth 연결에 대한 absolute lifetime 또는 slowloris deadline은 보이지 않습니다.

AP password를 아는 공격자가 HTTP header를 5초 이내 간격으로 조금씩 전송하면 유일한 socket을 계속 점유할 수 있습니다. PIN을 가진 정상 사용자는 `/bootstrap` 또는 `/session`에 접근하지 못합니다. authenticated idle timer는 아직 session owner가 없으므로 이 연결을 닫지 못합니다.

권고: pre-auth connection에 별도 absolute deadline과 incomplete-header/body close 정책을 추가하고, authentication 실패·비정상 pre-auth request를 즉시 종료하십시오. 이 시나리오를 실제 one-socket live test로 추가해야 합니다.

### P2-01 — HTTP live runner가 실제 security/session 경계를 충분히 검증하지 않음

`tests/security/bridge_http.py:132-138,156-212`

local host allowlist는 개선됐지만 live probe는 unauthenticated REST, Origin, oversize/malformed body, query-token WS rejection만 검사합니다. Valid authenticated session, one-client exclusivity, authenticated idle expiry, session close, real WebSocket frame, DNS flood, HTTP/ESP-NOW pressure는 검사하지 않습니다.

특히 query-token WS rejection에서 `HTTP 500`도 허용합니다(`212`). 서버 내부 오류가 발생해도 security rejection 성공으로 기록될 수 있습니다.

권고: expected status를 좁히고, valid auth/WS protocol을 실제로 수행하며, idle/close/one-socket/slowloris/rate/resource 시나리오를 추가하십시오.

### P2-02 — target resource 및 flood acceptance가 CI에서 fail-closed가 아님

`docs/tasks/T-400-diagnostic-bridge-bootstrap.md:64-72`  
`.github/workflows/foundation.yml:212-225`

CI target job은 build, `size-components`, sdkconfig 검사를 수행하지만 heap/PSRAM/stack/socket/DNS/WS budget이나 authenticated HTTP·ESP-NOW flood 시 deadline/fixed-pool 보존을 측정하지 않습니다.

`tools/check_esp32_core_coverage.py:1-5`도 HTTP/DNS/auth target runtime을 검사하지 않는다고 명시합니다.

권고: target job에 명시적 resource/flood thresholds와 fail-closed evidence를 추가하십시오.

### P2-03 — offline browser regression이 CI에 등록되지 않음

`tests/security/bridge_http.py:24,127-129`  
`.github/workflows/foundation.yml:54-60`  
`tests/ui/diagnostic-browser.cjs:328-332`

HTTP runner는 browser test 파일 존재만 확인합니다. Windows CI는 Python runner만 실행하며 Playwright browser suite는 실행하지 않습니다. 실제 실행도 `Cannot find module 'playwright'`로 실패했습니다.

권고: Playwright 의존성을 고정하고 browser runner를 CI/CTest에 등록하십시오.

## 통과한 주요 경계

- Bridge web component가 `canview_bridge_web_session.c`와 auth dependency를 CMake에 포함
- SDK defaults, board profile, pinmap, generated outputs 일치
- JSON 8 KiB, WS frame 512 byte, URI/header bounds
- duplicate/malformed/nested JSON 방어
- DNS fixed buffer, captive-only response, query quota
- `control_scope=0`, `vehicle_tx=false`, raw replay/control lease/CAN TX route 부재
- authenticated activity 갱신이 pre-auth `enter_request()`에서 분리됨
- cleanup 실패 시 HTTP handle/lock을 보존하는 수정 확인
- artifact manifest에 source revision과 provenance digest 추가 확인

## 읽은 파일

`docs/README.md`, `docs/resume.md`, `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`, 관련 diagnostic-bridge architecture/web-shell 문서, `.github/workflows/foundation.yml`, `CMakeLists.txt`, Diagnostic Bridge project/main/component CMake, `sdkconfig.defaults`, `partitions.csv`, `boards.json`, `hardware/bridge/pinmap.csv`, auth/web/session/DNS C 소스와 헤더, `app_main.c`, `bridge_bootstrap.c`, `tools/check_sdkconfig.py`, `generate_boards.py`, `generate_bridge_web_assets.py`, `check_generated.py`, `check_budgets.py`, `check_esp32_core_coverage.py`, `tests/foundation/test_sdkconfig.py`, `tests/foundation/test_generators.py`, `tests/bridge_web/test_session.c`, `tests/test_bridge_web_assets.py`, `tests/security/bridge_http.py`, `tests/ui/diagnostic-browser.cjs`, `tools/ui/check-browser.cjs`, `ui/diagnostic-web/bridge-shell.html`.

BLOCK
