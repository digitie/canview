## 독립 적대적 line-level review

- 실행 ID: `codex-thread-review-A-20260908-e430830`
- 시작: `2026-09-08T04:00:36.0121196Z`
- 종료: `2026-09-08T04:08:19.2542077Z`
- Candidate: `e430830e87ff55b1c717f58551e1f9ecf59aa6e0`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Candidate parent: `41fdc991b2d3747d34a92d91c3adf7f87085114c`
- 격리: immutable Git object 직접 읽기. repository 수정·commit·push·agent 호출 없음. `docs/reviews/**` 내용과 이전 report finding은 사용하지 않음. 종료 시 worktree clean.
- Candidate commit 자체는 문서 변경만 포함하지만, base→candidate의 누적 source tree를 검토함.

### P1

#### P1-001 — 토큰 만료/실패 후 keep-alive socket의 15초 pre-auth deadline 우회

위치:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:375-383`
- `:681-713`
- `:724-747`
- `:1000-1087`
- `:1741-1756`
- `:1973-1983`
- `firmware/diagnostic-bridge/components/canview_bridge_auth/canview_bridge_auth.c:282-307`

실패 시나리오:

1. 최초 연결 시 pre-auth deadline이 설정된다.
2. 인증 성공 시 `clear_pre_auth_client()`로 deadline이 제거된다.
3. 토큰이 자연 만료된 뒤 client가 public `/` 또는 `/api/v1/bootstrap`만 호출하면 token 검사가 실행되지 않는다.
4. 만료된 토큰으로 `DELETE /api/v1/session`을 호출해도 `authenticated_and_logout()`은 token check가 실패한 경로에서 deadline을 재설정하지 않는다.
5. `pre_auth_deadline_expired()`는 `pre_auth_client_valid == false`이면 만료되지 않은 것으로 처리한다.
6. `max_open_sockets = 1`이므로 해당 연결이 신규 client를 계속 막을 수 있다.

영향: 명시된 15초 pre-auth 제한과 single-client availability 경계가 인증 후 socket 재사용 경로에서 무력화된다.

권고: socket별 인증 상태와 pre-auth deadline을 하나의 상태 전이로 관리하고, token expiry·invalid auth·logout·public route 재진입 시 unauthenticated 상태로 원자적으로 전환해 deadline을 재설정한다. 만료된 DELETE와 public root/bootstrap keep-alive 회귀시험을 추가한다.

### P2

#### P2-001 — HTTP/DNS worker task liveness가 main watchdog에 포함되지 않음

위치:

- `firmware/platform/esp32s3/core_runtime.c:69-100`
- `firmware/diagnostic-bridge/main/app_main.c:218-226`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:132-218`
- `dns_server.c:229-237`
- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1979-1983`

실패 시나리오: main task는 `canview_esp_core_step()`으로 계속 watchdog을 갱신하지만, HTTPD task 또는 DNS static task가 내부 오류·deadlock·무한 처리에 빠져도 해당 worker의 heartbeat나 독립 TWDT subscription이 없다.

영향: web/DNS가 정지해도 장치가 healthy로 보고될 수 있으며, local diagnostic service가 복구되지 않는다.

권고: HTTPD/DNS worker별 TWDT subscription 또는 bounded heartbeat supervisor를 추가하고, deadline miss 시 worker restart 또는 전체 web service fail-closed를 수행한다.

### P3

#### P3-001 — pre-auth deadline이 `recv()` 중 최대 HTTPD timeout만큼 초과 가능

위치:

- `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1759-1775`
- `:1979`

실패: deadline은 blocking `recv()` 호출 전에만 검사한다. `recv_wait_timeout = 5U` 상태에서 deadline 직전에 진입하면 15초 제한보다 최대 약 5초 늦게 종료될 수 있다.

권고: 남은 deadline을 socket receive timeout에 반영하거나 nonblocking polling으로 deadline을 직접 강제한다.

### P0

없음.

### 검증 상태

- Candidate immutable source build: `NOT_RUN`
- Candidate immutable unit/integration tests: `NOT_RUN`
- 실제 HTTP/WebSocket probe: `NOT_RUN`
- Physical/HIL, board flash, power/reset/brownout, watchdog soak: `NOT_RUN`
- Vehicle CAN TX: `NO-GO`

시도한 Python 명령은 현재 worktree `HEAD=b0f23561aeb74eac9e32eaafa02298fd9bfdac64`에서 실행되어 candidate 검증으로 집계하지 않았다.

- `py -3 tests/security/bridge_http.py` — 해당 worktree에 파일이 없어 실행 실패
- `py -3 -m unittest tests/test_bridge_web_assets.py` — 3 tests passed지만 candidate object가 아닌 현재 worktree 결과

### 실제 읽은 저장소 파일

`docs/README.md`, `docs/resume.md`, `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`, `firmware/app/esp_core.c`, ESP core health/pool/interface/runtime files, ESP32-S3 platform GPIO/runtime files, Communicator BSP board/runtime/pins/CMake, Diagnostic Bridge bootstrap/BSP/button/app/web/auth/DNS source·header·CMake·sdkconfig, `tests/security/bridge_http.py`, `tests/test_bridge_web_assets.py`, `tests/bridge_web/test_session.c`, `tests/bridge_web/test_auth.c`, `tests/esp_core/test_bridge_integration.c`.

### 실행 명령

- `git rev-parse --verify "<hash>^{commit}"`
- `git show -s --format=...`
- `git status --porcelain=v1 --untracked-files=all`
- `git diff --find-renames --name-status`
- `git diff --find-renames --check`
- `git diff --find-renames --unified=80 <parent> <candidate> -- ...`
- `git diff-tree --no-commit-id --name-status -r`
- `git grep -n -E "..." <candidate> -- firmware tests ':!docs/reviews/**'`
- `git show "<candidate>:<path>" | Select-Object ...`
- `rg --files -g ...`
- `py -3 tests/security/bridge_http.py`
- `py -3 -m unittest tests/test_bridge_web_assets.py`
- `Get-Date -AsUTC -Format o`

### 미검토 범위

변경된 문서·CI·root CMake·generator/tooling·UI asset·dependency lock, 전체 Communicator application/protocol source, ESP core 나머지 test suite, 전체 `tests/`의 비우선순위 파일 및 `docs/reviews/**` 내용은 line-level 검토하지 않았다.

VERDICT: **BLOCK**
