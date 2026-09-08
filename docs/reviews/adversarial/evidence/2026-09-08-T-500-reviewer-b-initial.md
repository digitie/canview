**BLOCK — CV-HOSTILE-20260908-T500-B-02**

This is an administrative correction of the completed raw report, adding the supplied execution timestamps. It is **not a post-fix review**. The reviewed candidate, findings and verdict are unchanged. No additional tests or repository edits were performed for this correction.

| Field | Value |
|---|---|
| Execution ID | `CV-HOSTILE-20260908-T500-B-02` |
| Start UTC | `2026-09-08T11:46:09Z` |
| End UTC | `2026-09-08T11:53:53Z` |
| Candidate | `7c63183228d68c30c809ffdc323badf1bb259096` |
| Base | `50410ba23fcecfa1f28cea837d04a061c201d648` |
| Repository | `F:/dev/canview-wt/t500-hil` |
| Findings | **P0: 0 · P1: 3 · P2: 5 · P3: 0** |
| Disposition | All eight findings **OPEN** |
| Physical/HIL | **NOT_RUN** |
| Verdict | **BLOCK** |

Scope: Python portability/parser safety; path traversal, symlink and output handling; evidence tamper resistance and reproducibility; CI; negative/mutation coverage; dependency/DAG/documentation consistency; secret/private-material leakage.

**Isolation and candidate verification**

Immutable contents were inspected using `git -C F:/dev/canview-wt/t500-hil show` and `diff`. Software tests executed against the supplied worktree after verifying that HEAD and tracked contents matched the candidate. This was **not a detached review checkout**.

Final verification found the same HEAD, empty porcelain status and no tracked difference from the candidate. No source files were edited. Probes generated temporary fixtures and outputs through `tempfile.TemporaryDirectory()`.

The supplied UTC timestamps above replace the previous report’s statement that a separate start timestamp was unavailable.

**Findings**

**B-01 — P1 — Evidence validator accepts unsafe or absent evidence as PASS**

Location: [validate_evidence.py:157](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:157), particularly lines 157–180.

Independent mutations of a valid 12-scenario report produced:

| Exact mutation | Observed result |
|---|---|
| Replace `"tx_frames":0` with `"tx_frames":1` in `can-load.jsonl`, preserving lengths and offsets | Validator accepts PASS |
| Point every scenario at an empty JSONL and set `event_count=0`, `event_bytes=0` | Validator accepts PASS |
| Set `map_bytes=999999999` and a check’s `passed=false` | Validator accepts PASS |

The analyzer rejects the same nonzero channel-summary TX mutation. The validator only recognizes explicit `CAN_TX`/`VEHICLE_CAN_TX` kinds, permits zero events and does not evaluate reported metrics or checks.

**Impact:** Evidence validation can certify artifacts that contradict required safety and budget invariants.

**Recommendation:** Recompute verdicts against trusted scenario/budget definitions using shared invariant logic. Require actual events for completed scenarios and compare recomputed checks, violations and status with the report.

**B-02 — P1 — Relabeling host evidence permits physical PASS**

Location: [validate_evidence.py:104](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:104).

Starting with valid host evidence, change only:

```python
report["suite"] = "g2-readonly"
report["physical_hil"]["status"] = "PASS"
```

`validate(report_path, "PASS")` succeeds. The report still identifies the host simulator and retains disconnected/nonhardware adapter metadata.

**Impact:** Synthetic host evidence can cross the explicitly prohibited physical-evidence boundary.

**Recommendation:** Validate consistency between suite, adapter, execution provenance and physical status. Because the current contract has no connected lab backend, it must reject physical PASS. Future hardware success requires independently validated acquisition provenance.

**B-03 — P1 — One safe event masks unsafe safety decisions**

Locations: [stale-gates.yaml:12](F:/dev/canview-wt/t500-hil/tests/hil/scenarios/stale-gates.yaml:12), [analyze.py:131](F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:131).

Generate normal `stale-gates` records and modify only the second safety decision:

```python
records[1]["fields"].update(decision="ALLOW", vehicle_tx=True)
```

The analyzer returns `PASS` and `violations=[]`. Changing every safety decision to unsafe returns FAIL.

The scenario requires only one matching DENY event, allowing other checks to contradict it. Brownout and guardian assertions use the same existential pattern.

**Impact:** Passing safety scenarios do not establish safe behavior for each configured gate or fault case.

**Recommendation:** Require coverage and expected outcomes for every named case. Independently reject contradictory unsafe decisions or gate states. Add single-record mutations, including an unsafe record following a valid first record.

**B-04 — P2 — Identity and coverage claims are not verified**

Locations: [validate_evidence.py:95](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:95), [validate_evidence.py:190](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:190).

The validator still accepts PASS after:

- Changing the firmware commit to `"not-a-commit"`.
- Replacing firmware, harness and scenario digests with 64 zeroes.
- Changing a derived scenario seed to zero.
- Removing eleven results and adjusting the report’s inventory to the remaining result.

There is no trusted expected revision, inventory, scenario content or budget against which to verify these declarations.

Separately, `_read_json()` accepts duplicate report keys. Successive `"status":"FAIL"` and `"status":"PASS"` entries validate as PASS.

**Impact:** Accepted reports do not establish source identity, intended suite completeness or reproducibility. Structural event validation does not provide those guarantees.

**Recommendation:** Accept trusted expected identities and selected inventory; verify scenario seeds/digests and budget identity; reject duplicate JSON keys. Distinguish structural validation from verified provenance.

**B-05 — P2 — Scenario schema permits disabled assertions, unbounded computation and uncaught errors**

Locations: [scenario.py:144](F:/dev/canview-wt/t500-hil/tests/hil/scenario.py:144), [analyze.py:107](F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:107), [adapter.py:56](F:/dev/canview-wt/t500-hil/tests/hil/adapter.py:56).

| Input | Observed failure |
|---|---|
| `required_kinds="MISSING"` and `required_fields="MISSING"` | Assertions silently discarded; analyzer PASS |
| `radio_loss`, `packets=1000000000000` | Parses; child exceeds two-second timeout and is killed |
| `radio_delay` without `max_ms` | Uncaught `KeyError` |
| `radio_loss`, `packets=null` | Uncaught `TypeError` |
| `radio_delay`, `max_ms=1e309` | Infinity accepted during parsing; uncaught `OverflowError` |

File-size and action-count limits do not bound computation controlled by action parameters. `parse_constant` does not reject exponent overflow such as `1e309`.

**Impact:** Malformed scenarios can produce false success, consume unbounded execution time or terminate without controlled failure evidence.

**Recommendation:** Validate action-specific types, required fields, numeric ranges and execution limits before running. Reject malformed expectations and all nonfinite values. Convert invalid input into a controlled failure result.

**B-06 — P2 — Output writes follow links outside the output directory**

Locations: [run.py:125](F:/dev/canview-wt/t500-hil/tests/hil/run.py:125), [run.py:162](F:/dev/canview-wt/t500-hil/tests/hil/run.py:162), [events.py:75](F:/dev/canview-wt/t500-hil/tests/hil/events.py:75).

Two temporary-directory probes reproduced the failure:

1. Precreate `output/events` as a directory symlink to another directory. The runner writes `brownout.jsonl` outside the output directory and returns zero.
2. Precreate `output/report.json` as a hardlink to a separate sentinel file. The runner overwrites the sentinel and returns zero.

The validator rejects the escaped event symlink afterward, but the external write has already happened.

**Impact:** Existing output links can redirect writes or overwrite another file.

**Recommendation:** Validate containment and link handling before writing. Use safely created temporary files and atomic replacement, with an explicit existing-output policy. Do not truncate existing linked files.

**B-07 — P2 — Invalid private rig configuration leaks its absolute path into evidence**

Location: [run.py:221](F:/dev/canview-wt/t500-hil/tests/hil/run.py:221).

A malformed private rig file generated a BLOCKED report containing its complete absolute path in `failure.reason` and the physical failure reason. The exception is interpolated directly into report metadata and console output. Unsupported adapter diagnostics can also echo the supplied field value.

**Impact:** Shared failure evidence can expose private configuration paths or values despite the documented private/public metadata boundary.

**Recommendation:** Publish stable sanitized error codes. Keep detailed parser diagnostics in a separate private diagnostic channel.

**B-08 — P2 — Valid forbidden-TX failure evidence cannot validate as FAIL**

Location: [validate_evidence.py:166](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:166).

A custom scenario emitting `CAN_TX` correctly makes the runner return `1` and generate a FAIL report. However:

```python
validate(report_path, "FAIL")
```

raises:

```text
EvidenceError: capture-only TX event in forbidden-tx
```

**Impact:** The validator treats an accurately recorded scenario failure as invalid evidence, preventing uniform validation of the required negative artifact.

**Recommendation:** Recompute the violation and verify that the report faithfully records FAIL. Keep evidence validity separate from scenario success.

**Commands and results**

Python commands ran from `F:/dev/canview-wt/t500-hil`.

```powershell
git -C F:/dev/canview-wt/t500-hil rev-parse --show-toplevel
git -C F:/dev/canview-wt/t500-hil rev-parse 7c63183
git -C F:/dev/canview-wt/t500-hil rev-parse 50410ba23fcecfa1f28cea837d04a061c201d648
git -C F:/dev/canview-wt/t500-hil rev-parse HEAD
git -C F:/dev/canview-wt/t500-hil status --short
git -C F:/dev/canview-wt/t500-hil diff --exit-code 7c63183 --
git -C F:/dev/canview-wt/t500-hil diff --stat 50410ba23fcecfa1f28cea837d04a061c201d648 7c63183
git -C F:/dev/canview-wt/t500-hil diff --check 50410ba23fcecfa1f28cea837d04a061c201d648 7c63183
python --version
python -B -m unittest discover -s tests -p test_hil_runner.py
python -B -m unittest discover -s tests -p 'test_*.py'
python -B tools/validate_plan.py
python -B tools/validate_document_links.py
```

Final verification also executed:

```powershell
git -C F:/dev/canview-wt/t500-hil status --porcelain=v1
git -C F:/dev/canview-wt/t500-hil diff --exit-code 7c63183228d68c30c809ffdc323badf1bb259096 --
```

| Check | Result |
|---|---|
| Python | Windows Python 3.14.3 |
| T-500 unit tests | 13/13 PASS |
| Complete Python discovery | 64/64 PASS |
| Host baseline, seed 1 | 12 scenarios PASS; validator PASS |
| Plan validation | 49 tasks; zero errors |
| Document links | 272 documents; 1,268 targets; zero errors |
| Diff whitespace check | PASS |
| Final HEAD | Exact candidate |
| Final porcelain status | Empty |
| Candidate/worktree tracked diff | Exit 0 |

Host and mutation probes ran through PowerShell here-strings piped to `python -B -`. They imported `hil.run`, `hil.scenario`, `hil.adapter`, `hil.events`, `hil.analyze` and `hil.validate_evidence`.

The host baseline API invocation was:

```python
main(["--suite", "host", "--seed", "1", "--output", str(out)])
validate(out / "report.json", "PASS")
```

Each report mutation started from a fresh copy of the original baseline report. Event modifications were restored between independent probes.

The bounded computation probe used:

```python
subprocess.run(
    [sys.executable, "-B", "tests/hil/run.py",
     "--suite", "host", "--scenario-dir", str(scenarios),
     "--output", str(root / "slow")],
    capture_output=True, text=True, timeout=2,
)
```

Additional observed defenses:

- `../escaped.jsonl` was rejected by the validator.
- An event symlink resolving outside the report directory was rejected by the validator.
- Excessive scenario nesting raised `ScenarioError` on Python 3.14.3.
- Existing duplicate-scenario-key and tampered-offset tests passed.
- Missing/unavailable lab hardware did not make the runner claim success.

The immutable binary diff command was:

```powershell
git -C F:/dev/canview-wt/t500-hil diff --no-ext-diff --binary 50410ba23fcecfa1f28cea837d04a061c201d648 7c63183228d68c30c809ffdc323badf1bb259096
```

SHA-256 of its output:

```text
624a38ac1167f516c2c873a60405e174e87e0d4823a69ae6b2ff033fb75f7700
```

**Exact changed-file scope**

All 29 changed files were inspected through immutable Git objects/diffs:

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

Supporting inspection covered:

```text
F:/dev/canview-wt/t500-hil/AGENTS.md
F:/dev/canview-wt/t500-hil/.gitattributes
F:/dev/canview-wt/t500-hil/CMakeLists.txt
F:/dev/canview-wt/t500-hil/docs/README.md
F:/dev/canview-wt/t500-hil/docs/runbooks/agent-workflow.md
F:/dev/canview-wt/t500-hil/docs/development/windows.md
F:/dev/canview-wt/t500-hil/tools/validate_plan.py
F:/dev/canview-wt/t500-hil/docs/tasks/T-001-host-toolchain-ci.md
F:/dev/canview-wt/t500-hil/docs/tasks/T-003-espnow-codec-session.md
F:/dev/canview-wt/t500-hil/docs/tasks/T-004-uart-schema-codec.md
F:/dev/canview-wt/t500-hil/docs/tasks/T-103-stm32-fdcan-capture.md
```

**CI, dependencies, documentation and leakage assessment**

- New Windows workflow commands propagate failures and upload HIL output. Root CTest includes Python discovery, providing an indirect route for these tests in Linux jobs.
- No new third-party Python dependency was introduced.
- T-500 remains IN_PROGRESS. Dependency and document-link checks reported no inconsistency.
- Actual hardware execution remains deferred. The README’s statement that the lab command consumes the same scenario inventory describes intended behavior: the current lab stub returns before loading scenarios.
- Passing existing tests do not cover the reproduced semantic, provenance, per-case safety, output-link and malformed-action failures.
- Manual added-diff inspection found no actual secret, VIN or private capture material. Supplemental added-line scans found zero private-key headers, AWS access-key patterns, GitHub token patterns or VIN-pattern candidates. These limited scans do not negate B-07’s reproduced runtime diagnostic leakage.

**Unreviewed or unexecuted scope**

Physical boards/HIL, power/reset/brownout equipment, CAN analyzer, vehicle bus, provisioning and live lab backend: **NOT_RUN**.

Linux execution, remote GitHub Actions, C/target builds and target flashing were not executed during this review. Existing workflow wiring was inspected; remote execution success was not independently established. Temporary probes do not constitute hardware qualification or exhaustive fuzzing.

**Final verdict: BLOCK.** Three reproduced P1 findings prevent acceptance of the evidence and safety gates. All eight findings remain OPEN. Physical/HIL remains **NOT_RUN**, and the administrative timestamp correction establishes no new source, hardware or vehicle qualification.
