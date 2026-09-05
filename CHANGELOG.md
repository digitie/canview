# CHANGELOG

이 문서는 사용자에게 보이는 변경을 기록한다.

## [Unreleased]

### Added

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
