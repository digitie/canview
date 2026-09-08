**BLOCK — CV-HOSTILE-20260908-T500-B-03**

| Field | Value |
|---|---|
| Actual start UTC | `2026-09-08T12:17:17Z` |
| Actual end UTC | `2026-09-08T12:23:19Z` |
| Candidate | `8a1f13db4f40b3826376e0a2b62d9c1254bff534` |
| Base | `50410ba23fcecfa1f28cea837d04a061c201d648` |
| Repository | `F:/dev/canview-wt/t500-hil` |
| Current findings | **P0: 0 · P1: 2 · P2: 3 · P3: 0** |
| Physical/HIL | **NOT_RUN** |
| Vehicle CAN TX | **NO-GO** |
| Verdict | **BLOCK** |

This post-fix review inspected the stated candidate. It did not modify repository files or consume another reviewer’s findings.

**Isolation and immutable-source verification**

Source inspection used immutable `git show <candidate>:<path>` and base-to-candidate diffs. Tests ran against the supplied worktree, whose initial and final HEAD equaled the candidate. Initial and final porcelain status were empty; final tracked comparison against the candidate exited zero. This was not a detached checkout.

Both the specified base and previous candidate `7c63183228d68c30c809ffdc323badf1bb259096` were verified ancestors of the candidate.

Probes created temporary fixtures and outputs outside the repository. One explicitly identified diagnostic supplied a missing constant in memory using `unittest.mock.patch.object`; it changed no files.

**Disposition of all prior B-02 findings**

| Prior finding | Disposition at this candidate |
|---|---|
| B-01: Unsafe/absent evidence accepted as PASS | **PARTIALLY FIXED / OPEN.** Original TX-summary, zero-event, false-check and reported-overbudget mutations are rejected. Contradictory event metrics still validate: current F-01. |
| B-02: Host evidence relabeled physical PASS | **FIXED.** Exact relabeling probe is rejected; validator explicitly refuses physical PASS. |
| B-03: One safe record masks unsafe decisions | **PARTIALLY FIXED / OPEN.** Original later `ALLOW`/`True` mutation fails. Named-case coverage and malformed safety outcomes remain vulnerable: F-02. |
| B-04: Unverified identity/inventory | **PARTIALLY FIXED / OPEN.** Original digest, derived-seed, duplicate-key and truncated-inventory mutations fail. Coordinated seed relabeling still validates against a different trace: F-01. |
| B-05: Weak scenario schema/execution limits | **PARTIALLY FIXED / OPEN.** Original malformed expectations, trillion-packet count, missing/null arguments and overflow float are rejected. A parser-accepted event reaches a broken error-reporting path: F-04. |
| B-06: Output symlink/hardlink writes | **PARTIALLY FIXED / OPEN.** Original directory symlinks are rejected and hardlinked sentinel contents survive. Windows junctions bypass protection: F-03. |
| B-07: Private rig diagnostic leakage | **FIXED for reproduced cases.** Malformed private rig reports use sanitized reason codes without the private path/sentinel. |
| B-08: Recorded forbidden-TX FAIL rejected | **FIXED.** Runner exits 1 and its negative artifact validates as FAIL. |

**Current findings — all OPEN**

**F-01 — P1: Recomputed PASS still trusts metrics and execution claims that contradict the trace**

Locations: [validate_evidence.py:309](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:309), [validate_evidence.py:314](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:314), [analyze.py:241](F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:241).

Two independent reproductions passed validation:

1. Append a valid, monotonic `BUDGET_SAMPLE` to the trusted `radio-pressure` log containing:

   ```python
   {
       "map_bytes": 999999999,
       "stack_bytes": 99999999,
       "heap_free_bytes": 0,
       "queue_depth": 999999,
       "wcet_us": 999999,
       "latency_us": 999999,
   }
   ```

   Rebuild offsets/counts/bytes, retain the original passing report metrics, and populate checks through the candidate’s analyzer. `validate(..., "PASS")` returns PASS despite the over-budget event.

2. Change a seed-1 report’s top-level seed to 2 and update each scenario seed with `_seed_for_scenario(2, id)`, leaving event files unchanged. Validation returns PASS. An actual seed-2 run produces a different `espnow-loss.jsonl`.

The validator checks reported metrics against budgets and seed metadata against its derivation formula. It does not establish consistency between those claims and the deterministic execution trace.

**Impact:** Evidence containing budget failures or an incorrect reproduction seed can still be certified as PASS.

**Recommendation:** For this deterministic host adapter, replay trusted scenarios with the recorded seed and compare expected events and metrics. Alternatively, derive and cross-check metrics from a strictly defined trace while independently binding execution identity. Reject unexpected or contradictory samples; do not treat report-supplied summaries as measurement truth.

**F-02 — P1: Counts do not establish named safety-case coverage, and malformed safety outcomes pass**

Locations: [stale-gates.yaml:12](F:/dev/canview-wt/t500-hil/tests/hil/scenarios/stale-gates.yaml:12), [analyze.py:159](F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:159), [analyze.py:208](F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:208).

Reproductions:

- Replace every `SAFETY_DECISION.fields.check` with `"stale-revision"`. Rebuild valid offsets and byte counts. The unchanged result checks still validate as PASS, although `wrong-profile`, `expired-lease` and `hardware-gate-off` evidence is absent.
- Append a structurally valid safety record with `decision="allow"` and `vehicle_tx=1`. Recompute result fields using the candidate analyzer. Both analyzer and validator accept PASS.

The scenario requires four matching generic DENY records, not one record per named case. The unsafe scan recognizes exact `"ALLOW"` or boolean `True`; it does not reject noncontract decision values and nonboolean TX flags.

**Impact:** Missing safety cases and malformed potentially unsafe outcomes can be hidden inside passing evidence.

**Recommendation:** Require every configured case identifier and its expected outcome, rejecting missing or duplicate substitutions. Validate safety-event field types and enum values before evaluating outcomes. Reject any record that does not explicitly satisfy the safe contract.

**F-03 — P2: Windows junctions bypass output-link protection**

Locations: [run.py:103](F:/dev/canview-wt/t500-hil/tests/hil/run.py:103), [run.py:122](F:/dev/canview-wt/t500-hil/tests/hil/run.py:122), [events.py:95](F:/dev/canview-wt/t500-hil/tests/hil/events.py:95).

Temporary Windows junctions reported:

```text
is_symlink() = False
is_junction() = True
```

Results:

| Junction location | Runner result | External writes |
|---|---|---|
| Output root | Exit 0 | `report.json`, `events/brownout.jsonl` |
| Output’s `events` directory | Exit 0 | `brownout.jsonl` |

The implementation checks symbolic links but misses Windows directory junctions. Atomic file replacement does not prevent directory redirection.

**Impact:** On the canonical Windows platform, output writes can still escape through pre-existing filesystem links.

**Recommendation:** Reject junctions and other relevant reparse-point redirections throughout existing path components. Resolve and verify the intended destination before creating files. Add Windows junction regression tests alongside symlink/hardlink tests.

**F-04 — P2: Host failure-reporting paths crash and can leave an old PASS artifact**

Locations: [run.py:23](F:/dev/canview-wt/t500-hil/tests/hil/run.py:23), [run.py:200](F:/dev/canview-wt/t500-hil/tests/hil/run.py:200), [run.py:240](F:/dev/canview-wt/t500-hil/tests/hil/run.py:240), [run.py:285](F:/dev/canview-wt/t500-hil/tests/hil/run.py:285).

A scenario accepted by `parse_scenario()` contains:

```json
{"type":"event","fields":{"kind":"collision"}}
```

Execution raises a duplicate-argument `TypeError`; the handler calls `_blocked_report()`, which references an unimported `HOST_ADAPTER_VERSION` and raises `NameError`.

Additional reproductions:

- A symlinked individual event destination reaches the same `NameError`.
- A symlinked report destination raises uncaught `RunError`.
- Running the collision scenario into a directory containing earlier valid evidence leaves that old report validating as PASS after the failed invocation.

A diagnostic that supplied only the missing constant in memory exposed a second contract defect: the generated host BLOCKED report lacks `hardware_execution=False`, so its validator rejects it.

**Impact:** Expected input/output failures do not reliably produce controlled, valid failure evidence. Existing success artifacts can remain available after a failed invocation.

**Recommendation:** Repair the missing import and host adapter metadata; handle final report-write errors; reject or safely support colliding event field names. Define an output lifecycle that clearly supersedes previous results on a new invocation. Test host BLOCKED generation and validation end to end.

**F-05 — P2: Supported scenario selection produces PASS evidence that the validator rejects**

Locations: [run.py:332](F:/dev/canview-wt/t500-hil/tests/hil/run.py:332), [validate_evidence.py:199](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:199), [validate_evidence.py:322](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:322).

Reproduction:

```text
run --suite host --scenario brownout --output <temporary-output>
    → exit 0, PASS

validate(<temporary-output>/report.json, "PASS")
    → EvidenceError: scenario inventory does not match trusted inventory
```

The runner exposes scenario selection, but validation unconditionally demands all 12 trusted scenarios for PASS.

**Impact:** A legitimate supported invocation cannot complete the runner/validator evidence workflow.

**Recommendation:** Introduce an explicit trusted expected selection and distinguish selected-run success from complete-suite qualification. Keep CI’s full-inventory requirement explicit; do not accept a report’s self-declared subset as sufficient authority.

**Executed checks and observed results**

| Check | Result |
|---|---|
| Runtime | Windows Python 3.14.3 |
| Focused T-500 tests | **23/23 PASS**, no reported skips |
| Complete Python discovery | **74/74 PASS** |
| Host CLI, seed 1 | Exit 0; 12 scenarios PASS |
| Host evidence CLI | Exit 0; PASS |
| Example unavailable lab CLI | Exit 2; SKIPPED |
| Lab evidence CLI, expecting SKIPPED | Exit 0 |
| Plan validator | 49 tasks; zero errors |
| Document links | 274 documents; 1,268 targets; zero errors |
| Base ancestry | Exit 0 |
| Previous-candidate ancestry | Exit 0 |
| Final candidate/worktree comparison | Exit 0 |
| Final porcelain status | Empty |

Focused adversarial observations:

```text
Original reported-overbudget mutation             REJECTED
Original false check                              REJECTED
Original physical-PASS relabel                     REJECTED
Original identity/scenario-digest/seed mutations   REJECTED
Original truncated inventory                      REJECTED
Original empty logs                               REJECTED
Original nonzero TX channel summary                REJECTED
Duplicate report status keys                      REJECTED
Original later ALLOW/True safety record            FAIL
Duplicate safe cases replacing three gates        PASS — defect
Malformed additional safety record                PASS — defect
Original malformed scenario inputs                REJECTED
Host failure with colliding event field            NameError — defect
Private rig path/sentinel in report                ABSENT
Recorded forbidden-TX failure                      Validates as FAIL
Hardlinked report/event sentinel contents         PRESERVED
Output/events symbolic links                      Exit 2; no external writes
Output/events Windows junctions                   Exit 0; external writes — defect
Seed relabel without replay                       PASS — defect
Relabeled trace equals actual seed-2 trace         FALSE
Overbudget event with passing report metrics      PASS — defect
Selected brownout run                             Exit 0, then validator rejects — defect
```

A further parser probe found that an unknown report field containing `1e309` is accepted as infinity. This was not assigned a separate severity because the tested field did not influence a verdict; comprehensive strict-JSON validation remains unproven.

**Exact command record**

Commands ran from `F:/dev/canview-wt/t500-hil` unless `git -C` supplied the repository.

```powershell
git -C F:/dev/canview-wt/t500-hil rev-parse HEAD
git -C F:/dev/canview-wt/t500-hil rev-parse 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil rev-parse 50410ba23fcecfa1f28cea837d04a061c201d648
git -C F:/dev/canview-wt/t500-hil status --porcelain=v1
git -C F:/dev/canview-wt/t500-hil merge-base 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil merge-base --is-ancestor 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil merge-base --is-ancestor 7c63183228d68c30c809ffdc323badf1bb259096 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil diff --stat 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil diff --name-only 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil diff --no-ext-diff --binary 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil diff --check 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil diff --exit-code 8a1f13db4f40b3826376e0a2b62d9c1254bff534 --
python --version
python -B -m unittest discover -s tests -p test_hil_runner.py
python -B -m unittest discover -s tests -p 'test_*.py'
python -B tools/validate_plan.py
python -B tools/validate_document_links.py
```

Every implementation file listed below was read using:

```text
git -C F:/dev/canview-wt/t500-hil show 8a1f13db4f40b3826376e0a2b62d9c1254bff534:<repository-relative-path>
```

Documentation, workflow, scenarios and fixtures were also inspected with path-scoped `git diff --no-ext-diff <base> <candidate> -- <paths>`. CMake registration was checked with:

```powershell
git -C F:/dev/canview-wt/t500-hil grep -n -e python-unit -e unittest 8a1f13db4f40b3826376e0a2b62d9c1254bff534 -- CMakeLists.txt
```

Actual final CLI commands used `C:\Python314\python.exe -B`, with these arguments:

```text
tests/hil/run.py --suite host --seed 1 --output C:\Users\digit\AppData\Local\Temp\CV-B03-cli-27e4pyn2\host
tests/hil/validate_evidence.py C:\Users\digit\AppData\Local\Temp\CV-B03-cli-27e4pyn2\host --expect-status PASS
tests/hil/run.py --suite g2-readonly --rig-config tests/hil/rig.example.yaml --output C:\Users\digit\AppData\Local\Temp\CV-B03-cli-27e4pyn2\lab
tests/hil/validate_evidence.py C:\Users\digit\AppData\Local\Temp\CV-B03-cli-27e4pyn2\lab --expect-status SKIPPED
```

Adversarial probes ran as PowerShell here-strings piped to `python -B -`, importing the candidate’s modules directly. Junction creation used:

```powershell
New-Item -ItemType Junction -Path $env:CV_PROBE_LINK -Target $env:CV_PROBE_TARGET | Out-Null
```

Both environment variables pointed inside the probe’s temporary directory. Junctions were removed with `os.rmdir(link)` before temporary-directory cleanup.

The full binary diff SHA-256 was:

```text
52a1f04b4d218ff4c2f7eea4a163b8cb154b32cf08fffb61d688133137956649
```

Full `diff --check` exited 2 for 13 trailing-whitespace diagnostics in the archived reviewer-A report, at lines 3, 4, 63, 64, 111–113, 136–138 and 159–161. Implementation/documentation checking excluding archived reports exited zero. These archive formatting diagnostics were not treated as source defects.

An initial review-helper subprocess attempted to decode those diagnostics using CP949 and raised `UnicodeDecodeError`. The check was rerun using explicit UTF-8 decoding. This was a reviewer-helper error, not a candidate finding.

**Exact file scope**

The 29 implementation, test, workflow and task/documentation files inspected substantively were:

```text
F:/dev/canview-wt/t500-hil/.github/workflows/foundation.yml
F:/dev/canview-wt/t500-hil/docs/journal.md
F:/dev/canview-wt/t500-hil/docs/resume.md
F:/dev/canview-wt/t500-hil/docs/tasks.md
F:/dev/canview-wt/t500-hil/docs/tasks/T-500-bench-hil-harness.md
F:/dev/canview-wt/t500-hil/tests/hil/README.md
F:/dev/canview-wt/t500-hil/tests/hil/__init__.py
F:/dev/canview-wt/t500-hil/tests/hil/adapter.py
F:/dev/canview-wt/t500-hil/tests/hil/analyze.py
F:/dev/canview-wt/t500-hil/tests/hil/budget-manifest.json
F:/dev/canview-wt/t500-hil/tests/hil/events.py
F:/dev/canview-wt/t500-hil/tests/hil/fixtures/forbidden-can-tx.jsonl
F:/dev/canview-wt/t500-hil/tests/hil/rig.example.yaml
F:/dev/canview-wt/t500-hil/tests/hil/run.py
F:/dev/canview-wt/t500-hil/tests/hil/scenario.py
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/brownout.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/can-load.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/duplicate-command.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/espnow-loss.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/feedback-mismatch.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/guardian-timeouts.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/radio-pressure.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/reset-matrix.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/resource-exhaustion.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/security-injection.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/stale-gates.yaml
F:/dev/canview-wt/t500-hil/tests/hil/scenarios/uart-faults.yaml
F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py
F:/dev/canview-wt/t500-hil/tests/test_hil_runner.py
```

The complete delta contains 31 files. These two additional archived reports were inventoried and hashed, but their narrative findings were not used:

```text
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-a-initial.md
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-b-initial.md
```

Their SHA-256 values were respectively:

```text
66fad9b5a50390396648a29a519126271da9e6d3ac61ba9a25e09325ba1238d1
7368674f5f402bb3b9a0645cd224b2384be26210b77c34e161f7901db165004b
```

Supporting inspection covered root CMake Python-test registration. The unchanged repository instructions, document router, workflow rules and `.gitattributes` were retained from the earlier review; a candidate/base path comparison confirmed they were unchanged.

**CI, documentation, dependency and secret assessment**

- Windows CI runs the host harness and validator with explicit exit-code checks. CTest registers Python discovery, providing an indirect route for Linux jobs.
- The harness introduces no third-party Python dependency.
- Task state remains IN_PROGRESS. Recorded 23/74 test counts match this execution; plan and document-link checks pass.
- The lab backend remains a stub. The README’s claim that the lab command consumes scenario inventory is stronger than current behavior: `run_lab()` returns without loading scenarios.
- Existing tests miss the five reproduced current findings.
- Manual implementation-diff inspection and supplemental scans found no private-key headers, AWS access-key patterns, GitHub token patterns or VIN-pattern candidates. Archived review narratives were excluded from substantive secret scanning to preserve the requested review independence.
- Private malformed-rig diagnostics tested here no longer disclosed the private filename or absolute path.

**Unreviewed and unexecuted scope**

Physical boards/HIL, power/reset/brownout equipment, CAN analyzer, live lab backend, provisioning, target flashing and vehicle bus: **NOT_RUN**.

Linux execution, remote GitHub Actions and C/target builds were not executed. Workflow wiring was inspected; remote CI success was not independently established. No exhaustive fuzzing, concurrent filesystem-race testing or external acquisition-provenance qualification was performed. Archived reviewer narratives were not substantively reviewed.

**Final verdict: BLOCK.** The post-fix candidate was read and tested, but two P1 and three P2 findings remain open. Physical/HIL is **NOT_RUN** and vehicle CAN TX remains **NO-GO**.
