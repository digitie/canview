# CANView 개발환경

이 문서는 개발 도구와 실행 위치의 요약 정본이다. Controller·Communicator·Diagnostic Bridge별 상세 SDK와 핀·주변장치 설정은 [development-environments.md](development-environments.md)를 따른다.

## 1. 실행 원칙

- 개발 명령은 Linux 또는 WSL에서 실행한다.
- Git source of truth는 /mnt/f/dev/canview 같은 Linux 경로의 worktree다.
- 의존성 설치와 장기 실행은 WSL ext4 테스트 미러에서 수행한다.
- Windows native Git, Node/npm, Python, KiCad CLI는 표준 검증 경로로 사용하지 않는다.
- 차량 capture·provisioning key·VIN·대용량 evidence는 Git에 넣지 않는다.

## 2. 도구

| 대상 | 기준 |
|------|------|
| Controller | ESP-IDF, LVGL, Waveshare BSP |
| Communicator ESP32 | ESP-IDF, target esp32s3, N4R2 4 MB Flash/2 MB PSRAM |
| Communicator STM32 | CMake + Ninja + GNU Arm Embedded + 고정 STM32CubeG4 |
| host test | C11 compiler, CMake, CTest |
| DBC/profile | Python validator와 생성기 |
| static prototype | Linux Node의 node --check |
| hardware | KiCad schematic/PCB/ERC/netlist/BOM CLI |

정확한 SDK 버전과 dependency digest는 T-001에서 고정한다. STM32CubeG4_ROOT가 없는 환경에서는 STM32 image build를 성공으로 표시하지 않는다.

## 3. 테스트 미러

고정 worktree를 ext4 미러로 복사하고, 미러에서만 의존성 설치와 테스트를 실행한다.

    mkdir -p ~/dev/canview-codex-test
    rsync -a --delete --exclude .git --exclude build --exclude artifacts /mnt/f/dev/canview/ ~/dev/canview-codex-test/
    cd ~/dev/canview-codex-test

미러에서 발견한 소스 변경은 먼저 worktree에 반영한 뒤 commit한다. 미러에서 commit이나 push하지 않는다.

## 4. 검증 계층

1. 문서·protocol schema·생성물 정합성
2. host C test와 malformed-input/fuzz test
3. CMake/Ninja·ESP-IDF compile
4. KiCad ERC·netlist·BOM·pinmap consistency
5. HIL 및 power/reset/fault injection
6. read-only 차량 capture
7. 제한된 bench TX와 폐쇄 시험

실행 불가능한 gate는 통과로 간주하지 않는다. 도구가 없으면 미실행 이유, 영향을 받은 gate, 후속 task를 journal과 PR에 기록한다.
