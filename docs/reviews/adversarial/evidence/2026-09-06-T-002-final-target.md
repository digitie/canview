# T-002 최종 target build evidence

- 실행 시각: 2026-09-06 KST
- 기준 commit: `fb30b29e4e0ac27170bce8bb989e0d93e9d861a` (`fix: close T-002 schema review blockers`)
- SDK: ESP-IDF `6.0.3`, STM32CubeG4 `1.6.3`, Arm GNU Toolchain `15.3.Rel1`
- 검증 범위: STM32G474 Debug/Release, ESP32-S3 Communicator/Diagnostic Bridge/Controller와 public IDF fixture
- 판정: 모든 configure/build exit code 0; compiler/linker `warning`·`error` scan 0건

## 실행

```powershell
. .\tools\environment\foundation-windows.ps1
. .\tools\environment\setup-windows.ps1 -ToolRoot C:\cv -ArmGnuRoot C:\Users\digit\AppData\Local\CANView\toolchains\arm-gnu-toolchain-15.3.rel1 -ArmGnuArchive C:\Users\digit\AppData\Local\CANView\toolchains\downloads\arm-gnu-toolchain-15.3.rel1-mingw-w64-x86_64-arm-none-eabi.zip -VerifyOnly

Push-Location firmware/communicator/stm32
cmake --preset debug
cmake --build --preset debug --clean-first
cmake --preset release
cmake --build --preset release --clean-first
Pop-Location

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

| target | configuration | artifact | bytes | SHA-256 |
|---|---|---|---:|---|
| Communicator STM32 | Debug | `firmware/communicator/stm32/build/debug/canview-communicator-stm32.bin` | 1,348 | `b6088a39c3cc393be09c10c945b1e2739873965166c72dbee063f12fc99691cc` |
| Communicator STM32 | Release | `firmware/communicator/stm32/build/release/canview-communicator-stm32.bin` | 1,244 | `f6e264574b470e6bba48aa3bbfd411fc808c3f59cf815a1ec45750faae281c4b` |
| Communicator ESP32-S3 | default | `firmware/communicator/esp32/build/canview_communicator_esp32.bin` | 160,048 | `8d69327c9b7b2a32fb5c9a4ad18404351b8d0df3726d1f3935d667f411f91cd4` |
| Diagnostic Bridge ESP32-S3 | default | `firmware/diagnostic-bridge/build/canview_diagnostic_bridge.bin` | 155,648 | `b607cad6a331eb51da90fb513a5979ded411ef2bb88116d85f073189a717793e` |
| Controller ESP32-S3 | default | `firmware/controller/build/canview_controller.bin` | 159,712 | `aad4cac8bf35bedeaf22e273d6c683ca27909597496d3819b9c40bed8b854281` |
| public IDF fixture ESP32-S3 | default | `tests/fixtures/idf-public-component/build/canview_public_component_fixture.bin` | 144,816 | `c91dbe7f5bbd7c24eed192779dbb892362f9e5fe061b8d8e550bd5a8cf2aeda5` |

ESP-IDF가 출력한 기존 Kconfig `NOTE`/`HINT`는 compiler/linker warning이 아니며 warning/error scan 대상에 포함되지 않았다. 실제 board flash, reset/brownout, RF, CAN/UART HIL, 차량 CAN TX와 production OTA signing은 `NOT_RUN`이다.

실행 log root: `C:\Users\digit\AppData\Local\Temp\canview-t002-fb30b29-target-20260906-233730`
