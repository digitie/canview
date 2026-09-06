# T-004 target build evidence

- 기준 commit: `0b61d33` (`test: expand UART boundary and state coverage`)
- 실행일: 2026-09-07 KST
- checkout: `F:/dev/canview`
- working tree: target build 전후 `git status --porcelain=v1` 출력 없음
- SDK/toolchain: Arm GNU `15.3.Rel1`/GCC `15.3.1`, ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`, CMake `4.4.3`, Ninja `1.13.2`
- SDK 경로: `C:\cv\esp-idf-6.0.3`, `C:\cv\STM32CubeG4-1.6.3`

## 실행한 검증

```powershell
python -B -X utf8 tools/generate_protocol.py --schema protocol/schema/uart-v1.0.yaml --check
python -B -X utf8 tools/check_generated.py
cmake --build --preset host-debug --parallel 4
ctest --preset host-debug --output-on-failure
cmake --build --preset host-release --parallel 4
ctest --preset host-release --output-on-failure
wsl -d Ubuntu-26.04 -- bash -lc "cd /mnt/f/dev/canview && cmake -S . -B build/host-sanitize-wsl -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DCANVIEW_SANITIZE=ON && cmake --build build/host-sanitize-wsl --parallel 4 && ctest --test-dir build/host-sanitize-wsl --output-on-failure"
python tests/protocol/uart_fault_stream.py --seed 1 --duration-seconds 3600
```

- Windows Host Debug: CTest `62/62 PASS`
- Windows Host Release: CTest `62/62 PASS`
- WSL Clang ASan+UBSan: CTest `62/62 PASS`, 총 29.67초
- UART fault stream: virtual 3600초, 72,000 frame, seed 1, `PASS`
- 생성기와 generated output check: `PASS`
- UART coverage profile: `shared/protocol/src/canview_uart.c` 함수 `40/40` (100%), line `98.26%`, branch `90.46%`; 공용 `canview_wire.c` 함수/line `100%`, branch `99.63%`. 도달 불가능한 공개 API 방어 분기는 분모에 남아 있다.

## STM32 image

`firmware/communicator/stm32`에서 `cmake --preset debug|release`와 `cmake --build --preset debug|release --parallel 4`를 실행했다. 빌드 출력의 compiler/linker warning/error scan은 0건이다.

| image | 크기 | SHA-256 |
|---|---:|---|
| Debug `.bin` | 1,348 B | `b6088a39c3cc393be09c10c945b1e2739873965166c72dbee063f12fc99691cc` |
| Release `.bin` | 1,244 B | `f6e264574b470e6bba48aa3bbfd411fc808c3f59cf815a1ec45750faae281c4b` |

## ESP32-S3 image

ESP-IDF `idf.py build`를 Communicator, Diagnostic Bridge, Controller와 public component fixture에 각각 실행했다. 네 이미지 모두 exit code 0, binary 생성, compiler/linker warning scan 0건이다. ESP-IDF configure 단계의 기존 Kconfig `NOTE`는 compiler/linker warning이 아니며, source warning 판정에 포함하지 않았다.

| image | 크기 | SHA-256 |
|---|---:|---|
| Communicator `canview_communicator_esp32.bin` | 160,048 B | `414dd3b1e2f66617c0ee36c6ad869aa9ab94f91bb7b414a3f6642a9a5cd9d030` |
| Diagnostic Bridge `canview_diagnostic_bridge.bin` | 155,648 B | `d943ae3f91acd182242ea1f8d18252534b13ea42757c2adeab41d353f6a24ce0` |
| Controller `canview_controller.bin` | 159,712 B | `be1faa31d74314c634fcacb6606477d7bf97bf3ddc82d35f68a8904e5866479e` |
| public component fixture `canview_public_component_fixture.bin` | 144,816 B | `dd97c1a6bc405d0c873c909cc868699a2f52a7765d5813084b7ab4afb1185d4d` |

## 미실행 gate

실제 보드 flash/boot, UART 4 Mbps electrical waveform·RTS/CTS, DMA/ISR timing, PSRAM/clock, reset/brownout·전원 단전 fault injection, CAN/HIL, RF/CCMP runtime, production OTA signing/provisioning과 차량 bus 송신은 이 host/compile 작업의 범위 밖이며 `NOT_RUN`이다. 이 evidence는 target binary compile을 의미할 뿐 보드·차량 안전 승인을 의미하지 않는다.
