**BLOCK — CV-HOSTILE-20260908-T500-B-04**

| Field | Value |
|---|---|
| Actual start UTC | `2026-09-08T12:43:40Z` |
| Actual end UTC | `2026-09-08T12:50:17Z` |
| Candidate | `3e49ea8c4b04458c63bb28a4157c8541bbb371da` |
| Base | `50410ba23fcecfa1f28cea837d04a061c201d648` |
| Repository | `F:/dev/canview-wt/t500-hil` |
| Findings | **P0: 0 · P1: 0 · P2: 3 · P3: 0** |
| Physical/HIL | **NOT_RUN** |
| Vehicle CAN TX | **NO-GO** |
| Verdict | **BLOCK — final closure withheld** |

The candidate was inspected and tested. Three reproducible P2 defects remain open; no deferral was assumed.

Reviewer scope: complete T-500 implementation delta, prior B-03 findings, trusted replay and evidence identity, parser/resource bounds, EventLog ownership/atomicity, output redirection, safety invariants, failure locations, CI and documentation.

**Isolation and immutable verification**

Source reads used immutable Git objects and base-to-candidate diffs. Tests executed against the supplied worktree after confirming its HEAD matched the candidate. This was not a detached checkout.

Initial and final porcelain status were empty. Final tracked comparison against the candidate exited zero. Both the specified base and B-03 candidate `8a1f13db4f40b3826376e0a2b62d9c1254bff534` were verified ancestors.

No repository files were modified. Probes used temporary directories outside the repository. Archived review reports were treated as opaque bytes for inventory/hash accounting; no other reviewer’s findings were inspected or used.

**Disposition of every B-03 finding**

| B-03 finding | Disposition |
|---|---|
| F-01: Trace/metric/seed provenance | **FIXED for reproduced defects.** Coordinated seed relabeling, metric forgery, trace changes and inserted over-budget samples are rejected. |
| F-02: Named safety coverage/malformed outcomes | **PARTIALLY FIXED.** Duplicate named cases and the previous malformed scalar outcome now fail. Other malformed event values still crash analysis instead of producing controlled failure evidence: current F-01. |
| F-03: Windows junction writes | **FIXED for tested paths.** Output-root and events-directory junctions return 2 without external writes. |
| F-04: Host BLOCKED generation/stale artifacts | **PARTIALLY FIXED.** Missing import, reserved-field collision, invalid-source/seed handling and tested file-link failures are repaired. Other parser/analyzer exceptions still leave old PASS evidence: current F-01. |
| F-05: Selected-scenario validation | **PARTIALLY FIXED.** A legitimate selected run now validates. The validator trusts the report’s own selection, permitting removal of failed results: current F-02. |

**Current findings — OPEN**

**F-01 — P2: Malformed inputs still escape failure handling and leave previous PASS evidence valid**

Locations:

- [analyze.py:146](F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:146)
- [analyze.py:194](F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:194)
- [analyze.py:200](F:/dev/canview-wt/t500-hil/tests/hil/analyze.py:200)
- [scenario.py:244](F:/dev/canview-wt/t500-hil/tests/hil/scenario.py:244)
- [run.py:254](F:/dev/canview-wt/t500-hil/tests/hil/run.py:254)
- [run.py:364](F:/dev/canview-wt/t500-hil/tests/hil/run.py:364)

These parser-accepted event actions cause uncaught `TypeError` during set membership:

```json
{"type":"event","kind":"COMMAND_REPLAY","fields":{"request_token":"x","executed":false,"result":[]}}
{"type":"event","kind":"FEEDBACK_RESULT","fields":{"case":"x","result":{},"tx_permitted":false}}
{"type":"event","kind":"CAN_TX","fields":{"arbitration_id":[]}}
```

A JSON string containing an escaped lone surrogate, such as `"\ud800"`, instead causes uncaught `UnicodeEncodeError` at the event-field byte-size check.

For every tested case, starting with a valid report and reusing its output directory left the earlier report validating as PASS after the failed invocation.

**Impact:** Some malformed inputs bypass the advertised controlled failure-evidence path. Subsequent artifact consumers can encounter valid-looking success evidence from an earlier invocation.

**Recommendation:** Validate field types before set membership and reject invalid Unicode through `ScenarioError`. Cover parsing, analysis and serialization with controlled failure handling. Publish an invocation-specific non-success state before execution so unexpected failures cannot leave an earlier success representing the latest run.

**F-02 — P2: Removing a failed scenario converts accepted FAIL evidence into accepted PASS evidence**

Locations: [validate_evidence.py:199](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:199), [validate_evidence.py:213](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:213), [foundation.yml:63](F:/dev/canview-wt/t500-hil/.github/workflows/foundation.yml:63).

Reproduction:

1. Start with the complete 12-scenario report.
2. Change a `stale-gates` decision to `ALLOW`/`true`, rebuild its event offsets and recompute its result.
3. The resulting report correctly validates as FAIL.
4. Delete that scenario result, update inventory count/IDs to the remaining 11, and change the report status to PASS.
5. The same validator accepts PASS.

A simpler coordinated truncation to one successful scenario also validates.

Selected-run support now assigns `expected_ids` from the untrusted report itself. There is no caller-supplied expected selection or full-suite requirement. Consequently, it cannot distinguish an intended subset from deleted results.

**Impact:** Evidence consumers cannot use the validator to establish that the requested execution scope completed. The CI validation invocation supplies only expected status.

**Recommendation:** Add a trusted expected-selection/full-suite input. Have CI explicitly require the complete host inventory. Preserve selected-run support through an explicit expected selection, rather than treating self-declared inventory as authority.

**F-03 — P2: Custom FAIL evidence can misidentify the first violation or invent its verdict**

Locations: [validate_evidence.py:320](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:320), [validate_evidence.py:326](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:326), [validate_evidence.py:332](F:/dev/canview-wt/t500-hil/tests/hil/validate_evidence.py:332).

A custom scenario containing two forbidden CAN TX events correctly produced FAIL. Changing `first_violation` and its matching violation entry from the first TX to the second TX—sequence 2, byte offset 146—still validated as FAIL.

The check accepts any offending event, not the first offending event.

Separately, a custom non-TX failure accepted these alterations:

```text
first_violation.invariant = "made.up.invariant"
first_violation.message   = "not checked"
violations                = [that fabricated violation]
checks                    = []
```

The location referenced an existing event. Because the scenario was not in the trusted inventory, validation skipped verdict recomputation.

**Impact:** Accepted negative evidence can provide an incorrect first-failure location or unsupported diagnosis, undermining failure reproduction and auditability.

**Recommendation:** Recompute supported custom-negative invariants and require the correct first offending event. For arbitrary custom scenarios, require trusted scenario input or explicitly return structural-only validation rather than presenting it as verified failure evidence.

**Tests and adversarial results**

| Check | Result |
|---|---|
| Runtime | Windows Python 3.14.3 |
| Focused T-500 tests | **36/36 PASS**, no reported skips |
| Complete Python discovery | **87/87 PASS** |
| Host CLI, seed 1 | Exit 0; 12 scenarios PASS |
| Host evidence CLI | Exit 0 |
| Example unavailable lab | Exit 2; SKIPPED |
| Lab evidence CLI expecting SKIPPED | Exit 0 |
| Plan validation | 49 tasks; zero errors |
| Document links | 276 documents; 1,268 targets; zero errors |
| Base/B-03 ancestry | Both exit 0 |
| Final HEAD/worktree comparison | Exact candidate; clean |

Focused probes confirmed:

```text
Coordinated seed relabeling                 REJECTED
Report metric forgery                      REJECTED
Harness identity forgery                   REJECTED
Host-to-physical PASS relabeling            REJECTED
Duplicated named safety cases              Analyzer FAIL; validator rejects
Malformed scalar safety outcome            Analyzer FAIL; validator rejects
Nonzero CAN channel TX summary             Analyzer FAIL; validator rejects
CAN trace field drift                      Analyzer FAIL; validator rejects
Inserted over-budget sample                Analyzer FAIL; validator rejects
Selected brownout execution                Exit 0; validates PASS
Previously colliding nested "kind" field    Executes without the old collision
Invalid source/negative seed               Exit 2; valid BLOCKED report
Duplicate keys/exponent overflow/depth/
  oversized scenario document              Exit 2; valid BLOCKED report
Output/events junctions                    Exit 2; no external writes
Output/events symlinks                      Exit 2; no external writes
Individual event-file symlink              Exit 2; valid BLOCKED report
Individual report-file symlink             Exit 2; external sentinel preserved
Hardlinked report/event destinations       External sentinel contents preserved
EventLog nested ownership                  Deep-copy isolation preserved
NaN/oversized append                       EventLogError; records/byte count unchanged
Recorded forbidden-TX FAIL                 Validates FAIL
Impossible/non-TX failure location         REJECTED
First TX relocated to later TX             ACCEPTED — F-03
Custom fabricated failure diagnosis        ACCEPTED — F-03
Deleted failed scenario                    ACCEPTED PASS with 11 results — F-02
Malformed event containers/lone surrogate   Crash; old PASS remains valid — F-01
```

The lone-surrogate probe terminated one probe process with a traceback. It was then repeated under an exception-catching probe to confirm that the previous report remained valid. No success was inferred from that failed process.

**Commands run**

Python commands ran from `F:/dev/canview-wt/t500-hil`.

```powershell
git -C F:/dev/canview-wt/t500-hil rev-parse HEAD
git -C F:/dev/canview-wt/t500-hil status --porcelain=v1
git -C F:/dev/canview-wt/t500-hil merge-base 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil merge-base --is-ancestor 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil merge-base --is-ancestor 8a1f13db4f40b3826376e0a2b62d9c1254bff534 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil diff --stat 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil diff --stat 8a1f13db4f40b3826376e0a2b62d9c1254bff534 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil diff --name-only 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil diff --no-ext-diff --binary 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil diff --check 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da
git -C F:/dev/canview-wt/t500-hil diff --exit-code 3e49ea8c4b04458c63bb28a4157c8541bbb371da --
python --version
python -B -m unittest discover -s tests -p test_hil_runner.py
python -B -m unittest discover -s tests -p 'test_*.py'
python -B tools/validate_plan.py
python -B tools/validate_document_links.py
```

Source inspection used `git show 3e49ea8c4b04458c63bb28a4157c8541bbb371da:<path>` and path-scoped `git diff --no-ext-diff <base> <candidate> -- <paths>` for the substantive files listed below.

Additional scope/registration commands:

```powershell
git -C F:/dev/canview-wt/t500-hil diff --name-only 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da -- firmware shared protocol hardware config tools CMakeLists.txt
git -C F:/dev/canview-wt/t500-hil diff --name-only 50410ba23fcecfa1f28cea837d04a061c201d648 3e49ea8c4b04458c63bb28a4157c8541bbb371da -- AGENTS.md docs/README.md docs/runbooks/agent-workflow.md .gitattributes
git -C F:/dev/canview-wt/t500-hil grep -n -e python-unit 3e49ea8c4b04458c63bb28a4157c8541bbb371da -- CMakeLists.txt
```

Actual final CLI commands:

```powershell
C:\Python314\python.exe -B tests/hil/run.py --suite host --seed 1 --output C:\Users\digit\AppData\Local\Temp\CV-B04-cli-kjr1117t\host
C:\Python314\python.exe -B tests/hil/validate_evidence.py C:\Users\digit\AppData\Local\Temp\CV-B04-cli-kjr1117t\host --expect-status PASS
C:\Python314\python.exe -B tests/hil/run.py --suite g2-readonly --rig-config tests/hil/rig.example.yaml --output C:\Users\digit\AppData\Local\Temp\CV-B04-cli-kjr1117t\lab
C:\Python314\python.exe -B tests/hil/validate_evidence.py C:\Users\digit\AppData\Local\Temp\CV-B04-cli-kjr1117t\lab --expect-status SKIPPED
```

Adversarial probes ran as PowerShell here-strings piped to `python -B -`, importing the candidate modules and using `tempfile.TemporaryDirectory()`.

Junction probes used:

```powershell
New-Item -ItemType Junction -Path $env:CV_PROBE_LINK -Target $env:CV_PROBE_TARGET | Out-Null
```

Both variables pointed inside the temporary probe directory. Junctions were removed using `os.rmdir(link)` before temporary cleanup.

**Immutable diff and file scope**

Full binary diff SHA-256:

```text
57cc3df000bedf4e2c860836c372064616f2fa1e06092d722e34a693f8044370
```

All 29 substantive changed files were inspected:

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

The full delta contains 33 files. These four additional archives were inventoried and hashed without inspecting their findings:

```text
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-a-initial.md
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-a-postfix-8a1.md
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-b-initial.md
F:/dev/canview-wt/t500-hil/docs/reviews/adversarial/evidence/2026-09-08-T-500-reviewer-b-postfix-8a1.md
```

Full `diff --check` exited 2 with 15 archived-report whitespace diagnostics. The substantive implementation/documentation diff, excluding archives, passed. Archive formatting was not assigned a source finding.

**CI, documentation, generated scope and leakage**

- Windows CI propagates runner/validator failures. Root CTest registers Python discovery.
- No new third-party Python dependency was introduced.
- No changes were present under firmware, shared, protocol, hardware, config or tools, or in root CMake. No generated firmware/protocol artifact was changed.
- Repository instructions, document router, workflow rules and `.gitattributes` remain unchanged from the prior review.
- Task state remains IN_PROGRESS; documented 36/87 test counts match execution.
- The lab backend remains a stub. The README still says the lab command consumes scenario inventory, although `run_lab()` returns before loading it.
- Private malformed-rig diagnostics did not disclose the test’s private filename marker.
- Substantive added-line scans found zero private-key headers, AWS-key patterns, GitHub-token patterns or VIN-pattern candidates. Archived narratives were excluded from substantive scanning to preserve review independence.

**Unreviewed scope**

Physical boards/HIL, power/reset/brownout equipment, CAN analyzer, live lab backend, provisioning, target flashing and vehicle bus: **NOT_RUN**.

Linux execution, remote GitHub Actions, C/target builds, exhaustive fuzzing, concurrent filesystem races and acquisition authentication were not executed. Archived reviewer narratives were not substantively reviewed.

**Final verdict: BLOCK.** Trusted replay, named-case checks and tested Windows output protections improved substantially, but the three open P2 defects prevent final evidence-integrity closure. Physical/HIL remains **NOT_RUN**; vehicle CAN TX remains **NO-GO**.
