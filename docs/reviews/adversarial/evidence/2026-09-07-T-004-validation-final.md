# T-004 최종 host·target 검증 기록

- 구현 기준: `3c6967aa83829cedbf7dcb4b54f67fd564e021e9`
- 문서 정리 기준: `4a0b5fa0c40a3849e9e98e5aa4486aca28c5c144`
- 실행: 2026-09-07 KST, `F:/dev/canview`
- `027b22e` 이후 production C·schema·생성 ABI 변경은 없다. `3c6967a`는 header 계약 주석과 시험, `4a0b5fa`는 리뷰 기록만 바꾼다.

## Host

| 검증 | 결과 |
|---|---|
| Windows Clang23.1.0 Debug | CTest 68/68, 191.64초, exit0 |
| Windows Clang23.1.0 Release | CTest 68/68, 193.18초, exit0 |
| WSL Ubuntu26.04 Clang21.1.8 ASan+UBSan Release | CTest 68/68, 135.55초, exit0 |
| UART schema | 6/6, navigation1.1과 runtime1.0 분리 포함 |
| core coverage | app line/function/branch100%, wire line/function100%·branch99.63% |
| UART C coverage | function74/74(100%), line1344/1473(91.24%), branch807/1020(79.12%) |
| API 문서 | Doxygen1.18.0·Sphinx9.1.0 strict build PASS |

실행 명령:

```powershell
. .\tools\environment\foundation-windows.ps1 -IncludeDocs
cmake --preset host-debug
cmake --build --preset host-debug --parallel 4
ctest --preset host-debug --parallel 4
cmake --preset host-release -B build/host-release-final
cmake --build build/host-release-final --parallel 4
ctest --test-dir build/host-release-final --output-on-failure --parallel 4
cmake --preset host-coverage
cmake --build --preset host-coverage --parallel 4
python -B -X utf8 tools/check_coverage.py --build build/host-coverage --llvm-bin F:/dev/canview/.tools/llvm-23.1.0/clang+llvm-23.1.0-x86_64-pc-windows-msvc/bin
python -B -X utf8 tools/build_docs.py
```

ASan+UBSan은 WSL의 `/home/digitie/canview-validation/t004-sanitize`에 `-DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release -DCANVIEW_SANITIZE=ON`으로 구성하고 전체 CTest를 실행했다. 임시 `/tmp` build 경로가 사라진 한 번의 재빌드 시도는 실패로 기록하고 위 지속 경로에서 다시 configure/build/test했다. Windows에서 sanitizer를 실행했다고 집계하지 않는다.

UART coverage는 `LLVM_PROFILE_FILE=build/host-coverage/t004-final-uart.profraw`로 `canview-uart-tests.exe all`을 실행한 뒤 `llvm-profdata merge -sparse`와 `llvm-cov export -summary-only`로 계산했다. PowerShell에서 `-instr-profile=...profdata`를 따옴표 없이 전달한 첫 export는 인수 분리로 실패했고, 전체 인수를 인용해 재실행했다. Coverage 분모는 UART production C이며 target/ISR/HIL coverage를 의미하지 않는다.

## Target

고정 SDK: Arm GNU15.3.Rel1/GCC15.3.1, STM32CubeG4 1.6.3, ESP-IDF6.0.3, CMake4.4.3, Ninja1.13.2. foundation과 `setup-windows.ps1 -VerifyOnly -ToolRoot C:\cv -ArmGnuRoot C:\Users\digit\AppData\Local\CANView\toolchains\arm-gnu-toolchain-15.3.rel1 -ArmGnuArchive C:\Users\digit\AppData\Local\CANView\toolchains\downloads\arm-gnu-toolchain-15.3.rel1-mingw-w64-x86_64-arm-none-eabi.zip`을 dot-source해 버전·digest를 검증했다.

STM32는 debug/release 각각 configure와 `cmake --build --preset <preset> --clean-first --parallel 4`를 수행했다. ESP32 네 프로젝트는 [이전 clean build](2026-09-07-T-004-target-final.md) 위에서 `idf.py -C <project> build`로 마지막 header 주석 변경을 재컴파일·link했다. 모든 단계 exit0, `(?i)\bwarning\b|CMake Error|ninja: error|fatal error` 진단 일치0이다. 로컬 로그·manifest는 `build/t004-final-target/`에 남긴다.

빌드 중 리뷰 문서만 수정·커밋했으므로 ESP app description의 version은 Communicator `3c6967a`, Bridge/Controller `3c6967a-dirty`, fixture `4a0b5fa`다. 이 `dirty`는 리뷰 Markdown 변경이며 firmware/C/schema delta는 없다. 아래 값은 실제 생성된 파일의 hash이며 동일 timestamp/version의 재현 가능한 단일 bundle hash라고 주장하지 않는다.

| 파일 | byte | SHA-256 |
|---|---:|---|
| `firmware/communicator/stm32/build/debug/canview-communicator-stm32.bin` | 1348 | `b6088a39c3cc393be09c10c945b1e2739873965166c72dbee063f12fc99691cc` |
| `firmware/communicator/stm32/build/release/canview-communicator-stm32.bin` | 1244 | `f6e264574b470e6bba48aa3bbfd411fc808c3f59cf815a1ec45750faae281c4b` |
| `firmware/communicator/esp32/build/canview_communicator_esp32.bin` | 160048 | `205605544a30a2750c25cfd306f4fd634372235c363205c8db576325f0e356d4` |
| `firmware/diagnostic-bridge/build/canview_diagnostic_bridge.bin` | 155648 | `10561ed34ede88fdf288476816c3692a415886d03fd91c750bf2a10871f4e91e` |
| `firmware/controller/build/canview_controller.bin` | 159712 | `64175d6f6e17150205da65534b0d8dc20685d597291c9bd4735de74cb2ee1d32` |
| `tests/fixtures/idf-public-component/build/canview_public_component_fixture.bin` | 144816 | `8cc49d1383d0aadea57c359d50b63f7a77c11bb657110d2d55ea7c9b7ea69b94` |

## 미실행

보드 flash/boot, 실제 UART waveform·RTS/CTS·DMA/ISR, reset/brownout, CAN/RF/HIL, production provisioning, 차량 송신은 NOT_RUN이다. 코드·compile 검증은 이 gate를 대체하지 않는다. 장기 full-rate 바이트 시험 결과는 별도 soak evidence로 기록한다.
