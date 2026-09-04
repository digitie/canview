# T-300 Controller Waveshare BSP, RTC와 LVGL bootstrap

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G1/G2`
- 선행: `T-001`, `T-005`
- 병렬 가능: `T-102`, `T-200`, `T-400`

## 목표

Waveshare `ESP32-S3-Touch-LCD-3.5`에서 build/flash 가능한 ESP-IDF 5.5.2 app을 만들고 기존 LVGL 화면을 실제 display/touch/RTC/audio BSP 위에 올린다.

## 고정 target

- board ESP32-S3R8, 16 MB Flash, 8 MB PSRAM
- ST7796 320×480, FT6336 touch, AXP2101 PMIC
- QMI8658, PCF85063 RTC, ES8311/onboard microphone
- LVGL 8.4.x와 Waveshare 공식 ESP-IDF 5.5.2 baseline

## 구현 범위

- top-level IDF project, board component와 pinned Waveshare dependency
- separate `sdkconfig.defaults.controller`, partition table
- PMIC/display/touch/shared I²C/RTC/audio smoke driver
- single LVGL task, tick/timer, double-buffer display flush
- UI model mailbox skeleton과 command callback queue
- boot/reset/build metadata와 memory counters
- host mock BSP interface
- public `canview_protocol` IDF component dependency와 component include 경계 수정

## 수용 기준

- [ ] clean IDF 5.5.2에서 build/flash된다.
- [ ] 16 MB/8 MB 설정이 Communicator config와 섞이지 않는다.
- [ ] full-screen color, rotation, tearing, touch edge/coordinate test를 통과한다.
- [ ] touch·RTC·PMIC·IMU shared I²C가 24시간 충돌하지 않는다.
- [ ] LVGL API 호출 thread assertion이 동작한다.
- [ ] UI+ESP-NOW mock+audio DMA에서 internal/PSRAM budget을 만족한다.
- [ ] RTC oscillator-stop/invalid 상태가 quality로 보고된다.
- [ ] `canview_controller_can.h` consumer가 private include path 없이 build된다.

## 검증

```bash
cd firmware/controller
idf.py set-target esp32s3
idf.py build
idf.py size-components
python ../../tests/hil/controller_bsp_smoke.py
```

## 증거

보드 revision, official example commit, sdkconfig diff, LCD/touch 사진·영상, I²C error count, heap/PSRAM report를 남긴다.
