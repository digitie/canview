# T-200a 작성자 검증 evidence

## 1. 검증 범위와 기준

구현 candidate `55c7801`, strict API/CMake 보완 `1d85de5`와 후속 config 수정의 검증을 구별한다. C runtime은 `55c7801` 이후 변경하지 않았다. 후속 설정 gate 수정·독립 리뷰·최종 CI가 닫히기 전 DONE으로 보지 않는다.

- Windows native Clang23.1.0/CMake4.4.3/Ninja1.13.2
- Arm GNU15.3.Rel1, STM32CubeG4 1.6.3 `d11b194a9f05d1b143d154771f3dbc282c8052a5`
- ESP-IDF6.0.3 `76f5dedd9950a3012fee8fb7d5586df21fc67802`
- 보조 WSL Ubuntu26.04 GCC15.2/Clang21.1.8

## 2. Host/API

| 검사 | 실행 결과 | 로컬 로그 |
|---|---|---|
| Windows Debug 전체 | 89/89, 245.32초 | `build/t200a-debug-89.log` |
| Windows Release 전체 | 89/89, 233.38초 | `build/t200a-release-89.log` |
| GCC 새 ESP 범위 | 15/15; 기존 전체80/80 후 app8개 추가 | WSL `/tmp/t200a-gcc-current.log` |
| Clang ASan+UBSan 새 ESP 범위 | 15/15; 기존 전체81/81 후 app8개 추가 | WSL `/tmp/t200a-sanitize-current.log` |
| ThreadSanitizer 실제 pthread pool | 4 threads·8000 payload PASS | WSL `t200a-pool-tsan` |
| strict API | 29개 계약, warning0 | `build/t200a-docs-fixed.log` |
| config 수정 후 | 9개 시험 PASS, 독립 금지21개 삭제 변이 모두 검출 | `tests/foundation/test_sdkconfig.py` |
| 문서 fence parser | 4개 시험 PASS | `tests/test_document_links.py` |

SDK adapter host fixture는 실제 adapter C를 실행하지만 실제 FreeRTOS scheduling/PSRAM/flash/watchdog 동작의 대체 evidence가 아니다. pool의 thread test와 TSAN은 host 동시성 검사다.

Coverage `build/host-coverage/esp32-coverage-g2j6iceg`, `build/t200a-coverage-current.log`:

| 파일 | line/function | branch |
|---|---|---|
| health.c | 100% / 100% | 111/112, 99.107% |
| pool.c | 100% / 100% | 91/92, 98.913% |
| platform runtime.c | 176/176, 11/11 | 84/85, 98.824% |
| BSP runtime.c | 100% / 100% | 분기 없음 |
| app main.c | 36/36, 1/1 | 12/12, 100% |

## 3. 수정 전 clean target snapshot

`1d85de5d5a10103798c09ffc1bb10946b0a98fbf` clean 시작/종료, STM32 Debug/Release와 ESP32 네 프로젝트 모두 warning0, BIN/ELF/MAP·bootloader·partition 26개 파일을 크기/digest로 검증했다. manifest는 `build/t200a-final-1d85de5/artifacts.json`이다. config 후속 수정 전 snapshot이므로 최종본 gate를 대신하지 않는다.

| BIN | bytes | SHA-256 |
|---|---:|---|
| STM32 Debug | 4300 | `18592ca77f5d374e8c3a29dac7d35d7c7352ab707569d63cc4f7cf787fecdc95` |
| STM32 Release | 3800 | `17a6673aa65629ca51e873f012de080a5adb36c73a063c09540fbdd0053f87a6` |
| Communicator | 164944 | `cd656a14f676fc5cf2d4a6fbf4879eef977ce4c8f7a9bccbf7938dfb489069ef` |
| Bridge | 155648 | `31d7d810c346b0115bfa0d64aeec52660135adc857faba1050698450cc28b8a7` |
| Controller | 159712 | `b7dfeb1569956b2442e8c9b8d11c306407f356c59ddf8984623dca377c6dfe32` |
| ESP fixture | 144816 | `5294f6264dcec7216625004df90520c9cee69439d31647c81242aaff4a736cbc` |

## 4. Config 수정의 실제 SDK 부정 검사

정상 Communicator actual SDK build warning0 (`build/t200a-postfix-idf.log`). factory-reset+NVS 삭제 (`build/t200a-bad-factory.log`)와 HALT/PRINT_REBOOT 해제 (`build/t200a-bad-halt.log`)는 CMake actual SDKCONFIG 검사에서 예상대로 거부했다. 앞선 TWDT panic 해제도 거부했다. generated defaults만 검사한 것이 아니다.

개발 중 unquoted PowerShell `-DSDKCONFIG=F:/...`의 인자 해석 실패와 IDF export를 생략한 incremental build의 ccache 부재는 환경 호출 오류였다. 전체 인자 인용·IDF export 후 성공했으며 실패 실행은 PASS로 집계하지 않는다. 초기 CI의 tool 다운로드 실패도 최종 CI로 대체 확인해야 한다.

## 5. 남은 gate

독립 post-fix review와 최종 head CI/6종 target 증거는 후속 closure/merge evidence에 기록한다. 실제 R1 board·ST-LINK/계측기는 식별되지 않아 flash/HIL, PSRAM/heap/stack 실측, GPIO/USB/reset·watchdog 파형은 NOT_RUN이다. T-200 부모 task와 G1/G2, 차량 TX NO-GO를 유지한다.
