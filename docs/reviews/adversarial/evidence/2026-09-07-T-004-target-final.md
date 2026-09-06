# T-004 target build evidence

- 기준 commit: `6dc22183ed80961abe222c29f29d9117a3dfff68` (`fix: bind UART replay and queue admission to session`)
- 실행일: 2026-09-07 KST
- checkout: `F:/dev/canview`
- SDK/toolchain: Arm GNU `15.3.Rel1`/GCC `15.3.1`, ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`, CMake `4.4.3`, Ninja `1.13.2`
- SDK 경로: `C:\cv\esp-idf-6.0.3`, `C:\cv\STM32CubeG4-1.6.3`
- 검증: `setup-windows.ps1 -VerifyOnly`로 manifest·SDK commit·도구 버전을 확인한 뒤 clean target build를 수행했다.

## 실행한 검증

```powershell
. .\tools\environment\foundation-windows.ps1
. .\tools\environment\setup-windows.ps1 -VerifyOnly -ToolRoot C:\cv `
  -ArmGnuRoot C:\Users\digit\AppData\Local\CANView\toolchains\arm-gnu-toolchain-15.3.rel1 `
  -ArmGnuArchive C:\Users\digit\AppData\Local\CANView\toolchains\downloads\arm-gnu-toolchain-15.3.rel1-mingw-w64-x86_64-arm-none-eabi.zip
cmake --preset debug; cmake --build --preset debug --clean-first
cmake --preset release; cmake --build --preset release --clean-first
idf.py -C firmware/communicator/esp32 fullclean; idf.py -C firmware/communicator/esp32 set-target esp32s3; idf.py -C firmware/communicator/esp32 build
idf.py -C firmware/diagnostic-bridge fullclean; idf.py -C firmware/diagnostic-bridge set-target esp32s3; idf.py -C firmware/diagnostic-bridge build
idf.py -C firmware/controller fullclean; idf.py -C firmware/controller set-target esp32s3; idf.py -C firmware/controller build
idf.py -C tests/fixtures/idf-public-component fullclean; idf.py -C tests/fixtures/idf-public-component set-target esp32s3; idf.py -C tests/fixtures/idf-public-component build
```

모든 명령이 exit code 0이었다. 각 configure/build 출력에 대해 `warning`, `CMake Error`, `ninja: error`, `fatal error`를 검사한 결과 모든 단계의 진단 일치 수는 0건이었다. ESP-IDF configure의 기존 Kconfig `NOTE`는 compiler/linker warning이 아니므로 별도 기록하며 warning으로 세지 않았다.

## STM32 image

| image | 크기 | SHA-256 |
|---|---:|---|
| Debug `.bin` | 1,348 B | `b6088a39c3cc393be09c10c945b1e2739873965166c72dbee063f12fc99691cc` |
| Release `.bin` | 1,244 B | `f6e264574b470e6bba48aa3bbfd411fc808c3f59cf815a1ec45750faae281c4b` |

Debug linker map과 compiler-generated `.su` 파일도 생성됐다. 실제 보드에 배치한 뒤의 stack watermark/WCET 측정은 아직 수행하지 않았다.

## ESP32-S3 image

| image | 크기 | SHA-256 |
|---|---:|---|
| Communicator `canview_communicator_esp32.bin` | 160,048 B | `f748f212af062e06bca6782af0eaf7782aa98855feeb1168d87271dc8eb07176` |
| Diagnostic Bridge `canview_diagnostic_bridge.bin` | 155,648 B | `16e6bf5f90406a5fa5124e94095ffb0eac3b9969ea2808ebef34ba6b02c899b8` |
| Controller `canview_controller.bin` | 159,712 B | `f449a5b2f687dac89ce993bc3dc3db9cd59ccaee3f1427e05c12cbd858100c1db` |
| public component fixture `canview_public_component_fixture.bin` | 144,816 B | `e6ecd12dfb2eb9bd4a58679bd1737986b770c09bb37c1444fcd77662b53c8ab7c` |

## 미실행 gate

실제 보드 flash/boot, UART 4 Mbps electrical waveform·RTS/CTS polarity와 stall recovery, DMA/ISR timing·ownership, PSRAM/clock, reset/brownout·전원 단전 fault injection, CAN/HIL, RF runtime, production OTA signing/provisioning과 차량 bus 송신은 이 host/compile 작업의 범위 밖이며 `NOT_RUN`이다. 이 evidence는 target binary compile과 warning-free 진단만 의미하며 보드·차량 안전 승인을 의미하지 않는다.
