# CANView T-400 독립 적대적 리뷰 raw report

- 실행 ID: `CANVIEW-T400-B2-20260908-1BF7383`
- 시작: `2026-09-08T13:56:11.9283395+09:00` — immutable archive 생성 시각
- 종료: `2026-09-08T14:01:45.7992991+09:00`
- Candidate: `1bf738318b961dc8bfb7e1b36136cfa0dbe341b5`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- 격리: filtered Git archive `C:/Users/digit/AppData/Local/Temp/canview-review-B2-source-exact-1bf7383-20260908`
- `docs/reviews/**`: archive 생성 단계에서 제외 및 존재하지 않음
- source/commit/branch/report 변경: 없음
- 원 worktree: clean

주의: 초기 탐색에서 잘못된 glob으로 `docs/reviews/**` 매칭 줄이 출력되어 해당 시도를 폐기했습니다. 이후 B2 archive를 새로 생성해 검토를 재실행했으며, 이전 review 결과는 판단에 사용하지 않았습니다.

## 검토 파일

정확히 48개를 읽었습니다.

`package.json`, `package-lock.json`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`, `docs/development/windows.md`, `.github/workflows/foundation.yml`, `CMakeLists.txt`, `CMakePresets.json`, `firmware/boards/boards.json`, `firmware/communicator/esp32/sdkconfig.defaults`, `firmware/diagnostic-bridge/sdkconfig.defaults`, `firmware/diagnostic-bridge/partitions.csv`, `firmware/diagnostic-bridge/CMakeLists.txt`, `firmware/diagnostic-bridge/main/CMakeLists.txt`, `tools/check_sdkconfig.py`, `tools/generate_boards.py`, `tools/check_generated.py`, `tools/check_esp32_core_coverage.py`, `tests/foundation/test_sdkconfig.py`, `tests/security/bridge_http.py`, `tests/test_bridge_web_assets.py`, `tests/protocol/uart_fault_stream.py`, `tests/ui/diagnostic-browser.cjs`, `tools/ui/check-browser.cjs`, Bridge web/auth/DNS C/H source 11개, Bridge web/auth test 2개, `ui/diagnostic-web/bridge-shell.html`, `hardware/bridge/pinmap.csv`, `tests/esp_core/**` 8개.

## 명령 및 결과

- `git archive ...` filtered candidate: PASS
- candidate/base SHA 확인: PASS
- `git diff --check base candidate -- . ':(exclude)docs/reviews/**'`: PASS
- diff: 49 files, `+5690/-45`
- `python -B tools/generate_boards.py --check`: PASS
- `python -B tools/check_generated.py`: PASS — protocol header, 15 golden, 6 malformed, 4 compatibility, pairing
- `python -B tests/foundation/test_sdkconfig.py`: PASS — 13 tests
- `python -B tests/test_bridge_web_assets.py`: PASS — 5 tests
- `python -B -m unittest discover -s tests -p "test_*.py"`: PASS — 49 tests
- `python -B tools/check_budgets.py`: PASS
- `python -B tools/check_negative_fixtures.py`: PASS
- `python -B tests/security/bridge_http.py`: PASS static 48 checks; live `NOT_RUN`
- `python -B tests/security/bridge_http.py --require-live`: static PASS, live `NOT_RUN`, exit 2
- `python -B tests/protocol/uart_fault_stream.py --seed 1 --duration-seconds 1`: PASS
- `npm ci --ignore-scripts --fund=false --audit=false`: PASS
- package/lock 검증: PASS — lockfile v3, Playwright `1.63.0`, `playwright-core 1.63.0`, integrity 존재
- `node tools/ui/check-browser.cjs`: PASS — 74 checks, errors 0, external requests 0
- `node tests/ui/diagnostic-browser.cjs`: PASS — 10 checks, page errors 0, external requests 0
- Bridge C/H 금지 심볼 검색: PASS — raw replay, CAN TX, control lease, Wi-Fi connect, NAPT, PUT/PATCH 없음
- checked-in Bridge build artifact 검색: PASS — 없음
- `validate_document_links.py`: filtered archive에서 review 문서가 의도적으로 제외되어 62 missing targets; source failure로 판정하지 않음
- CMake/CTest/Ninja/ESP-IDF 확인: 모두 `NOT_FOUND`

## Finding

### B2-P1-01 — PR target artifact가 candidate commit에 고정되지 않음

- 파일: `.github/workflows/foundation.yml:128`, `285-287`, `311`
- 시나리오: workflow가 `pull_request`에서도 실행되지만 `actions/checkout`에 `ref`가 없습니다. 이후 manifest는 `git rev-parse HEAD`만 `sourceRevision`으로 기록합니다.
- 영향: PR target job에서는 candidate HEAD가 아닌 GitHub synthetic merge commit에서 target binary가 빌드될 수 있습니다. Manifest는 merge ref만 기록하므로 artifact가 검토 candidate `1bf7383...`에서 생성됐다는 사실을 독립적으로 입증하지 못합니다. target-build evidence audit gate를 안전하게 닫을 수 없습니다.
- 근거: `actions/checkout`은 PR HEAD를 명시하려면 `ref: ${{ github.event.pull_request.head.sha }}`를 사용하도록 문서화하고 있습니다. [actions/checkout README](https://github.com/actions/checkout)
- 권고: PR에서는 `github.event.pull_request.head.sha`를 명시적으로 checkout하거나, manifest에 PR head SHA·base SHA·merge SHA를 모두 기록하고 기대 candidate SHA와 일치하지 않으면 job을 실패시키십시오.

## 미검토/미실행 범위

- STM32/ESP32/Bridge/Controller 실제 target build, size, warning scan: `NOT_RUN` — CMake, Ninja, `idf.py` 부재
- Release host build/CTest: `NOT_RUN`
- 24시간 UART stream: 실제 실행 `NOT_RUN`; 등록만 확인
  - 기본: 1초, timeout 60초
  - long: 86400초, timeout 90000초
- Bridge 실제 HTTP/WebSocket endpoint probe: `NOT_RUN` — `CANVIEW_BRIDGE_URL` 없음
- ESP32 flash, AP association, Android/iOS: `NOT_RUN`
- physical/HIL, power/reset/brownout, 차량 capture: `NOT_RUN`
- 차량 CAN TX: `NO-GO`

## 판정

- P0: 0건
- P1: 1건
- 추가 P2/P3: 0건
- Bridge read-only/security source contract: `PASS`
- browser contract: `PASS`
- generated/config consistency: `PASS`
- 전체 판정: `BLOCK`
- 물리/HIL 판정: `NOT_RUN`
- 차량 CAN TX: `NO-GO`
