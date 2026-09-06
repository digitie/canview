# CANView 구현 task backlog

## 1. 사용법

열린 task의 요약은 이 문서, 완료 이력은 [tasks-done.md](tasks-done.md), 현재 진척은 [resume.md](resume.md)에 둔다. [상세 task](tasks/)의 수용 기준과 [작성 규약](tasks-rule.md)을 따른다. 요구→정본→task→남은 gate는 [요구사항 coverage](architecture/requirements-coverage.md)에 둔다.

이 directory에는 46개의 상세 작업이 있다. 기존 35개에서 PCB 설계·audio 송신 시험·SPORT 수신 근거 3개와 OTA 8단계를 추가했다. 이번 계획 보완은 실제 구현이나 실차 검증 완료가 아니다.

| 상태 | 의미 |
|---|---|
| `READY` | 명시된 선행과 입력이 준비되어 현재 저장소에서 시작 가능 |
| `BLOCKED` | 선행 task·하드웨어·근거·정본 충돌 해소가 필요 |
| `IN_PROGRESS` | 실제 작업 중이며 미완료 acceptance가 남음 |
| `DONE` | acceptance·evidence·리뷰를 충족한 PR이 main에 merge됨 |

task 시작/완료 시 상세 상태·branch/PR·실행한 검증·artifact를 갱신한다. 외부 gate는 `외부 선행`에 구분한다. 개발용 mock/boot image 제공은 전체 task 완료가 아니며 source evidence 조사와 실제 송신 승인은 별도다.

## 2. 공통 실행 규칙

1. [agent workflow](runbooks/agent-workflow.md)에 따라 기존 변경과 작업 branch를 보존한다. `범위 밖`을 같은 PR에 섞지 않는다.
2. 생성물은 source schema/generator와 함께 변경하고 golden·malformed·version/capability 시험을 유지한다.
3. 차량 TX는 profile·STM local check·lease·build mode·hardware gate·기능별 시험을 모두 요구한다. candidate/estimated 값을 정상 UI나 safety로 승격하지 않는다.
4. callback/ISR→bounded queue→worker와 LVGL 단일 owner는 [통합 설계](architecture/implementation-readiness.md)를 따른다. API별 buffer 수명·thread safety·중단/해제·overflow·reboot 동작을 해당 module README/public header에 기록한다.
5. 상세 task의 예정 경로·명령은 미래 산출물일 수 있다. **명령을 적었다는 사실은 파일 존재·실행·성공의 증거가 아니다.** 별도 표시가 없으면 해당 소비 task가 script/fixture/CTest target을 생성·등록하고 실패 fixture부터 검증한다. T-001은 CI 연결, T-500은 공용 rig 계약을 소유한다.
6. 실제 실행에는 tool version·정확한 명령·test 수·exit code·artifact digest를 남긴다. SDK/보드/차량/장비 부재는 NOT_RUN으로 기록하며 0 test/skip을 pass로 집계하지 않는다.
7. PR에는 Task/Gate/Risk/Tests/Evidence/Rollback을 남긴다. 원본 VIN·위치·capture·키는 public Git에 넣지 않는다.
8. 비단순 변경은 전문 리뷰어 2명의 독립 적대적 리뷰와 disposition·재시험을 요구한다. 이번 1차 계획 sidecar는 그 최종 독립 리뷰가 아니다.
9. hardware/OTA의 비가역 provisioning·발주·실차 시험은 해당 외부 승인과 장비 조건을 먼저 확인한다.
10. 아래 기계 검사는 문서 metadata/DAG 검사이며 제품 안전·동작·G0–G6 완료 판정기가 아니다.

```powershell
py -3 tools/validate_plan.py
py -3 -m unittest discover -s tests -p test_plan_validation.py -v
py -3 tools/validate_document_links.py
```

## 3. task 목록

### 계약·도구 기반

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-004](tasks/T-004-uart-schema-codec.md) | IN_PROGRESS | P0 | Communicator UART v1.0 schema와 codec | T-001, T-002 |
| [T-005](tasks/T-005-canonical-model.md) | BLOCKED | P0 | 공통 quality, evidence, time과 owner model | T-002 |
| [T-006](tasks/T-006-vehicle-profile-generator.md) | BLOCKED | P0 | vehicle profile schema와 분리 generator | T-001, T-005 |
| [T-007](tasks/T-007-ota-container.md) | BLOCKED | P0 | OTA-01 서명 컨테이너와 packager | T-001 |

### Communicator hardware·STM32

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-100](tasks/T-100-communicator-schematic.md) | IN_PROGRESS | P0 | Communicator 회로도, BOM과 hard TX gate | 없음 |
| [T-100a](tasks/T-100a-pcb-production-design.md) | BLOCKED | P0 | 네 보드 PCB 배치·배선과 제작 설계 gate | T-100 |
| [T-100b](tasks/T-100b-navigation-audio-bringup.md) | BLOCKED | P1 | R1 센서·원격 수음·전원 상태 firmware와 bring-up | T-002, T-004, T-101, T-200, T-201, T-202, T-203, T-300, T-303, T-304 |
| [T-101](tasks/T-101-hardware-bringup.md) | BLOCKED | P0 | Communicator PCB bring-up과 전원/reset fault 검증 | T-100a, T-500 |
| [T-102](tasks/T-102-stm32-platform.md) | BLOCKED | P0 | STM32 platform, clock, watchdog와 cooperative scheduler | T-001 |
| [T-103](tasks/T-103-stm32-fdcan-capture.md) | BLOCKED | P0 | STM32 3채널 FDCAN capture-only 경로 | T-004, T-102, T-500 |
| [T-104](tasks/T-104-stm32-uart-control.md) | BLOCKED | P0 | STM32 UART DMA, link state와 idempotency | T-004, T-102 |
| [T-105](tasks/T-105-stm32-safety-profile.md) | BLOCKED | P0 | STM32 generated safety profile runtime | T-006, T-103, T-104 |
| [T-106](tasks/T-106-stm32-command-executor.md) | BLOCKED | P0 | STM32 command executor와 송신 build gate | T-101, T-105, T-503, T-505a, T-500 |
| [T-107](tasks/T-107-stm32-mcuboot.md) | BLOCKED | P0 | OTA-03 G474 MCUboot와 보호 Flash map | T-007, T-102 |
| [T-108](tasks/T-108-ota-recovery-uart.md) | BLOCKED | P0 | OTA-04 ESP와 STM recovery UART | T-007, T-107, T-204 |

### Communicator ESP32

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-200](tasks/T-200-communicator-esp32-bootstrap.md) | BLOCKED | P0 | Communicator ESP32-S3-WROOM-1-N16R8 bootstrap | T-001, T-004 |
| [T-201](tasks/T-201-communicator-espnow.md) | BLOCKED | P0 | Communicator ESP-NOW provisioning, session과 QoS | T-003, T-200 |
| [T-202](tasks/T-202-communicator-uart-router.md) | BLOCKED | P0 | Communicator ESP32 UART link, router와 boot epoch | T-104, T-200 |
| [T-203](tasks/T-203-peer-subscriptions.md) | BLOCKED | P0 | peer별 subscription, quota와 observer union | T-201, T-202, T-103, T-204 |
| [T-204](tasks/T-204-esp-ota-recovery.md) | BLOCKED | P0 | OTA-02 ESP 파티션과 세 역할 복구 앱 | T-007, T-200, T-300, T-400 |
| [T-205](tasks/T-205-ota-policy-migration.md) | BLOCKED | P0 | OTA-06 호환성·영속 정책과 설정 migration | T-306, T-108, T-304, T-105 |

### Controller·공통 OTA 웹

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-300](tasks/T-300-controller-bootstrap.md) | BLOCKED | P0 | Controller Waveshare BSP, RTC와 LVGL bootstrap | T-001, T-005 |
| [T-301](tasks/T-301-controller-can-pipeline.md) | BLOCKED | P0 | Controller local allow-list, catalog와 freshness pipeline | T-003, T-006, T-203, T-300 |
| [T-302](tasks/T-302-controller-ui-model.md) | BLOCKED | P1 | Controller double-buffer UI model과 LVGL adapter | T-005, T-301 |
| [T-303](tasks/T-303-controller-fft.md) | BLOCKED | P1 | Controller I2S microphone, FFT와 noise feature pipeline | T-300, T-302 |
| [T-304](tasks/T-304-controller-local-config.md) | BLOCKED | P1 | Controller local 설정, RTC, 밝기와 전조등 경고 | T-300, T-302 |
| [T-305](tasks/T-305-controller-command-orchestration.md) | BLOCKED | P1 | Controller audio/SPORT command orchestration | T-003, T-302, T-201, T-105 |
| [T-306](tasks/T-306-ota-browser-session.md) | BLOCKED | P0 | OTA-05 세 장치 독립 AP와 브라우저 세션 | T-007, T-204, T-108 |

### Diagnostic Bridge

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-400](tasks/T-400-diagnostic-bridge-bootstrap.md) | BLOCKED | P1 | Diagnostic Bridge ESP-IDF, SoftAP와 인증 bootstrap | T-001, T-003 |
| [T-401](tasks/T-401-capture-cvtrace.md) | BLOCKED | P1 | observer, capture와 `.cvtrace` storage | T-203, T-400, T-103, T-204 |
| [T-402](tasks/T-402-diagnostic-api-web.md) | BLOCKED | P1 | Diagnostic OpenAPI, REST/WS와 모바일 web 통합 | T-401, T-304, T-305 |
| [T-403](tasks/T-403-signal-lab-evidence.md) | BLOCKED | P1 | Signal Lab, candidate와 evidence export | T-006, T-402 |

### 시험·차량 기능·release

| ID | 상태 | 우선순위 | 작업 | 선행 |
|---|---|---:|---|---|
| [T-500](tasks/T-500-bench-hil-harness.md) | BLOCKED | P0 | protocol/CAN fault bench와 HIL harness | T-001, T-003, T-004 |
| [T-501](tasks/T-501-tucson-bus-discovery.md) | BLOCKED | P0 | 2017 Tucson TL bus, bitrate와 connector discovery | T-101, T-103, T-203, T-401, T-500 |
| [T-502](tasks/T-502-readonly-signal-validation.md) | BLOCKED | P1 | 4WD, TPMS, DPF, 연비와 주행 신호 read-only 승격 | T-501, T-301, T-403 |
| [T-503](tasks/T-503-audio-command-validation.md) | BLOCKED | P0 | OEM audio command와 feedback 수신 조사 | T-501, T-403, T-500 |
| [T-503a](tasks/T-503a-audio-bench-release.md) | BLOCKED | P0 | OEM audio bench TX와 기능별 폐쇄시험 | T-503, T-106, T-305, T-500, T-508 |
| [T-504](tasks/T-504-adaptive-volume-release.md) | BLOCKED | P2 | 주행소음 기반 음량 자동화 release | T-303, T-305, T-503a |
| [T-505](tasks/T-505-auto-sport-validation.md) | BLOCKED | P2 | 자동 SPORT monitor, HIL과 폐쇄시험 | T-106, T-305, T-505a, T-500, T-508 |
| [T-505a](tasks/T-505a-sport-source-evidence.md) | BLOCKED | P0 | SPORT와 이전 mode의 read-only 근거 검증 | T-501, T-403, T-500 |
| [T-506](tasks/T-506-release-qualification.md) | BLOCKED | P0 | fault, security, soak와 release manifest | T-100b, T-502, T-503a, T-504, T-505, T-508 |
| [T-507](tasks/T-507-production-provisioning.md) | BLOCKED | P0 | OTA-07 제조 provisioning과 유선 복구 검증 | T-205, T-201, T-101 |
| [T-508](tasks/T-508-ota-power-can-hil.md) | BLOCKED | P0 | OTA-08 전원 차단·Flash·CAN 통합 HIL | T-507, T-500, T-101 |

## 4. critical path

아래는 병목 경로 설명이며 전체 DAG는 각 상세 task의 `선행`이 정본이다. 경로 밖 선행도 생략해 실행하지 않는다.

- 공용 시험 기반: T-001 → T-002 → T-003/T-004 → T-500. T-500은 T-103보다 먼저 rig/시나리오 API를 제공하고, 실제 FDCAN/전원 시험의 합격은 각 소비 task가 담당한다.
- 하드웨어: T-100 → T-100a → T-101. T-101에는 T-500·조립 PCB·최소 boot/fault image가 추가로 필요하며 회로/ERC만으로 G1이 닫히지 않는다.
- 수신 통합: STM capture/UART와 ESP transport → T-203 → T-401 → T-501. G2 실측/TX0 증거 전에는 차량 capture를 시작하지 않는다.
- UI·근거: T-301 → T-302 → local config/command adapter → T-402 → T-403. T-502/T-503/T-505a는 T-501 capture와 이 분석 경로를 결합한다.
- 송신: T-503(audio 수신 조사) 및 T-505a(SPORT 수신 근거) → T-106 → T-503a/T-505. T-503에 자기 후속 executor의 bench 결과를 선행으로 요구하지 않는다.
- 독립 OTA: T-007 → T-204(세 ESP recovery) 및 T-107(STM bootloader) → T-108 → T-306 → T-205 → T-507 → T-508. 각 단계의 BSP/config/runtime 선행도 충족한다. 이 개발 순서가 Communicator의 런타임 Controller/Bridge 의존성을 뜻하지 않는다.
- release: feature별 evidence·센서 통합·OTA fault qualification이 T-506에서 합쳐진다. 일부 기능의 통과를 제품 전체 완료로 확대하지 않는다.

T-503a/T-505의 최종 차량 시험은 T-508을 포함한 작업대·보안 gate 뒤에 실행한다. 초기 capture-only milestone은 OTA 미완료를 숨기지 않고 기존 검증 build/하드웨어에 한정하며, 새 OTA firmware의 차량 연결 허용과 구분한다.

## 5. gate 현황

| Gate | 상태 | 닫힌 이유 |
|---|---|---|
| G0 계약 | 미완료 | 기본 wire schema·codec·golden vector·전체 CI 미완료; 일부 host fixture만 존재 |
| G1 board boot | 미완료 | 회로/ERC 존재, land·PCB·조립·reset/rail/gate 실측 미완료 |
| G2 bench read-only | 미완료 | 통합 firmware·3 CAN/무선/UART/전원 HIL 미완료 |
| G3 차량 capture-only | 금지 | G2 근거 없음; 실제 bus/profile 승격 전 |
| G4 bench TX | 금지 | VERIFIED command source와 executor/bench matrix 없음 |
| G5 폐쇄시험 | 금지 | 기능별 G4·실차 안전 승인 미완료 |
| G6 release | 금지 | 기능·보안·OTA·환경 qualification 미완료 |

OTA-01–08은 OTA 정본의 구현 단계 ID이며 G0–G6을 대체하는 별도 차량 권한이 아니다. OTA task가 추가됐거나 문서 validator가 통과해도 차량 gate는 열리지 않는다.

## 6. 완료와 변경 기록

완료 시 상세 task와 이 요약/archive를 함께 갱신하고, 실제 상태가 바뀐 경우에만 resume·journal·정본을 갱신한다. [리뷰 archive](reviews/README.md)의 과거 finding을 현재 구현 완료의 근거로 사용하지 않는다.

2026-09-06 1차 계획 감사: N16R8/독립 OTA와 PCB 제작 설계 추적을 추가하고 수신 근거↔bench 송신의 순환을 분리했다. summary의 제목·상태·선행을 상세와 일치시키고 T-105/T-301/T-402/T-506 등 누락 선행을 보강했다. 실제 미구현 task의 상태는 올리지 않았다.
