# canview

현대·기아·제네시스 차량의 CAN 데이터를 별도 **Communicator**에서 수집하고, `ESP-NOW`로 Waveshare `ESP32-S3-Touch-LCD-3.5` 기반 **Controller**에 전달해 시각화하는 프로젝트다. Controller는 최대 3개의 CAN 버스에서 들어오는 데이터를 표시하고, 필요한 경우 안전하게 제한된 의도 명령을 Communicator에 전달한다.

현재 1차 대상은 다음 차량이다.

- 2017년식 Hyundai Tucson TL
- 2.0 디젤
- 4WD
- BlueLink 사양

차량별 CAN 정의는 `commaai/opendbc`의 공개 DBC를 기준으로 시작한다. upstream에 2017년식 Tucson 전용 항목이 확인되지 않으므로, 현재 저장된 DBC는 실차 캡처로 검증해야 하는 후보 정의다.

## 문서

- [하드웨어 및 개발환경](docs/hardware-and-development.md)
- [시스템 구조와 장치 명칭](docs/system-architecture.md)
- [Communicator 회로·IC·핀맵](docs/communicator-hardware.md)
- [장치별 개발환경](docs/development-environments.md)
- [Communicator 내부 4 Mbps UART 프로토콜](docs/communicator-uart-protocol.md)
- [DBC 파일과 차량 적용 지침](dbc/README.md)
- [1차 대상 차량 검증 메모](docs/target-vehicle-2017-tucson.md)
- [4WD·DPF·오디오·SPORT 기능 설계](docs/feature-design.md)
- [자동 밝기·소음 음량·SPORT 제어 로직](docs/automation-control.md)
- [ESP-NOW 양방향 프로토콜](docs/esp-now-protocol.md)
- [운전자 UI·LVGL 설계](docs/ui-design.md)

## UI prototype

320×480 세로 화면에 맞춘 주행·소리·FFT·자동화·설정 UI를 HTML prototype과 LVGL 8.4 코드로 제공한다. 주행 화면의 네 바퀴 게이지에는 구동 지수와 TPMS 공기압을 함께 표현한다. 아래 값은 레이아웃 검토용 데모이며 실차 측정값이 아니다.

| 주행 상태 | 소리 제어 | FFT 분석 |
|---|---|---|
| ![주행 화면](docs/images/ui-drive.png) | ![소리 화면](docs/images/ui-audio.png) | ![FFT 화면](docs/images/ui-fft.png) |

| SPORT 자동화 | 설정 | 자동화 상세 설정 |
|---|---|---|
| ![자동화 화면](docs/images/ui-automation.png) | ![설정 화면](docs/images/ui-settings.png) | ![소음·SPORT 설정 화면](docs/images/ui-settings-automation.png) |

- 브라우저 prototype: [`ui/prototype/`](ui/prototype/)
- LVGL 8.4 UI: [`ui/lvgl/`](ui/lvgl/)
- wire protocol C 구조: [`protocol/canview_protocol.h`](protocol/canview_protocol.h)

## 저장된 DBC

원본은 `dbc/opendbc/`에 upstream 파일을 수정하지 않고 저장했다.

- `hyundai_can.dbc`: 현재 Hyundai 공통 CAN generator source. 1차 대상의 주 후보
- `hyundai_2015_ccan.dbc`: 구형 Classic CAN 참고 정의
- `hyundai_2015_mcan.dbc`: 구형 Multimedia CAN 참고 정의
- `LICENSE`: DBC 원본 저장에 필요한 upstream MIT License

파일의 upstream commit, SHA-256, 적용 범위는 [dbc/README.md](dbc/README.md)에 고정해 두었다.

## 저장소 상태

DBC 원본, hardware 기준, 기능 안전 경계, ESP-NOW wire 구조, 화면 prototype과 LVGL UI 계층을 포함한 설계 기준 저장소다. 실제 Communicator와 Controller firmware 통합은 실차 capture로 신호를 검증하면서 단계적으로 추가한다.

## 주요 원문

- [Waveshare ESP32-S3-Touch-LCD-3.5 제품 문서](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5)
- [Waveshare 리소스 및 회로도](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/Resources-And-Documents)
- [Waveshare 예제 저장소](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/)
- [commaai/opendbc](https://github.com/commaai/opendbc)
