# CANView 개발환경

이 문서는 [문서 지도](../README.md)의 Windows 실행 위치와 도구 정책 정본이다. Controller·Communicator·Diagnostic Bridge별 상세 SDK와 핀·주변장치 설정은 [장치별 toolchain](toolchains.md)을 따른다.

## 1. 실행 원칙

- 개발 명령은 Windows의 PowerShell 또는 Windows native 도구에서 실행한다.
- Git source of truth는 `F:/dev/canview` 같은 Windows 경로의 checkout/worktree다.
- 의존성 설치와 장기 실행은 Windows checkout에서 수행하며 WSL ext4 테스트 미러는 필수가 아니다.
- Git for Windows, Node.js/npm, Python, KiCad CLI, CMake/Ninja, ESP-IDF와 GNU Arm Embedded의 Windows 설치본을 표준 검증 경로로 사용한다.
- 차량 capture·provisioning key·VIN·대용량 evidence는 Git에 넣지 않는다.

## 2. 도구

| 대상 | 기준 |
|------|------|
| Controller | ESP-IDF `v6.0.3`, LVGL, Waveshare BSP |
| Communicator ESP32 | ESP-IDF `v6.0.3`, target esp32s3, WROOM-1-N16R8 16 MiB Flash/8 MiB Octal PSRAM |
| Communicator STM32 | CMake `4.4.3` + Ninja `1.13.2` + Arm GNU `15.3.Rel1` + 고정 STM32CubeG4 `v1.6.3` |
| host test | 공용 기반 strict C99 Windows Clang, CMake/CTest; legacy prototype만 C11 분리 |
| DBC/profile | Python validator와 생성기 |
| static prototype | Windows Node 구문 검사 + Playwright 오프라인 상호작용·스크린샷 회귀 |
| hardware | KiCad `10.0.6` schematic/PCB/ERC/netlist/BOM CLI |

정확한 SDK 버전과 dependency digest는 [`tools/toolchain-versions.json`](../../tools/toolchain-versions.json)에 고정한다. `tools/environment/setup-windows.ps1`는 해당 manifest와 checkout commit을 검증한다. Arm GNU `15.3.Rel1` archive SHA-256도 같은 manifest에 둔다. STM32CubeG4_ROOT가 없는 환경에서는 STM32 image build를 성공으로 표시하지 않는다.

현재 기반 코드의 SHA256 고정 host tool bootstrap, coverage와 API 문서 명령은 [기반 개발 절차](foundation.md)를 따른다. SDK 전체 설치/검증과 구분한다. 대상 빌드 전에는 host tool과 target SDK를 각각 dot-source한다.

## 3. 필요 시 생성하는 임시 worktree

기본 작업은 `F:/dev/canview`의 작업 branch에서 수행하고 병렬·격리·독립 리뷰가 필요할 때만 임시 worktree를 만든다. 생성, reviewer detached 기준선 검증, CodeGraph 수명주기와 안전한 삭제 명령은 [agent workflow](../runbooks/agent-workflow.md)에만 둔다.

## 4. 검증 계층

1. 문서·protocol schema·생성물 정합성
2. host C test와 malformed-input/fuzz test
3. CMake/Ninja·ESP-IDF compile
4. KiCad ERC·netlist·BOM·pinmap consistency
5. HIL 및 power/reset/fault injection
6. read-only 차량 capture
7. 제한된 bench TX와 폐쇄 시험

실행 불가능한 gate는 통과로 간주하지 않는다. 도구가 없으면 미실행 이유, 영향을 받은 gate, 후속 task를 journal과 PR에 기록한다.
