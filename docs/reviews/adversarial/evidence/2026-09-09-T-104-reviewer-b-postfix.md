**독립 Reviewer B 원본 보고서 — T104-RB-20260909-07**

- 시작: `2026-09-09T16:26:36+09:00` — Asia/Seoul
- 종료: `2026-09-09T16:34:49+09:00` — Asia/Seoul
- 전문 영역: configuration, build/dependency, protocol boundary, resource exhaustion, generated output, 검증 evidence
- Candidate: `bdc67981a67d059518e9122f2c4c3eef3c5a4825`
- Base: `b17bdfc0bb2a1bfa9d300c1e7662cac05c96df40`
- 격리: `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b`의 detached checkout. 시작·종료 HEAD가 candidate와 일치했고 porcelain 상태 출력은 비어 있었다. `symbolic-ref -q HEAD`도 branch 이름을 반환하지 않았다.
- 저장소 수정·commit·push·외부 시스템 접근 없음. 빌드와 별도 재현 프로그램은 `C:/Users/digit/Documents/Codex/2026-09-09/t104-postfix-review-b2-20260909/work`에만 생성했다.
- reviewer report는 열지 않았다. 전체 diff의 내용 검사에서도 `docs/reviews/**`를 제외했다. `docs/resume.md`에 있는 과거 리뷰 요약은 이번 판정 근거로 사용하지 않았다.

확인된 finding은 **P2 3건, P3 1건**이다. 검토한 범위에서 P0/P1은 발견하지 못했다. 다만 candidate-specific target artifact·warning·identity 검증은 완료하지 못했으므로 요청된 완료 조건을 충족하지 않는다.

**Finding**

**RB-01 — P2 — SDK 금지 설정의 삭제 변이를 회귀시험이 놓침**

위치: [test_sdkconfig.py:26](F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/foundation/test_sdkconfig.py:26), 같은 파일 165행. 실제 금지 목록은 [check_sdkconfig.py:47](F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tools/check_sdkconfig.py:47).

독립 oracle인 `EXPECTED_COMMON_FORBIDDEN`에 다음 항목이 없다.

- `CONFIG_FLASH_ENCRYPTION_ENABLED`
- `CONFIG_APP_ROLLBACK_ENABLE`
- `CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT`

각 항목을 메모리상의 `FORBIDDEN`과 `REQUIRED_DISABLED`에서 제거한 뒤 전체 `SdkConfigTests`를 실행했다. 세 변이 모두 **13 tests, failures 0, errors 0**이었다. 그 상태에서 해당 설정을 `y`로 바꾼 fixture도 validator가 수락했다.

현재 candidate의 실제 validator는 이 설정들을 거부한다. 결함은 금지 규칙이 삭제되는 후속 변경을 시험이 탐지하지 못한다는 점이다. 이는 base에서 이어진 검증 결함이며 T-104가 새로 만든 runtime 우회라고 판단하지 않는다.

권고: 세 항목을 독립 oracle에 추가하고, 필수 비활성 항목의 독립 기대 목록도 유지한다. 금지 검사와 explicit-disabled 검사를 함께 제거하는 변이가 반드시 실패하도록 한다.

Disposition: **OPEN**.

**RB-02 — P2 — UART 오류 복구가 이미 알고 있는 폐기 byte 수를 누락**

위치: [uart_dma.c:606](F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/platform/stm32g474/uart_dma.c:606), 특히 609행의 `0U`.

`producer_valid`가 참이어도 `rx_error`가 있으면 `reset_rx_dma()`에 폐기량 0을 전달한다. 이후 RX buffer와 cursor가 초기화되어 unread data가 사라진다.

실제 candidate의 fake-register fixture와 컴파일된 adapter object를 사용한 별도 probe 결과:

```text
7 unread bytes before error:
discarded=0 read_total=0 errors=1 first_byte=0
```

재현 조건은 RX buffer에 7 byte를 넣고, `CNDTR=capacity-7`, `rx_error_pending=true`로 설정한 뒤 service를 호출하는 것이다. 오류 횟수는 기록되지만, 알고 있던 7 byte의 폐기는 `rx_discarded_bytes`에 반영되지 않는다.

영향: 오류·soak 분석에서 byte-loss accounting이 실제보다 작게 보고된다. 이 카운터를 “unaccounted byte loss 없음”의 근거로 사용할 수 없다. 차량 명령 송신 우회는 재현되지 않았다.

권고: 유효한 producer snapshot에서는 unread byte 수를 보존하여 기록한다. 정확한 손실량을 알 수 없는 오류는 별도 unknown-loss 상태로 표시한다. unread data와 RX error가 동시에 존재하는 회귀시험을 추가한다.

Disposition: **OPEN**.

**RB-03 — P2 — LINK_HELLO가 실제 build identity를 전송하지 않음**

위치: [uart_link.c:264](F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/module/uart_link.c:264).

`enqueue_hello()`는 payload를 0으로 초기화하고 다른 필드를 채우지만, offset 32의 16-byte `build_id_digest`를 채우지 않는다. 별도 probe에서도 다음을 확인했다.

```text
HELLO build digest nonzero bytes: 0
```

[UART architecture:183](F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/architecture/protocols/communicator-uart.md:183)은 HELLO에 build ID를 포함하도록 명시한다. [UART schema:85](F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/protocol/schema/uart-v1.0.yaml:85)에도 해당 필드가 있다.

실패 시나리오: 같은 firmware version 상수를 사용하는 서로 다른 candidate가 동일한 zero build digest를 송신한다. 수집된 HELLO만으로 실행 중인 image를 특정 artifact와 연결할 수 없다.

권고: immutable build metadata에서 digest를 주입하고, HELLO를 decode하여 기대 build digest와 비교하는 시험을 추가한다. boot ID와 device ID를 build identity의 대체물로 사용하지 않는다.

Disposition: **OPEN**.

**RB-04 — P3 — UART platform coverage 예외 설명이 현재 시험과 불일치**

위치: [check_stm32_coverage.py:25](F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tools/check_stm32_coverage.py:25), 27행, 67행.

UID 경로를 host에서 실행하지 않는다는 설명 아래 function threshold를 80%로 둔다. 그러나 candidate는 host UID hook과 `test_device_id_contract()`를 제공하며, 이번 fresh coverage에서 adapter 함수 **33/33, 100%**가 실행됐다. 최종 요약 문자열에는 platform의 별도 80/70/50 threshold도 표시되지 않는다.

영향: 현재 적용되는 coverage 예외와 근거를 독자가 오해할 수 있고, 이미 확보한 function coverage의 회귀를 불필요하게 허용한다.

권고: 설명을 현재 fixture와 일치시키고 platform threshold를 요약에 명시한다. 별도 정당화가 없다면 function threshold를 현재 달성한 100%로 올린다.

Disposition: **OPEN**.

**구성·coverage의 이전 문제에 대한 독립 disposition**

이전 finding 원문이나 ID는 제공되지 않았고 금지된 report를 읽지 않았으므로, 특정 과거 finding ID를 폐쇄했다고 주장하지 않는다. 요청된 두 영역의 현재 상태는 다음과 같다.

| 영역 | 이번 독립 확인 |
|---|---|
| 금지 SDK 설정의 실제 거부 | 현재 validator에서 확인. 기본 13개 시험 통과 |
| unknown key, duplicate, malformed configuration | 시험 통과 |
| Bridge routing/HTTP queue의 explicit-disabled 계약 | 시험 통과 |
| 금지 설정 삭제 mutation coverage | 완전하지 않음. RB-01 유지 |
| UART link의 coverage 누락 | 현재 gate에 포함되어 실제 실행됨 |
| UART DMA adapter의 coverage 누락 | 현재 gate에 포함되어 실제 실행됨 |
| UART platform coverage 예외 설명 | RB-04 유지 |
| 실제 ESP-IDF 생성 sdkconfig | 이번 실행에서 검증하지 못함 |

이번 STM32 coverage 수치는 다음과 같다.

| 파일 | Functions | Lines | Branches |
|---|---:|---:|---:|
| UART link | 100% | 73.22% | 58.16% |
| UART DMA adapter | 100% | 80.23% | 58.16% |

현재 명시된 UART thresholds는 통과한다. 기존 core/FDCAN 그룹도 해당 gate를 통과했다. 이 결과는 fake-register/host evidence다.

**기타 검토 결과**

- Board generator의 memory/SKU, pin availability, Bridge USB mapping, profile collision, partition bounds 검사와 생성물 비교가 통과했다.
- STM32 module은 portable codec에 의존하고, CMSIS register 접근은 platform adapter에 있다. CMake에서 UART module과 adapter의 host/target 연결을 확인했다.
- STM32 build-mode header는 forced include와 override rejection을 제공하며 실제 compiler negative probe가 통과했다.
- 실제 STM32 app의 authorizer는 `NULL`이다. lease acquire/renew는 capture-only compile 경계에서 거부된다. 검토한 경로에서 임의 외부 입력을 차량 CAN frame으로 변환하는 executor를 발견하지 못했다.
- Bridge CMake dependency와 URI 등록표에서 vehicle TX, control lease 또는 raw replay endpoint를 발견하지 못했다. session 생성/삭제와 관찰용 GET/WebSocket 구성이 유지된다. 전체 HTTP runtime 감사나 live endpoint 검증은 수행하지 않았다.
- bounded queue/pool, cache-full, duplicate/conflict, wrong-BSP, callback reentry, CTS offline, timeout 관련 host 시험이 통과했다.
- workflow는 immutable PR-head checkout, pinned action SHA, target diagnostic scan, artifact size/SHA-256 manifest 생성을 구성한다. 이 구성의 존재가 현재 candidate의 CI 성공을 증명하지는 않는다.
- reviewer report를 제외한 변경 파일 41개에 대한 제한된 secret-pattern scan에서 match는 없었다. 이 검사는 private-key marker, AWS access-key 형태, GitHub token 형태, 일부 credential assignment 형태에 한정된다. 모든 secret·VIN·위치정보의 부재를 증명하지 않는다.

**실제로 실행한 명령과 시험**

아래에서 `R`은 review worktree의 절대 경로, `B`는 scratch build 경로다.

```text
R = F:/dev/canview-wt/2026-09-09-t104-postfix-review-b
B = C:/Users/digit/Documents/Codex/2026-09-09/t104-postfix-review-b2-20260909/work/host-coverage
```

Git 명령은 command-local `-c safe.directory=R`를 사용했다.

- `rev-parse HEAD` — 시작·종료 확인.
- `status --porcelain=v1` — 시작·종료 확인.
- `diff --find-renames --stat <base> <candidate>`.
- 지정된 CMake, coverage, schema, generator, shared UART 파일의 `diff --find-renames <base> <candidate> -- <paths>`.
- `diff --find-renames <base> <candidate> -- . :(exclude)docs/reviews/**` — 6,622행을 로컬 Python으로 받아 제한된 material scan.
- `diff --name-only <base> <candidate> -- . :(exclude)docs/reviews/**`.
- `rev-parse <base>`.
- `symbolic-ref -q HEAD`.

탐색·읽기 명령:

- `Get-Date -Format o`.
- 아래 파일 inventory에 대한 `Get-Content`; 긴 파일은 `-TotalCount`, `Select-Object -Skip/-First`로 부분 재조회.
- `rg --files` — AGENTS, CMakeLists, sdkconfig, board, T-104, runbook, coverage, ESP fixture 파일 탐색.
- `rg -n` — UART 함수·authorization·cache·lease·queue·ISR, Bridge route, build digest, configuration/coverage finding 위치 검색.
- `Get-Item` — 상위 AGENTS 존재 확인.
- `Get-Command python,cmake,clang,llvm-cov,llvm-profdata,ninja`.
- `Get-ChildItem -Directory` — worktree 및 설치된 로컬 tool directory 확인.
- 로컬 `vswhere.exe`와 `Launch-VsDevShell.ps1` — 기존 MSVC SDK/linker 환경 선택.
- `ninja -C B -t commands canview-stm32-uart-platform-tests` — 기존 link 구성 조회.

검증 명령:

```text
python -X utf8 -B tests/foundation/test_sdkconfig.py -v
python -X utf8 -B tools/generate_boards.py --check
python -X utf8 -B tools/generate_uart_protocol.py --check
cmake -S R -B B -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DCANVIEW_COVERAGE=ON
cmake --build B -j 4
python -X utf8 -B tools/check_stm32_coverage.py --build B
python -X utf8 -B tools/check_esp32_core_coverage.py --build B
ctest --test-dir B --output-on-failure --timeout 60
  -R "^(esp32-|stm32-|uart-|espnow-|protocol-header|bridge-auth|bridge-web-session)"
  -E "stm32-build-mode-negative"
python -X utf8 -B tools/check_generated.py
python -X utf8 -B tools/check_generated.py --negative-fixture
python -X utf8 -B tools/check_stm32_build_mode.py
  --compiler <installed Clang 23.1.0>
  --header firmware/communicator/stm32/interface/canview_build_mode.h
```

결과:

- Fresh host configure/build: 성공, 124 build steps.
- Focused CTest: **80/80 성공**, 11.67초.
- SDK suite: **13/13 성공**.
- Board/UART generator: 성공.
- Generated output: **15 files**, drift 없음.
- Generated negative fixture: 변조 거부.
- STM32 및 ESP32 coverage gate: 성공.
- Forced capture-only/override negative fixtures: 성공.

80개 CTest는 다음 집합을 실행했다.

- `bridge-auth`, `bridge-web-session`.
- `esp32-app-*`, `esp32-bridge-app-*`: open, gpio, watchdog, memory, pool, wait, late, healthy, null-safe, null-idle, preflight.
- 양 보드의 runtime-sdk-model 및 core boot/health/faults/pool.
- Bridge app integration, 두 wrong-BSP composition, pool-threads.
- STM32 platform-contracts, fdcan-capture, uart-link, uart-platform, fdcan-platform, core-register-model, core boot/scheduler-validation/scheduler-healthy/scheduler-faults/queue.
- protocol-header C99/C11.
- ESP-NOW vectors/malformed/tlv-contracts/session/security/control/qos/pool-fuzz/reference/fault-transport.
- UART matrix/direction/malformed/stream/long-stream/soak-smoke/payload-lifetime/plan/plan-boundaries/command/command-boundaries/link/link-boundaries/limits/session/replay/schema/fault-stream.
- esp32-sdkconfig-negative.

추가 로컬 probe:

1. Python `unittest.mock.patch`로 RB-01의 세 항목을 각각 validator 메모리에서 제거하고 전체 13개 suite와 활성 설정 수락을 확인했다.
2. `check_stm32_core.check_source_safety()`를 STM32, shared app, shared protocol source에 호출했다. 세 scope 모두 통과했다.
3. scratch `uart_review_probe.c`를 작성해 candidate fixture를 include하고, fresh build의 adapter/fake-hardware/core/wire object·library와 Clang으로 link했다. RB-02와 RB-03의 위 출력이 재현됐다.
4. 변경된 비-review 파일 41개를 Python으로 읽어 제한된 secret-pattern scan을 수행했다.

환경상 주의: Git은 사용자 global ignore 파일에 대한 permission warning을 출력했다. 최초 상위 worktree 탐색도 다른 worktree의 일부 build directory에서 access-denied를 반환했다. 해당 directory나 다른 리뷰 결과는 근거로 사용하지 않았다. 일부 큰 읽기 출력은 잘렸으며 후속 조회는 필요한 구간을 대상으로 했다.

**실제 파일 읽기 inventory**

다음은 직접 내용 또는 관련 구간을 읽은 파일이다. 디렉터리 표기 아래 이름은 그 절대 경로에 속한다.

| 절대 디렉터리 | 파일 |
|---|---|
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b` | `AGENTS.md`, `CMakeLists.txt`, `CMakePresets.json` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/.github/workflows` | `foundation.yml` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/cmake` | `CanviewWarnings.cmake` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs` | `README.md`, `resume.md` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/tasks` | `T-104-stm32-uart-control.md` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/architecture` | `README.md` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/architecture/protocols` | `communicator-uart.md` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/development` | `windows.md` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/runbooks` | `agent-workflow.md` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tools` | `check_sdkconfig.py`, `generate_boards.py`, `check_stm32_coverage.py`, `check_esp32_core_coverage.py`, `check_generated.py`, `check_stm32_core.py`, `check_stm32_build_mode.py`, `generate_uart_protocol.py`, `foundation-tools.json` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tools/environment` | `foundation-windows.ps1` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/boards` | `boards.json`, `waveshare35-pins.json` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/controller` | `CMakeLists.txt`, `sdkconfig.defaults` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/controller/main` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/controller/components/canview_protocol` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/controller/components/canview_can` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/controller/components/canview_automation` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/esp32` | `CMakeLists.txt`, `sdkconfig.defaults` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/esp32/main` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/esp32/components/canview_protocol` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/diagnostic-bridge` | `CMakeLists.txt`, `sdkconfig.defaults` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/diagnostic-bridge/main` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/diagnostic-bridge/components/canview_bridge_auth` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/diagnostic-bridge/components/canview_bridge_web` | `CMakeLists.txt`, `canview_bridge_web.c`의 route 관련 구간 |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/components/canview_foundation` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/components/canview_esp_core` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/components/canview_esp32_runtime` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/components/canview_esp32_platform` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/module/esp_core` | `CMakeLists.txt` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32` | `CMakeLists.txt`, `README.md` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/app` | `main.c` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/bsp` | `board_pins.h` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/docs` | `core-bench.md`, `uart-dma.md` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/interface` | `canview_stm_uart.h`, `canview_build_mode.h` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/module` | `CMakeLists.txt`, `uart_link.c`의 queue/HELLO/authorization/command/lease 관련 구간과 함수 검색 |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/platform/stm32g474` | `uart_dma.h`, `uart_dma.c`의 설정/RX/TX/service/identity/ISR 관련 구간 |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/protocol` | `canview_uart_protocol.h`의 상수·payload·policy 관련 구간 |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/protocol/schema` | `uart-v1.0.yaml`의 catalog·control 관련 구간 |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/shared/protocol/include` | `canview_uart.h` 변경 구간 |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/shared/protocol/src` | `canview_uart.c` 변경 구간 |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/foundation` | `CMakeLists.txt`, `test_sdkconfig.py` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/esp_core` | `board_fixture.h`, `sdk_fixture.h`, `test_app.c`, `test_core.c`, `test_runtime.c`, `test_pool_threads.c`, `test_wrong_bsp.c`, `test_bridge_integration.c` |
| `F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/stm32` | `test_uart_dma_platform.c`, `test_uart_link.c`의 시험·assertion 검색 구간 |

프로그램에 의한 변경 파일 scan에는 위 목록 외에도 다음 파일들이 포함됐다. 이 파일들은 모두 사람 수준의 전체 내용 리뷰를 완료한 것으로 간주하지 않는다.

```text
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/api/Doxyfile
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/api/conf.py
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/api/index.rst
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/api/stm32_uart.rst
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/docs/tasks.md
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/platform/stm32g474/core_hw.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/platform/stm32g474/core_hw.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/firmware/communicator/stm32/tests/test_registers.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/hil/fixtures/t103-capture-only.jsonl
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/protocol/test_uart.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/stm32/fake_stm32/board_pins.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/stm32/fake_stm32/core_hw.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/stm32/fake_stm32/fake_hardware.c
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/stm32/fake_stm32/fake_hardware.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/stm32/fake_stm32/stm32g474xx.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/stm32/fake_stm32/stm32g4xx_ll_dmamux.h
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tests/test_t103_capture_helpers.py
F:/dev/canview-wt/2026-09-09-t104-postfix-review-b/tools/build_docs.py
```

Generator, compiler, coverage 및 source-safety 도구가 읽은 transitive input/header/library는 위 직접 리뷰 목록과 구분한다. 개별 filesystem-open 추적은 수집하지 않았으므로 그 전체 목록을 제공하지 못한다.

검토 지침으로 읽은 파일:

```text
C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md
C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md
C:/Users/digit/.codex/skills/embedded-documentation/SKILL.md
C:/Users/digit/.codex/skills/embedded-driver-design/SKILL.md
C:/Users/digit/.codex/skills/embedded-isr-design/SKILL.md
```

**미완료·잔여 범위**

- Candidate의 STM32 Debug/Release target build, ELF/MAP/BIN/HEX identity 및 target warning 로그 대조: **NOT_RUN**.
- Candidate의 실제 ESP-IDF 생성 sdkconfig 및 세 ESP target image 검증: **NOT_RUN**.
- Candidate CI run·artifact manifest와 실제 artifact bytes/SHA-256 대조: **NOT_RUN**. 외부 접근 금지에 따라 조회하지 않았다.
- 전체 source delta의 줄 단위 수동 리뷰, 전체 transitive file-read inventory: 미완료.
- 전체 CTest, Release host, ASan/UBSan/TSan, 문서 build/link/plan 검사, 24시간 accelerated soak: **NOT_RUN**.
- Physical board/flash, 전기 level·RTS/CTS·DMA/IRQ timing, reset/brownout/rail, watchdog 계측, physical 24시간 PRBS: **NOT_RUN**.
- HIL, CAN analyzer, vehicle bus/capture/TX, provisioning 및 security key 배치: **NOT_RUN**.

Host 및 fake-register 시험은 위 physical/electrical/HIL/vehicle/provisioning gate의 증거가 아니다. 차량 CAN TX 승인은 부여하지 않는다.

요청된 일부 source/check와 artifact identity 검증이 완료되지 않았으므로, 미완료 항목이 있으면 차단하라는 판정 규칙을 적용한다.

**최종 verdict: BLOCK**
