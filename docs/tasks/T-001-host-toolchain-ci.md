# T-001 재현 가능한 host toolchain과 CI

- 상태: `READY`
- 우선순위: `P0`
- Gate: `G0`
- 선행: 없음
- 병렬 가능: `T-100`

## 목표

어느 agent와 CI에서도 동일한 host test, generator, 정적 검사를 실행할 수 있게 version과 명령을 고정한다. 현재처럼 CMake가 없는 환경에서 일부 GCC 명령만 우회 실행하는 상태를 제거한다.

## 결정사항

- Windows PowerShell과 Windows native toolchain을 local 재현성의 기준 환경으로 사용한다.
- 필수 CI는 `windows-latest`에서 manifest의 CMake `4.4.3`, Ninja `1.13.2`, MSVC 또는 Windows LLVM, Python `3.12`를 실행한다. Linux GCC/Clang job은 cross-platform portability와 sanitizer를 위한 별도 matrix로 유지하되 Windows 기준을 대체하지 않는다.
- target job은 Arm GNU Toolchain `15.3.Rel1`, ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`의 source commit을 검증한다. SDK가 없는 host job을 target build 성공으로 집계하지 않는다.
- ESP-IDF와 GNU Arm build는 별도 container/job으로 두고 host job과 섞지 않는다.
- Python dependency는 hash가 고정된 lock file을 사용한다.
- ESP-IDF component lock, Waveshare example commit, STM32CubeG4 commit과 archive digest도 불변값으로 기록한다. 이동하는 branch/tag만 pin으로 인정하지 않는다.
- 첫 CI는 vehicle hardware나 secret을 요구하지 않는다.

## 구현 범위

- root `CMakeLists.txt`, `CMakePresets.json`, `CTest` 구성
- `pyproject.toml`과 lock file, generator/test dependency
- `.github/workflows/ci.yml`
- C/C++ warning, ASan/UBSan, Python lint/test, generated diff, Markdown link 검사
- `config/budgets/*.yaml`과 map/stack/runtime evidence checker skeleton
- 기존 `tests/automation`을 root test suite에 편입
- generated protocol을 host `INTERFACE` target과 ESP-IDF public component로 노출하고 consumer는 `REQUIRES canview_protocol`을 사용

## 범위 밖

- ESP-IDF application 기능 구현과 실제 board bring-up (별도 T-200/T-300)
- STM32 vendor source vendoring
- 차량 capture나 hardware-in-loop 실행

## 예상 변경 파일

```text
CMakeLists.txt
CMakePresets.json
pyproject.toml
uv.lock 또는 requirements.lock
.github/workflows/ci.yml
tests/CMakeLists.txt
tools/check_generated.py
```

## 구현 순서

1. 기존 C source를 warning-free host library target으로 묶는다.
2. GCC와 Clang debug preset에 `-Wall -Wextra -Wshadow -Wconversion -Wformat=2 -Werror`를 적용한다.
3. sanitizer preset을 추가한다.
4. Python environment와 `pytest`, schema validator, `cantools` version을 고정한다.
5. generated file drift 검사와 Markdown 상대 link 검사를 추가한다.
6. GitHub Actions에서 host matrix를 실행한다.
7. 향후 IDF/Arm job이 실패해도 host job 결과를 구분할 수 있게 job 이름을 고정한다.
8. public header가 include하는 protocol header를 `PRIV_INCLUDE_DIRS`에만 두는 현재 component 구성을 제거한다.

## 수용 기준

- [ ] Windows clean clone에서 문서에 적힌 PowerShell 명령으로 configure/build/test가 된다.
- [ ] GCC와 Clang 모두 기존 host test를 통과한다.
- [ ] 필수 `windows-latest` job과 Linux portability/sanitizer job의 실패가 구분되어 표시된다.
- [ ] ASan/UBSan에서 오류가 없다.
- [ ] 생성물 수동 변경 fixture가 CI에서 실패한다.
- [ ] 깨진 Markdown link fixture가 실패한다.
- [ ] CI에 vehicle secret·capture·USB device가 필요하지 않다.
- [ ] `canview_controller_can.h`만 include하는 외부 IDF component가 별도 compile되며 transitive public protocol dependency를 찾는다.
- [ ] lock file과 source digest가 바뀌지 않은 clean build에서 dependency resolution 결과가 동일하다.
- [ ] budget Markdown와 machine manifest가 어긋나거나 synthetic map/stack/latency가 한도를 넘으면 CI가 실패한다.

## 검증 명령

```powershell
cmake --preset host-debug
cmake --build --preset host-debug
ctest --preset host-debug --output-on-failure
cmake --preset host-sanitize
cmake --build --preset host-sanitize
ctest --preset host-sanitize --output-on-failure
py -3 -m pytest -q
py -3 tools/check_generated.py
```

## 증거와 rollback

PR에 tool version, 전체 CI URL, test 수를 남긴다. 기존 직접 GCC test가 새 CMake target과 같은 source를 빌드하는지 한 번 비교한다. CI가 불안정하면 검사를 삭제하지 말고 job을 분리하고 원인을 기록한다.
