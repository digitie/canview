# CHANGELOG

이 문서는 사용자에게 보이는 변경을 기록한다.

## [Unreleased]

### Added

- Bridge 없이 가능한 장치별 브라우저 OTA, STM 내부 이중 슬롯 복구와 설치 승인·버전 정책을 단일 [설계·리뷰 문서](docs/architecture/ota.md)에 추가했다. OTA 펌웨어 구현은 후속 단계다.
- Communicator N16R8/Octal PSRAM ECC 설정, ESP·STM reset 분리, 물리 서비스 인터록과 역구동 차단 buffer, Controller 복구 버튼 회로를 반영했다.

- Communicator·Bridge·Controller adapter·원격 I²S mic의 KiCad10 상세 회로·PDF·netlist·BOM·local footprint와 교차 검증기를 추가했다.
- 차량/USB 전원 분리, brownout/watchdog 재무장 latch, 외장 GNSS·MTi7 DR·BMP384와 상세 FW 핀맵을 추가했다. 제작·차량 시험 gate는 미완료다.
- 센서 protocol 확장 schema, host codec·golden 시험, 구독 적용 응답·재시도·예산 설계와 T-100b를 추가했다.
- 제조사 원문 PDF·출처/개정/hash와 참조 절, 미확보 원문 및 footprint 검증 제한을 기록했다.
- kor-travel-geo 방식의 에이전트 운영 문서 구조를 추가했다.
- 상위 아키텍처, ADR, runbook, resume, journal 정본을 추가했다.
- 열린 task 요약을 docs/tasks.md로 분리하고 상세 task 파일은 docs/tasks/에 유지했다.
- Windows 개발환경과 일회성 worktree 운영 결정을 ADR-003으로 추가했다.
- 중앙 문서 지도와 architecture·hardware·development·vehicle·UI 하위 구조를 추가했다.
- 실행별 적대적 리뷰 archive와 전문 리뷰어 서브에이전트 2인 독립 review gate를 추가했다.
- 계층형 문서 정보구조와 누적 리뷰 정책을 ADR-004로 기록했다.
- 최신 Windows 임베디드 toolchain manifest와 SDK 준비 스크립트를 추가했다.
- Controller와 Communicator ESP32의 독립 ESP-IDF bootstrap, public protocol component, module별 partition 설정을 추가했다.
- STM32 CMake preset을 최신 CMake/Ninja/Arm GNU baseline과 compiler version 검증에 맞췄다.

### Changed

- 기존 docs/tasks/README.md는 상세 task directory 안내 포인터로 변경했다.
- root README의 task 링크를 docs/tasks.md로 통일했다.
- AGENTS.md는 필수 정책과 단계별 문서 선택만 남기고 상세 절차를 runbook으로 분리했다.
- protocol과 시스템 설계 문서는 docs/architecture/ 아래로, 나머지 상세 문서는 분야별 하위 디렉터리로 이동했다.
- ESP32 baseline을 ESP-IDF `v6.0.3`으로, STM32CubeG4를 `v1.6.3`으로 갱신했다. 실제 BSP와 통신 기능은 후속 task에서 검증한다.
