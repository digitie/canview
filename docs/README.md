# CANView 문서 지도

이 문서는 상세 문서를 고르는 단일 진입점이다. 현재 상태는 [resume](resume.md), 작업 범위는 [tasks](tasks.md), 시스템 설계는 [architecture](architecture/README.md)가 각각 정본이다.

## 읽기 단계

| 단계 | 읽는 경우 | 문서 |
|---|---|---|
| 반드시 | 모든 저장소 작업 | `AGENTS.md`, 이 문서, `docs/resume.md`, 해당 task가 있으면 그 상세 파일 |
| 필요 시 | 변경 분야가 정해졌을 때 | 아래 분야별 인덱스와 관련 상세 문서 한두 개 |
| 특수 | 리뷰·과거 원인·결정 이력·복구가 필요할 때 | `reviews/`, `journal.md`, 관련 ADR, failure patterns |

이미 task가 지정됐으면 `tasks.md` 전체를 읽지 않는다. task가 없는 문서·review 작업은 관련 runbook 또는 template을 진입점으로 삼는다. ADR·review·journal은 기본 요구사항이 아니라 이력 자료이므로 현재 작업과 직접 관련된 항목만 검색한다.

## 정본 관계

```text
AGENTS.md                     모든 작업의 규칙·금지선
└─ docs/README.md             문서 라우터
   ├─ architecture/           현재 시스템·protocol·기능 설계
   ├─ hardware/               보드·회로 상세
   ├─ development/            Windows 환경·장치별 toolchain
   ├─ vehicle/                대상 차량·DBC·signal evidence
   ├─ ui/                     화면 원칙·LVGL 벤치마크
   ├─ tasks.md + tasks/       실행 범위·수용 기준
   ├─ adr/ + decisions.md     구조적 결정과 변경 이력
   ├─ runbooks/               반복 작업 절차
   └─ reviews/ + journal.md   감사·작업 이력
```

- architecture는 현재 구현이 따라야 할 설계를 설명한다.
- ADR은 왜 그 설계를 선택했는지 기록하며, 뒤집을 때 이전 ADR을 삭제하지 않는다.
- task는 architecture를 구현하는 원자 범위와 acceptance를 정의한다. task가 architecture를 재정의하지 않는다.
- review는 특정 commit/diff에 대한 역사 기록이다. finding을 반영해 architecture·ADR·task를 갱신하되 과거 report를 현재 정본처럼 사용하지 않는다.
- runbook은 작업 방법만 설명하며 제품 설계를 결정하지 않는다.

## 아키텍처

먼저 [아키텍처 개요와 상세 지도](architecture/README.md)를 읽고 필요한 문서만 선택한다.

| 관심사 | 상세 문서 |
|---|---|
| 장치명·데이터 흐름·권한 | [system](architecture/system.md) |
| 구현 경계·gate·목표 구조 | [implementation readiness](architecture/implementation-readiness.md) |
| ESP-NOW·UART 계약 | [protocol index](architecture/protocols/README.md) |
| 독립 OTA·전원 차단 복구·N16R8 회로 | [OTA 설계 및 리뷰](architecture/ota.md) |
| Controller 수신·DBC pipeline | [controller CAN pipeline](architecture/controller-can-pipeline.md) |
| Diagnostic Bridge·모바일 웹 | [diagnostic bridge](architecture/diagnostic-bridge.md) |
| 4WD·DPF·audio·SPORT 기능 | [features](architecture/features.md) |
| 밝기·FFT 음량·SPORT 자동화 | [automation](architecture/automation.md) |

## 하드웨어

| 대상 | 문서 |
|---|---|
| Waveshare Controller, pinmap, RTC, board peripheral | [Controller hardware](hardware/controller.md) |
| Communicator 전원·CAN PHY·MCU pinmap·reset | [Communicator hardware](hardware/communicator.md) |
| R1 네 보드 상세 회로·GNSS/INS·원격 수음·검증 | [R1 상세 지도](hardware/r1/README.md), [KiCad 산출물](../hardware/README.md) |

제안 pinmap만으로 제작하지 않는다. 제작 정본은 승인된 KiCad schematic/netlist와 일치해야 한다.

## 개발환경

| 상황 | 문서 |
|---|---|
| Windows source-of-truth, branch/worktree 전제, 검증 계층 | [Windows environment](development/windows.md) |
| Controller·ESP32·STM32·DBC의 설치와 build | [device toolchains](development/toolchains.md) |

branch·PR·리뷰 절차는 development가 아니라 [agent workflow](runbooks/agent-workflow.md)에 둔다.

## 차량과 신호

| 관심사 | 문서 |
|---|---|
| 1차 대상과 capture 승격 기준 | [2017 Tucson TL](vehicle/target-2017-tucson.md) |
| 표시·제어 후보 signal과 근거 등급 | [signal catalog](vehicle/signal-catalog.md) |
| GPS·RTC·시간 source 조사 | [GPS/time investigation](vehicle/gps-time-investigation.md) |
| upstream DBC snapshot·license·digest | [DBC README](../dbc/README.md) |

## UI

| 관심사 | 문서 |
|---|---|
| 화면 정보 구조·token·상태·LVGL mapping | [UI design](ui/design.md) |
| 공식 LVGL demo 채택·제외 근거 | [LVGL demo review](ui/lvgl-demo-review.md) |

구현 세부는 [정적 운전자 prototype](../ui/prototype/README.md), [LVGL 구현](../ui/lvgl/README.md), [진단 웹 prototype](../ui/diagnostic-web/README.md)에서 이어서 확인한다.

## 작업·운영·이력

| 목적 | 문서 | 읽는 시점 |
|---|---|---|
| 열린 작업 선택·의존성 | [tasks](tasks.md) | task를 선택하거나 gate를 볼 때 |
| task 작성 규칙 | [tasks rule](tasks-rule.md) | task 생성·완료 때 |
| 표준 개발·2인 리뷰·PR | [agent workflow](runbooks/agent-workflow.md) | 실제 변경을 시작·종료할 때 |
| 문서·journal·ADR 유지 | [documentation maintenance](runbooks/documentation-maintenance.md) | 기록을 갱신할 때 |
| 반복 실패 복구 | [failure patterns](runbooks/agent-failure-patterns.md) | 실패가 발생한 뒤 |
| 구조적 결정 | [ADR index](adr/README.md) | 기존 결정을 변경할 때 |
| 적대적 리뷰 이력 | [review archive](reviews/README.md) | 리뷰 수행·finding 추적 때 |
| 작업 이력 | [journal](journal.md) | 과거 원인을 추적할 때 |

## 탐색 규칙

1. 이 인덱스에서 분야를 고른다.
2. 상세 문서의 제목과 목차를 먼저 확인한다.
3. 필요한 절만 읽고 관련 식별자는 `rg`로 찾는다.
4. 링크가 배경 참고인지 규범 정본인지 문서 서두에서 확인한다.
5. 이동·이름 변경 뒤에는 모든 Markdown local link와 plain path reference를 검사한다.
