# Reviewer B 독립 hostile post-fix report

- Execution ID: `6835dca4-e8bb-49af-96cd-1254d3831029`
- UTC 시작: `2026-09-09T00:33:34.6150519Z`
- KST 시작: `2026-09-09T09:33:34.6155299+09:00`
- UTC 종료: `2026-09-09T00:42:40.3707297Z`
- KST 종료: `2026-09-09T09:42:40.3710350+09:00`
- Candidate: `91ec28f550f18575f22a205155cffe2cbf18422d`
- Base: `6db3998092812354afe3b3918892b682022990b4`
- Candidate diff: 49 files, `+7048/-48`
- Verdict: **BLOCK**
- P0: 0
- P1: 1
- P2: 0

Reviewer A report와 기존 통합 report는 읽지 않았다. Candidate object와 detached 임시 clone만 사용했으며 원래 worktree는 수정하지 않았다.

## P1-01 — T-103 target evidence가 최종 candidate에 귀속되지 않음

대상 파일:

- `docs/reviews/adversarial/evidence/2026-09-09-T-103-target.md:3-4`
- `docs/reviews/adversarial/evidence/2026-09-09-T-103-target.md:11-38`
- `docs/resume.md:12-18`

현재 committed evidence는 다음을 기록한다.

```text
source candidate: 3e13b2ca6e72a3aec5a32a6357285c614bc191f9
base: 6db3998092812354afe3b3918892b682022990b4
```

`3e13b2ca...`가 candidate의 ancestor인 것은 확인했지만, 최종 candidate `91ec28f...`와 동일한 source가 아니다. 문서의 Debug/Release artifact hash도 이 ancestor build 결과이며, requested evidence file은 존재하지 않는다.

재현:

```powershell
git -c safe.directory=F:/dev/canview -C F:/dev/canview cat-file -e `
  "91ec28f550f18575f22a205155cffe2cbf18422d:docs/reviews/adversarial/evidence/2026-09-09-T-103-target-91ec28f.md"
```

결과:

```text
fatal: path ... does not exist
```

ancestor 관계:

```powershell
git -c safe.directory=F:/dev/canview -C F:/dev/canview merge-base --is-ancestor `
  3e13b2ca6e72a3aec5a32a6357285c614bc191f9 `
  91ec28f550f18575f22a205155cffe2cbf18422d
```

결과: exit `0`.

영향:

- 기록된 target ELF/MAP/BIN/HEX와 warning scan 결과가 최종 candidate에 대한 증거인지 확인할 수 없다.
- target build 성공 주장이 이전 source에 묶여 있어 standalone audit가 불가능하다.
- T-103이 요구하는 candidate/source/harness/evidence 연결이 닫히지 않았다.

권고:

- candidate `91ec28f...`에서 STM32 Debug/Release clean-first build를 재실행한다.
- candidate, base, firmware source digest `f9ea109772edef0743fd22899f9c6c6d8c6035c3a43c03090a6d709bc2309212`, harness digest `00f0d68afcf3e30707f642e808ff645c1a60f62f91ee33e23ac0e7570d72ffbb`, artifact hash와 CI run을 candidate-specific evidence에 기록한다.
- 물리 flash/boot·전원·CAN analyzer·차량 evidence는 계속 `NOT_RUN`으로 명시한다.

## Candidate-bound identity 검증

이전 identity 문제는 현재 source에서 수정되어 있었다.

`tests/hil/run.py:50-77`에 LF canonicalization이 구현되어 있고, detached clone에서 계산한 값은 다음과 같았다.

```text
firmware source_sha256:
f9ea109772edef0743fd22899f9c6c6d8c6035c3a43c03090a6d709bc2309212

harness source_sha256:
00f0d68afcf3e30707f642e808ff645c1a60f62f91ee33e23ac0e7570d72ffbb
```

fixture와 test constant도 `f9ea...`와 일치했다.

`tests/hil/run_can_capture.py:22-107`의 candidate/firmware/harness SHA 검증과 event identity 검증을 실행했고, 다음 negative path도 통과했다.

- wrong candidate identity
- wrong firmware digest
- wrong harness digest
- executed `COMMAND_REPLAY`
- malformed/oversized/truncated evidence
- symlink/junction output directory
- newline-normalized source identity

## 실제 실행 결과

detached clone:

```text
HEAD = 91ec28f550f18575f22a205155cffe2cbf18422d
git status --porcelain=v1 = clean
```

Host tests:

```text
python -B -m unittest discover -s tests -p "test_*.py" -v
Ran 116 tests
OK
```

Focused tests:

```text
T-103 helper: 13/13 PASS
SDK config: 13/13 PASS
FDCAN layout: 4/4 PASS
```

Static/config/security checks:

```text
python -B tools/generate_boards.py --check
PASS

python -B tools/check_generated.py
PASS: generated output check (15 files)

python -B tools/check_negative_fixtures.py
PASS

python -B tools/check_budgets.py
PASS

python -B tools/validate_plan.py
PASS

python -B tools/validate_document_links.py
Checked 296 documents, 1294 local targets; errors=0

python -B tests/security/bridge_http.py
PASS: Bridge HTTP/WebSocket read-only source contract (64 checks)
NOT_RUN: live ESP32 probe
```

HIL host provenance:

```text
python -B tests/hil/run.py --suite host --seed 1 ...
PASS suite=host scenarios=12 seed=1 firmware=f9ea109772ed physical_hil=NOT_RUN

python -B tests/hil/validate_evidence.py ... --expect-status PASS
PASS

python -B tests/hil/run_can_capture.py ...
PASS: 5 complete JSONL records contain no CAN TX or ACK
```

## 실행하지 못한 검증

이 환경에는 `cmake`, `ninja`, `clang`가 없었다. 따라서 다음은 `NOT_RUN`이다.

- CMake configure/build
- CTest
- Clang ASan/UBSan
- LLVM coverage binary
- STM32 target Debug/Release ELF/MAP/BIN/HEX
- compiler/linker/CMake warning scan
- STM32CubeG4 1.6.3 실제 layout checker

Coverage scripts는 build directory 부재로 `FileNotFoundError`가 발생했다. 이는 candidate build 실패가 아니라 실행 환경에 target host build가 없는 상태다.

## 안전 경계 재확인

- Diagnostic Bridge source/config에는 `control_lease`, `raw_replay`, CAN TX API가 없다.
- Bridge static security contract 64개가 통과했다.
- STM32 FDCAN adapter는 monitor/RX FIFO-only 경로이며 TX request API는 없다.
- fixed STM32G4 Message RAM contract와 vendor/adapter drift mutation test가 통과했다.
- 실제 차량 CAN TX, flash/boot, rail/brownout/reset, CAN analyzer와 vehicle evidence는 `NOT_RUN`.
- 차량 CAN TX release는 **NO-GO**다.

최종 결론은 target evidence를 최종 candidate에 재생성·귀속하기 전까지 **BLOCK**이다.
