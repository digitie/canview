# T-303 Controller I2S microphone, FFT와 noise feature pipeline

- 상태: `BLOCKED`
- 우선순위: `P1`
- Gate: `G3`
- 선행: `T-300`, `T-302`
- 병렬 가능: `T-304`

## 목표

온보드 microphone으로 cabin FFT와 주행소음 특징을 안정적으로 만들고 음악·송풍·clipping을 road noise로 오인하지 않도록 품질과 confidence를 제공한다.

## 구현 범위

- Waveshare audio path/I2S configuration과 DMA ping-pong ring
- DC removal, window, FFT, smoothing, display 23-bin reducer
- peak frequency, peak dBFS, broadband level dBFS
- road/balanced/wind band energy와 baseline excess
- clipping, silence, underrun, sample-rate drift quality
- source volume/mute/manual change freeze input
- calibration record와 optional external microphone abstraction
- FFT task→model/automation queue
- 각 feature의 sample timestamp, age, quality, calibration generation과 source-volume revision

## 고정 규칙

- calibration 전에는 dB SPL이라고 표시하지 않는다.
- audio callback/DMA ISR에서 FFT/LVGL/NVS write를 하지 않는다.
- onboard speaker leakage와 music bass를 완전히 분리할 수 있다고 가정하지 않는다.
- clipping/underrun/invalid FFT는 volume automation evidence를 즉시 초기화한다.

## 수용 기준

- [ ] synthetic sine/noise golden input의 peak/bin/level 오차가 정한 허용값 안이다.
- [ ] negative dBFS를 signed 값으로 보존한다.
- [ ] DMA overrun, clipping, silence가 서로 다른 quality가 된다.
- [ ] FFT compute p99가 frame period의 50% 이하이고 UI frame budget을 침범하지 않는다.
- [ ] 음악/mute/blower/road capture별 false-positive report가 있다.
- [ ] external microphone 미연결 상태에서 onboard path가 정상이고 같은 GPIO 병렬 연결을 요구하지 않는다.
- [ ] 250 ms를 넘긴 feature가 valid automation evidence로 전달되지 않고 scheduler gap에서 dwell이 초기화된다.

## 검증

```bash
ctest --preset host-sanitize -R 'fft|noise-feature' --output-on-failure
python tests/audio/run_golden_wav.py tests/fixtures/audio
python tests/hil/controller_audio_soak.py --hours 8
```

## evidence

입력 WAV digest, expected/actual spectrum, CPU/heap/DMA drop, calibration 상태를 저장한다. 저작권 있는 음악 원본은 repo에 넣지 않는다.
