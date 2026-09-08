# 독립 적대적 Post-fix Review Report

- Execution ID: `codex-thread-review-A-20260908-1bf7383`
- 시작: `2026-09-08T04:46:34.2173723Z`
- 종료: `2026-09-08T04:50:41.1363710Z`
- Candidate: `1bf738318b961dc8bfb7e1b36136cfa0dbe341b5`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Candidate parent: `8ce8392a0dfdde389f570af56013be802bfb87a8`
- 격리: immutable Git object/archive만 사용. source, branch, commit, report는 수정하지 않음. `docs/reviews/**`는 읽지 않음. `docs/resume.md`는 AGENTS.md 필수 라우팅 때문에 읽었지만, 포함된 과거 review 결론은 사용하지 않음.
- Worktree: clean

## P0

없음.

## P1-001 — HTTPD heartbeat가 정상적인 pre-auth blocking I/O 때문에 watchdog reset/서비스 거부를 유발함

위치:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:327-350`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:354-386`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1879-1939`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1977-1978`
- `firmware/diagnostic-bridge/components/canview_bridge_web/include/canview_bridge_web.h:22-23`
- `firmware/diagnostic-bridge/sdkconfig.defaults:21`
- `firmware/diagnostic-bridge/main/app_main.c:211-247`

문제:

- HTTPD watchdog reset은 `httpd_queue_work()`로 HTTPD task에 전달되는 callback에서만 수행됩니다.
- pre-auth socket receive는 `SO_RCVTIMEO`를 최대 `CANVIEW_BRIDGE_WEB_PRE_AUTH_TIMEOUT_MS = 15000U`로 설정할 수 있습니다.
- HTTPD task가 slowloris/부분 요청을 처리하며 `recv()`에서 block된 동안 heartbeat callback을 실행하지 못합니다.
- heartbeat timeout은 `1500U`, global TWDT timeout은 `2s`입니다.

실패 시나리오:

1. SoftAP client가 연결 후 HTTP header/body를 매우 느리게 전송합니다.
2. HTTPD task가 `receive_with_pre_auth_deadline()` 내부 `recv()`에서 block됩니다.
3. queued heartbeat가 실행되지 않습니다.
4. 약 1.5초 후 main poll이 heartbeat timeout을 보고 web service를 중단하며, 약 2초에는 TWDT panic/reset까지 발생할 수 있습니다.

영향:

- 인증되지 않은 local client가 Diagnostic Bridge를 반복 재부팅하거나 서비스를 중단시킬 수 있습니다.
- CAN TX 권한 상승은 없지만 진단 availability와 liveness 보장이 깨집니다.

권고:

- HTTPD task가 수행할 수 있는 모든 blocking I/O의 최대 시간을 heartbeat deadline보다 작게 강제하고 실제 HTTPD event-loop 복귀를 검증하십시오.
- 또는 request worker와 liveness supervisor를 분리하고, 정상적인 장기 I/O와 worker 교착을 구분하는 별도 supervision 설계를 사용하십시오.
- slowloris/partial header/body와 send congestion에 대한 target/HIL 시험을 추가하십시오.

## P2-001 — 새 watchdog/DNS lifecycle은 문자열 contract만 검증되고 동작 시험이 없음

위치:

- `tests/test_bridge_web_assets.py:114-137`
- `tests/security/bridge_http.py:74-85`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:327-392`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:137-151`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:303-355`

문제:

- HTTPD heartbeat queue full/timeout, WDT user reset/delete 실패, DNS heartbeat failure/stop race, dynamic `SO_RCVTIMEO` 동작을 실제로 호출하는 host behavioral test가 없습니다.
- 현재 test는 대부분 source substring 존재 여부만 확인합니다.

영향:

- 현재 CI가 pass해도 worker liveness 및 cleanup 회귀를 탐지하지 못합니다.

권고:

- ESP-IDF callback/socket/WDT seam을 mock으로 분리하고 다음을 실행하는 host test를 추가하십시오:
  - heartbeat 성공·queue full·timeout
  - HTTPD stop 후 WDT user 삭제 순서
  - DNS WDT failure 및 bounded stop
  - pre-auth remaining timeout 계산과 exact deadline close

## P3

추가 P3 finding 없음.

## 확인된 경계

- DNS descriptor는 task-local ownership과 bounded `SO_RCVTIMEO`로 관리됨.
- DNS WDT user 및 health lifecycle은 source상 fail-closed 경로를 가짐.
- Diagnostic Bridge의 `control_scope=0`, `vehicle_tx=false`; raw replay/control lease 경로는 확인되지 않음.
- Communicator board는 `RUN_OK` low-first와 safe output ordering을 유지함.
- GPIO external pull 및 brownout/reset safety는 source만으로 확정할 수 없음.

## 실제 읽은 파일

필수 문서:

- `docs/README.md`
- `docs/resume.md`
- `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`

주요 source:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.h`
- `firmware/diagnostic-bridge/components/canview_bridge_web/include/canview_bridge_web.h`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web_session.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/include/canview_bridge_web_session.h`
- `firmware/diagnostic-bridge/main/app_main.c`
- `firmware/diagnostic-bridge/main/bridge_bootstrap.c`
- `firmware/diagnostic-bridge/main/bridge_bootstrap.h`
- `firmware/diagnostic-bridge/bsp/board.c`
- `firmware/diagnostic-bridge/bsp/runtime.c`
- `firmware/diagnostic-bridge/sdkconfig.defaults`
- `firmware/platform/esp32s3/core_runtime.c`
- `firmware/platform/esp32s3/canview_gpio.c`
- `firmware/app/esp_core.c`
- `firmware/module/esp_core/health.c`
- `firmware/interface/canview_esp_core.h`
- `firmware/interface/canview_esp_runtime.h`
- `firmware/interface/canview_gpio.h`
- `firmware/interface/canview_board.h`
- `firmware/communicator/esp32/bsp/board.c`
- `firmware/communicator/esp32/bsp/runtime.c`
- `firmware/communicator/esp32/bsp/board_pins.h`

검증/test:

- `tests/security/bridge_http.py`
- `tests/test_bridge_web_assets.py`
- candidate archive의 `tests/` 전체 discoverable Python tests

## 실행한 명령과 결과

- candidate/base `git rev-parse`, commit metadata: PASS
- `git status --porcelain`: clean
- `git diff-tree`, base→candidate diff, parent→candidate diff: PASS
- `git grep` watchdog/HTTPD/DNS/pre-auth/read-only 경계 검색: 완료
- `git diff --check`: PASS
- `git archive`로 candidate 임시 archive 생성: PASS
- `py -3 tests/security/bridge_http.py`: `PASS`, 48 checks; live probe `NOT_RUN`
- `py -3 -m unittest tests/test_bridge_web_assets.py`: 5 tests PASS
- `py -3 -m unittest discover -s tests -p "test_*.py"`: 49 tests PASS
- `cmake -S . -B build-review ...`: 실행 불가, 현재 환경에서 `cmake` command not found
- 최종 worktree status: clean

## 미검토/미실행 범위

- 실제 ESP-IDF target build 및 flash
- HTTPD/DNS runtime fault injection
- physical GPIO pull, reset, brownout, watchdog recovery
- SoftAP/phone/browser/HIL/network flood 시험
- PSRAM/heap/stack soak
- 실제 차량 bus/capture/TX

Physical/HIL: `NOT_RUN`
Vehicle CAN TX: `NO-GO`

## VERDICT

`BLOCK`
