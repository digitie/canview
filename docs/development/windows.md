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
| Controller | ESP-IDF, LVGL, Waveshare BSP |
| Communicator ESP32 | ESP-IDF, target esp32s3, N4R2 4 MB Flash/2 MB PSRAM |
| Communicator STM32 | CMake + Ninja + GNU Arm Embedded + 고정 STM32CubeG4 |
| host test | C11 compiler, CMake, CTest |
| DBC/profile | Python validator와 생성기 |
| static prototype | Windows Node의 `node --check` |
| hardware | KiCad schematic/PCB/ERC/netlist/BOM CLI |

정확한 SDK 버전과 dependency digest는 T-001에서 고정한다. STM32CubeG4_ROOT가 없는 환경에서는 STM32 image build를 성공으로 표시하지 않는다.

## 3. 필요 시 생성하는 임시 worktree

기본 작업은 `F:/dev/canview`에서 수행한다. 병렬 작업이나 격리가 필요할 때만 임시 worktree를 만들고, 작업이 끝나면 삭제한다.

    New-Item -ItemType Directory -Force F:/dev/canview-wt | Out-Null
    git -C F:/dev/canview fetch origin main
    git -C F:/dev/canview worktree add -b agent/codex-t001 F:/dev/canview-wt/codex-t001 origin/main
    Set-Location F:/dev/canview-wt/codex-t001

검증과 commit/push는 해당 Windows worktree에서 수행한다. 작업이 merge 또는 abandon으로 끝난 뒤 실행 중인 프로세스와 미커밋 변경이 없을 때만 다음으로 정리한다.

    git -C F:/dev/canview worktree remove F:/dev/canview-wt/codex-t001
    git -C F:/dev/canview worktree prune

worktree 제거 후에도 필요하면 원격 branch를 별도 확인해 정리한다. 사용자의 활성 worktree나 미커밋 변경을 강제로 제거하지 않는다.

## 4. 검증 계층

1. 문서·protocol schema·생성물 정합성
2. host C test와 malformed-input/fuzz test
3. CMake/Ninja·ESP-IDF compile
4. KiCad ERC·netlist·BOM·pinmap consistency
5. HIL 및 power/reset/fault injection
6. read-only 차량 capture
7. 제한된 bench TX와 폐쇄 시험

실행 불가능한 gate는 통과로 간주하지 않는다. 도구가 없으면 미실행 이유, 영향을 받은 gate, 후속 task를 journal과 PR에 기록한다.
