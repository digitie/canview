# CANView PR #28 최종 독립 적대적 리뷰

- 실행 ID: `codex-thread-review-A-20260908-9945f90`
- 시작 기준: `2026-09-08T02:21:15.5849843Z`
- 종료: `2026-09-08T02:14:33.6734133Z`
- Candidate: `9945f902e4279b53bc2fb69cdb76ac970058ddce`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Parent: `bc67eebad29d13891d19da29283c5e309f9b490a`
- 격리: candidate/base Git object만 읽음. working tree 상태·untracked 파일·prior/integrated report는 읽지 않음.
- Parent→candidate 변경: `firmware/diagnostic-bridge/main/app_main.c` cleanup retry 처리.
- Base→candidate에는 `.gitattributes`와 기존 source 변경이 포함됨.

## 요약

- P0: 없음
- P1: 없음
- 신규 P2: 1건
- 신규 P3: 1건
- Physical/HIL: `NOT_RUN`
- CAN TX: `NO-GO`

## 주요 검증 결과

- Health epoch 수정 확인:
  `C:\\Users\\digit\\.codex\\worktrees\\32a8\\canview\\firmware\\module\\esp_core\\health.c:140-150`

- DNS task lifetime 수정 확인:
  `C:\\Users\\digit\\.codex\\worktrees\\32a8\\canview\\firmware\\diagnostic-bridge\\components\\canview_bridge_web\\dns_server.c:213-218,254-269`

- `max_open_sockets=1` 확인:
  `...canview_bridge_web.c:1795-1806`

- `session_close_pending`은 정상 close callback이 실행되는 경우 fd 재사용을 막는다:
  `...canview_bridge_web.c:344-349,1893-1909`

- app cleanup retry 추가 확인:
  `C:\\Users\\digit\\.codex\\worktrees\\32a8\\canview\\firmware\\diagnostic-bridge\\main\\app_main.c:69-77,175-183,214-220`

## 신규 findings

### P2-001 — close trigger 실패 시 session이 영구 pending 상태에 고정됨

- 위치:
  - `C:\\Users\\digit\\.codex\\worktrees\\32a8\\canview\\firmware\\diagnostic-bridge\\components\\canview_bridge_web\\canview_bridge_web.c:1893-1910`
  - `...canview_bridge_web.c:344-349`
  - `...canview_bridge_web.c:1652-1665`
- 실패 시나리오:
  1. service expiry가 `session_close_pending=true`를 설정한다.
  2. `httpd_sess_trigger_close()`가 오류를 반환하거나 close callback이 실행되지 않는다.
  3. 반환값이 무시되어 pending 상태가 해제되지 않는다.
  4. 이후 모든 request가 `503 session closing`으로 거부된다.
- 영향: Bridge web service가 재시작 전까지 unavailable 상태가 될 수 있다.
- 권고: trigger 반환값 검사, bounded retry/restart, session generation 확인을 추가한다.

### P3-001 — 문서 candidate provenance가 현재 hash와 불일치

- 위치:
  - `C:\\Users\\digit\\.codex\\worktrees\\32a8\\canview\\docs\\resume.md:5,7,25`
  - `C:\\Users\\digit\\.codex\\worktrees\\32a8\\canview\\docs\\tasks\\T-400-diagnostic-bridge-bootstrap.md:40`
- 문제: 문서가 이전 candidate `2ecf5b9`를 current candidate로 표시한다.
- 영향: review/build provenance가 혼동된다.
- 권고: candidate `9945f902...` 기준으로 resume/task provenance를 갱신한다.

## 미검증 범위

- 실제 ESP-IDF build/CTest/target test를 실행하지 않았다.
- 보드 boot, Wi-Fi/HTTP/DNS runtime, FreeRTOS scheduling stress, watchdog latency는 미검증.
- reset/brownout, GPIO external pull, rail/electrical safe-state와 HIL은 `NOT_RUN`.
- 실제 Communicator STM32 safety path와 차량 CAN은 실행하지 않았다.
- Bridge source에는 CAN TX, raw replay, control lease, vehicle command route가 없음을 정적 검색으로 확인했다.

## 읽은 파일

주요 candidate object 파일:

- `firmware/app/esp_core.c`
- `firmware/module/esp_core/health.c`
- `firmware/module/esp_core/pool.c`
- `firmware/interface/canview_esp_core.h`
- `firmware/interface/canview_esp_pool.h`
- `firmware/interface/canview_esp_runtime.h`
- `firmware/platform/esp32s3/core_runtime.c`
- `firmware/platform/esp32s3/canview_gpio.c`
- `firmware/communicator/esp32/bsp/runtime.c`
- `firmware/diagnostic-bridge/bsp/runtime.c`
- `firmware/diagnostic-bridge/bsp/board.c`
- `firmware/diagnostic-bridge/bsp/board_pins.h`
- `firmware/diagnostic-bridge/main/bridge_bootstrap.c`
- `firmware/diagnostic-bridge/main/app_main.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.h`
- `firmware/diagnostic-bridge/components/canview_bridge_auth/canview_bridge_auth.c`
- 관련 CMake, `sdkconfig.defaults`, board generator/config 파일
- `docs/resume.md`
- `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`

`docs/reviews/**` 파일 내용은 읽지 않았다.

## 실행 명령

- `git cat-file -e` candidate/base 검증
- `git show -s --format=...` candidate metadata
- `git diff --find-renames --name-status base candidate`
- `git diff --find-renames --name-status parent candidate`
- `git diff --find-renames --unified=100 parent candidate -- app_main.c`
- `git diff --find-renames --check base candidate`
- `git show candidate:.gitattributes`
- prioritized source별 line-numbered `git show candidate:<path>`
- source 파일 line count 확인
- `git grep`으로 watchdog, task lifetime, CAN/TX, control, replay, lease, teardown 검색
- `git show candidate:docs/resume.md`
- `git show candidate:docs/tasks/T-400-diagnostic-bridge-bootstrap.md`
- `Get-Date -AsUTC -Format o`

빌드·테스트·flash·HIL은 실행하지 않았다.

최종 verdict: **CONDITIONAL**
