# T-002 target build evidence

- 실행 시각: 2026-09-06 KST
- 기준 commit: `cdaab07`
- SDK: ESP-IDF `6.0.3`, STM32CubeG4 `1.6.3`, Arm GNU Toolchain `15.3.Rel1`
- 검증 범위: STM32G474 debug/release, ESP32-S3 Communicator/Diagnostic Bridge/Controller와 공개 IDF fixture
- 판정: 네 프로젝트 모두 exit code 0; compiler/linker `warning`·`error` 출력 없음

## 실행

```powershell
. .\tools\environment\foundation-windows.ps1
. .\tools\environment\setup-windows.ps1 -ToolRoot C:\cv `
  -ArmGnuRoot C:\Users\digit\AppData\Local\CANView\toolchains\arm-gnu-toolchain-15.3.rel1 `
  -ArmGnuArchive C:\Users\digit\AppData\Local\CANView\toolchains\downloads\arm-gnu-toolchain-15.3.rel1-mingw-w64-x86_64-arm-none-eabi.zip `
  -VerifyOnly
cmake --preset debug; cmake --build --preset debug --clean-first
cmake --preset release; cmake --build --preset release --clean-first

idf.py -C firmware/communicator/esp32 fullclean
idf.py -C firmware/communicator/esp32 set-target esp32s3
idf.py -C firmware/communicator/esp32 build
idf.py -C firmware/diagnostic-bridge fullclean
idf.py -C firmware/diagnostic-bridge set-target esp32s3
idf.py -C firmware/diagnostic-bridge build
idf.py -C firmware/controller fullclean
idf.py -C firmware/controller set-target esp32s3
idf.py -C firmware/controller build
idf.py -C tests/fixtures/idf-public-component fullclean
idf.py -C tests/fixtures/idf-public-component set-target esp32s3
idf.py -C tests/fixtures/idf-public-component build
```

## 결과

| target | configuration | artifact | bytes |
|---|---|---|---:|
| Communicator STM32 | debug | `firmware/communicator/stm32/build/debug/canview-communicator-stm32.bin` | 1,348 |
| Communicator STM32 | release | `firmware/communicator/stm32/build/release/canview-communicator-stm32.bin` | 1,244 |
| Communicator ESP32-S3 | default | `firmware/communicator/esp32/build/canview_communicator_esp32.bin` | 160,048 |
| Diagnostic Bridge ESP32-S3 | default | `firmware/diagnostic-bridge/build/canview_diagnostic_bridge.bin` | 155,648 |
| Controller ESP32-S3 | default | `firmware/controller/build/canview_controller.bin` | 159,712 |
| public IDF fixture ESP32-S3 | default | `tests/fixtures/idf-public-component/build/canview_public_component_fixture.bin` | 144,816 |

ESP-IDF의 기존 `NVS_ENCRYPTION` unknown-symbol `NOTE`와 interrupt watchdog 기본값 `HINT`는 compiler/linker warning이 아니며 이번 변경으로 발생한 것이 아니다. 실제 board flash, RF, CAN/HIL, 차량 gate는 장비가 없어 `NOT_RUN`이다.
