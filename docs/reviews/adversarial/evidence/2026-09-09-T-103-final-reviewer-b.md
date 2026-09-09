# Reviewer B 적대적 감사 보고서

- Reviewer execution ID: `7d8c6724-ce1e-423b-92f9-0d47ed7a2ef6`
- 시작: `2026-09-08T22:58:56.6260765Z` / KST `2026-09-09T07:58:56.6307933+09:00`
- 종료: `2026-09-08T23:14:07.9926199Z` / KST `2026-09-09T08:14:07.9965091+09:00`
- Candidate: `04a1e9367138498bb03906befac2f904f2f943dd`
- Base: `6db3998092812354afe3b3918892b682022990b4`
- 범위: immutable Git object-only review
- 작업 트리 파일 수정·커밋·push·target build·physical/HIL 실행: 없음

## 최종 판정

**BLOCK**

P1 두 건이 남아 있어 T-103 final candidate를 merge/release-ready로 승인할 수 없습니다.

## Findings

### P1 — 실제 FDCAN RX FIFO 설정이 구성되지 않음

파일:

- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:192-215`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c:401-438`
- `tests/stm32/fake_stm32/stm32g474xx.h:30-48`
- `tests/stm32/test_fdcan_platform.c:125-134, 377-390`

`configure_instance()`는 `CCCR`, `NBTP`, `RXGFC`, interrupt 등을 설정하지만, 이후 `drain_fifo()`가 의존하는 FIFO base, FIFO depth, element size를 target register에 설정하지 않습니다. `drain_fifo()`는 고정된 `848/176/72/3` 레이아웃을 직접 계산해 읽습니다.

fake register map에도 `RXF0C`와 `RXESC`가 없으며, 테스트는 실제 설정 검증 없이 SRAM에 element를 직접 주입합니다.

시나리오: reset 직후 또는 FIFO layout이 다른 target 환경에서 RX interrupt가 발생해도 FIFO가 올바르게 구성되지 않아 frame이 누락되거나 잘못된 Message RAM 위치를 읽을 수 있습니다. Host fake 테스트는 이를 탐지하지 못합니다.

권고:

- 검증된 STM32G474 register map/HAL 설정으로 FIFO base, depth, element size를 명시적으로 구성할 것.
- 또는 reset default와 고정 layout의 근거를 STM32 target readback 및 공식 reference evidence로 증명할 것.
- fake register에 관련 설정 필드를 추가하고, 설정 값과 실제 drain 주소의 일치 테스트를 추가할 것.

### P1 — target build evidence가 final candidate에 결속되지 않음

파일:

- `docs/reviews/adversarial/evidence/2026-09-09-T-103-target.md:3-4`
- `docs/tasks/T-103-stm32-fdcan-capture.md:79-87`

Candidate evidence는 다음 source를 사용했다고 기록합니다.

```text
source candidate: 3e13b2ca6e72a3aec5a32a6357285c614bc191f9
```

검증 결과 `3e13b2ca...`는 candidate `04a1e936...`의 ancestor이지만 final candidate와 동일한 commit은 아닙니다. 따라서 기록된 ELF/BIN/HEX/MAP hash와 build 결과는 final candidate의 target build 증거가 아닙니다.

`foundation.yml`의 target job에는 checkout revision 검사가 있지만, 이 stale evidence 문서 자체는 final candidate artifact manifest와 연결되어 있지 않습니다.

권고:

- 정확히 `04a1e936...`에서 clean target debug/release build를 재실행할 것.
- artifact manifest에 commit SHA와 firmware source digest를 함께 기록할 것.
- 기존 `3e13b2ca...` 결과는 historical evidence로 분리하고 final gate evidence로 사용하지 말 것.

## 검증 결과

### Source identity 및 event identity

구현상 다음은 확인했습니다.

- `tests/hil/run.py:39-81`이 Git commit과 firmware source SHA-256을 생성함.
- `tests/hil/run.py:92-97`이 execution ID와 firmware identity를 각 event에 공급함.
- `tests/hil/run_can_capture.py:61-66`이 생성된 event log에 `assert_no_tx()`를 적용함.
- `tests/hil/assert_no_tx.py:105-109, 159-190`이 source, execution ID, firmware identity를 exact match로 검증함.
- `tests/test_t103_capture_helpers.py:42-55`가 모든 event의 identity 반복을 검사함.

단, runner의 일반 실행은 현재 checkout을 기준으로 self-identifying합니다. 별도의 expected candidate SHA 인자는 없습니다. CI는 workflow checkout revision 검사로 보완하지만, standalone 실행은 immutable candidate provenance를 독립적으로 증명하지 않습니다.

### Event kind/field allowlist

확인된 방어:

- `tests/hil/assert_no_tx.py:18-67`: record key, event kind, nested budget metric allowlist
- `tests/hil/assert_no_tx.py:117-156`: unknown kind/field, required field, channel, numeric field 검증
- `tests/hil/assert_no_tx.py:74-98`: duplicate JSON key, oversized integer, float, non-finite constant 거부
- `tests/hil/assert_no_tx.py:194-207`: bounded evidence/line size 및 truncated JSONL 거부
- `tests/hil/assert_no_tx.py:210-216`: expected identity 누락 시 `BLOCKED`
- `CAN_TX`, `VEHICLE_CAN_TX`는 forbidden kind으로 처리됨.

이 영역에서는 추가 P1/P2를 찾지 못했습니다.

### Fixture digest

Candidate fixture:

- Path: `tests/hil/fixtures/t103-capture-only.jsonl`
- Git blob size: `1606` bytes
- Blob object: `e2dcc9b28bcc4cd2af7a1df054472e4d3a2db805`
- Content SHA-256: `23715921ca6239b91f5872df8c860b7b4cb74c77f821d1b33da550d23eec010e`

현재 test는 fixture의 의미적 계약과 expected identity를 검증하지만 fixture content SHA-256 자체를 고정하지 않습니다. 이는 재현성 개선 사항이지만, 이번 BLOCK의 직접 원인은 아닙니다.

### API count

정적 header declaration inventory를 계산한 결과 unique public function count는 **50**입니다.

`tools/build_docs.py`의 count guard도 50으로 설정되어 있습니다. Doxygen 자체는 object-only 제한으로 실행하지 않았습니다.

### CI provenance

`foundation.yml`은 각 job에서 PR head SHA를 checkout하며 target job에서 `HEAD == expected source revision`을 검사합니다.

다만 현재 workflow의 host 실행은 직접적으로 다음을 호출합니다.

- `.github/workflows/foundation.yml:61-63`
  - `tests/hil/run.py`
  - `tests/hil/validate_evidence.py`

`python -m unittest discover`가 T-103 helper test를 간접 실행하므로 strict helper 코드 자체는 CI 대상입니다. 그러나 final target evidence는 위 P1처럼 stale commit입니다.

## No-finding 영역

다음 영역에서는 정적 검토상 별도 P1/P2를 찾지 못했습니다.

- ESP32 board profile 및 generated `sdkconfig.defaults`
- sdkconfig allowlist/forbidden security checks
- `boards.json`과 generator output consistency
- CMake dependency 및 `CAPTURE_ONLY` build-mode enforcement
- STM32 core TX symbol/source gate
- Controller/Communicator/Bridge raw CAN TX 경계
- Diagnostic Bridge read-only boundary
- static ring, pool, queue, event log resource bounds
- mutation/negative test coverage
- duplicate key 및 malformed JSONL 처리
- API documentation inventory
- compile warning 및 reproducibility gate의 정적 wiring
- FDCAN module의 bounded ring, timestamp wrap, drop accounting, sink retry 구조

## Physical/HIL 상태

- Physical board flash/boot: `NOT_RUN`
- STM32 target build 재실행: `NOT_RUN`
- 실제 3-bus CAN capture: `NOT_RUN`
- analyzer ACK/data TX 측정: `NOT_RUN`
- reset/brownout/rail/PHY/watchdog soak: `NOT_RUN`
- vehicle CAN/TX release: `NOT_RUN`

기존 target evidence의 compile/artifact claim은 final candidate에 유효한 증거로 인정하지 않았습니다.

## Object-only 감사 명령

검증에 사용한 주요 명령:

```powershell
git -c safe.directory=F:/dev/canview -C F:/dev/canview cat-file -e '04a1e9367138498bb03906befac2f904f2f943dd^{commit}'
git -c safe.directory=F:/dev/canview -C F:/dev/canview cat-file -e '6db3998092812354afe3b3918892b682022990b4^{commit}'
git -c safe.directory=F:/dev/canview -C F:/dev/canview rev-parse '04a1e9367138498bb03906befac2f904f2f943dd^{commit}'
git -c safe.directory=F:/dev/canview -C F:/dev/canview rev-parse '6db3998092812354afe3b3918892b682022990b4^{commit}'
git -c safe.directory=F:/dev/canview -C F:/dev/canview diff --find-renames --stat base candidate
git -c safe.directory=F:/dev/canview -C F:/dev/canview ls-tree -r --name-only candidate
git -c safe.directory=F:/dev/canview -C F:/dev/canview show candidate:<path>
git -c safe.directory=F:/dev/canview -C F:/dev/canview grep -n <pattern> candidate -- <paths>
git -c safe.directory=F:/dev/canview -C F:/dev/canview cat-file -t 3e13b2ca6e72a3aec5a32a6357285c614bc191f9
git -c safe.directory=F:/dev/canview -C F:/dev/canview rev-parse '3e13b2ca6e72a3aec5a32a6357285c614bc191f9^{commit}'
git -c safe.directory=F:/dev/canview -C F:/dev/canview merge-base --is-ancestor 3e13b2ca6e72a3aec5a32a6357285c614bc191f9 candidate
git -c safe.directory=F:/dev/canview -C F:/dev/canview cat-file -s candidate:tests/hil/fixtures/t103-capture-only.jsonl
```

Fixture raw blob은 PowerShell/.NET `ProcessStartInfo`로 `git cat-file blob`의 stdout bytes를 직접 SHA-256 계산했습니다.

## Read set

Repository context:

- `AGENTS.md`
- `docs/README.md`
- `docs/resume.md`
- `docs/runbooks/agent-workflow.md`

Candidate blobs:

- `.github/workflows/foundation.yml`
- `CMakeLists.txt`
- `firmware/communicator/stm32/CMakeLists.txt`
- `firmware/boards/boards.json`
- `firmware/communicator/sdkconfig.defaults`
- `firmware/bridge/sdkconfig.defaults`
- `firmware/controller/sdkconfig.defaults`
- `tools/check_sdkconfig.py`
- `tools/generate_boards.py`
- `tools/build_docs.py`
- `tools/check_stm32_core.py`
- `tools/sdkconfig-allowlist/esp32s3-idf-6.0.3.keys`
- `tests/foundation/test_sdkconfig.py`
- `docs/api/Doxyfile`
- `tests/hil/assert_no_tx.py`
- `tests/hil/events.py`
- `tests/hil/run.py`
- `tests/hil/run_can_capture.py`
- `tests/hil/validate_evidence.py`
- `tests/hil/fixtures/t103-capture-only.jsonl`
- `tests/test_t103_capture_helpers.py`
- `tests/test_stm32_core_gate.py`
- `docs/tasks/T-103-stm32-fdcan-capture.md`
- `docs/reviews/adversarial/evidence/2026-09-09-T-103-target.md`
- `firmware/communicator/stm32/interface/canview_stm_fdcan_capture.h`
- `firmware/communicator/stm32/module/fdcan_capture.c`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.c`
- `firmware/communicator/stm32/platform/stm32g474/fdcan_capture.h`
- `firmware/communicator/stm32/docs/fdcan-capture.md`
- `tests/stm32/test_fdcan_capture.c`
- `tests/stm32/test_fdcan_platform.c`
- `tests/stm32/fake_stm32/stm32g474xx.h`
- `tests/stm32/fake_stm32/fake_hardware.c`
- `tests/esp_core/board_fixture.h`
- `tests/esp_core/sdk_fixture.h`
- `tests/esp_core/test_app.c`
- `tests/esp_core/test_bridge_integration.c`
- `tests/esp_core/test_core.c`
- `tests/esp_core/test_pool_threads.c`
- `tests/esp_core/test_runtime.c`
- `tests/esp_core/test_wrong_bsp.c`

검토에 사용한 embedded skill 지침:

- `C:/Users/digit/.codex/skills/embedded-architecture/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-cstyle/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-rtos-design/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-isr-design/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-driver-design/SKILL.md`
- `C:/Users/digit/.codex/skills/embedded-documentation/SKILL.md`
