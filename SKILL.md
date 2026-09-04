# SKILL — canview 에이전트 매뉴얼

> 이 파일은 당신(AI 에이전트)이 작업을 시작하기 전 반드시 읽어야 한다.
> 하드웨어·차량 CAN·펌웨어 작업은 추측보다 문서와 검증을 우선한다.

## 1. 정체성

이 저장소(GitHub 이름 canview)는 현대·기아·제네시스 차량의 CAN 데이터를 표시하고 제한된 기능을 제어하는 Controller·Communicator·Diagnostic Bridge 시스템이다. Controller는 Waveshare ESP32-S3-Touch-LCD-3.5, Communicator는 ESP32-S3-MINI-1-N4R2와 STM32G474CEU6, Diagnostic Bridge는 별도 ESP32-S3를 사용한다.

저장된 DBC는 commaai/opendbc 원본을 기준으로 하며, 2017 Tucson TL 2.0 디젤 4WD BlueLink 차량에서 실제로 확인되기 전까지 후보 정의다. 차량 송신은 CAPTURE_ONLY, BENCH_TX, VEHICLE_TX 단계와 각 gate를 따른다.

### 식별자 매핑

| 항목 | 값 |
|------|----|
| GitHub 저장소 | canview |
| Controller | Waveshare ESP32-S3-Touch-LCD-3.5 |
| Communicator | ESP32-S3-MINI-1-N4R2 + STM32G474CEU6 |
| Diagnostic Bridge | 별도 ESP32-S3 read-only observer |
| ESP-NOW | v1.3 구현 기준 |
| 내부 UART | v1.0, 4 Mbps, 8-N-1, RTS/CTS |
| DBC 원본 | dbc/opendbc |
| task 요약 | docs/tasks.md |

### 개발 환경

- **모든 개발 명령은 Windows native 환경에서 실행**한다. PowerShell, Git for Windows, Windows Node.js/npm, Python, CodeGraph, KiCad CLI, CMake/Ninja, ESP-IDF, GNU Arm Embedded를 표준 경로로 삼는다. WSL/Linux shell은 보조 검증 환경일 뿐이다.
- **Git source of truth는 Windows Git이 읽는 checkout/worktree**다. 기본 경로는 `F:/dev/canview`이며 `/mnt/f/dev/canview`를 정본 경로로 사용하지 않는다.
- **테스트와 장기 실행은 Windows checkout**에서 수행한다. 고정 worktree를 유지하거나 WSL ext4 mirror로 복사하는 절차는 요구하지 않는다.
- **하드웨어 검증은 KiCad 산출물 기준**이다. schematic, PCB, BOM, ERC, netlist와 계산서가 서로 같은 revision인지 확인한다.
- **Git/CodeGraph 명령은 Windows 기준**이다. branch, commit, push, PR 준비, `codegraph sync/status`는 PowerShell에서 실행한다.
- **로컬 secret/capture**(`.env`, `sdkconfig.local`, provisioning, 차량 식별정보, raw capture)는 Git에 커밋하지 않는다.

### 필요 시 생성하는 worktree / CodeGraph

- 고정 에이전트 worktree를 만들지 않는다. 평소에는 `F:/dev/canview`에서 작업하고, 병렬 작업·격리·독립 리뷰가 필요할 때만 임시 worktree를 만든다.
- 예: `git worktree add -b agent/codex-<task> F:/dev/canview-wt/codex-<task> origin/main`
- 작업이 merge 또는 abandon으로 끝나면 활성 프로세스와 미커밋 변경이 없는지 확인한 뒤 `git worktree remove F:/dev/canview-wt/codex-<task>`와 `git worktree prune`으로 정리한다.
- 임시 worktree를 사용할 때만 처음에 `codegraph init -i`를 실행하고, branch 전환·pull·merge 뒤 `codegraph sync` → `codegraph status` 순서로 유지한다. 도구가 노출되지 않으면 그 사실을 journal에 남긴다.
- `.codegraph/`와 `.claude/`는 로컬 상태이므로 Git에 커밋하지 않는다.

## 2. 빠른 시작

    Set-Location F:/dev/canview
    git fetch origin main
    git switch -c agent/codex-<task> origin/main
    # 위 branch가 별도 격리를 필요로 할 때만, checkout 대신 다음을 사용한다.
    # git worktree add -b agent/codex-<task> F:/dev/canview-wt/codex-<task> origin/main
    # Set-Location F:/dev/canview-wt/codex-<task>
    codegraph sync
    codegraph status

상세 절차는 docs/dev-environment.md와 docs/runbooks/agent-workflow.md를 본다. 실제 차량 연결 전에는 docs/implementation-readiness.md와 docs/tasks.md의 gate를 확인한다.

## 3. 디렉토리 지도

    protocol/                         — ESP-NOW/UART ABI와 생성물
    dbc/                              — upstream DBC 원본과 라이선스
    firmware/controller/              — Controller ESP-IDF/LVGL
    firmware/communicator/esp32/      — Communicator 무선 MCU
    firmware/communicator/stm32/      — STM32 CMake/FDCAN/UART
    hardware/communicator/            — pinmap·KiCad·BOM·전원 계산
    ui/prototype/                     — Controller 화면 HTML prototype
    ui/lvgl/                          — LVGL 화면 코드
    ui/diagnostic-web/                — 모바일 진단 웹 prototype
    tests/                            — host·protocol·firmware·HIL 시험
    docs/architecture/                — 시스템 구조 정본
    docs/adr/                         — 결정 기록
    docs/runbooks/                    — 공용 운영 절차
    docs/tasks.md                     — 열린 task 요약
    docs/tasks/                       — task별 상세 문서

의존 방향은 source schema/profile → generated model/ABI → codec/transport → device runtime → UI/API 순서다. Controller와 Diagnostic Bridge가 STM32 안전 계층을 우회하지 않도록 한다.

## 4. 절대 하지 말 것 (DO NOT)

1. 생성 schema/header와 수동 편집 header를 불일치하게 두지 않는다.
2. Controller·ESP32·Diagnostic Bridge가 임의 CAN ID/data를 전달해 즉시 차량 송신하게 만들지 않는다.
3. CAPTURE_ONLY/BENCH_TX를 firmware flag 하나로 VEHICLE_TX로 바꾸지 않는다.
4. DBC 이름이나 단일 capture만으로 신호를 확정하지 않는다.
5. sender precondition bitmask를 STM32 local safety check 대신 사용하지 않는다.
6. 설치 전체가 공유하는 pairing secret을 사용하지 않는다. pair별 root와 device-local key를 사용한다.
7. MCU GPIO 초기화만으로 reset·brownout CAN 안전을 주장하지 않는다. 외부 gate, pull, rail 상태와 시험을 확인한다.
8. 정확한 MPN·package·footprint·connector pin을 추측해서 KiCad에 넣지 않는다.
9. upstream DBC 원본을 수정하지 않는다.
10. VIN, raw 위치정보, provisioning key, Wi-Fi secret, 승인되지 않은 차량 capture를 커밋하지 않는다.
11. block diagram이나 불완전한 부품 목록을 제작용 BOM/netlist라고 표시하지 않는다.
12. Diagnostic Bridge에 control lease 또는 차량 CAN TX 권한을 주지 않는다.
13. stale/candidate/estimated/pending 값을 정상 확정값처럼 표시하지 않는다.
14. raw observer stream이 safety/control queue와 ESP-NOW 대역폭을 잠식하게 하지 않는다.
15. protocol field 변경을 golden vector와 malformed-input test 없이 머지하지 않는다.

## 5. 자주 묻는 작업

| 작업 | 시작 파일 |
|------|-----------|
| protocol 변경 | protocol schema → generated header/codec → golden vector |
| STM32 CAN 수집 | firmware/communicator/stm32/ + T-103 |
| STM32 차량 제어 | generated safety profile + T-105/T-106 |
| Controller 화면 | ui/lvgl/ + ui/prototype/ + T-302 |
| Diagnostic 웹 | ui/diagnostic-web/ + T-402 |
| KiCad 회로/BOM | hardware/communicator/ + T-100 |
| DBC/profile | dbc/ + vehicle profile generator T-006 |
| 실제 차량 신호 | T-501 capture → T-502 read-only 승격 |
| 오디오 제어 | T-503 검증 이후에만 T-305/T-504 |

## 6. 도메인 어휘

| 용어 | 의미 |
|------|------|
| Controller | 운전자 화면과 의도 명령을 담당하는 Waveshare 보드 |
| Communicator | CAN 물리계층과 안전 송신을 담당하는 두 MCU 장치 |
| Diagnostic Bridge | 차량 명령 없이 관찰·capture·웹 진단만 하는 ESP32 |
| hard TX gate | firmware fault와 무관하게 송신을 차단하는 외부 계층 |
| CAPTURE_ONLY | CAN 수신/관찰만 허용하는 build mode |
| BENCH_TX | current-limited bench와 HIL에서만 제한 송신하는 build mode |
| VEHICLE_TX | 모든 안전·차량·release gate가 통과된 제한 송신 mode |
| candidate | DBC 또는 capture에서 추출했지만 실차 승격 전인 신호 |
| evidence | 신호·기능 판정의 근거와 provenance |
| freshness | 신호가 허용된 age 안에 들어온 상태 |
| control lease | 제한된 제어 명령을 발행할 수 있는 시간 제한 권한 |

## 7. 작업 후 체크리스트

- [ ] 관련 host/unit/HIL test 통과
- [ ] CMake/Ninja 또는 ESP-IDF build 통과
- [ ] KiCad ERC/netlist/BOM consistency 통과
- [ ] DBC/profile validator와 golden vector 통과
- [ ] docs/journal.md 갱신
- [ ] docs/resume.md 갱신
- [ ] docs/tasks.md 및 상세 task 상태 갱신
- [ ] 결정이 있으면 docs/decisions.md와 해당 ADR 갱신
- [ ] 사용자 가시 변경이면 CHANGELOG.md 갱신
- [ ] 차량 송신 변경이면 gate/evidence/rollback을 PR에 명시
