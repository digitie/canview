# T-003 target build evidence

- 실행 시각: 2026-09-07 KST
- 기준 commit: `6a076a3b0bcbd3b3615969f6f956efbf05c8c54c`
- SDK: ESP-IDF `6.0.3`, STM32CubeG4 `1.6.3`, Arm GNU Toolchain `15.3.Rel1`
- host tools: CMake `4.4.3`, Ninja `1.13.2`
- SDK root: `C:\cv`; Arm GNU root: `C:\Users\digit\AppData\Local\CANView\toolchains\arm-gnu-toolchain-15.3.rel1`
- 판정: 아래 clean target build 모두 exit code 0; `warning:`, `error:`, `CMake Error`, `ninja: error` scan 0건

## 실행

```powershell
. ..\..\..\tools\environment\setup-windows.ps1 -VerifyOnly -ToolRoot C:\cv `
  -ArmGnuRoot C:\Users\digit\AppData\Local\CANView\toolchains\arm-gnu-toolchain-15.3.rel1 `
  -ArmGnuArchive C:\Users\digit\AppData\Local\CANView\toolchains\downloads\arm-gnu-toolchain-15.3.rel1-mingw-w64-x86_64-arm-none-eabi.zip
cmake --preset debug
cmake --build --preset debug --clean-first --parallel 4
cmake --preset release
cmake --build --preset release --clean-first --parallel 4
```

STM32 명령은 `firmware/communicator/stm32`에서 실행했다. ESP32 명령은 repository root에서 각 project에 대해 다음 순서를 실행했다.

```powershell
idf.py -C <project> fullclean
idf.py -C <project> set-target esp32s3
idf.py -C <project> build
```

`<project>`는 `firmware/communicator/esp32`, `firmware/diagnostic-bridge`, `firmware/controller`, `tests/fixtures/idf-public-component`다. target 실행 전 `setup-windows.ps1 -VerifyOnly`로 SDK와 toolchain을 재검증했다.

## 결과

| target | configuration | artifact | bytes | SHA-256 |
|---|---|---|---:|---|
| Communicator STM32 | debug | `firmware/communicator/stm32/build/debug/canview-communicator-stm32.bin` | 1,348 | `b6088a39c3cc393be09c10c945b1e2739873965166c72dbee063f12fc99691cc` |
| Communicator STM32 | release | `firmware/communicator/stm32/build/release/canview-communicator-stm32.bin` | 1,244 | `f6e264574b470e6bba48aa3bbfd411fc808c3f59cf815a1ec45750faae281c4b` |
| Communicator ESP32-S3 | default | `firmware/communicator/esp32/build/canview_communicator_esp32.bin` | 160,048 | `a668a67d898b9d821613f9fd00be9e5a651e280a62fd74bf77ee63fc54b8c0a4` |
| Diagnostic Bridge ESP32-S3 | default | `firmware/diagnostic-bridge/build/canview_diagnostic_bridge.bin` | 155,648 | `f4c5b7f44ed53c5ca132b1dffc7fc86832de6f70713bfcf14fa7d91a7a892ff5` |
| Controller ESP32-S3 | default | `firmware/controller/build/canview_controller.bin` | 159,712 | `66569366631e3b5c3e81f02de04611c7a7bc83c32d3f4b27a839379f513b160e` |
| public IDF fixture ESP32-S3 | default | `tests/fixtures/idf-public-component/build/canview_public_component_fixture.bin` | 144,816 | `cecc171ede6d2f0f355ee12565853bb9f306cff05dd70cfb3586a6a1183883e2` |

ESP-IDF가 출력하는 기존 `NOTE`/`HINT`는 compiler/linker warning이 아니며 strict scan의 warning/error 결과에 포함되지 않았다. 실제 board flash, reset/brownout fault injection, RF, CAN/HIL, 차량, production OTA signing/provisioning은 장비·승인 범위 밖이므로 `NOT_RUN`이다.
