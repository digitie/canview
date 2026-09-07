## Post-fix reviewer B raw report

- Execution ID: `RVB-8F32C078-20260907T220054+0900`
- Start (recorded final audit): `2026-09-07T22:00:54.8785350+09:00`
- End: `2026-09-07T22:01:20.2770177+09:00`
- Base: `c8a725d0071e84a1329c30e2cca4409b4a42cfba`
- Candidate: `8f32c07810d76cd3e652ccd08b801400aac5a770`
- Scope: SDK config, board profile/generator, CMake/component boundary, ISR/pool guards, security widening, warnings/CI/artifact and generated-drift evidence. Object-only source/config/test/CI review.

The object was fetched and verified with `git cat-file -e '8f32…^{commit}'` and `git rev-parse --verify`. No checkout, edit, stage, commit, push, build, or test execution occurred.

Isolation: all base/candidate diffs used the user-supplied allowlisted pathspec only. No `docs/reviews/**`, `evidence/**`, reviewer report, or reviewer-output content was opened or searched. `docs/README.md` and `docs/resume.md` were read because repository policy mandates them; no conclusion below relies on their review-status text. A final `git status --porcelain=v1` exposed untracked filenames only; their contents were not opened.

Commands executed included:

```text
git fetch --all --prune
git cat-file -e '8f32c07810d76cd3e652ccd08b801400aac5a770^{commit}'
git rev-parse --verify '8f32c07810d76cd3e652ccd08b801400aac5a770^{commit}'
git diff --name-status <base> <candidate> -- <allowlisted pathspecs>
git diff --check <base> <candidate> -- <allowlisted changed pathspecs>
git diff --no-ext-diff --unified=80 <base> <candidate> -- <allowlisted changed pathspecs>
git show '<candidate>:<allowlisted file>'
git grep -n <review identifiers> <candidate> -- <allowlisted pathspecs>
```

Read source/config/test/CI files: all changed allowlisted files, plus allowlisted ESP CMake files, component CMake files, workflow, both SDK defaults, `tools/check_sdkconfig.py`, `tools/check_esp32_core_coverage.py`, and `tests/foundation/test_sdkconfig.py`. No non-allowlisted firmware, pin-source, or board implementation blob was read.

### P0

None.

### P1

None.

Static positive evidence: generated profiles bind canonicalized manifest/source bytes, reject zero values and in-manifest collisions; generated headers contain distinct profiles. `app_main()` calls preflight before safe GPIO/runtime open. Both real cross-linked BSP negative executables are registered as CTest tests, and their fixture rejects any GPIO or runtime-open call. Runtime callbacks reject ISR and re-entry; pool use rejects ISR context. No component dependency or control/TX capability widening was found in the reviewed CMake/config scope.

### P2

- `tools/check_esp32_core_coverage.py:29`

  Failure scenario: the coverage runner executes only `open gpio watchdog memory pool wait late healthy null-safe null-idle`; it omits the new `preflight` scenario registered by `CMakeLists.txt:43`. It also never runs the two real cross-linked wrong-BSP executables registered at `CMakeLists.txt:91` and `CMakeLists.txt:101`.

  Impact: Debug/Release CTest and sanitizer jobs execute the negative tests, but the coverage artifact can meet its threshold without collecting a profile for the new app-preflight rejection path or either actual wrong-BSP composition. This weakens coverage/evidence integrity for the candidate’s central safety claim and leaves mutations in that path less visible.

  Recommendation: add `preflight` to the app coverage arguments and add separate coverage groups for both wrong-BSP executables, exporting `firmware/app/esp_core.c` and failing if their `.profraw` evidence is absent.

### P3

None.

### Not reviewed

- Raw pin-source inputs, `board.c`, platform-port declaration, non-allowlisted generator inputs, toolchain scripts, actual generated target `sdkconfig`, and all non-allowlisted source.
- No CI run/artifact contents or reviewer/evidence contents were inspected.
- Therefore the source binding’s relationship to physical schematics/pinmaps was not independently verified.

### Physical/HIL

`NOT_RUN`: ESP-IDF/STM32 target build, board flash, GPIO observation, WDT/PSRAM fault behavior, ISR timing, HIL, CAN bus, vehicle interaction, and artifact execution.

### Verdict

**CONDITIONAL**

No P0/P1 blocker was found in the restricted object review. Address the P2 coverage-evidence omission before treating coverage evidence as proof of the new preflight/wrong-BSP protection.
