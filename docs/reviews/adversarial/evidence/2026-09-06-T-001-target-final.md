# T-001 최종 target build evidence

- 실행일: 2026-09-06
- 기준 commit: `3b71e42a5a643bc474b1c2d7a272d291dcbfdf69`
- 환경: Windows PowerShell, CMake `4.4.3`, Ninja `1.13.2`, Arm GNU `15.3.Rel1`, ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`
- Arm archive SHA-256: `b85669d3408e2ae713b17b0cc59bc4ea26369a7f2bd19108fd11df7095f159e6`
- provenance: archive entry inventory·설치 root 전체 7,360개 파일·marker hash를 3자 대조

## 실행 순서

1. `foundation-windows.ps1`로 host 도구 활성화
2. `setup-windows.ps1 -VerifyOnly -ArmGnuRoot ... -ArmGnuArchive ...`
3. STM32 debug/release `cmake --preset` 및 `--clean-first` build
4. 세 ESP32 프로젝트와 public component fixture `idf.py fullclean → set-target esp32s3 → build`
5. 각 로그에서 `warning:`, `CMake Warning`, `ld.exe: warning`, `error:`를 검색하고 artifact non-empty·ESP-IDF metadata를 확인

## 최종 binary

| 대상 | bytes | SHA-256 |
|---|---:|---|
| STM32 debug | 1,348 | `b6088a39c3cc393be09c10c945b1e2739873965166c72dbee063f12fc99691cc` |
| STM32 release | 1,244 | `f6e264574b470e6bba48aa3bbfd411fc808c3f59cf815a1ec45750faae281c4b` |
| Communicator ESP32-S3 | 160,048 | `7d6dc03bc71126785404b44eb964ade687501c988bc23d91c686b37673da005a` |
| Diagnostic Bridge ESP32-S3 | 155,648 | `cd0cd97222ae4c5392a1d1476af7f0aebb6f6466f1207ba8b22d225c15595ae4` |
| Controller ESP32-S3 | 159,712 | `eba481fd5140bb2e3a8f062d0de1d1964490c6efe1b5f3110a0fcdb656e8ee27` |
| public component fixture | 144,816 | `147beaff502b046040e0dd29f559f25b3b98dacb6317e7f1e2e55515bb523622` |

## 결과

- STM32·ESP32·fixture clean build: PASS
- 18개 `.bin/.elf/.map` non-empty: PASS
- warning/error scan: 0건
- ESP-IDF `project_name`, `project_path`, `target=esp32s3`, `app_bin`: PASS
- 실제 board flash, reset/brownout, CAN/UART/RF, HIL, 차량 CAN TX: 미실행
