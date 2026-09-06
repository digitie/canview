# T-001 재현 가능한 host toolchain과 CI

- 상태: `IN_PROGRESS`
- 우선순위: `P0`
- Gate: `G0`
- 선행: 없음
- 병렬 가능: `T-100`

## 목표

2026-09-06 foundation은 [구조/인수인계](../architecture/firmware-foundation.md), [검증 절차](../development/foundation.md)에 기록한다. root CMake/CTest·strict C99·CI workflow·hash 고정 API 문서·pin/schema generator·실패 fixture·budget evidence checker·target SDK job을 연결한다. 전체 ABI·HIL·전체 task 수용 조건은 이 task 범위가 아니므로 이 task를 완료해도 G0–G6이나 제품 release가 아니다.

어느 agent와 CI에서도 동일한 host test, generator, 정적 검사를 실행할 수 있게 version과 명령을 고정한다. 현재처럼 CMake가 없는 환경에서 일부 GCC 명령만 우회 실행하는 상태를 제거한다.

## 결정사항

- Windows PowerShell과 Windows native toolchain을 local 재현성의 기준 환경으로 사용한다.
- 필수 CI는 `windows-latest`에서 manifest의 CMake `4.4.3`, Ninja `1.13.2`, MSVC 또는 Windows LLVM, Python `3.14`를 실행한다. Linux GCC/Clang job은 cross-platform portability와 sanitizer를 위한 별도 matrix로 유지하되 Windows 기준을 대체하지 않는다.
- target job은 Arm GNU Toolchain `15.3.Rel1`, ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`의 source commit을 검증한다. SDK가 없는 host job을 target build 성공으로 집계하지 않는다.
- ESP-IDF와 GNU Arm build는 별도 container/job으로 두고 host job과 섞지 않는다.
- Python dependency는 hash가 고정된 lock file을 사용한다.
- ESP-IDF component lock, Waveshare example commit, STM32CubeG4 commit과 archive digest도 불변값으로 기록한다. 이동하는 branch/tag만 pin으로 인정하지 않는다.
- 첫 CI는 vehicle hardware나 secret을 요구하지 않는다.

## 구현 범위

- root `CMakeLists.txt`, `CMakePresets.json`, `CTest` 구성
- `tools/requirements-docs.lock`과 generator/test dependency
- `.github/workflows/foundation.yml`
- C/C++ warning, ASan/UBSan, Python unit/test, generated diff, Markdown link 검사
- `config/budgets/*.yaml`과 map/stack/runtime evidence checker skeleton
- `tests/fixtures/idf-public-component`의 외부 `canview_controller_can.h` compile fixture
- 기존 `tests/automation`을 root test suite에 편입
- generated protocol은 host `INTERFACE` target과 ESP-IDF public component로 노출한다. T-002 schema가 동결되기 전 application main은 직접 의존하지 않으며, public component fixture의 `canview_can → canview_protocol` transitive 경계만 compile로 검증한다.

## 범위 밖

- ESP-IDF application 기능 구현과 실제 board bring-up (별도 T-200/T-300)
- STM32 vendor source vendoring
- 차량 capture나 hardware-in-loop 실행

## 예상 변경 파일

```text
CMakeLists.txt
CMakePresets.json
tools/requirements-docs.lock
.github/workflows/foundation.yml
tools/check_generated.py
tools/check_budgets.py
tools/check_negative_fixtures.py
tests/fixtures/idf-public-component/
```

## 구현 순서

1. 기존 C source를 warning-free host library target으로 묶는다.
2. GCC와 Clang debug preset에 `-Wall -Wextra -Wshadow -Wconversion -Wformat=2 -Werror`를 적용한다.
3. sanitizer preset을 추가한다.
4. Python environment와 표준 library `unittest`, schema validator, `cantools` version을 고정한다.
5. generated file drift 검사와 Markdown 상대 link 검사를 추가한다.
6. GitHub Actions에서 host matrix를 실행한다.
7. 향후 IDF/Arm job이 실패해도 host job 결과를 구분할 수 있게 job 이름을 고정한다.
8. public header가 include하는 protocol header를 `PRIV_INCLUDE_DIRS`에만 두는 현재 component 구성을 제거한다.

## 수용 기준

- [x] Windows clean clone에서 문서에 적힌 PowerShell 명령으로 configure/build/test가 된다.
- [x] GCC와 Clang 모두 기존 host test를 통과하도록 Linux matrix와 Windows Clang gate를 연결한다.
- [x] 필수 `windows-latest` host/target job과 Linux portability/sanitizer job의 실패가 구분되어 표시된다.
- [x] ASan/UBSan job이 별도 job으로 오류를 전파한다.
- [x] 생성물 수동 변경 fixture가 CI에서 실패한다.
- [x] 깨진 Markdown link fixture가 CI에서 실패한다.
- [x] CI에 vehicle secret·capture·USB device가 필요하지 않다.
- [x] `canview_controller_can.h`만 include하는 외부 IDF component가 별도 compile되며 `REQUIRES canview_protocol` transitive public protocol dependency를 찾는다.
- [x] lock file과 source digest가 바뀌지 않은 clean build에서 dependency resolution 결과가 동일하도록 host archive, Arm archive SHA256과 SDK commit을 manifest에 고정한다.
- [x] budget Markdown와 machine manifest가 어긋나거나 synthetic map/stack/latency가 한도를 넘으면 CI가 실패한다.

## 계획 보완 수용 기준

- [x] `tools/validate_plan.py`와 `tests/test_plan_validation.py`를 읽기 전용 CI gate로 실행해 task 수/ID/상태/제목/선행/순환/요약 불일치를 검출한다.
- [x] task 검증 절의 script/fixture/CTest target을 구현 산출물로 추적한다. 현재 존재하는 링크/host/target 검사는 실행하고 미생성 HIL 명령은 성공으로 집계하지 않는다.

## 검증 명령

```powershell
. .\tools\environment\foundation-windows.ps1
cmake --preset host-debug
cmake --build --preset host-debug
ctest --preset host-debug --output-on-failure
cmake --preset host-sanitize
cmake --build --preset host-sanitize
ctest --preset host-sanitize --output-on-failure
python -B -m unittest discover -s tests -p "test_*.py"
python -B tools/check_generated.py
python -B tools/check_budgets.py
python -B tools/check_negative_fixtures.py
python -B tools/validate_document_links.py
idf.py -C tests/fixtures/idf-public-component set-target esp32s3
idf.py -C tests/fixtures/idf-public-component build
```

## 증거와 rollback

PR에 tool version, 전체 CI URL, test 수를 남긴다. 기존 직접 GCC test가 새 CMake target과 같은 source를 빌드하는지 한 번 비교한다. CI가 불안정하면 검사를 삭제하지 말고 job을 분리하고 원인을 기록한다.

## 현재 closure evidence

- 상태는 `IN_PROGRESS`를 유지한다. 수용 기준·evidence·리뷰는 닫혔지만, 이 저장소의 `DONE` 정의가 main merge까지 요구하므로 Draft PR 단계에서 임의로 `DONE`으로 바꾸지 않는다.
- branch/PR: `agent/codex-firmware-foundation`, [Draft PR #17](https://github.com/digitie/canview/pull/17)
- 최종 구현 commit: `9cb76e2`; 문서 closure head: `5d5d82c`; 최종 local target binary와 SHA-256은 [target evidence](../reviews/adversarial/evidence/2026-09-06-T-001-target-final.md)에 둔다.
- 독립 적대적 리뷰 최종 report: [T-001 report](../reviews/adversarial/2026-09-06-T-001.md). 최종 reviewer A/B P0–P3 finding은 0건이며 두 verdict는 `PASS`다.
- 직접 설치 SDK: Arm GNU `15.3.Rel1`, ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`; Arm archive와 설치 root 전체 file inventory를 검증했다.
- local 검증: host Debug/Release/Coverage CTest 각각 35/35, Python unittest 35, coverage gate, generated/budget/negative/plan/document/API gate PASS; STM32 debug/release와 ESP32-S3 세 이미지·public fixture clean build 및 warning/error scan PASS.
- 원격 검증: 최신 head의 [PR run 34023031785](https://github.com/digitie/canview/actions/runs/34023031785)와 [push run 34023029483](https://github.com/digitie/canview/actions/runs/34023029483)가 target 포함 전체 `success`다.
- 제한: 보드 flash, HIL, 차량 CAN TX, production OTA signing/provisioning은 별도 gate다.
