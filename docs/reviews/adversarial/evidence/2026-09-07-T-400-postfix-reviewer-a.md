# 검토 보고서 — 독립 적대적 post-fix reviewer A

- Execution ID: `fc6e0f43-51f1-407d-88fa-8bdd2e564cfc`
- 시작: `2026-09-07T12:56:18.4298931Z`
- 종료: `2026-09-07T13:00:49.9995402Z`
- Base: `c8a725d0071e84a1329c30e2cca4409b4a42cfba`
- Candidate: `8f32c07810d76cd3e652ccd08b801400aac5a770`

전문 범위: ESP embedded runtime, RTOS/task·callback ownership, watchdog/reset/brownout fail-safe behavior, memory bounds, GPIO contract, ISR rejection, callback reentry, wrong-BSP fail-closed, Communicator/Diagnostic Bridge 격리.

격리 방식:

- `git fetch --all --prune` 뒤 immutable object를 확인했다.
- `git cat-file -e '8f32…^{commit}'`, `git rev-parse --verify '8f32…^{commit}'` 및 base도 성공했다.
- 모든 `git diff`는 사용자가 지정한 source/test pathspec으로 제한했다. candidate file read도 허용 목록의 정확한 object path만 사용했다.
- `docs/reviews/**`, `evidence/**`, 다른 reviewer report/finding은 열거나 검색하지 않았다.
- checkout, edit, stage, commit, push, build, test는 수행하지 않았다.
- 최초 unquoted PowerShell `cat-file` invocation은 revision quoting 때문에 실패했고, 즉시 quoted revision으로 재실행해 성공했다.

주요 확인 결과:

- `firmware/app/esp_core.c:12-34`는 `canview_board_port()` 결과에 대해 preflight를 GPIO callback, BSP runtime, core boot, pool init 전에 수행한다. 불일치면 report 없이 terminal idle로 간다.
- Communicator 및 Bridge runtime의 `canview_esp_board_preflight()`는 각각 `runtime.c:6-13`에서 profile만 비교하며 callback/SDK/GPIO를 실행하지 않는다. `canview_esp_board_runtime()`도 다시 preflight한 뒤에만 `runtime_open`을 호출한다.
- `tests/esp_core/test_wrong_bsp.c:49-60`와 `CMakeLists.txt`의 두 swapped-link target은 양 방향으로 실제 `app_main` terminal lifecycle을 실행한다. 이 시험의 정상 결과는 runtime lifecycle 진입이 아니라, 그 전에 `gpio_calls == 0`, `runtime_open_calls == 0`, terminal idle 한 번이다.
- `firmware/platform/esp32s3/core_runtime.c:281-323`는 ISR context를 runtime/port 기록 전에 거부한다. `firmware/module/esp_core/pool.c:48-66`도 context validator를 pool 상태 초기화 전에 호출한다.
- `core_runtime.c:30-274`의 callback gate는 ISR, 비-owner task, nested callback을 거부하고 모든 정상/오류 반환 경로에서 `callback_active`를 해제한다.
- `tools/generate_boards.py:28-37,241-258`의 profile v2는 compile-time/generated tag이며 runtime input으로 변경되지 않는다. source/BOM 차이도 profile 불일치로 terminal idle을 유도하므로 source상 fail-open 경로를 만들지 않는다. 현재 manifest 내 32-bit profile collision은 generator가 거부한다.
- `firmware/communicator/esp32/bsp/board.c:16-44`는 RUN_OK low를 우선 시도하고 다른 safe outputs/inputs도 계속 safe configuration한 뒤 최초 오류를 반환한다.

읽은 허용 파일:

- `CMakeLists.txt`
- `firmware/app/esp_core.c`
- `firmware/module/esp_core/{health.c,pool.c}`
- `firmware/interface/{canview_esp_core.h,canview_esp_pool.h,canview_esp_runtime.h,canview_gpio.h}`
- `firmware/platform/esp32s3/core_runtime.c`
- Communicator/Bridge의 `bsp/{board.c,runtime.c,board_pins.h}`
- `shared/interface/canview_platform_port.h`
- `tests/esp_core/{test_app.c,test_runtime.c,test_wrong_bsp.c,sdk_fixture.h}`
- `tools/generate_boards.py`
- `tests/foundation/test_generators.py`

실행 명령:

- `git fetch --all --prune`
- `git cat-file -e …`, `git rev-parse --verify …`, `git merge-base --is-ancestor …`
- 허용 pathspec 제한 `git diff --name-status`, `git diff --unified=100/120`, `git diff --check`
- 허용 candidate object의 line-numbered `git show <candidate>:<exact-path>`

Findings:

- P0: 없음.
- P1: 없음.
- P2: 없음.
- P3: 없음.

검토하지 못한 범위:

- 허용 목록 밖 source, 실제 ESP-IDF GPIO adapter, linker/SDK configuration, 외부 TX gate 회로, 다른 integration target source.
- 작업 지시에 따라 build/test 실행 결과는 확인하지 않았다.

Physical/HIL: **NOT_RUN**

- 실제 ESP32-S3에서 ISR-context rejection, TWDT reset, brownout/reset 시 GPIO latch·external TX gate 상태, Communicator/Bridge physical cross-image behavior는 검증하지 않았다.

VERDICT: **CONDITIONAL**

Source-only 범위에서는 새로운 P0–P3 결함을 발견하지 못했다. 다만 wrong-BSP preflight 실패 시 GPIO를 의도적으로 건드리지 않는 설계이므로, reset/brownout 동안 external TX gate가 fail-closed임을 HIL로 확인하기 전에는 차량 안전 release 관점의 PASS로 승격할 수 없다.
