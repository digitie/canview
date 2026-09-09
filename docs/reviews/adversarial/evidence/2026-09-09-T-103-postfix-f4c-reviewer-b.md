# Reviewer B 적대적 재검토 보고서

- 실행 ID: `941a8972-63ee-4c86-b784-08a55d397d57`
- 대상 후보: `f4c413a153dcf2ff5c771c457275e3b4af25f385`
- 비교 기준: `6db3998092812354afe3b3918892b682022990b4`
- 시작 시각: 정확한 시작 marker 미기록
- 종료 시각: `2026-09-09T00:03:29.8760500Z` / KST `2026-09-09T09:03:29.8765345+09:00`
- 작업 방식: 후보 commit object-only 조회
- 변경·commit·push: 없음
- Reviewer A 보고서: 조회하지 않음

## 최종 판정

**BLOCK**

- P0: 0
- P1: 2
- P2: 1
- 물리 차량/HIL 검증: `NOT_RUN`

## P1-1 — T-103 target evidence가 최종 후보에 묶여 있지 않음

`docs/reviews/adversarial/evidence/2026-09-09-T-103-target.md:3-4`는 source candidate를 최종 후보가 아닌 ancestor `3e13b2ca...`로 기록한다. 빌드 artifact와 hash도 같은 문서 `:11-38`의 이전 후보 결과다.

또한 문서에 후보 전용 evidence ID `CV-T103-TARGET-20260909-F4C413A`와 최종 후보 digest가 없다.

영향:

- 제공된 target build 성공 주장을 immutable candidate 안에서 재현·검증할 수 없다.
- 이전 후보의 clean build 결과가 현재 후보의 STM32G474 binary/layout/no-TX 결과라는 보장이 없다.

필수 조치:

- 정확한 후보 `f4c413...`로 clean-first Debug/Release target build 재실행
- evidence에 candidate, artifact SHA-256, corrected firmware source digest `dd81fb27da6898600e1a03d2264693adfe11f24962857129ddf66eaf076761b1`, evidence ID를 기록
- 물리 차량/HIL을 실행하지 않았다면 계속 `NOT_RUN`으로 명시

## P1-2 — firmware identity digest와 fixture/document identity가 불일치하며 cross-platform canonicalization도 없음

후보 코드의 `tests/hil/run.py:50-72`는 filesystem raw bytes와 path를 직접 hash하며 LF canonicalization을 하지 않는다. `.gitattributes`는 일부 `protocol`·`shared/protocol` 파일만 `eol=lf`로 보장하고 모든 `firmware/**/*.c/h`를 보장하지 않는다.

동시에 다음 artifact가 모두 stale identity를 포함한다.

- `tests/hil/fixtures/t103-capture-only.jsonl:1-5`: `f321910e...`
- `tests/test_t103_capture_helpers.py:21-26`: `EXPECTED_FIRMWARE_IDENTITY = f321910e...`
- `docs/tasks/T-103-stm32-fdcan-capture.md:87`: 이전 digest `513a...`
- 후보 전체에서 corrected digest `dd81...`는 발견되지 않음

`tests/hil/run_can_capture.py:22-70`은 외부에서 전달된 expected commit/source digest와 결과를 strict 비교하므로, corrected `dd81...` target evidence와 immutable candidate의 stale fixture/test contract를 동시에 만족할 수 없다.

영향:

- Windows/Unix checkout line ending에 따라 identity가 달라질 수 있다.
- fixture와 target evidence가 최종 후보 및 canonical digest를 증명하지 못한다.
- source identity 검증이 재현성 있는 provenance contract가 아니다.

필수 조치:

- LF canonicalization 또는 Git-object 기반의 단일 digest algorithm 도입
- fixture, test, task, target evidence를 `dd81...`로 갱신
- dynamic `_git_commit()` 대신 후보-bound expected identity를 검증하는 회귀시험 추가
- cross-platform checkout에서 digest 불변성을 검증

## P2 — strict no-TX helper가 실행된 command replay 의미를 차단하지 않음

`tests/hil/assert_no_tx.py:28-57`은 `COMMAND_REPLAY`와 `executed` 필드를 허용한다. 그러나 `executed=true`인 command replay 자체를 거부하지 않으며, no-TX 검사는 주로 `vehicle_tx`, `tx_permitted`, `tx_frames`, `ack_frames` 등에 의존한다 (`:230-309`).

따라서 다른 schema 조건을 만족하는 `COMMAND_REPLAY` 기록이 `executed=true` 및 성공 결과를 포함해도 strict no-TX evidence를 통과할 여지가 있다.

권고:

- T-103 capture-only evidence에서 `COMMAND_REPLAY`를 금지하거나
- `COMMAND_REPLAY.executed=true`를 명시적으로 거부하고
- control/action event에 대한 negative mutation test를 추가할 것

## 이전 FIFO/Message RAM 지적 재평가

이전의 “STM32G4 FDCAN FIFO 설정 누락” 지적은 현재 후보에서 **CLOSED/RESOLVED**로 판단한다.

근거:

- `firmware/communicator/stm32/platform/stm32g474/fdcan_message_ram.h:1-64`
  - STM32G4 fixed Message RAM contract
  - `RXF0C`/`RXESC` 비사용
  - 표준/확장 filter, FIFO, TEF, TFQ offset과 instance size compile-time 검증
- `tools/check_stm32_fdcan_layout.py:101-171`
  - vendor HAL 상수와 contract 대조
  - `RXF0S`/`RXF0A` 요구
  - `RXF0C`/`RXESC` 거부
- `tests/test_stm32_fdcan_layout.py:27-58`
  - vendor drift, adapter drift, 금지 register mutation 시험

단, 실제 pinned STM32CubeG4 1.6.3 target build와 layout checker 실행 자체는 이 audit에서 `NOT_RUN`이다.

## 확인된 범위

다음 영역은 source-level 및 host-test 구조상 중대한 신규 finding을 발견하지 못했다.

- SDK config allowlist와 board profile/generator
- CMake/component dependency 및 capture-only forced include
- STM32 read-only Bridge TX boundary
- ISR fast-path와 bounded ring/resource exhaustion
- malformed input, mutation, replay, wrong identity negative tests
- CI immutable PR-head checkout, artifact hash/provenance 구조
- generated board drift 및 API count guard
- FDCAN fixed Message RAM contract

## 실행한 주요 검증 명령

```powershell
git -c safe.directory=F:/dev/canview -C F:/dev/canview cat-file -e "f4c413a153dcf2ff5c771c457275e3b4af25f385^{commit}"
git -c safe.directory=F:/dev/canview -C F:/dev/canview cat-file -e "6db3998092812354afe3b3918892b682022990b4^{commit}"
git -c safe.directory=F:/dev/canview -C F:/dev/canview rev-parse "f4c413a153dcf2ff5c771c457275e3b4af25f385^{commit}"
git -c safe.directory=F:/dev/canview -C F:/dev/canview diff --find-renames --stat 6db3998092812354afe3b3918892b682022990b4 f4c413a153dcf2ff5c771c457275e3b4af25f385
git -c safe.directory=F:/dev/canview -C F:/dev/canview ls-tree -r --name-only f4c413a153dcf2ff5c771c457275e3b4af25f385
git -c safe.directory=F:/dev/canview -C F:/dev/canview show "<candidate>:<path>"
git -c safe.directory=F:/dev/canview -C F:/dev/canview grep -n -E "<pattern>" f4c413... -- <path>
```

후보 diff는 49 files, `6909 insertions`, `44 deletions`로 확인했다. 모든 후보 파일은 commit object에서 읽었으며 worktree source를 수정하거나 target build를 실행하지 않았다.

최종적으로 현재 후보는 target evidence provenance와 corrected identity contract가 정리되기 전까지 승인할 수 없다.
