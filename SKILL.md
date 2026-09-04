# SKILL — CANView 작업 라우터

이 파일은 CANView 작업에 필요한 상세 문서를 빠르게 고르는 라우터다. 저장소 정책과 금지선의 정본은 [AGENTS.md](AGENTS.md)이며, 이 파일에 같은 규칙을 복제하지 않는다.

## 1. 최소 컨텍스트

모든 저장소 작업은 다음 순서로 시작한다.

1. [AGENTS.md](AGENTS.md)
2. [docs 문서 지도](docs/README.md)
3. [현재 상태](docs/resume.md)
4. 해당 상세 task가 있는 경우 그 파일 한 개

전체 `docs/`, 모든 ADR, 모든 task, 과거 review와 journal을 한꺼번에 읽지 않는다. 작업 대상이 정해지지 않았을 때만 [task 요약](docs/tasks.md)을 연다. task가 없는 문서·review 요청은 해당 runbook 또는 review template을 기준으로 삼는다.

## 2. 작업별 시작점

| 작업 | 첫 문서 | 이어서 볼 대상 |
|---|---|---|
| 시스템 책임·구조 | [architecture](docs/architecture/README.md) | 관련 상세 architecture 문서 하나 |
| ESP-NOW | [protocol index](docs/architecture/protocols/README.md) | [ESP-NOW](docs/architecture/protocols/esp-now.md), T-002/T-003 |
| STM32↔ESP32 UART | [protocol index](docs/architecture/protocols/README.md) | [UART](docs/architecture/protocols/communicator-uart.md), T-004/T-104 |
| Controller CAN pipeline | [pipeline](docs/architecture/controller-can-pipeline.md) | T-301과 관련 profile task |
| 자동화·차량 기능 | [features](docs/architecture/features.md) | [automation](docs/architecture/automation.md), 해당 검증 task |
| Diagnostic Bridge | [diagnostic architecture](docs/architecture/diagnostic-bridge.md) | T-400~T-403 |
| Controller·Communicator 회로 | [hardware 문서](docs/README.md#하드웨어) | 대상 보드 문서와 T-100/T-101 |
| Windows build·CI | [Windows 환경](docs/development/windows.md) | [장치별 toolchain](docs/development/toolchains.md), T-001 |
| 차량 신호·DBC | [vehicle 문서](docs/README.md#차량과-신호) | 대상 문서와 T-501~T-503 |
| LVGL·UI | [UI 설계](docs/ui/design.md) | 관련 prototype/LVGL README와 T-302 |
| 적대적 리뷰 | [review archive](docs/reviews/README.md) | 새 report template과 관련 diff |
| branch·PR·merge | [agent workflow](docs/runbooks/agent-workflow.md) | 실패했을 때만 failure patterns |

## 3. 저장소 영역

    protocol/               — ESP-NOW/UART schema와 생성 ABI
    dbc/                    — upstream DBC 원본과 라이선스
    firmware/controller/    — Controller ESP-IDF/LVGL
    firmware/communicator/  — ESP32와 STM32 firmware
    hardware/communicator/  — pinmap·KiCad·BOM·전원 계산
    ui/                     — HTML prototype과 LVGL 구현
    tests/                  — host·protocol·fault·HIL 시험
    docs/architecture/      — 현재 시스템·protocol·기능 설계 정본
    docs/hardware/          — 보드·회로 상세
    docs/development/       — Windows 환경과 toolchain
    docs/vehicle/           — 대상 차량·DBC·signal evidence
    docs/ui/                — UI 원칙과 벤치마크
    docs/reviews/           — 변경하지 않는 review 기록과 색인
    docs/tasks/             — task별 실행 명세

## 4. 핵심 용어

| 용어 | 의미 |
|---|---|
| Controller | 운전자 UI와 의미 명령 요청을 담당하는 Waveshare 보드 |
| Communicator | CAN 물리계층과 최종 안전 송신을 담당하는 ESP32+STM32 장치 |
| Diagnostic Bridge | 차량 제어 권한 없는 capture·Signal Lab·웹 장치 |
| CAPTURE_ONLY | CAN 수신/관찰만 허용하는 build mode |
| BENCH_TX | current-limited bench/HIL에서만 송신하는 mode |
| VEHICLE_TX | 승인된 profile·hardware·evidence gate 뒤 제한 송신하는 mode |
| candidate | DBC/capture에서 찾았지만 대상 차량에서 승격되지 않은 신호 |
| evidence | signal·기능 판정의 provenance와 검증 결과 |
| control lease | 제한된 의미 명령을 발행할 수 있는 시간 제한 권한 |

## 5. 종료 경로

검증, 전문 리뷰어 2인의 독립 적대적 리뷰, 문서 갱신, 보안 점검, PR과 worktree 정리는 [agent workflow](docs/runbooks/agent-workflow.md)를 따른다. 문서 역할과 기록 형식은 [documentation maintenance](docs/runbooks/documentation-maintenance.md)를 필요할 때만 읽는다.
