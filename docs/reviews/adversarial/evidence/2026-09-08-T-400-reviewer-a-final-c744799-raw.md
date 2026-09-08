# 독립 적대적 리뷰 보고서

- 실행 ID: `codex-thread-review-A-20260908-c744799`
- 시작: `2026-09-08T03:39:26.2970910Z`
- 종료: `2026-09-08T03:44:15.9787431Z`
- Candidate: `c744799f3c9e518fa351e3f46b6c336896346d16`
- Candidate parent: `343250a9af932ef9152eba2bf3a92c14a8904daf`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- 격리: immutable Git object 직접 읽기만 수행. worktree clean 확인. 파일 수정·commit·push·agent 호출·prior report 및 `docs/reviews/**` 내용 참조 없음.

### P1

#### P1-001 — logout/토큰 만료 후 pre-auth deadline이 재설정되지 않음

- 위치:
  - `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:700-714`
  - `:1708-1723`
  - `:1751-1782`
  - `:1940`
- 실패:
  1. `open_connection()`이 최초 연결에만 15초 pre-auth timer를 설정한다.
  2. 정상 인증 시 timer 상태를 지운다.
  3. 같은 keep-alive 소켓에서 `authenticated_and_logout()`이 token을 폐기하지만 pre-auth timer를 다시 설정하지 않는다.
  4. 이후 `pre_auth_deadline_expired()`는 `pre_auth_client_valid == false`일 때 만료되지 않은 것으로 처리한다.
- 영향: 인증이 풀린 기존 소켓이 명시된 15초 pre-auth 제한을 우회할 수 있다. `max_open_sockets = 1`이므로 느린/악성 클라이언트가 유효한 신규 연결을 장시간 차단할 수 있다.
- 권고: logout, expired/invalid token, 인증 상태가 unauthenticated로 전환되는 모든 경로에서 동일한 `enter_pre_auth(client_fd, now)`를 호출하고, keep-alive socket 재인증 시나리오를 실제 HTTP 테스트로 추가한다.

### P2

#### P2-001 — HTTPD `close_fn`에서 fd를 직접 닫아 소켓 소유권이 중복될 위험

- 위치:
  - `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1785-1807`
  - `:1947-1948`
- 실패:
  - `close_session()`이 `httpd_config.close_fn`으로 등록되어 있는데, lock 실패 경로와 정상 경로 모두 `close(client_fd)`를 직접 호출한다.
  - HTTPD가 callback 전후에 fd를 닫는 계약이라면 double-close가 된다. descriptor 재사용과 겹치면 잘못된 연결을 닫을 가능성이 있다.
- 영향: 연결 종료 시 비결정적 fd 오류 또는 다른 연결 종료.
- 권고: SDK의 fd ownership 계약을 명시적으로 확인하고, 소유자가 하나만 닫도록 수정한다. 일반적으로 close callback은 상태 정리만 수행하고 실제 close는 HTTPD에 맡긴다.

#### P2-002 — bounded cleanup 실패 후 활성 자원이 남은 채 watchdog 없이 idle 상태로 진입 가능

- 위치:
  - `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:399-480`
  - `firmware/diagnostic-bridge/main/app_main.c:69-77`
  - `:168-180`
  - `:221-229`
- 실패:
  - web start/stop cleanup이 실패하면 두 번의 retry 후에도 상태를 보존한 채 반환한다.
  - start 실패 후 `web_started`가 false인 상태로 watchdog 없이 idle loop에 남을 수 있다.
- 영향: HTTP/DNS/Wi-Fi 자원 또는 lock이 남은 반초기화 상태가 장치 수명 동안 유지될 수 있다.
- 권고: cleanup 실패 시 재시도 루프를 유지하거나, watchdog reset/명시적 fatal reset으로 전환한다. “서비스가 정리되지 않았지만 idle” 상태를 정상 terminal state로 허용하지 않는다.

### P0

없음.

### P3

없음.

### 검증 상태

- Host build: `NOT_RUN`
- Unit/integration test: `NOT_RUN`
- Live HTTP/slow-client probe: `NOT_RUN`
- Physical/HIL: `NOT_RUN`
- Vehicle CAN TX: `NO-GO`
- Raw replay/control lease 경로는 Diagnostic Bridge 코드에서 확인되지 않았으나, 위 P1/P2 finding과 미실행 HIL gate 때문에 승인 불가.

### 직접 읽은 파일

`canview_bridge_web.c`, `canview_bridge_web.h`, `canview_bridge_web_session.c`, `canview_bridge_web_session.h`, `dns_server.c`, `dns_server.h`, web `CMakeLists.txt`, `canview_bridge_auth.c`, `canview_bridge_auth.h`, auth `CMakeLists.txt`, `app_main.c`, `bridge_bootstrap.c`, `runtime.c`, `board.c`, `board_pins.h`, `bridge_button.c`, `bridge_button.h`, `sdkconfig.defaults`, `health.c`, `pool.c`, `canview_esp_core.h`, `canview_esp_pool.h`, `canview_esp_runtime.h`, `core_runtime.c`, `canview_gpio.c`, communicator `board.c`, communicator `runtime.c`, communicator `board_pins.h`, `tests/bridge_web/test_session.c`, `tests/bridge_web/test_auth.c`, `tests/esp_core/test_bridge_integration.c`, `tests/security/bridge_http.py`, `tests/test_bridge_web_assets.py`.

`test_auth.c`와 `test_bridge_integration.c`는 출력 일부가 truncate되어 전체 테스트 내용을 line-by-line 검토하지 못했다.

### 검토하지 않은 변경 범위

Base→candidate의 나머지 문서, CI, CMake, dependency lock, board generator, SDK allowlist, UI asset, 전체 test/tool 변경 및 `docs/reviews/**` 내용은 본 리뷰의 근거로 사용하지 않았다. Candidate commit 자체는 문서와 review evidence 중심 변경이며, 위 finding은 candidate tree의 누적 firmware source를 직접 검토한 결과다.

### 실제 실행 명령

- Git object 검증 및 commit metadata:
  - `git cat-file -e "<hash>^{commit}"`
  - `git show -s --format=...`
- Base/candidate 및 parent/candidate diff:
  - `git diff --find-renames --name-status`
  - `git diff --find-renames --check`
  - `git diff --find-renames --unified=80`
  - `git diff-tree --no-commit-id --name-status -r`
- Candidate tree source inspection:
  - `git show "<candidate>:<path>" | Select-Object ...`
- Symbol/source search:
  - `git grep -n -E '...' "<candidate>" -- firmware tests ':!docs/reviews/**'`
- Worktree 상태:
  - `git status --porcelain=v1 --untracked-files=all`
- 시간 측정:
  - `Get-Date -AsUTC -Format o`

빌드, CTest, `idf.py`, flash, HIL은 실행하지 않았다.

VERDICT: **BLOCK**
