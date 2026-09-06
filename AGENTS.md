# AGENTS.md

## 1. 목표와 안전 경계

canview는 현대·기아·제네시스 차량의 CAN 데이터를 수집·표시하고, 차량 기능을 제어하는 임베디드 시스템이다.

| 장치 | 기준 하드웨어 | 책임 |
|---|---|---|
| Controller | Waveshare ESP32-S3-Touch-LCD-3.5 | 운전자 UI, 설정, 검증된 의미 명령 요청 |
| Communicator | ESP32-S3-WROOM-1-N16R8 + STM32G474CEU6 + 3 CAN PHY | CAN 수집, ESP-NOW/UART 전달, 최종 차량 송신 안전 판정 |
| Diagnostic Bridge | 별도 ESP32-S3 | read-only 관찰, capture, Signal Lab, 휴대폰 웹 |

1차 대상은 2017 Tucson TL 2.0 디젤 4WD BlueLink다. 공개 DBC와 실제 차량 evidence를 구분하며, DBC에 존재한다는 이유만으로 신호나 제어 기능을 확정하지 않는다.

차량 CAN 송신은 생성된 vehicle profile, STM32 local safety check, control lease, build mode, hardware TX gate와 단계별 시험을 모두 통과한 기능에만 허용한다. Controller는 승인된 음량·fader/balance·mute·rear mute·audio restore와 SPORT 전환 같은 의미 명령만 요청할 수 있고 raw CAN frame을 만들 수 없다.

## 2. 작업 원칙

- 요청이 모호하거나 해석에 따라 구현이 달라지면 중요한 가정을 드러내고 확인한다.
- 요청을 해결하는 최소 범위만 변경하고 관련 없는 코드·문서·형식을 건드리지 않는다.
- 사용자의 기존 변경과 dirty worktree를 보존한다. 충돌을 피할 수 없을 때만 사용자에게 알린다.
- 버그 수정은 재현과 회귀시험, 리팩터링은 동작 보존 근거, 설계 변경은 검증 가능한 수용 기준을 요구한다.
- 도구·SDK·하드웨어가 없어 실행하지 못한 gate를 통과로 표시하지 않는다.
- 구현보다 근거를 우선한다. 확인된 사실, 후보 해석, 실차 evidence를 서로 다른 상태로 관리한다.

### Ruthless Review

- 코드가 동작한다는 이유만으로 검증이 끝났다고 여기지 말 것
- 적대적 리뷰어로 세워 숨겨진 취약점과 부작용을 집요하게 찾아낼 것
- 당연하다고 믿은 가정을 의심하고 코드가 실패하는 시나리오를 찾을 것
- 숨겨진 부작용과 취약점이 소명되기 전까지는 완료로 보지 말 것

## 3. 문서 읽기 정책

`AGENTS.md`는 모든 작업에 적용되는 짧은 규칙의 정본이고, [docs/README.md](docs/README.md)는 상세 문서 라우터다. 문서 전체를 선제적으로 읽지 말고 아래 단계만 따른다.

### 반드시 참조

저장소 작업을 시작할 때 다음만 먼저 읽는다.

1. 이 `AGENTS.md`
2. [docs/README.md](docs/README.md) — 문서 분류와 정본 위치
3. [docs/resume.md](docs/resume.md) — 현재 상태, 다음 작업, 차단 조건
4. 구현 task가 정해졌다면 해당 `docs/tasks/T-NNN-*.md` 한 파일

task를 고르는 작업일 때만 [docs/tasks.md](docs/tasks.md)를 읽는다. 이미 task가 지정됐다면 전체 backlog를 다시 읽지 않는다.

### 필요할 때만 참조

| 상황 | 먼저 읽을 문서 | 다음 문서 |
|---|---|---|
| 구조·책임·데이터 흐름 변경 | [architecture/README.md](docs/architecture/README.md) | 그 문서가 가리키는 관련 상세 설계 한두 개 |
| branch·검증·PR·merge | [agent-workflow.md](docs/runbooks/agent-workflow.md) | 실패했을 때만 failure patterns |
| Windows toolchain·빌드 | [development/windows.md](docs/development/windows.md) | 대상 장치가 정해졌을 때만 toolchains |
| 하드웨어·차량·UI | [docs 문서 지도](docs/README.md) | 변경 대상 하위 디렉터리의 관련 문서 |
| 기존 결정을 변경 | [ADR 색인](docs/adr/README.md) | 관련 ADR 본문만 |
| 문서·task·journal 유지 | [documentation-maintenance.md](docs/runbooks/documentation-maintenance.md) | 필요한 기록 문서만 |

### 특수한 경우에만 참조

- `docs/reviews/`: 적대적 리뷰를 수행하거나 과거 finding을 추적할 때만 읽는다.
- `docs/journal.md`: 과거 작업의 원인·명령·결과를 추적할 때만 검색한다. 일반 작업 전부를 읽지 않는다.
- `docs/adr/` 전체: 구조적 결정의 이력을 감사할 때만 읽는다. 보통은 색인에서 관련 ADR 하나만 고른다.
- `docs/tasks/` 전체와 `docs/tasks-done.md`: backlog 감사나 의존성 재설계 때만 읽는다.
- raw DBC, capture, datasheet 전체: 해당 신호·회로·evidence를 검증할 때만 연다.

### 토큰 절약 규칙

- 인덱스 → 관련 상세 문서 → 필요한 절의 순서로 점진적으로 읽는다.
- `rg`로 제목·식별자·링크를 먼저 찾고, 관련 없는 긴 문서나 과거 로그를 통째로 읽지 않는다.
- 한 문서에서 답이 확인되면 단순 배경 링크를 연쇄적으로 따라가지 않는다.
- review·journal·ADR는 역사 기록이다. 현재 요구사항은 accepted ADR, architecture 정본과 열린 task에서 확인한다.
- 문서가 충돌하면 조용히 선택하지 말고 정본 우선순위와 충돌 위치를 밝힌다.

## 4. 문서 정본과 우선순위

지시 우선순위는 다음과 같다.

1. 사용자 요청
2. 이 `AGENTS.md`
3. accepted ADR
4. `docs/architecture/README.md`와 관련 상세 architecture 문서
5. 선택한 상세 task
6. 관련 hardware·vehicle·UI·development 문서
7. 코드와 테스트
8. review·journal 등 역사 기록
9. 최소한의 되돌릴 수 있는 가정

`SKILL.md`는 작업별 문서 라우터이며 별도의 정책 정본이 아니다. 구조적 결정은 ADR, 현재 설계는 architecture, 실행 범위와 수용 기준은 task, 반복 절차는 runbook에 둔다. 같은 내용을 여러 문서에 복제하지 않는다.

모든 Markdown/RST 문서는 한글로 작성한다. 공식 필드명, 코드 식별자, 명령어, URL, 제공자 원문은 필요한 범위에서 영어를 유지한다.

## 5. 개발·worktree·리뷰 진입점

- 정본 개발환경은 Windows PowerShell과 Windows native 도구다. 상세 설치·경로·검증 계층은 [Windows 개발환경](docs/development/windows.md)에만 둔다.
- 기본은 `F:/dev/canview`의 작업 branch다. 병렬 작업·격리·독립 리뷰가 실제로 필요할 때만 임시 worktree를 만들고, merge 또는 abandon 뒤 안전하게 제거한다. 명령과 CodeGraph 수명주기는 [agent workflow](docs/runbooks/agent-workflow.md)에만 둔다.
- 비단순 변경은 분야가 다른 전문 리뷰어 서브에이전트 2명의 독립 적대적 리뷰와 finding 반영을 거친다. 적용 범위·산출물·면제 조건은 [agent workflow](docs/runbooks/agent-workflow.md)와 [review archive](docs/reviews/README.md)를 따른다.

## 6. 절대 하지 말 것

1. schema/generator가 있는 protocol header와 생성물을 수동으로 불일치하게 만들지 않는다.
2. Controller·Communicator ESP32·Diagnostic Bridge에서 임의 CAN ID/data를 STM32로 보내 즉시 송신하게 만들지 않는다.
3. `CAPTURE_ONLY` 또는 `BENCH_TX` 제한을 firmware flag 하나로 `VEHICLE_TX`로 바꾸지 않는다.
4. DBC 이름·단일 capture·추정 scale만으로 신호를 정상 UI나 safety 입력으로 승격하지 않는다.
5. sender precondition을 최종 권한으로 신뢰하지 않는다. STM32가 local snapshot과 각 TX 직전에 다시 검사한다.
6. installation-wide 공유 pairing secret을 두지 않는다. pair별 root와 device-local key를 사용한다.
7. MCU GPIO 초기화만으로 reset·brownout CAN 안전을 주장하지 않는다. 외부 gate·pull·rail 상태와 시험 근거를 함께 둔다.
8. 데이터시트와 symbol을 대조하지 않은 pin·package·MPN·connector를 확정하지 않는다.
9. upstream DBC 원본을 수정하지 않는다. profile과 evidence는 별도 경로에 둔다.
10. VIN, raw 위치정보, provisioning key, Wi-Fi secret, 개인 음성/capture 원본을 public Git에 넣지 않는다.
11. 불완전한 block diagram이나 부품 목록을 제작용 KiCad netlist/BOM으로 표시하지 않는다.
12. Diagnostic Bridge에 control lease, raw replay 또는 차량 CAN TX 권한을 주지 않는다.
13. stale·candidate·estimated·pending 값을 정상 확정값처럼 표시하지 않는다.
14. observer/raw stream이 safety/control queue와 ESP-NOW 예산을 잠식하게 하지 않는다.
15. protocol wire 변경을 golden vector, malformed input, version/capability 시험 없이 완료하지 않는다.

## 7. 외부 원문과 evidence

- Waveshare, Espressif, ST, TI, Analog Devices, commaai/opendbc 원문은 공식 URL과 version/commit 또는 문서 revision을 기록한다.
- upstream DBC는 라이선스와 digest를 보존한다. 차량별 profile은 실제 capture evidence와 분리한다.
- datasheet package pin과 KiCad symbol pin을 대조하기 전에는 회로 연결을 확정하지 않는다.
- 네트워크·차량·하드웨어가 필요한 검증은 미실행 사유, 영향받는 gate와 후속 task를 기록한다.

## 8. 완료와 push

- 변경 범위에 맞는 host/target/UI/KiCad/profile 검증을 수행한다.
- 비단순 변경은 전문 리뷰어 2명의 finding을 수정, ADR 또는 task로 disposition하고 필수 검증을 다시 실행한다.
- 관련될 때만 `docs/resume.md`, `docs/tasks.md`, 상세 task, ADR, `docs/journal.md`, `CHANGELOG.md`를 갱신한다.
- push 전 staged diff를 직접 읽고 secret·VIN·key·private material과 로컬 파일이 포함되지 않았는지 확인한다. `git add -A`와 `git add .`은 사용하지 않는다.
- 실패한 검증, 미해결 P0/P1 finding 또는 닫히지 않은 차량 gate를 숨긴 채 완료·release 가능으로 표시하지 않는다.
