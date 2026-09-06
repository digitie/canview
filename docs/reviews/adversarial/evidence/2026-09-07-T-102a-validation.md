# T-102a 소프트웨어 검증 evidence

## 기준과 경계

target 실행 코드 기준은 `6d4757c663878b30de17f076f60b98942dcdd724`다. `5ee60bc4690b92fb90fa03e771984161ac3ad9fd`는 host register 시험의 NMI `setjmp`를 반복문 밖으로 옮긴 변경과 원문 리뷰 보존뿐이며 target 실행 코드·API·SDK 설정은 같다. 두 commit의 바이너리 embedded version이 같다고 주장하지 않는다. merge head의 target 재검증은 PR 최종 CI와 별도 산출물로 확인한다.

Windows native가 정본이다. Clang23.1.0, CMake4.4.3, Ninja1.13.2, Arm15.3.Rel1, ESP-IDF6.0.3, CubeG4 1.6.3을 사용했다. `foundation-windows.ps1`와 `setup-windows.ps1 -VerifyOnly`로 고정 설치를 검사했다. WSL Ubuntu26.04 Clang21.1.8/ASan+UBSan과 GCC15.2는 보조 검증이다.

## host와 API

`6d4757c`에서 시작한 전체 Windows Debug/Release CTest는 각각74/74 PASS, 247.31/251.97초였다. 같은 코드의 WSL ASan+UBSan도74/74 PASS, 198.64초였다. 로그는 로컬 `build/t102a-final-host-{debug,release}-{build,test}.log`, `build/t102a-final-docs.log`와 WSL `/home/digitie/canview-validation/t102a-sanitize/Testing/Temporary/LastTest.log`다. 후자의 CTest 임시 로그는 다음 재실행으로 교체될 수 있다.

```powershell
. .\tools\environment\foundation-windows.ps1 -IncludeDocs
cmake --build --preset host-debug
ctest --preset host-debug --parallel 8
cmake --build --preset host-release
ctest --preset host-release --parallel 8
cmake --build --preset host-coverage
python -B tools/check_stm32_coverage.py
python -B tools/build_docs.py
```

Doxygen public API20개 brief/parameter/return 계약과 strict Sphinx는 warning0으로 통과했다. `5ee60bc`의 변경 시험은 Windows Debug/Release 및 ASan+UBSan 각각 STM32 6/6 PASS, GCC15.2 `-O3 -DNDEBUG -std=c99`와 기존 전체 strict warning 옵션으로 register 실행 PASS였다. 경고 옵션을 완화하거나 `volatile`로 가리지 않았다. 최신 전체 회귀는 PR head에서 다시 실행한다.

| 파일 | line/function | branch |
|---|---:|---:|
| boot.c | 100%/100% | 22/22, 100% |
| queue.c | 100%/100% | 50/50, 100% |
| scheduler.c | 100%/100% | 109/110, 99.09% |
| core_hw.c host model | 100%/100% | 87/90, 96.67% |

최신 profile은 `build/host-coverage/stm32-coverage-1d0l47ok/{portable,register}/export.json`이다. model의 수치가 실제 peripheral 검증률은 아니다. 두 required-worker 변이(any-vote 허용, feed 뒤 vote clear 제거)는 모두 컴파일 성공 뒤 `test_core.c:356` assertion으로 검출했다. Reviewer B도 두 변이를 독립 재실행했다.

## clean target 파일

`6d4757c`의 시작·종료 HEAD와 clean을 확인하고 새로운 `build/t102a-6d4757c/`에서 STM32 Debug/Release와 ESP32 네 프로젝트를 생성했다. ESP32는 두 프로젝트씩 병렬 빌드했다. configure/build 종료 코드0, warning/error scan0, ELF/BIN/MAP 생성, ESP32 target `esp32s3`와 embedded project version `6d4757c`를 확인했다. 로그와 JSON manifest는 해당 디렉터리의 `logs/`, `artifacts.json`에 있다.

| 산출물 | BIN bytes | SHA-256 |
|---|---:|---|
| STM32 Debug | 4300 | `18592ca77f5d374e8c3a29dac7d35d7c7352ab707569d63cc4f7cf787fecdc95` |
| STM32 Release | 3800 | `17a6673aa65629ca51e873f012de080a5adb36c73a063c09540fbdd0053f87a6` |
| Communicator ESP32 | 160048 | `c5c77b80e4351fb584b8b07b32f22a3782c418ee03e96cff90921ba472671c22` |
| Diagnostic Bridge | 155648 | `4f50d1e26c46c453673462345fdbae06317dbee8585fdc14588c882aeb8abef1` |
| Controller | 159712 | `c30f0267a38f6ff7e70602e14bbde0ae5cabfb65d248fe6365928dc795559fbc` |
| IDF public component fixture | 144816 | `9cddd1391dc77e0da27eb3fb234c9e0785ede0fa1b3888258ab9b6baa0eda0a1` |

STM32 text/data는 Debug4296/4 B, Release3796/4 B다. bss와 예약 stack은 두 구성 모두8580 B이고 최대 개별 stack frame은928/992 B다. compile database의15개 C object stack evidence와47개 CMSIS/model 상수를 대조했다. heap·CAN TX 금지 symbol 부재, memory/linker budget도 통과했다. 개별 frame 검사로 전체 call-chain 또는 실측 stack watermark를 증명하지 않는다.

`CANVIEW_BUILD_MODE=VEHICLE_TX` configure와 target의 `CANVIEW_STM_REGISTER_TEST` 정의를 별도 부정 fixture에서 거부했다. logs `build/t102a-rejected-mode.log`, `build/t102a-rejected-host-model.log`다.

## 실패 이력과 미실행

- 최초 strict API16 warnings는 `e928cf6`에서 수정했다.
- `e928cf6` target 전체 재생성은 deadline/NMI finding 확인 후 중단했으며 부분 결과를 최종 evidence로 쓰지 않았다.
- `34064143948` Windows CI는 Doxygen 공식 archive 다운로드 실패로 코드 시험 전에 종료됐다. `6d4757c`의 다음 Windows CI는 통과했다.
- `34064871690` Linux GCC job은 register 시험의 반복 변수와 `longjmp`에 대한 `-Werror=clobbered`로 실패했다. 로컬 GCC15.2에서도 재현했고 `5ee60bc`에서 NMI 시험을 반복문 밖으로 분리했다. 이전 실패 run을 전체 PASS로 표시하지 않는다.
- 보드 flash, clock/IWDG/reset 파형, IRQ latency/WCET/watermark, brownout/PHY gate, UART/CAN/HIL, OTA/provisioning과 차량 TX는 NOT_RUN이다. host named-register fixture는 MCU simulator나 물리 계측을 대신하지 않는다. T-102의 G1/G2는 계속 BLOCKED다.
