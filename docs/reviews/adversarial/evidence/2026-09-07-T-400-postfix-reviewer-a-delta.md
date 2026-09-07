# 검토 보고서 — 독립 post-fix delta reviewer A

- Execution ID: `4878ed66-87b7-4836-828d-0ca37b7f317d`
- 시작: `2026-09-07T13:10:21.6529981Z`
- 종료: `2026-09-07T13:12:28.3079844Z`
- Base: `8f32c07810d76cd3e652ccd08b801400aac5a770`
- Candidate: `813d19cfaf0893e67c9591750f73363e9b67aaa1`

전문 범위: runtime safety source delta, app preflight coverage inclusion, 양방향 wrong-BSP composition의 독립 profile/function evidence, early fail-closed coverage threshold 분리.

격리:

- `git fetch --all --prune` 후 candidate/base의 full hash `cat-file -e`, `rev-parse --verify`, ancestry를 확인했다.
- `docs/reviews/**`, `evidence/**`, 다른 reviewer output은 열거나 검색하지 않았다.
- allowlist pathspec만 사용했다.
- checkout, edit, stage, commit, push, build, test는 수행하지 않았다.

실제 명령:

- `git fetch --all --prune`
- `git cat-file -e '<hash>^{commit}'`, `git rev-parse --verify '<hash>^{commit}'`
- `git merge-base --is-ancestor …`
- allowlist 제한 `git diff --name-status`, `git diff --unified=160`, `git diff --exit-code`, `git diff --check`
- allowlist candidate object의 line-numbered `git show`

읽은 파일:

- `tools/check_esp32_core_coverage.py`
- `firmware/docs/esp-core-bench.md`
- `CMakeLists.txt`
- `tests/esp_core/test_app.c`
- `tests/esp_core/test_wrong_bsp.c`
- `firmware/app/esp_core.c`

판정 근거:

- Delta에서 실제 변경된 것은 `tools/check_esp32_core_coverage.py`, `firmware/docs/esp-core-bench.md`뿐이다. 허용된 `firmware/app/esp_core.c`, CMake, app/wrong-BSP tests는 base와 동일하다.
- `tools/check_esp32_core_coverage.py:27-30`은 두 board app coverage group 모두에 `preflight` scenario를 포함한다.
- `tests/esp_core/test_app.c:119-122,162-165`는 preflight failure가 safe callback, watchdog, report 전에 terminal idle로 가는 것을 강제한다.
- `tools/check_esp32_core_coverage.py:31-36,38-55`는 두 swapped-BSP binary에 각각 별도 temporary subgroup과 `%p.profraw`를 사용하고, profile 누락·coverage scope 불일치를 실패 처리한다.
- `tools/check_esp32_core_coverage.py:59-63`은 각 wrong-BSP group의 `firmware/app/esp_core.c` function count가 0이 아니고 모든 function이 covered인지 강제한다.
- `CMakeLists.txt:84-102`의 양방향 실제 cross-link composition과 `tests/esp_core/test_wrong_bsp.c:49-60`의 `app_main()` 실행 및 GPIO/runtime-open 0회 assertion은 유지된다.
- wrong-BSP group만 line/branch threshold를 제외하지만, `subprocess.run(..., check=True)`가 composition safety assertion을 강제하고 일반 app group은 기존 function 100%/line 95%/branch 90% threshold와 `preflight` scenario를 계속 적용한다. 따라서 early fail-closed path 분리는 해당 safety assertion을 약화시키지 않는다.

Findings:

- P0: 없음.
- P1: 없음.
- P2: 없음.
- P3: 없음.

검토하지 못한 범위:

- allowlist 밖 runtime implementation, CI workflow 및 coverage checker의 외부 호출 지점.
- 실제 `.profraw`, merged `.profdata`, `llvm-cov export` 산출물. 실행이 금지되어 생성·검증하지 않았다.

Physical/HIL: **NOT_RUN**

- ESP32 target build, host coverage execution, reset/brownout/GPIO/TWDT 및 external gate HIL은 수행하지 않았다.

VERDICT: **CONDITIONAL**

Delta의 source-level coverage gate 설계는 requested evidence 조건을 충족하며, runtime safety source regression은 allowlist 내에서 없다. 실제 instrumented coverage run과 physical/HIL은 미실행이므로 실행 결과까지 포함한 PASS는 보류한다.
