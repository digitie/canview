# CANView 아키텍처

이 문서는 CANView의 장치 관계, 데이터 흐름, 권한 경계와 저장소 구조를 설명하는 상위 아키텍처 정본이다. 세부 안전 규칙과 구현 순서는 [구현 준비 기준과 통합 설계](../implementation-readiness.md), 결정의 역사는 [ADR 색인](../adr/README.md), 현재 작업은 [tasks.md](../tasks.md)를 따른다.

## 1. 시스템 범위

CANView는 2017 Tucson TL 2.0 디젤 4WD BlueLink를 1차 대상으로 하는 차량 CAN viewer/controller다. 동일한 구조로 현대·기아·제네시스 차량 profile을 확장할 수 있지만, DBC에 존재한다는 이유만으로 차량별 신호나 제어 명령을 확정하지 않는다.

시스템은 다음 세 장치로 나뉜다.

| 장치 | 하드웨어 | 주 책임 |
|------|----------|---------|
| Controller | Waveshare ESP32-S3-Touch-LCD-3.5 | LVGL 운전자 UI, 터치, Controller-local 설정, ESP-NOW 명령 요청 |
| Communicator | ESP32-S3-MINI-1-N4R2 + STM32G474CEU6 + 3 CAN PHY | CAN 수집, timestamp, 안전 상태, 검증된 제한 송신 |
| Diagnostic Bridge | 별도 ESP32-S3 | read-only CAN 관찰, capture, signal lab, 휴대폰용 SoftAP 웹 |

## 2. 데이터 흐름

    차량 CAN 1/2/3
          │
          ▼
    TCAN1046AV-Q1 2채널 + MAX3055 1채널
          │
          ▼
    STM32G474CEU6
      ├─ FDCAN raw capture / bus fault / timestamp
      ├─ local safety profile / command executor
      └─ UART 4 Mbps, RTS/CTS
          │
          ▼
    Communicator ESP32-S3-MINI-1-N4R2
      ├─ ESP-NOW secure session
      ├─ peer별 filter / quota / retry
      └─ Controller·Diagnostic Bridge routing
          │
          ├──────────────► Controller
          │                 LVGL·상태표시·제한 명령 의도
          │
          └──────────────► Diagnostic Bridge
                            capture·후보 분석·HTTP/WebSocket
                                      │
                                      ▼
                                   휴대폰

Controller와 Communicator 사이에는 ESP-NOW 양방향 링크를 사용한다. Diagnostic Bridge는 별도 peer로 관찰 stream을 받으며, 차량 CAN 송신 경로에는 연결하지 않는다. broadcast raw telemetry와 무제한 multi-hop mesh는 사용하지 않는다.

## 3. 책임과 신뢰 경계

| 계층 | 허용 책임 | 금지 책임 |
|------|-----------|-----------|
| Controller | 운전자에게 검증된 상태 표시, 입력 검증, local 설정, 의미 있는 명령 요청 | 임의 CAN frame 생성, 최종 safety 판정, 후보 신호의 확정 표시 |
| Communicator ESP32 | ESP-NOW 인증·session·queue·peer 권한·UART routing | DBC 의미 해석, CAN peripheral 직접 제어, 최종 TX 허용 |
| Communicator STM32 | CAN timestamp/error, generated profile, local safety 재검사, 제한 executor | 무선 payload를 그대로 신뢰, raw replay, UI 상태를 차량 사실로 사용 |
| Diagnostic Bridge | read-only inventory·capture·candidate·evidence·웹 진단 | control lease, vehicle CAN TX, raw replay |
| CAN PHY | 논리 신호와 물리 bus 변환, standby·fault 처리 | bitrate·signal 의미·제어 권한 결정 |

차량 CAN 송신은 다음 네 요소가 동시에 맞아야 한다.

1. 검증된 vehicle profile과 command builder
2. 최신이고 유효한 STM32 local safety snapshot
3. Primary Controller의 control lease와 인증된 명령
4. build mode·전원·외부 watchdog·hard TX gate의 허용 상태

하나라도 빠지면 송신하지 않고 이유와 revision을 기록한다.

## 4. 안전 상태와 build mode

| build mode | 허용 동작 | 실제 차량 연결 |
|------------|-----------|----------------|
| CAPTURE_ONLY | CAN 수신·통계·표시·capture | 수신 전용으로만 허용 |
| BENCH_TX | current-limited 전원과 HIL에서 제한 송신 | 차량 연결 금지 |
| VEHICLE_TX | 모든 hardware·firmware·profile·evidence gate 통과 뒤 제한 송신 | release qualification 후에만 허용 |

전원·reset·brownout·watchdog·통신 단절 시 기본 상태는 CAN 송신 차단이다. TCAN 채널은 standby, MAX3055는 Power-On Standby, 외부 TX gate는 off로 수렴해야 한다. firmware GPIO 초기화만으로 이 조건을 보장한다고 간주하지 않는다.

## 5. 저장소 구조

    protocol/                         — ESP-NOW/UART schema와 생성 ABI
    dbc/                              — upstream DBC 원본과 라이선스
    firmware/controller/              — Controller ESP-IDF/LVGL
    firmware/communicator/esp32/      — Communicator 무선 MCU
    firmware/communicator/stm32/      — STM32 CMake/FDCAN/UART
    hardware/communicator/            — pinmap·KiCad·BOM·전원 계산
    ui/prototype/                     — 화면 검토용 HTML prototype
    ui/lvgl/                          — LVGL 화면 코드
    ui/diagnostic-web/                — 모바일 진단 웹 prototype
    tests/                            — host·protocol·fault·HIL test
    docs/architecture/                — 상위 구조와 subsystem 문서
    docs/adr/                         — 결정 기록
    docs/runbooks/                    — 에이전트 운영 절차
    docs/tasks.md                     — 열린 task 요약
    docs/tasks/                       — task별 상세 문서

## 6. 문서 정본

| 목적 | 정본 |
|------|------|
| 전체 구현 기준 | [implementation-readiness.md](../implementation-readiness.md) |
| 장치·데이터 흐름 | 이 문서와 [system-architecture.md](../system-architecture.md) |
| 하드웨어 | [communicator-hardware.md](../communicator-hardware.md), [hardware-and-development.md](../hardware-and-development.md) |
| protocol | [esp-now-protocol.md](../esp-now-protocol.md), [communicator-uart-protocol.md](../communicator-uart-protocol.md) |
| 차량 신호·기능 | [can-signal-catalog.md](../can-signal-catalog.md), [feature-design.md](../feature-design.md) |
| 진단 웹 | [can-diagnostics-web.md](../can-diagnostics-web.md) |
| 결정 | [docs/adr/README.md](../adr/README.md), [docs/decisions.md](../decisions.md) |
| 진척 | [docs/resume.md](../resume.md), [docs/journal.md](../journal.md) |
| 작업 | [docs/tasks.md](../tasks.md)와 [docs/tasks/](../tasks/) 상세 문서 |
