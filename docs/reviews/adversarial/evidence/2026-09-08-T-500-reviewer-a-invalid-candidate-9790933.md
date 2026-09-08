**Verdict: BLOCK — the requested candidate commit does not resolve.**

| Field | Value |
|---|---|
| Execution ID | `CV-HOSTILE-20260908-T500-A-08` |
| UTC start | `2026-09-08 13:34:07` |
| UTC end | `2026-09-08 13:34:29` |
| Requested candidate | `9790933c4e3fd9834cb37e15b0d2ef26f3d0de82` |
| Base | `50410ba23fcecfa1f28cea837d04a061c201d648` |
| Repository | `F:/dev/canview-wt/t500-hil` |
| Scope | Typed events, bounded JSON decoding/serialization, terminal reports, first-failure identity |

The base resolves successfully. The requested candidate does not. The clean worktree’s HEAD—and the matching short prefix `9790933`—resolve to a **different** commit:

```text
979093327c974c40be7a7fa2ca1c240a449bb6b1
```

That commit was not substituted for the requested candidate.

Commands executed:

```powershell
git -C F:/dev/canview-wt/t500-hil rev-parse --verify '9790933c4e3fd9834cb37e15b0d2ef26f3d0de82^{commit}'
# Exit 128: fatal: Needed a single revision

git -C F:/dev/canview-wt/t500-hil rev-parse --verify '50410ba23fcecfa1f28cea837d04a061c201d648^{commit}'
# Exit 0: requested base

git -C F:/dev/canview-wt/t500-hil rev-parse HEAD
git -C F:/dev/canview-wt/t500-hil rev-parse --verify '9790933^{commit}'
# Both resolve to 979093327c974c40be7a7fa2ca1c240a449bb6b1

git -C F:/dev/canview-wt/t500-hil show 9790933c4e3fd9834cb37e15b0d2ef26f3d0de82:tests/hil/events.py
# Exit 128; no candidate source retrieved

git -C F:/dev/canview-wt/t500-hil status --porcelain=v1
# Empty: tracked and untracked tree clean
```

**Isolation:** Read-only local Git checks. No repository modifications. No other reviewer archive or conclusions used.

**P0/P1/P2/P3 findings:** Not assessed. This is an identity blocker, not a completed source review.

**Reviewed-but-not-found:** No source-level claims. Candidate source/tests were unavailable, so deep JSON, report-size agreement, selected FAIL results, custom expectations, malformed fields, Unicode, output reuse, inventory integrity, and first-failure probes are all **NOT_RUN**.

**Recommendation:** Supply the corrected full candidate hash or make the exact requested object available, then initiate a fresh review.

**Physical/HIL: NOT_RUN. Vehicle CAN TX: NO-GO. Final verdict: BLOCK.**
