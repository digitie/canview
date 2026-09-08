**Raw post-fix review report — CV-HOSTILE-20260908-T500-A-03**

**Verdict: BLOCK**  
**Open findings: P0 0 · P1 3 · P2 2 · P3 0**  
**Physical/HIL: NOT_RUN · Vehicle CAN TX: NO-GO**

The candidate was read and tested. Six A-02 findings are fixed; three are partially addressed. The supplied 23 tests pass, but extended adversarial probes still produce false PASS evidence.

| Review metadata | Value |
|---|---|
| Execution ID | `CV-HOSTILE-20260908-T500-A-03` |
| Actual review start UTC | `2026-09-08T12:17:17Z` |
| Actual review end UTC | `2026-09-08T12:23:36Z` |
| Timestamp source | Clock tool readings |
| Repository | `F:/dev/canview-wt/t500-hil` |
| Candidate | `8a1f13db4f40b3826376e0a2b62d9c1254bff534` |
| Base | `50410ba23fcecfa1f28cea837d04a061c201d648` |
| Prior reviewed candidate | `7c63183228d68c30c809ffdc323badf1bb259096` |
| Runtime | CPython 3.14.3, Windows |
| Reviewer | Independent hostile reviewer A |

**Isolation and integrity**

Source inspection used immutable Git objects. Executed harness modules and the supplied test module were compiled directly from candidate blobs in memory. Scenario/test inputs were compared with candidate blobs before testing. All 31 changed worktree files matched candidate blobs at final verification, allowing checkout CRLF normalization.

HEAD matched the candidate and worktree status was clean before and after testing. Base ancestry was confirmed with exit 0. No repository files were edited. Test fixtures and generated evidence used temporary directories outside the checkout.

The other reviewer’s archived report was inspected only as file metadata and opaque bytes for integrity comparison; its findings were not displayed or analyzed. This is the explicit exception to semantic inspection of the changed-file inventory, required by the review-independence instruction.

| Integrity check | Result |
|---|---|
| Full immutable binary diff SHA-256 | `52a1f04b4d218ff4c2f7eea4a163b8cb154b32cf08fffb61d688133137956649` |
| Base ancestry | Exit 0 |
| Final HEAD | `8a1f13db4f40b3826376e0a2b62d9c1254bff534` |
| Final porcelain status | Empty |
| Changed-file byte comparison | 31/31 matched |
| `git diff --check` | Exit 2: Markdown hard-break trailing spaces in the archived A-02 report; details below |

**A03-P1-01 — Counts still substitute for per-case coverage, causal ordering, and completion**

**Locations:**

- `F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:203–234`
- `F:/dev/canview-wt/t500-hil/tests/hil/scenarios/stale-gates.yaml:11–13`
- `F:/dev/canview-wt/t500-hil/tests/hil/scenarios/duplicate-command.yaml:11–15`
- `F:/dev/canview-wt/t500-hil/tests/hil/adapter.py:108–122`
- Corresponding scenario expectation blocks listed in the file manifest.

**Status: OPEN. Related prior finding: A02-P1-04.**

Starting with valid stock host evidence, the following mutations still passed both analyzer and saved-evidence validation:

| Mutation | Analyzer | Validator |
|---|---|---|
| Replace all safety-check names with `stale-revision` | PASS | PASS |
| Replace all brownout stages with `config` | PASS | PASS |
| Replace all security vectors with `control-tag-mutation` | PASS | PASS |
| Replace all UART fault names with `insert` | PASS | PASS |
| Replace all guardian names with `acc` | PASS | PASS |
| Replace every command token with `token-001` | PASS | PASS |
| Reverse reset events and rebuild valid timestamps/sequences/offsets | PASS | PASS |
| Reverse command replay events similarly | PASS | PASS |
| Remove `HARNESS_COMPLETE` and `TX_GATE_STATE` from brownout evidence | PASS | PASS |

The adapter now distinguishes initially accepted tokens from duplicates. However, assertions do not bind these outcomes to the requested token history. Likewise, changing `result-before-ack` from `TIMEOUT` to the label `RESULT_BEFORE_ACK` does not provide or verify an ACK/result ordering sequence.

**Failure and impact:** Duplicate cases can replace required cases; causally reversed or incomplete execution remains acceptable evidence. The expanded inventory does not yet establish its claimed scenario semantics.

**Recommendation:** Bind assertions to every requested case identifier and token history, enforce causal event order, and require a correctly positioned completion record and final gate state. Represent result-before-ACK as an ordered interaction rather than only a result label.

**A03-P1-02 — Later malformed safety outcomes remain fail-open**

**Location:** `F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:158–171`

**Status: OPEN. Related prior finding: A02-P1-04.**

After valid stock safety events, insert either of these events **before the final gate/completion records**:

```python
{
    "kind": "SAFETY_DECISION",
    "fields": {
        "check": "expired-lease",
        "decision": "DENY",
        "vehicle_tx": 1,
    },
}
```

```python
{
    "kind": "SAFETY_DECISION",
    "fields": {
        "check": "expired-lease",
        "decision": "UNKNOWN",
        "vehicle_tx": False,
    },
}
```

Both return analyzer PASS and validator PASS.

The safety predicate rejects only exact `decision == "ALLOW"` or `vehicle_tx is True`. It does not require a valid DENY decision and an explicitly false boolean on every safety event.

**Failure and impact:** Unknown decisions and malformed active-TX indications escape the fail-closed contract. Earlier valid events satisfy the positive assertions and conceal the later invalid outcome.

**Recommendation:** Validate the complete typed contract of every safety event. For CAPTURE_ONLY, require `decision == "DENY"` and `vehicle_tx is False`; reject missing, unknown, and incorrectly typed values.

**A03-P1-03 — Budget recomputation still trusts summaries that contradict logged measurements**

**Locations:**

- `F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:236–258`
- `F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:314–319`

**Status: OPEN. Related prior finding: A02-P1-02.**

Starting with valid `resource-exhaustion` evidence, insert either event before final gate/completion, retain the normal report metrics, and recompute the reported analyzer result:

```python
{
    "kind": "RESOURCE_SUMMARY",
    "fields": {
        "pool": "observer",
        "queue_depth": 999,
        "heap_free_bytes": 1,
        "rejected": 0,
        "observer_drops": 0,
    },
}
```

Or:

```python
{
    "kind": "BUDGET_SAMPLE",
    "fields": {
        "metrics": {
            "map_bytes": 999999,
            "stack_bytes": 9999,
            "heap_free_bytes": 1,
            "queue_depth": 999,
            "wcet_us": 999999,
            "latency_us": 999999,
        },
    },
}
```

**Observed for both:** analyzer PASS, validator PASS.

**Failure and impact:** Recomputing the analyzer against report-supplied metrics does not establish consistency with the event log. Explicitly logged overruns are accepted when final metrics remain within budget.

**Recommendation:** Derive or reconcile budget extrema from applicable event samples. Reject contradictory summaries and evaluate every relevant sample. For deterministic host evidence, another option is to replay the trusted scenario and verify the generated measurements and events.

**A03-P2-01 — Host failure reporting crashes and leaves reusable PASS evidence behind**

**Locations:**

- `F:/dev/canview-wt/t500-hil/tests/hil/run.py:23`
- `F:/dev/canview-wt/t500-hil/tests/hil/run.py:191–209`
- `F:/dev/canview-wt/t500-hil/tests/hil/run.py:230–242`
- `F:/dev/canview-wt/t500-hil/tests/hil/run.py:345–351`

**Status: OPEN. Related prior finding: A02-P2-02; new error-path regression.**

`_blocked_report()` references `HOST_ADAPTER_VERSION`, but that name is not imported.

A valid, accepted **336,013-byte** scenario containing 256 reset actions, each with 128 `"target"` entries, reaches the EventLog output limit. Instead of producing BLOCKED evidence:

```text
LOG_LIMIT_RUN_EXCEPTION NameError name 'HOST_ADAPTER_VERSION' is not defined
```

When the output directory previously contained a valid full host run:

```text
OLD_REPORT_UNCHANGED True
OLD_REPORT_VALIDATOR PASS
```

An accepted generic event with `fields={"kind":"nested-kind"}` also reaches this broken error path through an argument collision. On fresh output:

```text
ENCODING_COLLISION_EXCEPTION NameError ... report_exists False
```

Separately, a parser-rejected retry returns 2 but leaves the previous PASS report intact and still valid.

**Failure and impact:** The command does not falsely return success, but it fails to publish the promised machine-readable failure. Reused output can continue presenting an earlier PASS as the available evidence after a failed attempt.

**Recommendation:** Fix the missing import and validate generated host BLOCKED reports against the validator contract. Establish attempt identity or mark an attempt incomplete before execution. Publish terminal failure evidence for parse, execution, and output failures so reused directories cannot silently retain an apparently current PASS.

**A03-P2-02 — Custom FAIL evidence accepts impossible first-violation locations**

**Location:** `F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:301–308`

**Status: OPEN. New regression adjacent to the A02-P2-04 fix.**

Generate a legitimate custom forbidden-TX FAIL report. Change its first violation to:

```python
first_violation["log_offset"] = 999999
first_violation["event_sequence"] = 999999
violations[0] = copy.deepcopy(first_violation)
```

Keep the actual small event log unchanged.

**Observed:**

```text
FORGED_FAIL_OFFSET ACCEPTED FAIL
```

For an unknown/custom scenario, validation checks the TX-related invariant name and then skips recomputation. It does not establish that the recorded sequence and offset identify the offending event.

**Failure and impact:** A validated failure artifact can point outside its log, breaking the promised first-invariant/location evidence contract.

**Recommendation:** Validate scenario-independent invariants and their exact event locations for custom FAIL reports. Require the referenced sequence and byte offset to exist and match the relevant violation. Use an explicit trusted custom-scenario input if scenario-specific recomputation is required.

**Disposition of every A-02 finding**

| Prior finding | Disposition | Post-fix evidence |
|---|---|---|
| A02-P1-01 — Physical HIL relabeling | **FIXED** | Relabeled physical PASS rejected; relabeling to `g2-readonly` with NOT_RUN also rejected |
| A02-P1-02 — Forged/empty/contradictory PASS evidence | **PARTIALLY FIXED** | Empty logs, direct/summary TX, missing metrics, false checks, changed metrics, and partial inventory rejected; logged budget overruns still pass under A03-P1-03 |
| A02-P1-03 — Invalid channel TX counts | **FIXED** | Integer positive, float, boolean, string, null, negative, and missing counts all fail |
| A02-P1-04 — Weak scenario assertions | **PARTIALLY FIXED** | Original simple counterexamples now fail; case substitution, ordering/completion, and malformed later outcomes remain under A03-P1-01/02 |
| A02-P2-01 — Malformed assertions/event contracts | **FIXED for the reported reproductions** | Wrong assertion container types and the original malformed event now fail |
| A02-P2-02 — Execution/output limits | **PARTIALLY FIXED** | Trillion-packet input and oversized event fields rejected; EventLog enforces output limits, but handling exhaustion crashes under A03-P2-01 |
| A02-P2-03 — EventLog copying/atomicity | **FIXED** | Nested mutation leaves stored events unchanged; failed append leaves records and byte count unchanged |
| A02-P2-04 — Legitimate forbidden-TX FAIL rejected | **FIXED** | Custom forbidden-TX run returns 1 and its correctly recorded FAIL validates; forged locations are separately tracked as A03-P2-02 |
| A02-P2-05 — Budget extrema overwritten | **FIXED** | Queue depth remains 999 and heap minimum remains 1 after later normal samples; both budget violations reported |

No prior finding was omitted or closed solely because supplied tests passed.

**Executed tests and raw results**

| Verification | Result |
|---|---|
| Candidate HIL unit tests | **23 run; 0 failures; 0 errors; 0 skipped** |
| Stock host suite, seed 1 | **12/12 PASS** |
| Stock host suite exercised by unit tests, seed 7 | PASS |
| Original stock evidence validation | PASS |
| Determinism | **36 paired runs identical**: 12 scenarios × seeds `0`, `1`, `2^64−1` |
| Original malformed TX-count probes | All FAIL |
| Original malformed expectation probes | FAIL |
| Original malformed event probe | FAIL |
| Original six simple semantic counterexamples | All FAIL |
| Deep-copy accounting | Stored/actual offset both 128; stored/actual bytes both 241 |
| Failed NaN append | `EventLogError`; zero records retained; zero bytes |
| Physical-PASS relabel | Rejected |
| Lab relabel retaining NOT_RUN | Rejected |
| Empty scenario log | Rejected |
| Positive integer channel TX summary | Rejected |
| Missing metrics / false checks | Rejected |
| Modified metric without matching result | Rejected |
| Partial results with matching self-declared partial inventory | Rejected against trusted inventory |
| Missing commit/zero source digest probe | Rejected |
| Legitimate custom forbidden-TX FAIL | Runner exit 1; validator accepts FAIL |
| Oversized generic event fields | Parser rejects; runner exit 2 |
| Trillion-packet action | `ScenarioError` |
| Deeply nested scenario | `ScenarioError` |
| Missing lab configuration | Exit 2, BLOCKED |
| Example unavailable rig | Exit 2, SKIPPED |
| Rig claiming availability without backend | Exit 2, BLOCKED |
| Symlinked output test | Rejected; test passed |
| Extended semantic/resource probes | False PASS results detailed above |
| Output-limit handling | `NameError`; earlier PASS preserved |
| Forged custom FAIL location | Incorrectly accepted |

The budget-extrema recheck used empty scenario expectations to isolate budget behavior. It reported:

```text
heap_free_bytes = 1
queue_depth = 999
violations = [
    "budget.heap_free_bytes.minimum",
    "budget.queue_depth.maximum",
]
```

**Commands and execution method**

Identity and diff commands executed included:

```powershell
git -C F:/dev/canview-wt/t500-hil rev-parse '8a1f13db4f40b3826376e0a2b62d9c1254bff534^{commit}'
git -C F:/dev/canview-wt/t500-hil rev-parse '50410ba23fcecfa1f28cea837d04a061c201d648^{commit}'
git -C F:/dev/canview-wt/t500-hil rev-parse HEAD
git -C F:/dev/canview-wt/t500-hil status --porcelain=v1
git -C F:/dev/canview-wt/t500-hil merge-base --is-ancestor 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil diff --stat 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil diff --name-only 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil diff --binary --no-ext-diff 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
git -C F:/dev/canview-wt/t500-hil diff --check 50410ba23fcecfa1f28cea837d04a061c201d648 8a1f13db4f40b3826376e0a2b62d9c1254bff534
```

Candidate source reads used:

```text
git -C F:/dev/canview-wt/t500-hil show 8a1f13d:<repository-relative-file>
```

Scoped base-to-candidate diffs covered the workflow, documentation, complete scenario inventory, README, manifest, fixture, and rig template. The policy/context files were compared against the previous reviewed candidate and were unchanged.

Python tests and probes ran through PowerShell here-strings piped to `python -B -`. The immutable loader used:

```python
repo = Path("F:/dev/canview-wt/t500-hil")
rev = "8a1f13db4f40b3826376e0a2b62d9c1254bff534"

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

The test module was also compiled from its blob and executed using:

```python
unittest.TextTestRunner(verbosity=2).run(
    unittest.defaultTestLoader.loadTestsFromModule(m)
)
```

Runner probes called `hil.run.main()` with these argument patterns:

```text
--suite host --seed 1 --output <temporary-output>
--suite host --scenario-dir <temporary-scenarios> --output <temporary-output>
--suite g2-readonly --output <temporary-output>
--suite g2-readonly --rig-config <rig-file> --output <temporary-output>
```

Saved-evidence probes used `validate(path, "PASS")`, `validate(path, "FAIL")`, or `validate(path, "BLOCKED")`.

For event mutations, each log was rebuilt through `EventLog.append()` so sequences, timestamps, offsets, and byte lengths remained valid. The report’s checks and violations were recomputed with the candidate analyzer. This isolates semantic acceptance from ordinary serialization corruption. Safety and budget insertions were verified before final completion as well as after it.

One archive-inspection command encountered a terminal `UnicodeEncodeError`; it was rerun with UTF-8 stdout. This did not affect source execution.

`git diff --check` reported trailing spaces only in:

```text
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-a-initial.md
```

Affected lines: `3, 4, 63, 64, 111, 112, 113, 136, 137, 138, 159, 160, 161`. These are Markdown hard-break spaces in the archived raw report. The nonzero check is recorded; no separate functional P3 finding is asserted.

**Exact file scope**

Semantically inspected changed files:

```text
F:/dev/canview-wt/t500-hil/.github/workflows/foundation.yml
F:/dev/canview-wt/t500-hil/docs/journal.md
F:/dev/canview-wt/t500-hil/docs/resume.md
F:/dev/canview-wt/t500-hil/docs/tasks.md
F:/dev/canview-wt/t500-hil/docs/tasks/T-500-bench-hil-harness.md
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-a-initial.md
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

Metadata/integrity only, with findings deliberately excluded:

```text
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-b-initial.md
```

Its recorded Git blob is `721d5cfc6b1ce5360e92bbcb32fb1a6cee3dc270`, size 16,948 bytes.

Previously read governing files were confirmed unchanged from A-02:

```text
F:/dev/canview-wt/t500-hil/AGENTS.md
F:/dev/canview-wt/t500-hil/docs/README.md
F:/dev/canview-wt/t500-hil/docs/architecture/README.md
F:/dev/canview-wt/t500-hil/docs/runbooks/agent-workflow.md
```

**Unreviewed or unexecuted scope**

- Physical board flashing, execution, and HIL: **NOT_RUN**.
- Physical CAN traffic and electrical TX-zero measurements: **NOT_RUN**.
- Power/reset/rail/brownout, clock, UART/FDCAN, and radio measurements: **NOT_RUN**.
- Vehicle integration and vehicle CAN transmission: **NOT_RUN; NO-GO**.
- Full repository regression, target builds, and remote CI: **NOT_RUN in this review**. Documentation’s broader test claims were not independently rerun.
- Cross-platform/runtime determinism and exhaustive maximum-workload performance: not established.
- Other reviewer’s findings: deliberately not consumed.

No physical CAN transmission was performed. Host simulation does not establish physical TX-zero evidence.

**Final disposition: BLOCK.** Three P1 and two P2 findings remain OPEN. The post-fix candidate improves several boundaries but does not yet provide fail-closed scenario and evidence qualification.
