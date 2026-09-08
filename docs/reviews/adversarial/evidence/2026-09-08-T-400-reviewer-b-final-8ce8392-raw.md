# Reviewer B 원시 독립 리뷰 보고서

- Execution ID: `codex-thread-review-B-20260908-8ce8392`
- Candidate: `8ce8392a0dfdde389f570af56013be802bfb87a8`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- 시작: `2026-09-08T13:10:57.8538843+09:00 이후`
  exact first-call timestamp는 캡처되지 않음
- 종료: `2026-09-08T13:36:37.5957194+09:00`
- 격리: candidate Git archive와 별도 build 디렉터리만 사용. 소스 수정·commit·push 없음.
- Archive: `C:/Users/digit/AppData/Local/Temp/canview-review-B-source-exact-8ce8392-20260908`
- Base→candidate: 47 files, +5339/-45

참고: 초기 broad `rg` 명령이 실수로 `docs/reviews/**` 결과 일부를 출력했으나, 해당 결과는 폐기했고 최종 판단에는 사용하지 않았습니다. 이후 검색은 해당 경로를 제외했습니다.

## Verdict

`CONDITIONAL`

P0/P1은 확인되지 않았습니다. Host/static 범위는 통과했지만 live endpoint, browser runtime, ESP-IDF target build, physical/HIL은 검증되지 않았으므로 완전한 PASS로 판정할 수 없습니다.

## Findings

### P2-01 — Browser 회귀 테스트가 Foundation CI에 연결되지 않음

위치:

- `.github/workflows/foundation.yml:54`
- `tests/ui/diagnostic-browser.cjs:328`
- `tools/ui/check-browser.cjs:1-111`

시나리오:

- Diagnostic Bridge HTML/JS가 변경되어도 Foundation workflow는 Python static contract만 실행합니다.
- Playwright browser suite는 workflow에서 호출되지 않습니다.
- 현재 환경에서도 `node tests/ui/diagnostic-browser.cjs`는 `Cannot find module 'playwright'`로 실패했습니다.

영향:

- offline browser interaction, malformed UI state, stale/gap handling, external request 차단 회귀가 기본 CI에서 검출되지 않습니다.
- source asset generation 통과만으로 browser 동작을 증명할 수 없습니다.

권고:

- lockfile 기반 Playwright 설치와 `tools/ui/check-browser.cjs` 실행을 별도 required job으로 추가하십시오.
- browser dependency가 없는 환경은 PASS가 아니라 명시적 `NOT_RUN`으로 처리하십시오.

## 확인된 수정 및 비-finding

- 기본 CTest UART fault stream은 `--duration-seconds 1`, timeout 60초입니다.
- `CANVIEW_LONG_TESTS=ON`에서만 `uart-fault-stream-24h`가 등록되고 `--duration-seconds 86400`, timeout 90000초를 사용합니다.
- pre-auth 실패 경로는 missing/malformed/wrong-scheme/expired credential 모두 `arm_pre_auth_for_request()`로 연결됩니다.
- DNS descriptor는 task가 소유하고 외부 `shutdown()`을 수행하지 않습니다.
- HTTPD `max_open_sockets=1`, `lru_purge_enable=false` 구성은 single-client 의도와 일치합니다. ESP-IDF 문서도 이 필드를 연결된 client 수 제한으로 설명합니다. [ESP-IDF HTTP Server configuration](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html)
- Bridge source에서 raw replay, CAN TX, control lease, external Wi-Fi connect, NAPT, HTTP PUT/PATCH 경로는 확인되지 않았습니다.

## 검증 결과

통과:

- board generation check
- generated artifact check
- SDK config tests: 13 tests
- repository unittest discovery: 49 tests
- bridge web asset tests: 5 tests
- Bridge HTTP static contract: 42 checks
- Debug build: 103/103
- Debug CTest: 114/114
- Release build: 103/103
- Release CTest: 114/114
- coverage:
  - common core line/function 100%, branch ≥99%
  - STM32 function 100%, line ≥95%, branch ≥90%
  - ESP32 core/pool 및 SDK adapter coverage gate 통과
- negative fixtures
- budget check
- document-link validation
- `git diff --check`
- generated/build artifact drift scan

별도 long-test registration:

- `CANVIEW_LONG_TESTS=ON` configure 성공
- `uart-fault-stream-24h` 등록 및 timeout 90000 확인
- 실제 24시간 실행은 bounded review에서 실행하지 않음

미실행:

- `bridge_http.py --require-live`: endpoint 없음, `NOT_RUN`, exit 1
- browser test: Playwright module 없음
- `idf.py`: 현재 환경에 없음
- ESP-IDF target build/size/warning scan: `NOT_RUN`
- physical/HIL: `NOT_RUN`
- vehicle CAN TX: `NO-GO`

## 실제 읽은 파일

Candidate archive에서 다음을 읽었습니다.

- `docs/README.md`
- `docs/resume.md`
- `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`
- `.github/workflows/foundation.yml`
- `CMakeLists.txt`
- `CMakePresets.json`
- `firmware/boards/boards.json`
- `firmware/communicator/esp32/sdkconfig.defaults`
- `firmware/diagnostic-bridge/sdkconfig.defaults`
- `firmware/diagnostic-bridge/partitions.csv`
- 관련 Diagnostic Bridge project/component/main CMakeLists
- `tools/check_sdkconfig.py`
- `tools/generate_boards.py`
- `tools/check_generated.py`
- `tools/check_esp32_core_coverage.py`
- `tools/check_coverage.py`
- `tools/check_stm32_coverage.py`
- `tests/foundation/test_sdkconfig.py`
- `tests/esp_core/board_fixture.h`
- `tests/esp_core/sdk_fixture.h`
- `tests/esp_core/test_app.c`
- `tests/esp_core/test_bridge_integration.c`
- `tests/esp_core/test_core.c`
- `tests/esp_core/test_pool_threads.c`
- `tests/esp_core/test_runtime.c`
- `tests/esp_core/test_wrong_bsp.c`
- `tests/security/bridge_http.py`
- `tests/test_bridge_web_assets.py`
- `tests/protocol/uart_fault_stream.py`
- `tests/bridge_web/test_auth.c`
- `tests/bridge_web/test_session.c`
- `tests/ui/diagnostic-browser.cjs`
- `tools/ui/check-browser.cjs`
- Bridge web/auth/session/DNS C source 및 headers
- `main/app_main.c`
- `main/bridge_bootstrap.c/.h`
- `ui/diagnostic-web/bridge-shell.html`
- `firmware/diagnostic-bridge/dependencies.lock`
- 관련 generated/documentation 파일

## 주요 실행 명령

```text
python -B tools/generate_boards.py --check
python -B tools/check_generated.py
python -B tests/foundation/test_sdkconfig.py
python -B tests/test_bridge_web_assets.py
python -B -m unittest discover -s tests -p "test_*.py"
python -B tests/security/bridge_http.py
python -B tests/security/bridge_http.py --require-live
python -B tools/check_budgets.py
python -B tools/check_negative_fixtures.py
python -B tools/validate_plan.py
python -B tools/validate_document_links.py
node tests/ui/diagnostic-browser.cjs
cmake configure/build Debug
ctest --test-dir <debug-build> --output-on-failure --no-tests=error
cmake configure/build Release
ctest --test-dir <release-build> --output-on-failure --no-tests=error
cmake configure CANVIEW_LONG_TESTS=ON
ctest -N -V -R '^uart-fault-stream-24h$'
python -B tools/check_coverage.py --build <coverage-build>
python -B tools/check_stm32_coverage.py --build <coverage-build>
python -B tools/check_esp32_core_coverage.py --build <coverage-build>
git diff --check <base> <candidate> -- ':!docs/reviews/**'
git ls-tree -r --name-only <candidate> -- firmware/diagnostic-bridge
```

소스는 변경하지 않았습니다.
