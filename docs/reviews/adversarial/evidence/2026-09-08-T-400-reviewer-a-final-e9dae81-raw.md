# 독립 적대적 Post-fix Review Report

- Execution ID: `codex-thread-review-A-20260908-e9dae8`
- 시작: `2026-09-08T05:05:09.3020624Z`
- 종료: `2026-09-08T05:08:12.7197336Z`
- Candidate: `e9dae8119b09aa66ca77eded3741bd31f41b1199`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Candidate parent: `7142f4b9ac08547f08a57013dfddc5453c3eaa14`
- Candidate delta: `tools/build_docs.py`의 API count `30 → 32`
- 격리: immutable Git object와 임시 archive만 사용. source, branch, commit, report는 수정하지 않음. `docs/reviews/**`는 읽지 않음.
- Worktree: clean

## P0

없음.

## P1-001 — HTTPD heartbeat starvation이 정상적인 blocking I/O를 서비스 중단/reset으로 승격시킴

위치:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:327-357`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:359-398`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1911-1968`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:2166-2167`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:296-310`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1683-1697`
- `firmware/diagnostic-bridge/components/canview_bridge_web/include/canview_bridge_web.h:22-24`
- `firmware/diagnostic-bridge/sdkconfig.defaults:21`
- `firmware/diagnostic-bridge/main/app_main.c:211-247`

문제:

- HTTPD liveness ACK는 `httpd_queue_work()`로 HTTPD task에 전달된 callback에서만 갱신됩니다.
- pre-auth `recv()`는 1초 timeout으로 직접 WDT를 reset하지만, queued heartbeat ACK는 갱신하지 않습니다.
- slowloris client가 1초보다 짧은 간격으로 byte를 계속 보내면 HTTPD task는 request parser/receive 경로에 머물러 heartbeat callback을 실행하지 못합니다.
- authenticated `httpd_resp_send()`와 `httpd_ws_send_frame()`에는 직접 checkpoint가 없고, `send_wait_timeout=5U`인데 heartbeat deadline은 `1500U`입니다.
- main poll은 heartbeat timeout을 `ESP_ERR_TIMEOUT`으로 처리하고 service loop를 종료합니다. 이후 cleanup과 core watchdog 미갱신으로 재부팅까지 이어질 수 있습니다.

실패 시나리오:

1. SoftAP client가 partial pre-auth header/body를 1초 이내 간격으로 계속 전송합니다.
2. `receive_with_pre_auth_deadline()`은 매번 WDT user만 reset합니다.
3. HTTPD queued heartbeat ACK는 갱신되지 않습니다.
4. 약 1.5초 후 main task가 heartbeat timeout을 보고 web service를 중단합니다.
5. 별도로 인증된 client가 응답 수신을 멈추면 `httpd_resp_send()`/WebSocket send가 최대 5초 blocking되어 같은 결과가 발생합니다.

영향:

- local client가 Diagnostic Bridge web service를 반복 중단하거나 장치를 재부팅시킬 수 있습니다.
- CAN TX 권한 상승은 없지만 진단 availability와 watchdog/lifecycle 계약이 깨집니다.

권고:

- receive/send 진행을 HTTPD liveness ACK에도 반영하거나, queued callback ACK와 request I/O progress를 동일한 worker-progress 기준으로 통합하십시오.
- 모든 blocking send/receive 구간의 최악시간을 heartbeat deadline보다 작게 만들고, slowloris·blocked receiver·partial request 시험을 target에서 수행하십시오.
- 정상 I/O와 실제 HTTPD task 교착을 구분할 수 있도록 heartbeat supervisor 설계를 재검토하십시오.

## P2-001 — 새 HTTPD/DNS watchdog lifecycle의 behavioral test 부재

위치:

- `tests/test_bridge_web_assets.py:114-137`
- `tests/security/bridge_http.py:74-85`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:327-398`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:137-151`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:303-355`

문제:

- 테스트는 heartbeat, WDT user, `SO_RCVTIMEO`, DNS health 문자열의 존재를 주로 검사합니다.
- heartbeat queue starvation/timeout, WDT user delete 실패, DNS task stop race, blocked send/receive를 실제 호출하는 host behavioral test가 없습니다.

영향:

- 현재 host tests가 pass해도 핵심 liveness/cleanup 회귀를 탐지하지 못합니다.

권고:

- ESP-IDF socket/WDT/HTTPD seam을 mock으로 만들고 heartbeat success/timeout, cleanup ordering, DNS failure/stop, send/receive deadline 시험을 추가하십시오.

## P3

추가 P3 finding 없음.

## 추가 공격 시나리오 검토 결과

추가 finding 없음:

- malformed/oversize JSON 및 WebSocket frame: 고정 상한과 host contract test 확인.
- DNS descriptor ownership/stop: task-local descriptor, 1초 receive timeout, bounded stop 확인.
- token expiry/logout/service-window close: auth reconcile 및 pre-auth re-arm 경로 확인.
- pool exhaustion/stale token: 16-slot fixed pool, generation/token ownership 확인.
- wrong-board profile: Bridge/Communicator preflight가 GPIO/runtime open보다 선행.
- ISR/reentry/owner: runtime owner check, ISR rejection, callback reentry latch 확인.
- Diagnostic Bridge raw replay/control lease/vehicle TX: source boundary에서 확인되지 않음.

남은 불확실성:

- 실제 ESP-IDF 6.0.3 HTTPD work queue scheduling과 `httpd_resp_send()` blocking behavior는 target/HIL에서 검증하지 못했습니다.
- 따라서 P1은 source contract와 timing 관계에 근거한 정적 finding입니다.

## 실제 읽은 파일

필수 문서:

- `docs/README.md`
- `docs/resume.md`
- `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`

Runtime/core:

- `firmware/app/esp_core.c`
- `firmware/module/esp_core/health.c`
- `firmware/module/esp_core/pool.c`
- `firmware/interface/canview_esp_core.h`
- `firmware/interface/canview_esp_pool.h`
- `firmware/interface/canview_esp_runtime.h`
- `firmware/platform/esp32s3/core_runtime.c`

Board/isolation:

- `firmware/communicator/esp32/bsp/runtime.c`
- `firmware/communicator/esp32/bsp/board.c`
- `firmware/communicator/esp32/bsp/board_pins.h`
- `firmware/diagnostic-bridge/bsp/runtime.c`
- `firmware/diagnostic-bridge/bsp/board.c`
- `firmware/diagnostic-bridge/main/bridge_bootstrap.c`
- `firmware/diagnostic-bridge/main/bridge_bootstrap.h`
- `firmware/platform/esp32s3/canview_gpio.c`

Bridge/auth/web:

- `firmware/diagnostic-bridge/components/canview_bridge_auth/canview_bridge_auth.c`
- `firmware/diagnostic-bridge/components/canview_bridge_auth/include/canview_bridge_auth.h`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.h`
- `firmware/diagnostic-bridge/components/canview_bridge_web/include/canview_bridge_web.h`
- `firmware/diagnostic-bridge/components/canview_bridge_web/include/canview_bridge_web_session.h`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web_session.c`
- `firmware/diagnostic-bridge/main/app_main.c`
- `firmware/diagnostic-bridge/sdkconfig.defaults`

Tooling/tests:

- `tools/build_docs.py`
- `tests/security/bridge_http.py`
- `tests/test_bridge_web_assets.py`
- candidate archive에서 `tests/`의 discoverable `test_*.py` 전체

## 실행한 명령과 결과

- candidate/base `git rev-parse`, commit metadata: PASS
- `git status --porcelain`: clean
- `git diff-tree`, parent→candidate diff, base→candidate diff: PASS
- `git grep` 및 line-numbered `git show`: 완료
- `git diff --check`: PASS
- `git archive` candidate 임시 archive 생성: PASS
- `py -3 tests/security/bridge_http.py`: 48 checks PASS; live probe `NOT_RUN`
- `py -3 -m unittest tests/test_bridge_web_assets.py`: 5 tests PASS
- `py -3 -m unittest discover -s tests -p "test_*.py"`: 49 tests PASS
- `py -3 tools/build_docs.py`: FAIL, `doxygen` executable unavailable
- `cmake -S . -B build-review -G Ninja -DCMAKE_BUILD_TYPE=Release`: FAIL, `cmake` executable unavailable
- read-only boundary search: source clean
- final worktree status: clean

## 미검토/미실행 범위

- 실제 ESP-IDF target build/flash
- HTTPD/DNS fault injection 및 blocked send/receive HIL
- physical GPIO pull, reset, brownout, watchdog recovery
- SoftAP/phone/browser/network flood 시험
- PSRAM/heap/stack soak
- 실제 차량 CAN/capture/TX

Physical/HIL: `NOT_RUN`
Vehicle CAN TX: `NO-GO`

## VERDICT

`BLOCK`
