# Controller firmware

Waveshare `ESP32-S3-Touch-LCD-3.5`용 ESP-IDF project가 들어갈 위치다.

- 기준 ESP-IDF: `v5.5.2`
- UI: [`../../ui/lvgl/`](../../ui/lvgl/)
- hardware/pinmap: [`../../docs/hardware-and-development.md`](../../docs/hardware-and-development.md)
- 개발환경: [`../../docs/development-environments.md`](../../docs/development-environments.md)
- ESP-NOW: [`../../docs/esp-now-protocol.md`](../../docs/esp-now-protocol.md)

온보드 ES8311 microphone의 16 kHz mono capture에서 1024-point FFT를 계산해 23개 표시 bin과 peak 주파수·레벨만 LVGL 모델에 전달한다. raw PCM은 UI task나 ESP-NOW로 전달하지 않는다.

Waveshare BSP와 실제 보드 bring-up 설정을 고정하기 전에는 placeholder project를 양산 firmware로 사용하지 않는다.
