# CANView 구현 task backlog

## 1. 사용법

열린 task의 요약은 이 문서에 두고, 완료·종료 이력은 [docs/tasks-done.md](tasks-done.md)에 둔다. 현재 진척과 다음 한 작업은 [docs/resume.md](resume.md), 작성 규약은 [docs/tasks-rule.md](tasks-rule.md), 상세 내용은 [docs/tasks/](tasks/)가 정본이다.

이 directory에는 34개의 원자 작업이 있으며 각 문서는 한 PR로 완료할 수 있는 범위다. 구현 agent는 한 번에 하나의 task를 선택하고, 문서의 결정사항을 다시 설계하지 않는다. 의존 task가 완료되지 않았거나 필요한 hardware/evidence가 없으면 코드를 추측해 채우지 않고 상태를 `BLOCKED`로 유지한다.

상태 의미는 다음과 같다.

| 상태 | 의미 |
|---|---|
| `READY` | 현재 저장소만으로 시작할 수 있음 |
| `BLOCKED` | 선행 task, hardware 또는 실차 evidence가 필요함 |
| `IN_PROGRESS` | 한 branch/PR에서 작업 중 |
| `DONE` | acceptance와 evidence를 포함한 PR이 main에 merge됨 |

task를 시작할 때 문서 상단의 상태, 담당 branch, PR을 갱신한다. 완료할 때 모든 checkbox를 채우고 검증 로그 또는 생성 artifact 경로를 남긴다. checkbox만 채우고 실패한 test를 숨기면 완료가 아니다.

## 2. 공통 실행 규칙

1. branch는 최신 `origin/main`에서 만든다.
2. task의 `범위 밖`을 같은 PR에 넣지 않는다.
3. 생성물은 source schema/generator와 함께 변경한다. 생성 header만 수정하지 않는다.
4. 차량 TX 관련 task는 요구 gate보다 낮은 환경에서 실행하지 않는다.
5. 새 protocol 필드는 golden vector와 malformed input test 없이 merge하지 않는다.
6. ESP-NOW/Wi-Fi callback, ISR, LVGL task ownership 규칙은 [통합 설계](architecture/implementation-readiness.md)를 따른다.
7. UI에 candidate/estimated 값을 정상 숫자로 표시하지 않는다.
8. validation command를 실행할 수 없는 환경이면 미실행 사유를 적고 task를 완료하지 않는다.
9. PR 본문에는 `Task`, `Gate`, `Risk`, `Tests`, `Evidence`, `Rollback`을 쓴다.
10. 차량 capture와 secret은 원본 그대로 public Git에 commit하지 않는다. 익명화·승인된 fixture만 넣는다.
11. 비단순 변경은 [agent workflow](runbooks/agent-workflow.md)의 전문 리뷰어 서브에이전트 2인 독립 적대적 리뷰를 통과한다.

## 3. task 목록

### 계약·도구 기반

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-001](tasks/T-001-host-toolchain-ci.md) | READY | P0 | 재현 가능한 host toolchain과 CI | 없음 |
| [T-002](tasks/T-002-espnow-schema-v1.3.md) | BLOCKED | P0 | ESP-NOW v1.3 schema·생성 header 동결 | T-001 |
| [T-003](tasks/T-003-espnow-codec-session.md) | BLOCKED | P0 | ESP-NOW codec/parser/session/QoS | T-002 |
| [T-004](tasks/T-004-uart-schema-codec.md) | BLOCKED | P0 | UART v1.0 schema·COBS/CRC codec | T-001, T-002 |
| [T-005](tasks/T-005-canonical-model.md) | BLOCKED | P0 | quality·evidence·time·owner 공통 model | T-002 |
| [T-006](tasks/T-006-vehicle-profile-generator.md) | BLOCKED | P0 | vehicle profile schema와 분리 generator | T-001, T-005 |

### Communicator hardware·STM32

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-100](tasks/T-100-communicator-schematic.md) | READY | P0 | 회로도·BOM·hard TX gate·전원 상태 | 없음 |
| [T-101](tasks/T-101-hardware-bringup.md) | BLOCKED | P0 | PCB bring-up과 reset/power fault 검증 | T-100, 실물 PCB |
| [T-102](tasks/T-102-stm32-platform.md) | BLOCKED | P0 | STM32 clock·startup·watchdog·scheduler | T-001 |
| [T-103](tasks/T-103-stm32-fdcan-capture.md) | BLOCKED | P0 | 3채널 FDCAN capture-only 경로 | T-004, T-102, T-500 |
| [T-104](tasks/T-104-stm32-uart-control.md) | BLOCKED | P0 | UART DMA·link state·idempotency | T-004, T-102 |
| [T-105](tasks/T-105-stm32-safety-profile.md) | BLOCKED | P0 | generated safety profile runtime | T-006, T-104 |
| [T-106](tasks/T-106-stm32-command-executor.md) | BLOCKED | P0 | build mode·hard gate·command executor | T-101, T-105, T-503, T-500 |

### Communicator ESP32

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-200](tasks/T-200-communicator-esp32-bootstrap.md) | BLOCKED | P0 | ESP-IDF N4R2 project bootstrap | T-001, T-004 |
| [T-201](tasks/T-201-communicator-espnow.md) | BLOCKED | P0 | provisioning·peer·session·QoS | T-003, T-200 |
| [T-202](tasks/T-202-communicator-uart-router.md) | BLOCKED | P0 | UART link와 boot epoch/session 교체 | T-104, T-200 |
| [T-203](tasks/T-203-peer-subscriptions.md) | BLOCKED | P0 | peer별 filter·quota·observer union | T-201, T-202, T-103 |

### Controller

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-300](tasks/T-300-controller-bootstrap.md) | BLOCKED | P0 | Waveshare BSP·RTC·LVGL app bootstrap | T-001, T-005 |
| [T-301](tasks/T-301-controller-can-pipeline.md) | BLOCKED | P0 | local allow-list·catalog·freshness | T-003, T-006, T-201, T-300 |
| [T-302](tasks/T-302-controller-ui-model.md) | BLOCKED | P1 | double-buffer model·LVGL adapter·token 생성 | T-005, T-301 |
| [T-303](tasks/T-303-controller-fft.md) | BLOCKED | P1 | I2S microphone·FFT·noise feature | T-300, T-302 |
| [T-304](tasks/T-304-controller-local-config.md) | BLOCKED | P1 | RTC·밝기·유휴·전조등 local 설정 | T-300, T-302 |
| [T-305](tasks/T-305-controller-command-orchestration.md) | BLOCKED | P1 | audio/SPORT 명령 orchestration과 UI 상태 | T-003, T-302, T-201, T-105 |

### Diagnostic Bridge

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-400](tasks/T-400-diagnostic-bridge-bootstrap.md) | BLOCKED | P1 | Bridge ESP-IDF·SoftAP·인증 bootstrap | T-001, T-003 |
| [T-401](tasks/T-401-capture-cvtrace.md) | BLOCKED | P1 | observer·capture·`.cvtrace` storage | T-203, T-400, T-103 |
| [T-402](tasks/T-402-diagnostic-api-web.md) | BLOCKED | P1 | OpenAPI·REST/WS·모바일 UI 통합 | T-401 |
| [T-403](tasks/T-403-signal-lab-evidence.md) | BLOCKED | P1 | Signal Lab·candidate·evidence export | T-006, T-402 |

### 시험·차량 기능·release

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-500](tasks/T-500-bench-hil-harness.md) | BLOCKED | P0 | protocol/CAN fault bench와 HIL harness | T-001, T-003, T-004 |
| [T-501](tasks/T-501-tucson-bus-discovery.md) | BLOCKED | P0 | Tucson TL bus·bitrate·connector capture | T-101, T-103, T-203, T-401, T-500 |
| [T-502](tasks/T-502-readonly-signal-validation.md) | BLOCKED | P1 | 4WD·TPMS·DPF·연비·속도 read-only 승격 | T-501, T-301, T-403 |
| [T-503](tasks/T-503-audio-command-validation.md) | BLOCKED | P0 | OEM audio 명령·counter·feedback 검증 | T-501, T-403, T-500 |
| [T-504](tasks/T-504-adaptive-volume-release.md) | BLOCKED | P2 | 자연스러운 주행소음 음량 자동화 | T-303, T-305, T-503 |
| [T-505](tasks/T-505-auto-sport-validation.md) | BLOCKED | P2 | SPORT monitor→HIL→폐쇄시험 | T-106, T-305, T-501, T-500 |
| [T-506](tasks/T-506-release-qualification.md) | BLOCKED | P0 | fault/security/soak와 release manifest | G5 완료 |

## 4. critical path

`T-001 → T-002 → T-003/T-004/T-005 → T-006/T-102/T-200/T-300 → T-103/T-104/T-201 → T-202/T-203 → T-500 → G2`

hardware는 `T-100 → T-101`로 병렬 진행한다. 두 경로가 G2에서 합쳐진 뒤에만 `T-501` 차량 capture를 시작한다. audio와 SPORT 송신 경로는 `T-503`의 검증된 evidence 없이는 열리지 않는다.

## 5. gate 현황

| Gate | 상태 | 닫힌 이유 |
|---|---|---|
| G0 계약 | 미완료 | protocol schema·codec·golden vector·CI 없음 |
| G1 board boot | 미완료 | 승인 회로·PCB·hard TX gate 없음 |
| G2 bench read-only | 미완료 | 완성 firmware·HIL 없음 |
| G3 차량 capture-only | 금지 | G2 미완료 |
| G4 bench TX | 금지 | verified command profile 없음 |
| G5 폐쇄시험 | 금지 | G4 미완료 |
| G6 release | 금지 | 전체 qualification 미완료 |

## 6. task 완료 후 갱신

완료 PR은 이 index와 해당 상세 task를 갱신하고, 실제로 바뀐 정본만 함께 갱신한다.

- [구현 준비 기준과 통합 설계](architecture/implementation-readiness.md)
- 해당 subsystem README
- public 동작이 바뀌면 root `README.md`
- 적대적 리뷰를 실행했다면 기존 report를 수정하지 않고 [review archive](reviews/README.md)에 새 report

설계가 바뀌면 기존 task를 조용히 수정하지 말고 변경 이유, 영향을 받는 task, migration 여부를 task 문서의 `결정 변경 기록`에 남긴다.
