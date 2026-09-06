# T-001 post-fix 재리뷰 B 원문 evidence

- 실행 ID: `01a0757e-c21c-74d2-9ceb-8c7da4da42f1` (Gauss)
- 전문 영역: CI·재현성·protocol/build integration·evidence integrity
- 기준선: `208bec5b4e621cbc06c8bbd012f891b0b7c8a7cc`
- 검토 commit: `f447d86e9084ab74c141327259e6b672dfdb2251`
- 격리 방식: commit/object 기준 읽기 전용 검토
- 저장소 상태: `git status --short` 출력 없음
- 시작·종료 시각: 2026-09-06, orchestrator dispatch와 완료 notification 사이. subagent API가 독립 wall-clock을 제공하지 않음.
- 전달 요청: CI/reproducibility/evidence integrity 관점에서 이전 finding closure와 새 회귀를 확인하고 파일을 변경하지 말 것.
- 최종 verdict: `CONDITIONAL`

## 이전 finding closure

| 이전 finding | 판정 | 근거 |
|---|---|---|
| ENV-001 annotated tag/peeled commit 불일치 | FIXED | `setup-windows.ps1:98-111`에서 tag commit과 HEAD를 full commit으로 비교 |
| ENV-002 `VerifyOnly`의 임의 SDK 허용 | FIXED | remote, tag, HEAD, clean status, recursive submodule 검사: `setup-windows.ps1:93-144` |
| ENV-003 ESP-IDF export 후 tool drift | FIXED | export 후 CMake/Ninja/Arm GCC 재검증 및 pinned path 복원: `setup-windows.ps1:239-261` |
| ENV-004 incomplete protocol application 연결 | REOPENED | 아래 B-P2-01 |
| ENV-005 Windows 문서 명령 불일치 | FIXED | 현재 Windows 명령과 link validator 결과 일치 |

P0/P1은 발견하지 않았고 P2 3건, P3 2건으로 `CONDITIONAL` 판정했다.

## 원문 findings

### B-P2-01 — 이전 protocol 경계 finding 재개방

위치: `firmware/communicator/esp32/main/CMakeLists.txt:5`, `firmware/controller/main/CMakeLists.txt:5`

두 application main이 `REQUIRES canview_protocol`을 직접 갖도록 바뀌었으나 T-002가 아직 `BLOCKED`이고 ESP-NOW/UART ABI가 동결되지 않았다. 이전 ENV-004 disposition은 T-002 전 bootstrap main이 incomplete protocol component를 link/include하지 않는 것이었으므로 finding이 닫히지 않고 재개방된다. 권고는 T-002 전 main 직접 의존성을 제거하고 public component fixture만 `canview_can → canview_protocol` 경계를 검증하는 것이다.

### B-P2-02 — local Arm archive provenance가 setup에서 강제되지 않음

위치: `tools/environment/setup-windows.ps1:165-195`, `tools/README.md:20`

CI는 archive SHA-256을 확인하지만 local setup은 `-ArmGnuRoot`의 executable과 GCC version prefix만 검사한다. 동일한 GCC prefix의 다른 build나 교체된 binary가 local `VerifyOnly`를 통과할 수 있다.

### B-P2-03 — target artifact 보존이 부분적이며 missing artifact가 warning으로 끝날 수 있음

위치: `.github/workflows/foundation.yml:134-177`

fixture binary가 upload path에 없고 `if-no-files-found: error`가 없어 `.elf`/`.map` 일부 누락이 warning 후 성공할 수 있다. 권고는 fixture binary/map, 필요한 `.su`, `if-no-files-found: error`, 별도 existence assertion이다.

### B-P3-01 — budget manifest schema가 완전히 closed-world가 아님

위치: `tools/check_budgets.py:95-139`, `tools/check_negative_fixtures.py:72-97`

duplicate key와 malformed/unknown evidence metric은 개선됐지만 top-level unknown key, metric definition unknown field, `limit=True`가 명시적으로 거부되지 않았다. schema drift와 typo 방지 negative fixture가 부족하다.

### B-P3-02 — warning-free 판정은 로그 정규식 의존

위치: `.github/workflows/foundation.yml:120-132`, `docs/development/foundation.md:80-81`

native exit code와 일반적인 warning/error 표현은 fail-closed로 검사하지만 setup/export 출력과 모든 toolchain warning 표기를 완전히 포괄하지 않는다. 리뷰에서는 실제 target build나 원격 GitHub Actions를 재실행하지 않았다.

## synthetic budget 범위와 실행 결과

`config/budgets/foundation.md`가 명시하듯 budget evidence는 synthetic이다. target `.map`, `.su`, runtime latency로 연결하지 않으며 target memory/stack/runtime budget은 T-001에서 미실행·미검증이다. 이 범위는 정직하게 문서화돼 P1로 올리지 않았다.

실행 결과:

- commit object/parent, `git diff --check 208bec5 f447d86`, `git status --short`: PASS/clean
- YAML custom structure 및 PowerShell AST: PASS
- `foundation-windows.ps1` 후 `setup-windows.ps1 -VerifyOnly -ToolRoot C:\cv`: CMake 4.4.3, Ninja 1.13.2, Arm GCC 15.3, ESP-IDF 6.0.3, STM32CubeG4 확인, exit 0
- budget/generated/plan/document link 검사: PASS; task 46, 문서 151개·local target 1011개
- `python -B -m unittest discover -s tests -p "test_*.py"`: 35 tests OK
- 실제 target configure/build, 원격 CI, artifact upload/download, HIL: 실행하지 않음

## 잔여 위험

기존 ignored target binary만 읽었고 이번 리뷰에서 재빌드하지 않았다. synthetic budget, warning scan의 정규식 한계, 차량 CAN TX, board bring-up, HIL, reset/brownout, PSRAM/clock/DMA 검증은 여전히 NO-GO다.
