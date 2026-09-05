# Controller firmware

Waveshare `ESP32-S3-Touch-LCD-3.5`용 ESP-IDF application이다. 현재는 IDF build와 public protocol/component 경계를 검증하는 안전한 bootstrap 단계다.

- 기준 ESP-IDF: `v6.0.3` (`esp32s3`)
- Flash/PSRAM: 16 MB / 8 MB Octal PSRAM
- UI: [`../../ui/lvgl/`](../../ui/lvgl/)
- hardware/pinmap: [Controller hardware](../../docs/hardware/controller.md)
- 개발환경: [장치별 toolchain](../../docs/development/toolchains.md)
- ESP-NOW: [ESP-NOW protocol](../../docs/architecture/protocols/esp-now.md)
- CAN 수신·DBC 파이프라인: [Controller CAN pipeline](../../docs/architecture/controller-can-pipeline.md)

Controller 펌웨어가 Controller 로컬 CAN 수신 필터와 DBC signal catalog/decoder를 소유한다. Communicator는 raw CAN record만 보내므로 새 signal이나 차량 profile은 필요할 때 Controller catalog와 allow-list만 바꿔 추가할 수 있으며 Communicator firmware는 바꾸지 않는다.

Primary Controller는 read-only 장치가 아니며, 활성 차량 profile에서 검증된 audio·SPORT 기능별 의도 명령을 요청할 수 있다. 주행 소음 기반 volume offset, 취침·뒷좌석 강화 profile의 fader/balance·main/rear mute, OEM snapshot 복원과 SPORT 자동화가 대상이다. Controller는 임의 CAN ID/payload를 만들지 않고 Communicator STM32가 control lease, capability, allow-list, 최신 차량 상태와 feedback을 최종 검사한다.

온보드 ES8311 microphone의 16 kHz mono capture에서 1024-point FFT를 계산해 23개 표시 bin과 peak 주파수·레벨만 LVGL 모델에 전달한다. raw PCM은 UI task나 ESP-NOW로 전달하지 않는다.

`components/canview_automation/`은 ESP-IDF에 바로 포함할 수 있는 순수 C component다.

- 차량 tail/low-beam·rheostat CAN 상태로만 backlight 목표를 만들고 1.2초 ramp를 적용한다.
- FFT focus-band excess가 설정된 시간 동안 유지될 때 음량 offset을 한 step씩 올리고, release가 유지되면 더 느리게 내린다.
- `표준/보통/자연스럽게` preset은 160–1,250 Hz, +5.0/+2.5 dB 문턱, 5초/12초 dwell로 매핑한다.
- 실제 LEDC write와 ESP-NOW command enqueue는 component 호출자가 담당한다.

통합 수치와 실패 처리는 [자동 제어 로직](../../docs/architecture/automation.md)을 따른다.

## Windows build

저장소 루트에서 ESP-IDF와 공통 toolchain을 먼저 준비한다.

```powershell
. .\tools\environment\setup-windows.ps1
idf.py -C firmware/controller set-target esp32s3
idf.py -C firmware/controller build
idf.py -C firmware/controller size-components
```

`set-target`이 생성하는 `sdkconfig`와 `build/`는 로컬 산출물이며 Git에 커밋하지 않는다. `sdkconfig.defaults`와 `partitions.csv`가 이 프로젝트의 기본 target 설정이며 NVS encryption key partition도 예약한다. 현재 bootstrap은 단일 factory image다. OTA A/B는 실제 UI image 크기와 secure provisioning을 측정한 뒤 별도 partition 설계로 추가한다.

현재 `app_main()`은 filter store 초기화와 protocol version log만 수행한다. Waveshare BSP, LVGL task, RTC/audio, UART/ESP-NOW transport와 실제 보드 bring-up은 후속 task에서 추가하며 그 전에는 양산 firmware로 사용하지 않는다.
