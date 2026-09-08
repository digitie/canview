**Raw final review — CV-HOSTILE-20260908-T500-A-04**

**Verdict: BLOCK**  
**Open findings: P0 0 · P1 0 · P2 2 · P3 0**  
**Physical/HIL: NOT_RUN · Vehicle CAN TX: NO-GO**

The candidate was read and tested. All 36 supplied tests pass. The A-03 safety, scenario-coverage, ordering, budget, and impossible-location reproductions are corrected. Two P2 issues remain in malformed-input handling and custom failure-location validation.

| Metadata | Value |
|---|---|
| Execution ID | `CV-HOSTILE-20260908-T500-A-04` |
| Actual review start UTC | `2026-09-08T12:43:40Z` |
| Actual review end UTC | `2026-09-08T12:48:38Z` |
| Timestamp source | Clock tool readings |
| Repository | `F:/dev/canview-wt/t500-hil` |
| Candidate | `3e49ea8c4b04458c63bb28a4157c8541bbb371da` |
| Base | `50410ba23fcecfa1f28cea837d04a061c201d648` |
| Reviewer scope | Host/lab harness, parsing, EventLog, analyzer, evidence integrity, scenario semantics, output links, deterministic replay, selected inventory, tests, workflow, and documentation |
| Runtime | CPython 3.14.3, Windows |

**Isolation and integrity**

Source reads used immutable Git objects. Executed harness modules and the test module were compiled directly from candidate blobs in memory. The 24 harness/test inputs matched candidate blobs before testing. At final verification, all 33 changed worktree files matched candidate blobs, allowing checkout CRLF normalization.

HEAD matched the candidate and porcelain status was empty before and after testing. Base ancestry returned exit 0. No repository files were edited. Tests used temporary directories outside the checkout.

Other reviewers’ findings were not displayed or analyzed. Their two archive files were included only in mechanical byte comparisons and whole-diff hashing, not semantic review.

| Check | Result |
|---|---|
| Base ancestry | Exit 0 |
| Full immutable binary diff SHA-256 | `57cc3df000bedf4e2c860836c372064616f2fa1e06092d722e34a693f8044370` |
| Final HEAD | `3e49ea8c4b04458c63bb28a4157c8541bbb371da` |
| Final porcelain status | Empty |
| Changed-file comparison | 33/33 matched |
| Diff whitespace check, excluding other reviewers’ archives | Exit 2; archived Markdown hard-break spaces, detailed below |

**A04-P2-01 — Malformed event fields crash the analyzer and preserve an older PASS report**

**Locations:**

- `F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:146`
- `F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:194`
- `F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:200`
- `F:/dev/canview-wt/t500-hil/tests/hil/run.py:243–254`

**Status: OPEN. Related to the remaining error-path concern in A03-P2-01.**

The generic event action accepts JSON field maps containing these payloads:

```python
("COMMAND_REPLAY", {
    "request_token": "t",
    "executed": False,
    "result": [],
})

("FEEDBACK_RESULT", {
    "case": "feedback-success",
    "result": {},
    "tx_permitted": False,
})

("CAN_TX", {
    "arbitration_id": [],
})
```

All three scenarios pass parsing and event serialization. The analyzer then performs set membership with an unhashable list or dictionary and raises `TypeError`.

`run_host()` catches adapter and event-output failures, but its call to `analyze()` lies outside those handlers.

**Observed results:**

```text
MALFORMED_COMMAND_REPLAY EXCEPTION TypeError
PREVIOUS_REPORT_UNCHANGED True VALIDATOR PASS

MALFORMED_FEEDBACK_RESULT EXCEPTION TypeError
PREVIOUS_REPORT_UNCHANGED True VALIDATOR PASS

MALFORMED_CAN_TX EXCEPTION TypeError
PREVIOUS_REPORT_UNCHANGED True VALIDATOR PASS
```

The previous report came from a successful selected `brownout` run in the same output directory. Each malformed retry left that report unchanged.

**Failure and impact:** The malformed invocation does not return success, but it fails to produce machine-readable FAIL/BLOCKED evidence. Reused output continues exposing an earlier valid PASS after the new attempt crashes. The new typed-outcome checks therefore remain incomplete for accepted generic event payloads.

**Recommendation:** Validate scalar types before set membership. Malformed event IDs and enums should produce explicit violations. Protect analyzer execution with a terminal failure-report path so unexpected analysis errors cannot retain apparently current PASS evidence. Add end-to-end tests for these three payloads using reused output.

**A04-P2-02 — Custom FAIL validation accepts a later TX as the first violation**

**Location:** `F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:322–339`

**Status: OPEN. New extension of the failure-location validation concern.**

Generate a custom scenario containing:

```text
sequence 1: CAN_TX
sequence 2: OBSERVATION
sequence 3: CAN_TX
```

The runner correctly produces FAIL with the first TX at sequence 1. Change `first_violation.log_offset` and `event_sequence` to identify the second TX at sequence 3, and make `violations[0] match the changed first violation.

**Observed results:**

```text
VALID_CUSTOM_FAIL 1 FAIL
IMPOSSIBLE_LOCATION REJECTED
NONOFFENDING_LOCATION REJECTED
SECOND_TX_AS_FIRST ACCEPTED FAIL
```

The validator now verifies that the location exists and identifies an offending event. However, its `any(...)` check accepts any matching TX, and the custom-scenario branch then skips recomputation.

**Failure and impact:** A validated report can identify the wrong first offending event. This violates the promised first-invariant/location contract and can misdirect incident reconstruction.

**Recommendation:** Compare against the earliest offending event under the analyzer’s invariant ordering, rather than any offending event. Recompute scenario-independent violations for custom FAIL evidence, including first-event identity and location.

**Disposition of every A-03 finding**

| A-03 finding | Disposition | Evidence |
|---|---|---|
| A03-P1-01 — Counts substitute for case coverage, order, completion | **FIXED** | All six duplicate-case substitutions, reversed reset/replay order, and removed completion/gate probes now fail. Ordered case identities and feedback sequence representation are present. |
| A03-P1-02 — Malformed later safety decisions | **FIXED** | Numeric `vehicle_tx=1` and `decision="UNKNOWN"` inserted before completion now fail. |
| A03-P1-03 — Logged overruns contradict report metrics | **FIXED** | Inserted resource and budget overruns fail analysis. PASS validation also compares events and metrics with deterministic replay. |
| A03-P2-01 — Broken host BLOCKED reporting and stale output | **PARTIALLY FIXED** | Missing import, output-limit handling, parser failure, and invalid-seed paths are corrected. Analyzer exceptions still preserve older PASS evidence under A04-P2-01. |
| A03-P2-02 — Impossible custom FAIL locations | **FIXED for the reported failure** | Out-of-range and real-but-nonoffending locations are rejected. Incorrect selection of a later offending event is separately tracked as A04-P2-02. |

No A-03 P1 finding remains open. The remaining findings concern host evidence integrity; no physical-HIL success is inferred.

**Executed verification**

| Test or probe | Result |
|---|---|
| Candidate HIL unit tests | **36 run, 0 failures, 0 errors, 0 skipped** |
| Stock host suite, seed 1 | **12/12 PASS** |
| Stock host suite exercised by unit tests, seed 7 | PASS |
| Unmodified stock evidence | Validator PASS |
| Determinism | **36 paired runs identical:** 12 scenarios × seeds `0`, `1`, `2^64−1` |
| Six duplicate-case substitutions from A-03 | All analyzer FAIL |
| Reversed reset event order | Analyzer FAIL |
| Reversed command replay order | Analyzer FAIL |
| Missing completion/final gate | Analyzer FAIL |
| Numeric/unknown safety outcomes before completion | Analyzer FAIL |
| Resource/budget overruns before completion | Analyzer FAIL |
| Changed radio drop/delivery counts | Analyzer alone PASS; deterministic evidence replay rejects |
| Changed seed with correctly recomputed scenario seeds, retaining old traces | Replay rejects at `espnow-loss` |
| Physical-HIL relabeling | Rejected |
| Missing results with unchanged selected inventory | Rejected |
| Explicit selected trusted subset | Accepted, as documented |
| Selected `brownout` run | Runner and validator PASS |
| Original custom forbidden-TX FAIL | Runner exit 1; validator accepts FAIL |
| Impossible custom failure offset/sequence | Rejected |
| Nonoffending custom failure location | Rejected |
| Later TX relabeled as first | **Incorrectly accepted** |
| Output-limit retry on reused output | Exit 2; valid BLOCKED report replaces earlier report |
| Parser-failure retry | Exit 2; valid BLOCKED report |
| Invalid seed | Unit test passes; BLOCKED evidence generated |
| Reserved field names | Supported; generic event run returns 0 |
| Deep-copy and atomic failed-append tests | Pass |
| Boolean EventLog timestamp | Rejected |
| Oversized action/nonfinite/duplicate-key/channel tests | Pass |
| Malformed enum/ID payloads | **Unhandled TypeError; earlier PASS remains** |

The selected-inventory behavior is now explicitly documented: validation certifies the declared trusted subset. A matching explicit subset is therefore not treated as a full-suite result.

**Output-link safety probes**

Tests used only temporary files and directories.

| Probe | Result |
|---|---|
| Top-level output symlink | Rejected; unit test passed |
| Top-level Windows junction | Rejected; unit test passed |
| `events/` directory symlink | Exit 2; outside directory remained empty |
| `events/` directory junction | Exit 2; outside directory remained empty |
| `report.json` symlink | Exit 2; outside target unchanged |
| Existing report hardlink | Atomic replacement succeeded; outside target unchanged |
| Existing event-log hardlink | Atomic replacement succeeded; outside target unchanged |

The hardlink case produced valid selected-host evidence:

```text
HARDLINK_OUTPUT 0 PASS
REPORT_TARGET_UNCHANGED True
EVENT_TARGET_UNCHANGED True
```

No race-condition guarantees are claimed from these fixed-link tests.

**Commands and execution method**

Identity and immutable comparison commands included:

```powershell
git -C F:/dev/canview-wt/t500-hil rev-parse '3e49ea8c4b04458c63bb28a4157c8541bbb371da^{commit}'
git -C F:/dev/canview-wt/t500-hil rev-parse '50410ba23fcecfa1f28cea837d04a061c201d648^{commit}'
git -C F:/dev/canview-wt/t500-hil rev-parse HEAD
git -C F:/dev/canview-wt/t500-hil status --porcelain=v1
git -C F:/dev/canview-wt/t500-hil merge-base --is-ancestor 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil diff --stat 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil diff --name-only 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil diff --binary --no-ext-diff 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da
```

Candidate source reads used:

```text
git -C F:/dev/canview-wt/t500-hil show 3e49ea8:<repository-relative-file>
```

Scoped base-to-candidate diffs covered all scenario files, workflow, documentation, README, fixture, manifests, and rig template. Governing files were compared against A-03 and were unchanged.

Tests and probes ran through PowerShell here-strings piped to `python -B -`. The immutable module loader was:

```python
repo = Path("F:/dev/canview-wt/t500-hil")
rev = "3e49ea8c4b04458c63bb28a4157c8541bbb371da"

def blob(p):
    return subprocess.check_output(
        ["git", "-C", str(repo), "show", rev + ":" + p]
    )

pkg = types.ModuleType("hil")
pkg.__path__ = [str(repo / "tests/hil")]
sys.modules["hil"] = pkg

for name in [
    "events", "scenario", "adapter",
    "analyze", "run", "validate_evidence",
]:
    p = "tests/hil/" + name + ".py"
    m = types.ModuleType("hil." + name)
    m.__file__ = str(repo / p)
    m.__package__ = "hil"
    sys.modules[m.__name__] = m
    exec(compile(blob(p), m.__file__, "exec"), m.__dict__)
```

The candidate test module was compiled from its blob and run with:

```python
unittest.TextTestRunner(verbosity=2).run(
    unittest.defaultTestLoader.loadTestsFromModule(m)
)
```

Runner probes invoked `hil.run.main()` with these argument patterns:

```text
--suite host --seed 1 --output <temporary-output>
--suite host --scenario brownout --output <temporary-output>
--suite host --scenario-dir <temporary-scenarios> --output <temporary-output>
--suite g2-readonly --output <temporary-output>
--suite g2-readonly --rig-config <rig-file> --output <temporary-output>
```

Evidence probes called `validate(path, "PASS")`, `validate(path, "FAIL")`, or `validate(path, "BLOCKED")`.

Event mutations were rebuilt through EventLog so timestamps, sequences, offsets, and byte lengths were internally consistent. Report checks and violations were recomputed where appropriate. Seed replay testing recomputed each scenario seed correctly before submitting the old trace.

Hardlinks used `os.link()`. Junction creation used:

```text
cmd.exe /c mklink /J <temporary-junction> <temporary-target>
```

The whitespace check executed was:

```powershell
git -C F:/dev/canview-wt/t500-hil diff --check 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da -- . ':(exclude)docs/reviews/adversarial/evidence/*reviewer-b*'
```

It returned 2 for Markdown hard-break spaces in the A-02 archive at lines `3, 4, 63, 64, 111, 112, 113, 136, 137, 138, 159, 160, 161`, and the A-03 archive at lines `3, 4`. This is recorded without asserting a separate functional P3 finding.

**Exact changed-file scope**

Semantically inspected:

```text
F:/dev/canview-wt/t500-hil/.github/workflows/foundation.yml
F:/dev/canview-wt/t500-hil/docs/journal.md
F:/dev/canview-wt/t500-hil/docs/resume.md
F:/dev/canview-wt/t500-hil/docs/tasks.md
F:/dev/canview-wt/t500-hil/docs/tasks/T-500-bench-hil-harness.md
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-a-initial.md
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-a-postfix-8a1.md
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

Excluded from semantic inspection to preserve independence:

```text
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-b-initial.md
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-b-postfix-8a1.md
```

Previously read governing files, confirmed unchanged:

```text
F:/dev/canview-wt/t500-hil/AGENTS.md
F:/dev/canview-wt/t500-hil/docs/README.md
F:/dev/canview-wt/t500-hil/docs/architecture/README.md
F:/dev/canview-wt/t500-hil/docs/runbooks/agent-workflow.md
```

**Unreviewed or unexecuted scope**

- Physical board flashing and HIL: **NOT_RUN**.
- CAN analyzer, physical bus traffic, electrical TX-zero evidence: **NOT_RUN**.
- Power/reset/rail/brownout, clock, UART/FDCAN, and radio measurements: **NOT_RUN**.
- Vehicle integration and vehicle CAN transmission: **NOT_RUN; NO-GO**.
- Full repository regression, target builds, remote CI, and documentation validators: **NOT_RUN in this review**. The documented 87-test result was not independently reproduced.
- Cross-platform/runtime determinism, exhaustive maximum-workload performance, and concurrent link-replacement races: not established.
- Other reviewers’ findings: not consumed.

No physical CAN transmission was performed. Passing host simulations and deterministic replay do not establish physical-HIL qualification.

**Final disposition: BLOCK for final harness closure pending the two P2 corrections.** No P0/P1 finding remains, but the candidate does not yet satisfy its malformed-input failure-reporting and first-violation evidence contracts.
