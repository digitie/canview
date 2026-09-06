# 기반 코드 빌드·시험·API 문서

현재 구현 범위와 후속 agent 계약은 [기반 아키텍처](../architecture/firmware-foundation.md), SDK 설치는 [Windows 정본](windows.md)에 둔다. 아래 명령은 저장소 root의 PowerShell 기준이다. flash/erase 명령은 포함하지 않는다.

## Windows host 도구

Visual Studio C++ Build Tools와 Windows SDK, Python3.14가 필요하다. 전역 SDK나 사용자 도구를 교체하지 않는다.

```powershell
. tools/environment/foundation-windows.ps1 -IncludeDocs
cmake --preset host-debug
cmake --build --preset host-debug
ctest --preset host-debug
cmake --preset host-release
cmake --build --preset host-release
ctest --preset host-release
```

첫 script는 tools/foundation-tools.json의 공식 URL/SHA256 archive를 .tools에 내려받고 현재 shell PATH와 PYTHONUTF8=1을 설정한다. CTest의 Python 진입점도 -X utf8을 지정해 영문 Windows의 redirected 한글 진단이 cp1252 오류로 중단되지 않게 한다.
Clang23.1.0 GNU frontend, CMake4.4.3, Ninja1.13.2, Doxygen1.18.0을 고정한다. LLVM archive는 약901 MB로 첫 설치 비용이 크다.
download/digest/version 오류는 중단하고 기존 archive를 자동 삭제/덮어쓰지 않는다.
PowerShell에서 명령을 개별 실행할 때도 각 exit code를 확인한다. CI는 모든 단계 실패를 즉시 전파한다.

새 core와 BSP는 -std=c99, extensions OFF, -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef를 사용한다.
Clang이 Windows linker/SDK를 사용하더라도 cl.exe의 C99 검증 결과로 표시하지 않는다.
기존 tests/automation은 C11 prototype 시험 12개이며 별도 target에 그대로 유지한다.

## 생성물과 시험 범위

```powershell
python -B tools/generate_transport.py --check
python -B tools/generate_boards.py --check
cmake --preset host-coverage
cmake --build --preset host-coverage
python -B tools/check_coverage.py
```

coverage script는 새 임시 profile 디렉터리를 만들고 9개 core CTest를 재실행한다. 이전 profile을 합치지 않는다.
측정 대상은 shared/protocol/src/canview_wire.c와 shared/app/src/canview_app.c 두 파일이며 line/function100%, branch99% 이상을 요구한다.
BSP mock, SDK/driver, Python generator, legacy 자동화는 이 분모에 포함하지 않는다.

| 시험 | 주요 공격 범위 |
|---|---|
| CRC/envelope | 표준 check, 모든 합법 payload 길이, 최대 길이, unaligned, version/reserved/flags, 전체 bit 손상 |
| COBS/stream | 0..1024바이트 여러 패턴, 254 경계, 부족한 출력, oversized packet 폐기와 재동기화 |
| CAN batch | 0..12 record, 11/29-bit ID 한계, 3bus/DLC/flags/RTR/padding/time overflow |
| sequence | 중복, 재정렬, 63/64 경계, modulo32 wrap, half-range, 큰 점프 |
| null/noise/app | 공개 API null, 결정적 잡음5000회, 역할4개, 초기화 실패 고정·반복 시작 거부 |
| 독립 golden | Python struct/zlib/독립 COBS와 C의 2195개 frame 직렬화 대조 |
| BSP4종 | 순서·실제 pin·safe level·open-drain, 각 GPIO 호출 실패에서 중단 |
| generator | source/output drift, BOM, 중복 pin, PSRAM 금지 pin, partition overlap/크기, clock, SDK SHA 길이 |

임의 잡음 시험은 sanitizer/지속 fuzzing의 대체가 아니다. Linux 보조 CI에서 GCC Release와 Clang ASan+UBSan을 별도 실행한다.
GitHub workflow는 .github/workflows/foundation.yml이며 Windows job과 Linux job을 구분한다. SDK/target 성공을 나타내는 job은 아직 없다.

## API 문서

```powershell
python -m venv .tools/api-venv
.tools/api-venv/Scripts/python.exe -m pip install --require-hashes -r tools/requirements-docs.lock
.tools/api-venv/Scripts/python.exe -B tools/build_docs.py
```

결과는 build/api/html/index.html이다. Sphinx9.1.0/Breathe4.36.0/Furo2025.12.19와 Doxygen1.18.0을 실제 실행한다.
Doxygen 경고와 Sphinx -n -W 경고는 실패다. 별도로 XML에서 공개 함수14개의 brief·param·return 누락을 실패 처리한다.
새 API를 추가할 때 checker의 기대 개수도 review한다. callback type은 C 함수 type typedef와 필드 pointer를 분리해 문서 추출기의 함수 pointer 표기 모호성을 피했다. ABI 비용 변화는 없다.
lock 재생성은 별도 도구 환경의 pip-tools7.5.2로 수행한다.

```powershell
pip-compile --generate-hashes --strip-extras --output-file tools/requirements-docs.lock tools/requirements-docs.in
```

## target 구성 확인과 남은 gate

고정 SDK 환경을 준비한 뒤 각 ESP 프로젝트에서 idf.py build, STM32 프로젝트에서 cmake --preset debug와 cmake --build --preset debug를 실행한다.
대상 경로는 firmware/controller, firmware/communicator/esp32, firmware/diagnostic-bridge, firmware/communicator/stm32다.
새 IDF main component는 canview_foundation/esp_driver_gpio/esp_psram에만 의존하고 legacy component를 image에서 제외한다.

현재 세션에서 target SDK/Arm toolchain이 준비되지 않아 실제 target build와 HIL은 미실행이다.
기존 setup-windows.ps1 -VerifyOnly는 일반 shell에서 CMake 검색 실패, 새 고정 host 도구 활성화 뒤에는 arm-none-eabi-gcc 부재로 실패했다. 새 host 도구 활성화는 Arm/IDF 설치 완료가 아니다.
STM32CubeG4 commit의 기존 39자리 오기를 공식 v1.6.3 전체40자리로 수정했으나 이것만으로 target build 성공이라 하지 않는다.

## 작성자 측정 기록

- 고정 Windows Clang23.1.0/CMake4.4.3/Ninja1.13.2 Debug 및 Release: 각각 CTest31/31 PASS, 새 C99·legacy 각각 적용 경고0.
- 공용 core 측정: 실행 line100%, function100%, branch99.66% (protocol99.63%, app100%).
- API: Doxygen XML 계약14개 PASS, Sphinx strict warning0.
- 새 profile로 coverage gate를 재실행했고 hash lock 설치 및 API strict build도 통과했다. 독립 리뷰 결과는 review record에서 별도로 추적한다.
- 실제 보드의 pin 파형/PSRAM/clock/DMA/UART4 Mbps/전원 단전/CAN 송신 안전은 미검증이다.
