# T-002 최종 target build evidence

- 실행 시각: 2026-09-06 KST
- 기준 commit: `e1b0e8e` (`fix: close final ESP-NOW schema policy findings`)
- SDK: ESP-IDF `6.0.3`, STM32CubeG4 `1.6.3`, Arm GNU Toolchain `15.3.Rel1`
- 검증 범위: STM32G474 Debug/Release, ESP32-S3 Communicator/Diagnostic Bridge/Controller와 public IDF fixture
- 판정: 모든 configure/build exit code 0; compiler/linker `warning`·`error` scan 0건

## 실행

```powershell
. .\tools\environment\foundation-windows.ps1
. .\tools\environment\setup-windows.ps1 -ToolRoot C:\cv -VerifyOnly

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
| Communicator ESP32-S3 | default | `firmware/communicator/esp32/build/canview_communicator_esp32.bin` | 160,048 | `b707d56606cd66fe2eb52098fa60bed8cd7e817634f47e756904bbcd703e9a15` |
| Diagnostic Bridge ESP32-S3 | default | `firmware/diagnostic-bridge/build/canview_diagnostic_bridge.bin` | 155,648 | `02102d47c0ad1809c867e600e8ddf109990ec9efa4a3fad5f8284178db753e83` |
| Controller ESP32-S3 | default | `firmware/controller/build/canview_controller.bin` | 159,712 | `a3049885dd3813103278808e1fc80f0a1315993e857cf680288b1cb48247ecff` |
| public IDF fixture ESP32-S3 | default | `tests/fixtures/idf-public-component/build/canview_public_component_fixture.bin` | 144,816 | `fb9dde2a31fcae497caf5bd1b20dd9275d1a334df5be88517e50c6684d3be24c` |

ESP-IDF가 출력한 기존 Kconfig `NOTE`/`HINT`는 compiler/linker warning이 아니며 warning/error scan 대상에 포함되지 않았다. 실제 board flash, reset/brownout, RF, CAN/UART HIL, 차량 CAN TX와 production OTA signing은 `NOT_RUN`이다.
