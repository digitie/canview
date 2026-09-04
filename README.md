# canview

현대·기아·제네시스 차량의 CAN 데이터를 별도 **Communicator**에서 수집하고, `ESP-NOW`로 Waveshare `ESP32-S3-Touch-LCD-3.5` 기반 **Controller**에 전달해 시각화하는 프로젝트다. Controller는 최대 3개의 CAN 버스에서 들어오는 데이터를 표시하고, 필요한 경우 안전하게 제한된 의도 명령을 Communicator에 전달한다.

현재 1차 대상은 다음 차량이다.

- 2017년식 Hyundai Tucson TL
- 2.0 디젤
- 4WD
- BlueLink 사양

차량별 CAN 정의는 `commaai/opendbc`의 공개 DBC를 기준으로 시작한다. upstream에 2017년식 Tucson 전용 항목이 확인되지 않으므로, 현재 저장된 DBC는 실차 캡처로 검증해야 하는 후보 정의다.

## 문서

- [구현 준비 기준과 통합 설계](docs/implementation-readiness.md)
- [원자 구현 task backlog](docs/tasks/README.md)
- [독립 적대적 설계 리뷰와 조치](docs/adversarial-design-review.md)
- [하드웨어 및 개발환경](docs/hardware-and-development.md)
- [시스템 구조와 장치 명칭](docs/system-architecture.md)
- [Communicator 회로·IC·핀맵](docs/communicator-hardware.md)
- [장치별 개발환경](docs/development-environments.md)
- [Communicator 내부 4 Mbps UART 프로토콜](docs/communicator-uart-protocol.md)
- [DBC 파일과 차량 적용 지침](dbc/README.md)
- [1차 대상 차량 검증 메모](docs/target-vehicle-2017-tucson.md)
- [내비·엔진·4WD·BCM·IPS·오디오 CAN 신호 후보 카탈로그](docs/can-signal-catalog.md)
- [4WD·DPF·오디오·SPORT 기능 설계](docs/feature-design.md)
- [자동 밝기·소음 음량·SPORT 제어 로직](docs/automation-control.md)
- [ESP-NOW 양방향 프로토콜](docs/esp-now-protocol.md)
- [미확정 CAN 신호용 Diagnostic Bridge·모바일 웹 UI](docs/can-diagnostics-web.md)
- [Controller CAN 수신·DBC 파이프라인](docs/controller-can-pipeline.md)
- [CAN 신호의 GPS·시간 조사](docs/can-gps-time-investigation.md)
- [운전자 UI·LVGL 설계](docs/ui-design.md)
- [LVGL 공식 데모 전체 검토](docs/lvgl-demo-review.md)

## UI prototype

320×480 세로 화면에 맞춘 주행·소리·FFT·자동화·설정 UI를 HTML prototype과 LVGL 8.4 코드로 제공한다. 현대 표준형 5W의 짙은 청색 tone과 LVGL 공식 eBike·Music 등 전체 데모의 유효한 패턴을 선별했다. 주행 화면 높이의 약 75%를 순정형 4WD 구동계에 할당하고, 중앙 차량·앞뒤 differential·shaft·네 바퀴 분절 torque bar를 표시한다. 순간연비는 차량 중앙에 두며 DPF와 작은 원형 속도·RPM이 이어진다. 현재 속도와 제한속도는 모든 화면에서 유지한다. 아래 값은 레이아웃 검토용 데모이며 실차 측정값이 아니다.

| 주행 상태 | 소리 제어 | FFT 분석 |
|---|---|---|
| ![주행 화면](docs/images/ui-drive.png) | ![소리 화면](docs/images/ui-audio.png) | ![FFT 화면](docs/images/ui-fft.png) |

| SPORT 자동화 | 설정 | 자동화 상세 설정 |
|---|---|---|
| ![자동화 화면](docs/images/ui-automation.png) | ![설정 화면](docs/images/ui-settings.png) | ![소음·SPORT 설정 화면](docs/images/ui-settings-automation.png) |

- 브라우저 prototype: [`ui/prototype/`](ui/prototype/)
- LVGL 8.4 UI: [`ui/lvgl/`](ui/lvgl/)
- wire protocol C 구조: [`protocol/canview_protocol.h`](protocol/canview_protocol.h)

미확정 CAN ID·bit·scale은 별도 `Diagnostic Bridge`가 ESP-NOW read-only peer로 수신하고, 휴대폰이 Bridge의 고정-channel SoftAP에 접속해 확인하는 구조를 기본안으로 한다. 아래 정적 prototype은 세 CAN bus 상태, 행동 전후 capture, 64-bit 변화 지도와 후보 decoder의 모바일 정보 구조를 보여준다. 실제 REST·WebSocket firmware는 [구현 명세](docs/can-diagnostics-web.md)의 단계에 따라 추가한다.

| Diagnostic Bridge 상태 | Signal Lab |
|---|---|
| ![진단 상태 화면](docs/images/can-debug-overview.png) | ![신호 분석 화면](docs/images/can-debug-signal-lab.png) |

- 모바일 진단 prototype: [`ui/diagnostic-web/`](ui/diagnostic-web/)

## 저장된 DBC

원본은 `dbc/opendbc/`에 upstream 파일을 수정하지 않고 저장했다.

- `hyundai_can.dbc`: 현재 Hyundai 공통 CAN generator source. 1차 대상의 주 후보
- `hyundai_2015_ccan.dbc`: 구형 Classic CAN 참고 정의
- `hyundai_2015_mcan.dbc`: 구형 Multimedia CAN 참고 정의
- `LICENSE`: DBC 원본 저장에 필요한 upstream MIT License

파일의 upstream commit, SHA-256, 적용 범위는 [dbc/README.md](dbc/README.md)에 고정해 두었다.

## 저장소 상태

DBC 원본, hardware 기준, 기능 안전 경계, ESP-NOW wire 구조, 화면 prototype과 LVGL UI 계층을 포함한 설계 기준 저장소다. 실제 Communicator와 Controller firmware 통합은 실차 capture로 신호를 검증하면서 단계적으로 추가한다.

현재 저장소는 차량 송신 가능 상태가 아니다. protocol payload ABI·codec, 완성 firmware project, 승인 회로도와 firmware 독립 hard TX gate, 대상 차량 evidence가 모두 미완료다. 구현 순서와 차량 연결 gate는 [구현 준비 기준](docs/implementation-readiness.md)과 [task backlog](docs/tasks/README.md)를 따른다.

## 주요 원문

- [Waveshare ESP32-S3-Touch-LCD-3.5 제품 문서](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5)
- [Waveshare 리소스 및 회로도](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/Resources-And-Documents)
- [Waveshare 예제 저장소](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/)
- [commaai/opendbc](https://github.com/commaai/opendbc)
