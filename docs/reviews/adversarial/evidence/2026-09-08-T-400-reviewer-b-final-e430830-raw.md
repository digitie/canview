# Reviewer B 원시 독립 리뷰 보고서

- Execution ID: `codex-thread-review-B-20260908-e430830`
- 시작: `2026-09-08T13:00:34.1439633+09:00`
- 종료: `2026-09-08T13:10:57.8538843+09:00`
- Candidate: `e430830e87ff55b1c717f58551e1f9ecf59aa6e0`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- Candidate parent: `41fdc991b2d3747d34a92d91c3adf7f87085114c`
- 격리: immutable Git archive만 사용, 소스 수정·commit·push 없음. 기존 `docs/reviews/**` 보고서는 읽지 않음.
- Base→candidate 변경: 47 files, +5202/-43

## Verdict

`BLOCK`

P0는 확인되지 않았습니다. 그러나 기본 CI의 필수 CTest 경로가 24시간 UART fault test를 300초 timeout으로 실행하도록 구성되어 있어, 필수 검증·증거 경로가 신뢰할 수 없습니다.

## Findings

### P1-01 — 기본 CI의 24시간 UART fault test timeout 불일치

위치:

- [CMakeLists.txt](C:/Users/digit/.codex/worktrees/6153/canview/CMakeLists.txt:194)
- [.github/workflows/foundation.yml](C:/Users/digit/.codex/worktrees/6153/canview/.github/workflows/foundation.yml:22)
- [uart_fault_stream.py](C:/Users/digit/.codex/worktrees/6153/canview/tests/protocol/uart_fault_stream.py:150)

근거:

- CTest test command는 `--duration-seconds 86400`을 사용합니다.
- 동일 test의 CTest timeout은 `300`초입니다.
- 테스트 구현은 duration_seconds * 10 frame을 동기적으로 처리합니다.
- Foundation Windows job timeout은 40분이며 Debug·Release 전체 CTest를 실행합니다.
- `ctest -N`에서 전체 114개 테스트 중 해당 test가 실제 등록됨을 확인했습니다.
- 해당 test를 직접 실행했을 때 100초 이상 완료되지 않아 bounded하게 중단했습니다.
- 나머지 113개 테스트는 통과했지만, 이 test는 의도적으로 실행하지 않았으므로 전체 CTest 통과로 표시할 수 없습니다.

영향:

- 기본 PR CI가 구성된 “24시간” 검증을 실제로 수행하지 못하거나 300초에서 종료될 수 있습니다.
- timeout을 86400초로 단순 변경하면 40분 workflow timeout과 다시 충돌합니다.
- 현재는 필수 CI 증거가 완료되었다고 판정할 수 없습니다.

권고:

- PR 기본 CTest에는 bounded fault test를 두고, 24시간 soak은 명시적·예약된 별도 job으로 분리하십시오.
- 실제 시간 기반 soak인지 virtual frame loop인지 명칭과 acceptance criteria를 일치시키십시오.
- 해당 test가 기본 gate라면 CTest timeout과 workflow timeout을 측정된 실행시간에 맞춰 재설계하고, full run을 실제로 통과시키십시오.

### P2-01 — Authorization parse 실패 시 pre-auth deadline이 재설정되지 않음

위치:

- `canview_bridge_web.c`
- `canview_bridge_web.c`
- `canview_bridge_web.c`
- `canview_bridge_web.c`
- `firmware/diagnostic-bridge/docs/web-shell.md:23`

시나리오:

1. 클라이언트가 정상 인증되어 keep-alive session을 보유합니다.
2. 토큰이 만료됩니다.
3. 클라이언트가 `Authorization` 헤더 없이 요청하거나 malformed/wrong-scheme 헤더를 반복 전송합니다.
4. `parse_bearer()`가 false를 반환하므로 token_authenticated_and_record()가 호출되지 않고 arm_pre_auth_client()도 실행되지 않습니다.
5. 기존 `session.active_client_fd`가 유지된 채 authenticated idle timeout인 약 300초까지 소켓이 점유될 수 있습니다.

영향:

- `max_open_sockets=1`, `lru_purge_enable=false` 설정에서 새 정상 클라이언트의 접속을 최대 수 분간 막을 수 있습니다.
- 코드의 의도와 문서가 설명하는 “토큰 만료·재인증 거부 시 15초 pre-auth deadline 재설정” 동작이 모든 실패 경로에 적용되지 않습니다.

권고:

- Authorization 부재·malformed·wrong scheme·invalid token을 모두 동일한 locked failure path로 보내 해당 FD의 pre-auth deadline을 재설정하십시오.
- 토큰 만료 후 헤더 부재 및 malformed 헤더를 사용하는 keep-alive 회귀시험을 추가하고 15초 내 종료를 검증하십시오.

### P2-02 — DNS socket 중지 경로의 descriptor lifetime race

위치:

- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:132-143`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:205-214`
- `firmware/diagnostic-bridge/components/canview_bridge_web/dns_server.c:240-251`

시나리오:

1. `canview_bridge_dns_stop()`이 `socket_fd`를 복사합니다.
2. DNS task가 해당 descriptor를 닫고 `socket_fd=-1`로 갱신합니다.
3. 다른 네트워크 경로가 descriptor 번호를 재사용합니다.
4. stop 함수의 뒤늦은 `shutdown()`이 DNS socket이 아닌 재사용된 다른 socket에 적용될 수 있습니다.

영향:

- teardown 중 다른 HTTP/network descriptor가 예기치 않게 shutdown될 수 있습니다.
- `volatile`은 descriptor ownership 또는 lifetime 동기화를 보장하지 않습니다.

권고:

- descriptor close는 DNS task가 단독 소유하도록 하고, task notification/event로 종료를 요청한 뒤 task 종료를 기다리십시오.
- 복사한 FD에 대해 task 외부에서 `shutdown()`하는 방식을 제거하거나 명시적인 mutex/reference lifetime protocol을 두십시오.

### P2-03 — 필수 runtime/browser/target evidence가 기본 CI에 연결되지 않음

위치:

- [bridge_http.py](C:/Users/digit/.codex/worktrees/6153/canview/tests/security/bridge_http.py:169)
- [bridge_http.py](C:/Users/digit/.codex/worktrees/6153/canview/tests/security/bridge_http.py:240)
- [.github/workflows/foundation.yml](C:/Users/digit/.codex/worktrees/6153/canview/.github/workflows/foundation.yml:54)
- [diagnostic-browser.cjs](C:/Users/digit/.codex/worktrees/6153/canview/tests/ui/diagnostic-browser.cjs:328)
- `tools/check_esp32_core_coverage.py:1-4`
- `docs/tasks/T-400-diagnostic-bridge-bootstrap.md:65-72`

근거:

- `bridge_http.py`는 `CANVIEW_BRIDGE_URL`이 없으면 live probe를 `NOT_RUN`으로 처리합니다.
- Foundation workflow는 `--require-live` 없이 해당 스크립트를 실행합니다.
- 브라우저 시험은 Playwright 설치가 없어 실행되지 않았습니다.
- ESP32 target build 및 `idf.py` 기반 검증은 이 환경에서 실행되지 않았습니다.
- Coverage tool은 Bridge HTTP/DNS/auth target runtime을 명시적으로 제외합니다.
- task acceptance에는 authenticated HTTP/ESP-NOW flood 및 single-client/bounds runtime 검증이 포함되어 있습니다.

영향:

- static source contract와 host test는 통과하지만, 실제 인증 session·slowloris·second client·DNS flood·WebSocket exchange·target memory/runtime 경계는 증명되지 않았습니다.
- 이 상태에서 Diagnostic Bridge의 runtime safety/security gate를 통과했다고 표시할 수 없습니다.

권고:

- provisioned target을 사용하는 별도 required/HIL workflow를 연결하십시오.
- browser job에서 Playwright 설치를 명시하고 실행 결과를 artifact로 보존하십시오.
- live probe가 없는 경우 성공이 아니라 `NOT_RUN`/conditional 상태로 CI와 release evidence에 남기십시오.

## 확인된 비-finding

이번 후보에서 이전 후보의 HTTPD LRU eviction 문제는 수정되었습니다.

- `canview_bridge_web.c:1973-1978`
- `http_config.max_open_sockets = 1U`
- `http_config.lru_purge_enable = false`

따라서 authenticated owner를 HTTPD가 자동 eviction할 수 있는 구성은 이번 리뷰의 finding으로 올리지 않았습니다.

## 실행한 검증

통과:

- board generation check
- generated artifact check
- SDK config tests: 13 tests
- repository unittest discovery: 49 tests
- Bridge HTTP static contract: 42 checks
- bridge web asset tests: 5 tests
- host Debug configure/build: 103/103
- host CTest: 113/114
  - UART fault stream 1개는 86400초 설정 때문에 bounded review에서 실행하지 않음
- ESP core coverage check
- `git diff --check`
- generated/build artifact drift scan

실패 또는 미실행:

- `bridge_http.py --require-live`: endpoint 미제공으로 live probe `NOT_RUN`
- browser test: `playwright` module missing
- `idf.py`: PATH에 없음
- physical target/HIL: `NOT_RUN`
- vehicle CAN TX: `NO-GO`

## 실제 읽은 파일

immutable candidate archive에서 다음 범위를 직접 읽었습니다.

- `docs/README.md`
- `docs/resume.md`
- `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`
- `.github/workflows/foundation.yml`
- `CMakeLists.txt`
- `CMakePresets.json`
- `tools/environment/foundation-windows.ps1`
- `tools/check_sdkconfig.py`
- `tools/generate_boards.py`
- `tools/generate_bridge_web_assets.py`
- `tools/check_generated.py`
- `tools/check_budgets.py`
- `tools/check_esp32_core_coverage.py`
- `firmware/boards/boards.json`
- `firmware/communicator/esp32/sdkconfig.defaults`
- `firmware/diagnostic-bridge/sdkconfig.defaults`
- `firmware/diagnostic-bridge/partitions.csv`
- `firmware/diagnostic-bridge/CMakeLists.txt`
- Bridge component 및 `main` CMakeLists
- `canview_bridge_web.c/.h`
- `canview_bridge_web_session.c/.h`
- `canview_bridge_auth.c/.h`
- `dns_server.c/.h`
- `main/app_main.c`
- `main/bridge_bootstrap.c/.h`
- `tests/foundation/test_sdkconfig.py`
- `tests/security/bridge_http.py`
- `tests/test_bridge_web_assets.py`
- `tests/bridge_web/test_auth.c`
- `tests/bridge_web/test_session.c`
- `tests/esp_core/*` 8개 파일
- `tests/protocol/uart_fault_stream.py`
- `tests/ui/diagnostic-browser.cjs`
- `ui/diagnostic-web/bridge-shell.html`
- `hardware/bridge/pinmap.csv`
- 관련 generated/docs 파일

## 실행 명령

주요 명령은 다음과 같습니다.

```text
git rev-parse --verify <candidate>
git rev-parse --verify <base>
git show -s --format=fuller <candidate>
git diff --stat <base> <candidate> -- ':!docs/reviews/**'
python -B tools/generate_boards.py --check
python -B tools/check_generated.py
python -B tests/foundation/test_sdkconfig.py
python -B tests/security/bridge_http.py
python -B tests/security/bridge_http.py --require-live
python -B tests/test_bridge_web_assets.py
python -B -m unittest discover -s tests -p "test_*.py"
node tests/ui/diagnostic-browser.cjs
Get-Command idf.py
cmake --preset host-debug
cmake --build --preset host-debug
ctest -N
ctest --test-dir <build> --output-on-failure -I 1,76,1
ctest --test-dir <build> --output-on-failure -I 78,114,1
ctest -N -V -R '^uart-fault-stream$'
python -B tools/check_esp32_core_coverage.py --build <coverage-build>
git diff --check <base> <candidate> -- ':!docs/reviews/**'
git ls-tree -r --name-only <candidate> -- firmware/diagnostic-bridge
```

소스 변경, commit, push는 수행하지 않았습니다.
