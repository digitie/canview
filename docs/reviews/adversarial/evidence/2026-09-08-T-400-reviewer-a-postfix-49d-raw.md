# CANView PR #28 정적 적대적 리뷰 원문 보고서

- 실행 ID: `codex-thread-review-A-20260908-49d53f9`
- 시작: `2026-09-08T01:30:09.8342822Z`
- 종료: `2026-09-08T01:48:08.7460630Z`
- PR: `#28`
- Candidate: `49d53f9163ec21fcb358787c8e1448d6839f048a`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Candidate parent: `2ecf5b96aae091756e47b4a17b66f7c2934034de`
- 검토 위치: `C:\Users\digit\.codex\worktrees\32a8\canview`
- 격리: 현재 worktree에서 read-only 정적 검토만 수행. 소스 수정, commit, push, sub-agent 실행 없음.
- `docs/reviews/**`, 다른 reviewer report, integrated report는 읽지 않았고 근거로 사용하지 않음.

## 요약

- 신규 P0: 없음
- 신규 P1: 2건
- 신규 P2: 2건
- 신규 P3: 1건
- 기존 P1/P2: 5건 모두 source-level 기준 수정 확인
- 물리 보드/HIL: `NOT_RUN`
- CAN TX: `NO-GO`

## 기존 finding 재검증

| ID | 상태 | 근거 |
|---|---|---|
| P1-001 | `FIXED` | `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\canview_bridge_web.c:313-355`에서 `request_lock`만 장시간 유지하고 `state_lock`은 요청 검사 후 해제한다. `httpd_req_recv`/response가 `state_lock`을 잡은 채 실행되지 않는다. |
| P1-002 | `FIXED` | `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\main\bridge_bootstrap.c:27-30`의 deferred boot 후 `app_main.c:154-173`에서 NVS/Wi-Fi/HTTP 초기화가 끝난 뒤 watchdog을 arm한다. 기존의 조기 TWDT 구독 문제는 제거됐다. 다만 아래 P1-003이 별도로 남는다. |
| P2-001 | `FIXED` | `canview_bridge_web.c:1885-1903`에서 service expiry 시 active fd를 해제하고 `httpd_sess_trigger_close()`를 호출한다. 단, fd 재사용 경합이 신규 P2-005로 남는다. |
| P2-002 | `FIXED` | `canview_bridge_web.c:380-464`의 ownership flag 기반 cleanup이 HTTPD, Wi-Fi, netif, event loop, semaphore를 역순으로 정리한다. 실제 ESP-IDF 동작은 미실행이다. |
| P2-003 | `FIXED` | `canview_bridge_web.c:761-843`에서 body size와 JSON nesting을 cJSON parse 전에 제한하고, `1012-1020`에서 pre-parser를 먼저 호출한다. target parser stress는 미실행이다. |

## 신규 findings

### P1-003 — deferred boot health epoch가 갱신되지 않아 첫 health step이 정상 startup을 fault시킬 수 있음

- 파일/라인:
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\module\esp_core\health.c:99-102`
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\module\esp_core\health.c:119-142`
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\module\esp_core\health.c:164-174`
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\interface\canview_esp_core.h:12,103-121`
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\main\app_main.c:146-197`
- 실패 조건:
  1. `canview_esp_core_boot_deferred()`가 boot 완료 시점의 `last_progress_us`를 기록한다.
  2. Bridge가 그 뒤 NVS/Wi-Fi/HTTP/DNS 초기화를 수행한다.
  3. `canview_esp_core_arm_watchdog()`는 watchdog만 arm하고 `last_progress_us`를 새로 설정하지 않는다.
  4. 외부 초기화가 `CANVIEW_ESP_CORE_DEADLINE_US = 250000`us를 넘으면 첫 `canview_esp_core_step()`이 `CANVIEW_ESP_FAULT_DEADLINE`을 반환한다.
- 영향: deferred API가 허용하는 정상 외부 초기화 경로가 첫 health step에서 실패해 Bridge 서비스가 유지되지 않을 수 있다.
- 권고:
  - watchdog arm 시 검증된 `now_us`로 health epoch를 재설정한다.
  - 또는 arm 직후 sample/revalidation을 수행한다.
  - 250ms를 초과한 deferred init 후 첫 step이 성공하는 host regression test를 추가한다.

### P1-004 — DNS task가 self-delete 전에 task storage를 종료 완료로 표시함

- 파일/라인:
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\dns_server.c:20-32`
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\dns_server.c:212-215`
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\dns_server.c:237-263`
- 실패 조건:
  - DNS task가 `task = NULL`, `started = false`를 기록한 직후 `vTaskDelete(NULL)`를 호출한다.
  - 동시에 stop 호출자는 `task == NULL`을 종료 완료로 판단하고 `dns_state` 전체를 `memset()`한다.
  - `dns_state`에는 `StaticTask_t task_buffer`와 task stack이 포함된다.
- 영향: self-delete가 실제 완료되기 전에 static TCB/stack/state가 zeroing 또는 재사용될 수 있다. 반복 start/stop 또는 service cleanup 중 RTOS corruption, crash, reset이 가능하다.
- 권고:
  - `task == NULL`을 join 완료 신호로 사용하지 않는다.
  - task가 실행을 완전히 멈춘 뒤 owner가 삭제하도록 owner-side deletion protocol을 사용한다.
  - 또는 별도 completion object와 안전한 task lifetime protocol을 둔다.
  - scheduling을 강제한 반복 start/stop stress test를 추가한다.
  - `volatile` 또는 atomic store만 추가하는 것으로는 self-delete 전 preemption window가 해결되지 않는다.

### P2-005 — service expiry close와 새 client claim 사이에 numeric fd 재사용 경합

- 파일/라인:
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\canview_bridge_web.c:1885-1903`
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\canview_bridge_web.c:313-355`
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\canview_bridge_web.c:1645-1658`
- 실패 조건:
  - expiry가 lock을 해제한 뒤 실제 `httpd_sess_trigger_close()`를 호출한다.
  - 그 사이 새 요청이 같은 numeric fd를 active client로 등록할 수 있다.
  - 이전 session의 close callback이 동일 fd만 비교하므로 새 session을 logout 또는 clear할 수 있다.
- 영향: one-client ownership 및 service availability가 간헐적으로 깨질 수 있다.
- 권고:
  - close pending 상태 또는 generation token으로 새 client claim을 막는다.
  - numeric fd만 비교하지 말고 session identity/generation을 비교한다.

### P2-006 — one-client 정책이 HTTP request 도달 이후에만 적용되어 idle socket이 자원을 고갈시킬 수 있음

- 파일/라인:
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\canview_bridge_web.c:1787-1798`
  - `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\canview_bridge_web.c:313-355`
- 실패 조건:
  - HTTPD가 `max_open_sockets=4`로 설정된다.
  - active-client 검사는 request handler 진입 시점에만 수행된다.
  - 로컬 AP peer가 incomplete-header 또는 idle keep-alive socket을 여러 개 열면 handler 진입 전부터 socket pool을 점유한다.
- 영향: 정상 단일 client가 연결되지 못하는 local availability DoS가 가능하다.
- 권고:
  - accept/open 단계에서 socket 수명과 one-client reservation을 제한한다.
  - incomplete-header flood 및 keep-alive flood test를 추가한다.

### P3-001 — candidate provenance 문서가 현재 candidate hash와 불일치

- 파일/라인:
  - `C:\Users\digit\.codex\worktrees\32a8\canview\docs\resume.md:5,7,25`
  - `C:\Users\digit\.codex\worktrees\32a8\canview\docs\tasks\T-400-diagnostic-bridge-bootstrap.md:40`
- 문제: 현재 candidate는 `49d53f9163ec21fcb358787c8e1448d6839f048a`인데 문서가 이전 `2ecf5b9...`를 기준으로 표시한다.
- 영향: review/build provenance와 resume 상태가 혼동될 수 있다.
- 권고: review 완료 후 candidate hash, task 상태, 다음 gate를 동일한 commit 기준으로 갱신한다.

## 보드·안전·검증 상태

- Bridge safe GPIO 및 board profile은 source-level로 확인했다.
- GPIO external pull, reset/brownout, 실제 rail 상태는 물리 검증하지 않았다.
- watchdog critical section, ESP-IDF HTTPD session semantics, DNS task scheduling은 target runtime에서 검증하지 않았다.
- 실제 ESP32-S3 boot, Wi-Fi, HTTP/DNS, PSRAM, TWDT, reset/brownout, HIL은 `NOT_RUN`.
- CAN 송신, vehicle profile, Communicator STM32 local safety path는 실행하지 않았다.
- CAN TX 상태: `NO-GO`.

## 읽은 파일

스킬 지침:

- `C:\Users\digit\.codex\skills\embedded-architecture\SKILL.md`
- `C:\Users\digit\.codex\skills\embedded-cstyle\SKILL.md`
- `C:\Users\digit\.codex\skills\embedded-rtos-design\SKILL.md`
- `C:\Users\digit\.codex\skills\embedded-isr-design\SKILL.md`
- `C:\Users\digit\.codex\skills\embedded-driver-design\SKILL.md`

문서·하드웨어:

- `C:\Users\digit\.codex\worktrees\32a8\canview\docs\README.md`
- `C:\Users\digit\.codex\worktrees\32a8\canview\docs\resume.md`
- `C:\Users\digit\.codex\worktrees\32a8\canview\docs\architecture\diagnostic-bridge.md`
- `C:\Users\digit\.codex\worktrees\32a8\canview\docs\tasks\T-400-diagnostic-bridge-bootstrap.md`
- `C:\Users\digit\.codex\worktrees\32a8\canview\docs\tasks\T-400a-bridge-core-bench.md`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\docs\web-shell.md`
- `C:\Users\digit\.codex\worktrees\32a8\canview\hardware\bridge\README.md`
- `C:\Users\digit\.codex\worktrees\32a8\canview\hardware\bridge\pinmap.csv`
- `C:\Users\digit\.codex\worktrees\32a8\canview\hardware\bridge\connectivity.json`
- `C:\Users\digit\.codex\worktrees\32a8\canview\hardware\bridge\erc.json`
- `C:\Users\digit\.codex\worktrees\32a8\canview\docs\hardware\r1\README.md`
- `C:\Users\digit\.codex\worktrees\32a8\canview\docs\hardware\r1\verification.md`

Bridge/runtime/source:

- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\main\bridge_bootstrap.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\main\bridge_bootstrap.h`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\main\app_main.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\main\CMakeLists.txt`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\CMakeLists.txt`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\sdkconfig.defaults`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\canview_bridge_web.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\dns_server.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_auth\canview_bridge_auth.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_auth\include\canview_bridge_auth.h`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_web\CMakeLists.txt`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\components\canview_bridge_auth\CMakeLists.txt`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\module\esp_core\health.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\app\esp_core.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\app\pool.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\platform\esp32s3\core_runtime.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\platform\esp32s3\canview_gpio.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\interface\canview_esp_core.h`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\interface\canview_platform_port.h`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\interface\canview_esp_pool.h`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\interface\canview_board.h`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\interface\canview_esp_runtime.h`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\interface\canview_gpio.h`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\bsp\board.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\bsp\board_pins.h`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\diagnostic-bridge\bsp\runtime.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\communicator\esp32\bsp\board.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\communicator\esp32\bsp\board_pins.h`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\communicator\esp32\bsp\runtime.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\communicator\stm32\bsp\board.c`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\communicator\stm32\bsp\board_pins.h`

Build/profile:

- `C:\Users\digit\.codex\worktrees\32a8\canview\CMakeLists.txt`
- `C:\Users\digit\.codex\worktrees\32a8\canview\firmware\boards\boards.json`
- `C:\Users\digit\.codex\worktrees\32a8\canview\tools\generate_boards.py`
- `C:\Users\digit\.codex\worktrees\32a8\canview\tools\check_sdkconfig.py`

## 실행 명령 기록

- `git fetch origin codex/t400-bridge-web-bootstrap`
- `git cat-file -e '49d53f9163ec21fcb358787c8e1448d6839f048a^{commit}'`
- `git cat-file -e '9fe46c753be151e6aa23f0fdc95fd527f86cf82d^{commit}'`
- `git rev-parse 49d53f9163ec21fcb358787c8e1448d6839f048a`
- `git rev-parse 9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- `git show -s --format='candidate=... parent=... author=... committer=... subject=...' 49d53f9163ec21fcb358787c8e1448d6839f048a`
- `git diff --find-renames --name-status 9fe46c753be151e6aa23f0fdc95fd527f86cf82d 49d53f9163ec21fcb358787c8e1448d6839f048a`
- `git diff --find-renames --name-status b0f23561... 49d53f9163ec21fcb358787c8e1448d6839f048a`
- `git diff --find-renames --unified=100 b0f23561... 49d53f9163ec21fcb358787c8e1448d6839f048a -- <검토 대상 경로>`
- `git diff --find-renames --unified=60 b0f23561... 49d53f9163ec21fcb358787c8e1448d6839f048a -- health.c app_main.c bridge_bootstrap.c dns_server.c`
- `git diff --find-renames --unified=8 2ecf5b96aae091756e47b4a17b66f7c2934034de 49d53f9163ec21fcb358787c8e1448d6839f048a -- docs/resume.md docs/tasks/T-400-diagnostic-bridge-bootstrap.md`
- `git diff --check 9fe46c753be151e6aa23f0fdc95fd527f86cf82d 49d53f9163ec21fcb358787c8e1448d6839f048a`
- `git show 49d53f9163ec21fcb358787c8e1448d6839f048a:<path>`를 사용한 관련 C/H/설정/문서 파일 내용 열람
- `git grep -n -E 'canview_esp_core_(boot|boot_deferred|arm_watchdog)|canview_bridge_web_(start|stop|poll)|canview_bridge_dns_(start|stop)' 49d53f... -- '*.c' '*.h'`
- `git grep`를 사용한 watchdog/deferred boot, ISR context, teardown, CAN/control/TX/replay/lease 키워드 검색
- `git ls-tree -r --name-only 49d53f9163ec21fcb358787c8e1448d6839f048a`
- `git ls-tree -r --name-only 49d53f9163ec21fcb358787c8e1448d6839f048a firmware/communicator`
- `Get-Content -Raw -LiteralPath <각 SKILL.md>`
- `Get-Content <SKILL.md> | Measure-Object -Line`
- `git status --short --branch`
- `Get-Date -AsUTC -Format o`

존재하지 않는 경로를 확인한 probe도 기록한다. 해당 명령은 파일을 읽지 못했다.

- `git show 49d53f...:firmware/diagnostic-bridge/main/board_pins.h`
- `git show 49d53f...:firmware/platform/esp32s3/CMakeLists.txt`
- `git show 49d53f...:firmware/diagnostic-bridge/bsp/CMakeLists.txt`
- `git show 49d53f...:firmware/interface/canview_platform_port.h`
- `git show 49d53f...:docs/tasks/T-400-diagnostic-bridge.md`

## 미검증 범위

- 실제 ESP-IDF build, host test, target test, HIL, physical board boot은 실행하지 않았다.
- 실제 watchdog timing, HTTPD fd/session 동작, FreeRTOS preemption window, DNS task deletion은 target에서 확인하지 않았다.
- GPIO electrical safe-state, external pull, reset/brownout CAN gate, rail behavior는 확인하지 않았다.
- 차량 CAN 송신 및 Communicator STM32 final safety path는 실행하지 않았다.

최종 verdict: **BLOCK**
