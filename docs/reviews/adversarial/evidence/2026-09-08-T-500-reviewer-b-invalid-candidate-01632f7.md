**BLOCK — CV-HOSTILE-20260908-T500-B-05**

The specified candidate does not exist in the supplied repository. Source review and tests could not proceed against that immutable hash.

| Field | Value |
|---|---|
| Start UTC | `2026-09-08T13:10:03Z` |
| End UTC | `2026-09-08T13:10:26Z` |
| Requested candidate | `01632f7c6e3c1b8b1b5dbf4cda23e75fca7f5e9d` — **unresolvable** |
| Verified base | `50410ba23fcecfa1f28cea837d04a061c201d648` |
| Observed HEAD | `01632f7fee247c446c94d0e129367385319a9e63` |
| Repository | `F:/dev/canview-wt/t500-hil` |
| Worktree | Clean before and after |
| P0/P1/P2/P3 | **NOT_ASSESSED** |
| Physical/HIL | **NOT_RUN** |
| Vehicle CAN TX | **NO-GO** |
| Verdict | **BLOCK — invalid candidate manifest** |

Intended scope: evidence/config/build integrity, expected selection, structural-only custom evidence, replay identity, schemas, malformed input, output handling, resource bounds, CI/documentation and reproducibility.

Isolation consisted of read-only Git identity checks. No repository files were modified. No source files or review reports were read. The observed HEAD was not substituted for the requested candidate.

Commands executed:

```powershell
git -C F:/dev/canview-wt/t500-hil rev-parse HEAD
git -C F:/dev/canview-wt/t500-hil status --porcelain=v1
git -C F:/dev/canview-wt/t500-hil merge-base 50410ba23fcecfa1f28cea837d04a061c201d648 01632f7c6e3c1b8b1b5dbf4cda23e75fca7f5e9d
git -C F:/dev/canview-wt/t500-hil diff --stat 50410ba23fcecfa1f28cea837d04a061c201d648 01632f7c6e3c1b8b1b5dbf4cda23e75fca7f5e9d
git -C F:/dev/canview-wt/t500-hil diff --stat 3e49ea8c4b04458c63bb28a4157c8541bbb371da 01632f7c6e3c1b8b1b5dbf4cda23e75fca7f5e9d
git -C F:/dev/canview-wt/t500-hil cat-file -e '01632f7c6e3c1b8b1b5dbf4cda23e75fca7f5e9d^{commit}'
git -C F:/dev/canview-wt/t500-hil rev-parse --verify '50410ba23fcecfa1f28cea837d04a061c201d648^{commit}'
git -C F:/dev/canview-wt/t500-hil rev-parse --verify '01632f7^{commit}'
git -C F:/dev/canview-wt/t500-hil status --porcelain=v1
```

The candidate-dependent ancestry/diff commands returned invalid-commit/bad-object errors. `cat-file` independently confirmed the requested object was unavailable. The short prefix `01632f7` resolves to the different observed HEAD above.

**Unreviewed:** All requested source, test, adversarial-probe and contract work. There are no reviewed-but-not-found source areas, and no prior finding is closed by this execution.

Please provide the corrected immutable candidate hash, or make the specified object available.
