## Post-fix delta re-review B raw report

- Execution ID: `RVB2-813D19CF-20260907T221023+0900`
- Start: `2026-09-07T22:10:23.3179022+09:00`
- End: `2026-09-07T22:12:11.9099816+09:00`
- Base: `8f32c07810d76cd3e652ccd08b801400aac5a770`
- Candidate: `813d19cfaf0893e67c9591750f73363e9b67aaa1`
- Scope: ESP32 preflight/wrong-BSP coverage evidence closure; CMake/CI dependency, warnings, and artifact-evidence regression review.

C2 P2 closure: **CLOSED**.

`tools/check_esp32_core_coverage.py:28-29` now runs `preflight` for both app coverage binaries. `:32-35` adds each wrong-BSP executable as a distinct group. `:38-46` creates a unique temporary group directory, sets group-local `LLVM_PROFILE_FILE`, runs the binary, and rejects an absent `.profraw`. `:50-54` exports only `firmware/app/esp_core.c` and rejects scope drift. `:58-63` requires a nonzero, fully covered app-function set for early-only wrong-BSP groups. Thus an executable that does not actually call `app_main()` cannot pass merely by returning zero.

The early-only groups use independent `.profdata` and `enforce_thresholds=False`; they neither dilute nor merge into normal app line/branch thresholds. Normal app groups retain function 100%, line ≥95%, and branch ≥90% thresholds, including the preflight failure scenario.

`CMakeLists.txt:13-15` instruments `canview_warnings` when coverage is enabled. Both wrong-BSP executables link that target at `:89-90` and `:99-100`. The CI workflow invokes the coverage runner and fails immediately on a nonzero result at `.github/workflows/foundation.yml:42-43`. No allowlisted CMake, test, app, or workflow delta regresses warnings or artifact upload behavior.

### Isolation

All candidate/base diffs used only the supplied allowlist. No `docs/reviews/**`, `evidence/**`, reviewer report, or reviewer-output content was opened or searched. Repository-required `docs/README.md` and `docs/resume.md` were read; no conclusion relies on their review-status text.

No checkout, edit, stage, commit, push, build, or test execution occurred.

### Commands executed

```text
git fetch --all --prune
git cat-file -e '813d19cfaf0893e67c9591750f73363e9b67aaa1^{commit}'
git rev-parse --verify '813d19cfaf0893e67c9591750f73363e9b67aaa1^{commit}'
git diff --name-status <base> <candidate> -- <allowlisted pathspecs>
git diff --check <base> <candidate> -- <allowlisted pathspecs>
git diff --no-ext-diff --unified=100 <base> <candidate> -- <allowlisted pathspecs>
git show '<candidate>:<allowlisted file>'
git grep -n <coverage/preflight/wrong-bsp identifiers> <candidate> -- <allowlisted pathspecs>
```

Read allowlisted files:

- `tools/check_esp32_core_coverage.py`
- `firmware/docs/esp-core-bench.md`
- `CMakeLists.txt`
- `tests/esp_core/test_app.c`
- `tests/esp_core/test_wrong_bsp.c`
- `firmware/app/esp_core.c`
- `.github/workflows/foundation.yml`

### P0

None.

### P1

None.

### P2

None.

### P3

None.

### Not reviewed

All files outside the seven-path allowlist; actual CI results/artifacts; compiler/LLVM behavior at execution time; target firmware binaries; physical board behavior.

### Physical/HIL

`NOT_RUN`: host build/test, LLVM profile generation, ESP-IDF/STM32 target build, board flash, GPIO observation, WDT behavior, HIL, CAN, and vehicle testing.

### Verdict

**PASS** — source/object-only closure review passes.
