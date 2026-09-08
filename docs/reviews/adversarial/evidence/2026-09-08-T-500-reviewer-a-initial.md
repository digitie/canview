**Corrected raw review report — CV-HOSTILE-20260908-T500-A-02**

**Verdict: BLOCK**  
**Findings: P0 0 · P1 4 · P2 5 · P3 0 — all nine findings OPEN**  
**Physical/HIL: NOT_RUN · Vehicle CAN TX: NO-GO**

This is an administrative correction of the completed review’s timestamps. The candidate, findings, results, and verdict are unchanged. No post-fix source review was performed and no repository files were edited.

| Review metadata | Value |
|---|---|
| Execution ID | `CV-HOSTILE-20260908-T500-A-02` |
| Start UTC | **2026-09-08T11:46:00Z** |
| End UTC | **2026-09-08T11:54:15Z** |
| Timestamp provenance | Completed-turn metadata: `startedAt=1788867960`, `completedAt=1788868455` |
| Repository | `F:/dev/canview-wt/t500-hil` |
| Candidate | `7c63183228d68c30c809ffdc323badf1bb259096` |
| Base | `50410ba23fcecfa1f28cea837d04a061c201d648` |
| Scope | Deterministic host adapter, event ordering/offsets, analyzer fail-closed behavior, CAPTURE_ONLY zero CAN TX, lab adapter boundary, scenario semantics, resource exhaustion |
| Reviewer role | Independent hostile reviewer A |

**Baseline, isolation, and integrity**

The original review verified that the base is an ancestor of the candidate, with command exit 0. HEAD matched the candidate at the beginning and end, and worktree status was clean at both checks.

Source inspection used immutable `git show` and `git diff` content. Executed Python harness modules and the supplied test module were compiled directly from candidate Git blobs in memory. Scenario files were compared against candidate blobs. At the final integrity check, all 29 changed worktree files matched the candidate, allowing checkout CRLF normalization.

Tests created temporary fixtures outside the checkout. No repository files were edited. No other reviewer’s candidate findings were consumed.

| Integrity evidence | Result |
|---|---|
| Immutable binary diff SHA-256 | `624a38ac1167f516c2c873a60405e174e87e0d4823a69ae6b2ff033fb75f7700` |
| Final HEAD | `7c63183228d68c30c809ffdc323badf1bb259096` |
| Final `status --porcelain=v1` | Empty |
| Final integrity-check UTC | `2026-09-08T11:51:49.595879+00:00` |
| Execution runtime | CPython 3.14.3, Windows |

**A02-P1-01 — Host evidence can be relabeled as physical HIL PASS**

- **Location:** `F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:112–126`
- **Status:** OPEN

Starting with a valid generated host report, changing only:

```python
report["suite"] = "g2-readonly"
report["physical_hil"] = {"status": "PASS"}
```

causes `validate(report_path, "PASS")` to accept physical PASS. The report still identifies the disconnected host adapter, `hardware_execution=False`, and host-simulator events.

**Observed result:**

```text
RELABEL_HOST_AS_PHYSICAL ACCEPTED PASS
```

**Failure and impact:** The saved-evidence boundary permits host results to be promoted to physical HIL evidence. Although the lab runner correctly blocks execution without a backend, the validator does not preserve that boundary.

**Recommendation:** Enforce consistency among suite, adapter, execution origin, connection state, and physical status. Until a physical backend and its evidence contract exist, reject physical PASS.

**A02-P1-02 — Saved-evidence validation trusts PASS despite contradictory or absent evidence**

- **Locations:**  
  `F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:157–180`  
  `F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:190–200`
- **Status:** OPEN

Independent mutations of a generated host report were accepted:

1. Replace a scenario log with a valid `CAN_CHANNEL_SUMMARY` containing integer `tx_frames=1`; update count and byte length.
2. Replace a scenario log with an empty file; set count and byte length to zero.
3. Remove scenario metrics and insert a failed `capture_only.can_tx_zero` check.

**Observed results:**

```text
POSITIVE_INTEGER_TX_SUMMARY ACCEPTED PASS
EMPTY_EVENT_LOG ACCEPTED PASS
MISSING_METRICS_FALSE_CHECKS ACCEPTED PASS
```

**Failure and impact:** Validator success does not establish TX zero, scenario execution, or budget compliance. The validator accepts self-declared status despite missing or contradictory evidence.

**Recommendation:** Require nonempty scenario evidence and recompute applicable invariants against identified scenario and budget inputs. Include channel-summary TX counts. Reject contradictions among checks, violations, metrics, and status.

**A02-P1-03 — Invalid channel TX counts bypass CAPTURE_ONLY enforcement**

- **Location:** `F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:79–87`
- **Status:** OPEN

Using the stock `can-load` scenario, provide its required channel-1 zero-TX summary followed by a channel-2 summary. The analyzer behaves as follows:

| Channel 2 `tx_frames` | Result |
|---|---|
| `1` | FAIL |
| `1.0` | PASS |
| `True` | PASS |
| `"1"` | PASS |
| `None` | PASS |
| `-1` | PASS |
| Missing | PASS |

The `_is_int()` condition excludes malformed representations from the TX-event set instead of rejecting the evidence.

**Failure and impact:** Unknown or malformed TX counts are treated as zero TX.

**Recommendation:** Require a present, non-boolean, nonnegative integer count for every channel summary. Invalid counts must fail the evidence contract; positive counts must fail CAPTURE_ONLY.

**A02-P1-04 — Mandatory scenario assertions accept missing coverage and unsafe outcomes**

- **Locations:**  
  `F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:107–145`  
  `F:/dev/canview-wt/t500-hil/tests/hil/adapter.py:95–115`  
  Stock scenario files enumerated in the file manifest below.
- **Status:** OPEN

With stock expectations and normal budget values, all of these counterexamples return PASS:

| Scenario | Accepted counterexample |
|---|---|
| `duplicate-command` | One `COMMAND_REPLAY` with `executed=True`, `result="EXECUTED"` |
| `security-injection` | One `SECURITY_REJECT` with `accepted=True` |
| `feedback-mismatch` | One `FEEDBACK_RESULT` with `result="WRONG"` |
| `resource-exhaustion` | One summary with zero rejections and zero observer drops |
| `can-load` | Only channel 1; channels 2 and 3 absent |
| `stale-gates` | One DENY followed by an expired-lease ALLOW with `vehicle_tx=True` |

The adapter also maps `result-before-ack` to one `TIMEOUT` result. It produces no ACK/result ordering sequence for that case. Duplicate-command handling emits `DUPLICATE` for every supplied token without maintaining command history.

**Failure and impact:** Expected-label presence can conceal missing cases and contradictory outcomes. The passing inventory does not reliably qualify the named host semantics.

**Recommendation:** Assert per-case coverage, outcome, multiplicity, and ordering, including forbidden outcomes. Implement required host state transitions or explicitly classify unsupported semantics as incomplete. These corrections do not require physical hardware.

**A02-P2-01 — Malformed assertion and event contracts silently pass**

- **Locations:**  
  `F:/dev/canview-wt/t500-hil/tests/hil/scenario.py:145–154`  
  `F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:68`  
  `F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:108–119`
- **Status:** OPEN

These malformed expectations pass against an unrelated event:

```python
{"required_kinds": "MISSING_EVENT"}
{"required_fields": "invalid"}
```

The analyzer replaces invalid values with empty lists. Scenario parsing validates only the outer `expect` object.

A separate analyzer probe passed with `sequence=True`, missing schema/source, and `log_offset=-999`, while retaining the stock required channel fields.

**Failure and impact:** Configuration mistakes can disable assertions. Analyzer PASS does not guarantee a valid event contract.

**Recommendation:** Validate typed assertion structures before execution. Enforce event schema, sequence types, source, fields, and offset requirements. Reject malformed inputs instead of discarding their checks.

**A02-P2-02 — Accepted scenarios have unbounded execution cost and can produce unreadable evidence**

- **Locations:**  
  `F:/dev/canview-wt/t500-hil/tests/hil/scenario.py:145–150`  
  `F:/dev/canview-wt/t500-hil/tests/hil/adapter.py:56–60`  
  `F:/dev/canview-wt/t500-hil/tests/hil/events.py:54–79`
- **Status:** OPEN

A **196-byte** scenario containing this action passes parsing:

```json
{"type":"radio_loss","rates":[0],"packets":1000000000000}
```

It enters the trillion-iteration loop. The review subprocess remained running after two seconds and was terminated by its timeout.

An accepted generic event with fields equivalent to:

```python
{"data": ["x" * 4096] * 17}
```

produced:

```text
runner: PASS, exit 0
validator: EvidenceError: event line is too large
```

**Failure and impact:** Collection and string limits do not bound numeric work or expanded output. Producer limits do not match reader limits. Small accepted inputs can monopolize the runner or generate evidence that its own validator rejects.

**Recommendation:** Bound numeric action parameters, total execution work, event count, line bytes, and total log bytes. Reject oversized workloads before execution and enforce limits during emission.

**A02-P2-03 — EventLog shallow copies invalidate offsets; failed appends alter state**

- **Location:** `F:/dev/canview-wt/t500-hil/tests/hil/events.py:44–73`
- **Status:** OPEN

Mutating nested fields through the record returned by `append()` changes the stored record without updating byte accounting.

**Observed result:**

```text
stored second-event offset = 128
actual second-event offset = 227
stored total bytes = 241
actual total bytes = 340
```

Appending a field containing `float("nan")` raises `ValueError` but leaves one record stored with `byte_length=0`, because insertion occurs before serialization.

**Failure and impact:** Consumers can corrupt timeline byte offsets despite the documented immutable-record contract. A failed append also leaves inconsistent internal state.

**Recommendation:** Snapshot nested values, serialize and validate before committing state, and prevent returned records from sharing mutable nested storage.

**A02-P2-04 — Validator rejects correctly recorded forbidden-TX FAIL evidence**

- **Location:** `F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:166–169`
- **Status:** OPEN

A valid custom CAPTURE_ONLY scenario containing synthetic `CAN_TX` produces a FAIL report and runner exit 1, preserving its violation. Calling:

```python
validate(report_path, "FAIL")
```

raises:

```text
EvidenceError: capture-only TX event in probe-tx
```

**Failure and impact:** The validator cannot validate an authentic failure artifact for the harness’s principal negative invariant. It conflates a failed test with invalid evidence.

**Recommendation:** Accept well-formed FAIL evidence when TX events and recorded violations agree. Continue rejecting PASS evidence containing TX.

**A02-P2-05 — Later budget actions erase earlier resource violations**

- **Location:** `F:/dev/canview-wt/t500-hil/tests/hil/adapter.py:138–141`
- **Status:** OPEN

The stock resource-exhaustion scenario was executed with these actions:

```python
(
    {"type": "resource", "queue_depth": 999, "heap_free_bytes": 1},
    {
        "type": "budget",
        "values": {"queue_depth": 64, "heap_free_bytes": 8192},
    },
)
```

**Observed result:** Analyzer PASS. Final metrics report queue depth 64 and free heap 8192 despite the earlier resource event.

**Failure and impact:** Final-value overwrite hides queue peaks and heap minima that crossed manifest limits.

**Recommendation:** Evaluate every budget sample or preserve metric-specific extrema across all actions. Later recovery must not erase an earlier violation.

**Executed commands and results**

Identity and immutable-source commands included:

```powershell
git -C F:/dev/canview-wt/t500-hil rev-parse --show-toplevel
git -C F:/dev/canview-wt/t500-hil rev-parse '7c63183^{commit}'
git -C F:/dev/canview-wt/t500-hil rev-parse '50410ba23fcecfa1f28cea837d04a061c201d648^{commit}'
git -C F:/dev/canview-wt/t500-hil rev-parse HEAD
git -C F:/dev/canview-wt/t500-hil status --porcelain=v1
git -C F:/dev/canview-wt/t500-hil merge-base --is-ancestor 50410ba23fcecfa1f28cea837d04a061c201d648 7c63183228d68c30c809ffdc323badf1bb259096
git -C F:/dev/canview-wt/t500-hil diff --stat 50410ba23fcecfa1f28cea837d04a061c201d648 7c63183
git -C F:/dev/canview-wt/t500-hil diff --check 50410ba23fcecfa1f28cea837d04a061c201d648 7c63183
git -C F:/dev/canview-wt/t500-hil diff --binary --no-ext-diff 50410ba23fcecfa1f28cea837d04a061c201d648 7c63183228d68c30c809ffdc323badf1bb259096
git -C F:/dev/canview-wt/t500-hil ls-tree -r --name-only 7c63183228d68c30c809ffdc323badf1bb259096 tests/hil tests/test_hil_runner.py
git -C F:/dev/canview-wt/t500-hil diff --name-only 50410ba23fcecfa1f28cea837d04a061c201d648 7c63183228d68c30c809ffdc323badf1bb259096
```

Initial unquoted `^{commit}` expressions failed in PowerShell. The quoted commands above succeeded.

File contents were read with:

```text
git -C F:/dev/canview-wt/t500-hil show 7c63183:<repository-relative-file>
```

Tests and probes ran through PowerShell here-strings piped to `python -B -`. The executed immutable module-loading mechanism was:

```python
repo = Path("F:/dev/canview-wt/t500-hil")
rev = "7c63183228d68c30c809ffdc323badf1bb259096"

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

The candidate test module was compiled from its Git blob and executed with:

```python
unittest.TextTestRunner(verbosity=2).run(
    unittest.defaultTestLoader.loadTestsFromModule(m)
)
```

Runner probes called `hil.run.main()` using:

- `--suite host --seed 1 --output <temporary-output>`
- Custom temporary `--scenario-dir` fixtures for forbidden TX and oversized events.
- `--suite g2-readonly` with missing configuration, the unavailable example rig, and a temporary rig declaring `available=True`.

Evidence probes called `validate(path, "PASS")`, `validate(path, "FAIL")`, and `validate(path, "BLOCKED")`. The resource subprocess used `timeout=2`; Python terminated and waited for it on timeout.

| Executed verification | Result |
|---|---|
| Supplied HIL unit tests | **13 run, 0 failures, 0 errors** |
| Stock host inventory | **12/12 PASS** for seed 1; supplied tests also exercised seed 7 |
| Unmodified generated host evidence | Validator PASS |
| Determinism | **36 paired runs**: 12 scenarios × seeds `0`, `1`, `2^64−1`; identical records and metrics |
| Direct forbidden CAN TX fixture | Analyzer FAIL with expected invariant |
| Supplied tampered-offset test | Rejected |
| Missing lab configuration | Exit 2, BLOCKED |
| Example unavailable rig | Exit 2, SKIPPED |
| Rig declaring `available=True` | Exit 2, BLOCKED; no backend |
| Deeply nested scenario probe | Rejected with `ScenarioError` |
| Diff whitespace check | Clean |
| Candidate file comparison after testing | All 29 changed files matched |
| Final worktree status | Clean |

Additional observed validator weakness, included under the evidence-integrity concern rather than counted separately: removing `firmware.git_commit` and replacing the source digest with 64 zeros was accepted as PASS.

**Exact changed-file scope inspected**

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

Supporting repository instructions and context inspected:

```text
F:/dev/canview-wt/t500-hil/AGENTS.md
F:/dev/canview-wt/t500-hil/docs/README.md
F:/dev/canview-wt/t500-hil/docs/architecture/README.md
F:/dev/canview-wt/t500-hil/docs/runbooks/agent-workflow.md
F:/dev/canview-wt/t500-hil/docs/reviews/README.md
```

The architecture index was inspected for relevant harness/HIL/gate passages. The workflow and archive were inspected for review requirements. No unrelated firmware implementation audit was claimed.

**Unreviewed or unexecuted scope**

- Physical board flashing and execution: **NOT_RUN**.
- Physical CAN analyzer, bus traffic, and electrical TX-zero measurements: **NOT_RUN**.
- Power/reset/rail/brownout, clock, UART/FDCAN, and radio measurements: **NOT_RUN**.
- Vehicle integration and vehicle CAN transmission: **NOT_RUN; NO-GO**.
- Full repository regression, target builds, and remote CI: **NOT_RUN in this review**.
- Cross-runtime/cross-platform determinism: not established; observed determinism applies to the stated Windows Python runtime.
- Post-fix source verification: not performed; this administrative correction leaves the candidate unchanged.

No physical CAN transmission was performed. The inspected change adds no operational CAN transmission backend, but source inspection and host tests do not establish physical TX-zero evidence.

**Final disposition: BLOCK.** The four P1 findings require correction and immutable post-fix review before merge. The five P2 findings remain OPEN and require disposition. Passing supplied tests does not close the reproduced failures. Physical/HIL remains **NOT_RUN**.
