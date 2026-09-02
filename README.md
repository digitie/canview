# canview

현대·기아·제네시스 차량의 CAN 데이터를 별도 CAN 게이트웨이에서 수집하고, `ESP-NOW`로 Waveshare `ESP32-S3-Touch-LCD-3.5` 화면에 전달해 시각화하는 프로젝트다. 화면 장치는 최대 3개의 CAN 버스에서 들어오는 데이터를 표시하고, 필요한 경우 안전하게 제한된 제어 명령을 게이트웨이에 전달한다.

현재 1차 대상은 다음 차량이다.

- 2017년식 Hyundai Tucson TL
- 2.0 디젤
- 4WD
- BlueLink 사양

차량별 CAN 정의는 `commaai/opendbc`의 공개 DBC를 기준으로 시작한다. upstream에 2017년식 Tucson 전용 항목이 확인되지 않으므로, 현재 저장된 DBC는 실차 캡처로 검증해야 하는 후보 정의다.

## 문서

- [하드웨어 및 개발환경](docs/hardware-and-development.md)
- [DBC 파일과 차량 적용 지침](dbc/README.md)
- [1차 대상 차량 검증 메모](docs/target-vehicle-2017-tucson.md)

## 저장된 DBC

원본은 `dbc/opendbc/`에 upstream 파일을 수정하지 않고 저장했다.

- `hyundai_can.dbc`: 현재 Hyundai 공통 CAN generator source. 1차 대상의 주 후보
- `hyundai_2015_ccan.dbc`: 구형 Classic CAN 참고 정의
- `hyundai_2015_mcan.dbc`: 구형 Multimedia CAN 참고 정의
- `LICENSE`: DBC 원본 저장에 필요한 upstream MIT License

파일의 upstream commit, SHA-256, 적용 범위는 [dbc/README.md](dbc/README.md)에 고정해 두었다.

## 저장소 상태

초기 문서와 DBC 원본을 포함한 설계 기준 저장소다. 화면 펌웨어와 3채널 CAN 게이트웨이 펌웨어는 이 문서의 핀 점유 및 프로토콜 기준을 따라 추가한다.

## 주요 원문

- [Waveshare ESP32-S3-Touch-LCD-3.5 제품 문서](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5)
- [Waveshare 리소스 및 회로도](https://docs.waveshare.com/ESP32-S3-Touch-LCD-3.5/Resources-And-Documents)
- [Waveshare 예제 저장소](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.5/)
- [commaai/opendbc](https://github.com/commaai/opendbc)
