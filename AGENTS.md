# AGENTS.md

## 목표

canview는 현대·기아·제네시스 차량의 CAN 데이터를 수집·표시하고, 차량 기능을 제어하는 임베디드 시스템이다.

- **원천 데이터**: commaai/opendbc의 공개 DBC와 실제 차량에서 익명화·승인된 capture/evidence를 함께 사용한다.
- **기본 운용**: Waveshare ESP32-S3-Touch-LCD-3.5 기반 Controller가 운전자 화면을 제공하고, 별도 Communicator가 최대 3개 CAN bus를 수집한다.
- **진단 운용**: 별도 Diagnostic Bridge가 read-only CAN 관찰·capture·모바일 웹 진단을 담당한다.
- **안전 경계**: 차량 CAN 송신은 생성된 profile, STM32 local safety check, control lease, hardware TX gate와 단계별 시험을 모두 통과한 경우에만 허용한다.

## Think Before Coding

- 요청이 모호할 때는 해석을 조용히 정하지 말 것
- 중요한 가정은 숨기지 말고 드러낼 것
- 해석에 따라 구현 방향이 크게 달라지면 그 차이를 먼저 표면화할 것
- 안전하게 진행하기 어려울 정도로 혼란스러우면 추측하지 말고 확인할 것

## Simplicity First

- 요청을 완전히 해결하는 최소한의 코드만 작성할 것
- 요청되지 않은 기능을 추가하지 말 것
- 일회성 용도를 위해 추상화를 만들지 말 것
- 구체적인 필요 없이 설정 가능성이나 유연성을 늘리지 말 것
- 구현이 문제에 비해 커졌다고 느껴지면 줄일 것

## Surgical Changes

- 요청을 처리하는 데 필요한 코드만 변경할 것
- 작업이 요구하지 않으면 주변 로직까지 다시 쓰지 말 것
- 관련 없는 코드의 포맷, 이름, 스타일을 건드리지 말 것
- 사용자가 더 넓은 변경을 원한 것이 아니라면 기존 패턴을 맞출 것
- 관련 없는 문제를 발견하면 패치에 섞지 말고 따로 언급할 것

## Goal-Driven Execution

- 모호한 요청을 구체적이고 검증 가능한 결과로 바꿀 것
- 버그 수정은 재현 없이 바로 신뢰하지 말 것
- 리팩터링은 동작 보존을 전제로 전후 기대를 확인할 것
- 넓고 막연한 점검보다 목적이 분명한 검증을 선호할 것
- 완전한 검증이 불가능하면 무엇이 아직 미검증인지 밝힐 것

## Practical Bias

- 비단순 작업에서는 성급함보다 신중함을 우선할 것
- 변경 내역은 리뷰 가능한 범위와 요청 범위에 가깝게 유지할 것
- 아주 단순하고 명백한 한 줄 작업은 과하게 무겁게 다루지 말 것

## Ruthless Review

- 코드가 동작한다는 이유만으로 검증이 끝났다고 여기지 말 것
- 적대적 리뷰어로 세워 숨겨진 취약점과 부작용을 집요하게 찾아낼 것
- 당연하다고 믿은 가정을 의심하고 코드가 실패하는 시나리오를 찾을 것
- 숨겨진 부작용과 취약점이 소명되기 전까지는 완료로 보지 말 것

## 문서 언어 정책

이 저장소의 모든 Markdown/RST 문서는 한글로 작성한다. 공식 API 필드명, 코드 식별자, 명령어, URL, 제공자 원문처럼 그대로 보존해야 하는 값만 영어를 유지한다. 새 문서나 기존 문서를 수정할 때도 이 규칙을 우선한다.
저장된 DBC는 upstream 정의를 수정하지 않고 보관한다. DBC에 있는 신호라도 1차 대상 차량에서 확인되기 전에는 후보 또는 진단용으로만 취급하며, 차량 제어 profile에 자동 승격하지 않는다.

## 식별자 (혼동 방지)

| 항목 | 값 |
|------|----|
| GitHub 저장소 이름 | canview |
| Controller | Waveshare ESP32-S3-Touch-LCD-3.5 |
| Communicator | ESP32-S3-MINI-1-N4R2 + STM32G474CEU6 |
| CAN PHY | TCAN1046AV-Q1 2채널 + MAX3055 1채널 |
| Diagnostic Bridge | 별도 ESP32-S3 read-only observer |
| ESP-NOW protocol | v1.3 구현 기준 |
| Communicator UART | v1.0, 4 Mbps, 8-N-1, RTS/CTS |
| build mode | CAPTURE_ONLY, BENCH_TX, VEHICLE_TX |
| 환경변수 prefix | CANVIEW_* |

## 개발 환경 정책 (Windows)

모든 개발 명령은 Windows 환경의 PowerShell 또는 Windows native 도구로 실행한다. Git for Windows, PowerShell, Windows Node.js/npm, Python, CodeGraph, KiCad CLI, CMake/Ninja, ESP-IDF, GNU Arm Embedded를 표준 경로로 삼는다. WSL/Linux shell과 `/mnt/...` 경로는 보조 검증 환경일 뿐 정본 개발 경로가 아니다.

- **메인 repo**: `F:/dev/canview` 또는 Windows 로컬 checkout을 사용한다. Git/CodeGraph 정본 경로에 Linux `/mnt/f/...`를 사용하지 않는다.
- **작업 worktree**: 평소에는 메인 checkout 또는 단일 작업 branch를 사용하고, 병렬 작업·격리·리뷰가 필요할 때만 `F:/dev/canview-wt/<agent>-<task>` 같은 임시 worktree를 만든다. 작업 또는 PR 종료 후 worktree를 제거하고 `git worktree prune`으로 메타데이터를 정리한다.
- **테스트/빌드**: Windows checkout에서 host test, CMake/Ninja, ESP-IDF, Node, KiCad 검증을 실행한다.
- **하드웨어 도구**: Windows KiCad GUI/CLI, ESP-IDF PowerShell 환경, GNU Arm Embedded, CMake/Ninja를 기준으로 한다. 생성 파일과 검증 명령은 Windows에서 재현 가능해야 한다.
- **Git 실행 기준**: Git for Windows만 정본 branch·commit·push·PR 준비에 사용한다. `.git`/worktree metadata가 WSL 경로를 가리키는 checkout을 정본으로 삼지 않는다.
- **CodeGraph 실행 기준**: CodeGraph가 연결된 환경에서는 branch 전환·pull·merge 뒤 `codegraph sync` 후 `codegraph status`를 순서대로 실행한다. 임시 worktree에서는 작업 시작 후 초기화하고 worktree 제거와 함께 로컬 상태도 폐기한다.
- **원천·capture 데이터**: DBC 원본은 `dbc/` 아래에 보관한다. 차량 capture, 키, provisioning 파일과 대용량 evidence는 익명화·승인된 fixture만 Git에 넣고 나머지는 local artifact로 둔다.
- **로컬 키**: `.env`, `sdkconfig.local`, provisioning 파일, 차량 식별 정보, `*.local.md` 등은 각 checkout/worktree에 복사하되 Git에 커밋하지 않는다.
- **펌웨어·UI 실행**: ESP-IDF와 CMake/Ninja build는 Windows 도구로 실행한다. 정적 UI prototype은 Windows Node의 `node --check`와 필요한 browser test로 검증한다.
- **Playwright**: 브라우저 e2e는 Windows Playwright를 기준으로 실행한다. 다른 환경을 fallback으로 사용한 경우 PR 설명이나 `docs/journal.md`에 사유와 명령을 남긴다.

## 에이전트 공용 runbook (필독)

docs/runbooks/ — Codex/Claude/Antigravity가 공유하는 운영 runbook. 작업 전 두 개는 훑는다:

- docs/runbooks/agent-workflow.md — 표준 1-PR 흐름(Windows checkout → branch → host/target gate → PR → CI green → 머지 → 임시 worktree 정리)과 문서 갱신 절차.
- docs/runbooks/agent-failure-patterns.md — 반복 실패 패턴(CMake/ESP-IDF 환경, KiCad library/파워 핀, UART/CAN timing, stale evidence, merge 충돌)과 회피·복구.

인덱스: docs/runbooks/README.md. 환경 1차 문서는 docs/dev-environment.md와 docs/agent-guide.md다. 설계 정본은 docs/architecture/architecture.md와 docs/implementation-readiness.md다.

## 필요 시 생성하는 작업 worktree와 CodeGraph

AI 에이전트는 고정 worktree를 보유하지 않는다. 기본 작업은 `F:/dev/canview`의 Windows checkout에서 수행하고, 병렬 작업·실험 격리·독립 리뷰처럼 별도 checkout이 실제로 필요한 경우에만 임시 worktree를 생성한다.

- 예: `git worktree add -b agent/codex-<task> F:/dev/canview-wt/codex-<task> origin/main`
- 같은 branch를 여러 worktree에서 checkout하지 않는다. branch 이름에는 `agent/<agent>-<task>`처럼 소유자를 넣는다.
- 작업이 merge 또는 abandon으로 끝나고 해당 worktree에 실행 중인 작업이 없을 때 `git worktree remove F:/dev/canview-wt/codex-<task>`를 실행한다. 이후 `git worktree prune`으로 stale metadata를 정리한다.
- Git worktree 생성·status·commit·push는 Windows Git으로 실행한다. 활성 작업 중인 worktree나 사용자의 미커밋 변경이 있는 worktree는 삭제하지 않는다.
- CodeGraph가 연결된 임시 worktree는 처음 사용할 때 `codegraph init -i`, branch 전환·pull·merge 뒤 `codegraph sync` → `codegraph status` 순서로 관리한다. worktree를 제거하면 해당 로컬 CodeGraph 상태도 함께 폐기한다.
- 동기화 상태를 확인할 CodeGraph 도구가 현재 세션에 노출되지 않으면 그 사실을 작업 로그에 남기고 가능한 CLI 점검으로 대체한다.
- `.codegraph/`와 `.claude/`는 로컬 상태·secret이므로 Git에 커밋하지 않는다.

작업 전에 반드시 다음을 읽는다:

1. README.md — 프로젝트 개요와 현재 상태
2. SKILL.md — DO NOT 룰, 도메인 어휘와 작업 규칙
3. docs/architecture/architecture.md — 장치·펌웨어·프로토콜·UI 관계
4. docs/resume.md — 현재 진척도와 다음 한 작업
5. docs/adr/README.md — 관련 ADR 인덱스
6. docs/tasks.md — 열린 task와 의존성

새 세션이나 환경 복구에서는 docs/dev-environment.md와 docs/runbooks/agent-failure-patterns.md를 함께 읽는다.

## 지시 우선순위

1. 사용자 요청
2. 이 AGENTS.md
3. SKILL.md
4. docs/architecture/architecture.md, docs/implementation-readiness.md, docs/adr/README.md, docs/agent-guide.md, docs/tasks.md
5. README.md 및 나머지 docs/
6. 기존 코드와 테스트
7. 최소한의, 되돌릴 수 있는 가정

## 절대 하지 말 것 (DO NOT)

1. **생성 ABI 우회 금지** — schema/source generator가 있는 protocol header와 생성물을 직접 불일치하게 수정하지 않는다.
2. **차량 raw TX API 금지** — Controller·ESP32·Diagnostic Bridge가 임의 arbitration ID/data를 STM32에 전달해 즉시 송신하게 만들지 않는다.
3. **차량 송신 gate 우회 금지** — CAPTURE_ONLY 또는 BENCH_TX의 제한을 firmware flag만으로 VEHICLE_TX로 바꾸지 않는다.
4. **후보 신호 승격 금지** — DBC 이름·단일 capture·추정 scale만으로 정상 UI 또는 safety 입력으로 승격하지 않는다.
5. **안전 조건 생략 금지** — sender precondition bitmask를 최종 권한으로 신뢰하지 않고 STM32가 local snapshot과 각 TX 직전에 다시 검사한다.
6. **공유 비밀키 금지** — 설치 전체가 공유하는 pairing secret을 두지 않고 pair별 root와 device-local key를 사용한다.
7. **reset 안전 상태 추측 금지** — MCU GPIO 초기화만으로 CAN TX 안전을 보장한다고 쓰지 않는다. 외부 gate·pull·전원 상태와 시험 근거를 함께 둔다.
8. **미확정 핀·부품 추측 금지** — KiCad symbol pin, package, MPN, connector pin을 문서만 보고 확정하지 않는다.
9. **DBC 원본 변경 금지** — upstream DBC를 수정하지 않고 profile과 evidence를 별도 파일로 둔다.
10. **민감정보 커밋 금지** — 차량 VIN, raw 위치정보, provisioning key, Wi-Fi secret, 개인 음성/capture 원본을 public Git에 넣지 않는다.
11. **가짜 netlist/BOM 금지** — 완성되지 않은 block diagram이나 부품 목록을 제작용 KiCad netlist/BOM이라고 표시하지 않는다.
12. **Diag 경로 제어 전환 금지** — Diagnostic Bridge에는 control lease와 차량 CAN TX 권한을 부여하지 않는다.
13. **UI freshness 은폐 금지** — stale, candidate, estimated, pending 값을 정상 확정값처럼 표시하지 않는다.
14. **대역폭·queue 한도 무시 금지** — observer/raw stream이 safety/control queue와 전체 ESP-NOW 예산을 잠식하지 않게 한다.
15. **생성된 테스트 없는 wire 변경 금지** — protocol field 변경은 golden vector, malformed input, version/capability 시험과 함께 변경한다.

## 외부 원문·제공자 사용 원칙

- Waveshare, Espressif, ST, TI, Analog Devices, commaai/opendbc 원문은 공식 URL, 버전/commit, SHA-256 또는 문서 revision을 기록한다.
- 원문에서 확인한 사실, 후보 해석, 실차에서 관찰한 evidence를 문서에서 분리한다.
- 공식 데이터시트의 package pin과 KiCad symbol pin을 대조하기 전에는 회로 연결을 확정하지 않는다.
- upstream DBC의 라이선스와 원본 파일은 보존하고, 로컬 차량 profile은 원본과 다른 경로에 둔다.
- 외부 서비스가 없어도 CAPTURE_ONLY와 host test를 수행할 수 있게 하며, 네트워크·차량·하드웨어가 필요한 검증은 미실행 사유와 남은 위험을 기록한다.

## 작업 후 체크리스트

- [ ] 관련 host/unit test 통과
- [ ] CMake/Ninja 또는 ESP-IDF build 통과(해당 task)
- [ ] KiCad ERC/netlist/BOM consistency 통과(하드웨어 task)
- [ ] DBC/profile validator 및 golden vector 통과(프로토콜·차량 task)
- [ ] 정적 UI node/browser test 통과(UI task)
- [ ] docs/journal.md에 작업 항목 추가
- [ ] docs/resume.md의 진척도와 다음 한 작업 갱신
- [ ] 의사결정이 있었다면 docs/decisions.md와 해당 ADR 갱신
- [ ] 열린 task 상태와 docs/tasks.md 갱신
- [ ] 사용자 가시 변경이면 CHANGELOG.md 갱신
- [ ] 차량 송신 관련 변경이면 gate/evidence/rollback을 PR에 명시

## Remote push 전 보안 감사 절차 (필수)

원격(git push)에 올리기 전 항상 아래를 수행한다.

1. **스테이징 diff 직접 확인** — git diff --staged로 무엇이 올라가는지 눈으로 본다.
2. **비밀 패턴 스캔** — diff에서 secret/API 키/패스워드/DB DSN/private key/VIN/실차 식별정보를 grep으로 점검한다.
3. **로컬 비밀 파일 확인** — .env, .env.local, sdkconfig.local, provisioning, *.local.md, settings.local.json이 스테이징·추적되지 않았는지 git status로 확인한다.
4. **git add -A / git add . 금지** — 변경 파일을 명시적으로 add 한다.
5. **보안 민감 변경**(인증·session·key·권한·암호화)이면 가능한 보안 리뷰를 변경 diff에 수행하고 결과를 반영한다.
6. 의심되면 push하지 말고 사용자에게 확인한다.

## 검증

저장소 상태:

    git status --short --branch
    git diff --check

host tests (PowerShell):

    cmake -S tests/automation -B build/automation -G Ninja
    cmake --build build/automation
    ctest --test-dir build/automation --output-on-failure

STM32 firmware(STM32CubeG4_ROOT가 준비된 경우):

    cmake --preset debug -S firmware/communicator/stm32
    cmake --build --preset debug

정적 prototype:

    node --check ui/prototype/prototype.js
    node --check ui/diagnostic-web/prototype.js

Python 검증기가 추가된 경우 (PowerShell):

    py -3 -m pytest -q
    py -3 -m ruff check .

도구·SDK·하드웨어가 없는 경우 명령을 억지로 성공 처리하지 말고, 실행하지 못한 이유와 영향을 받은 gate를 기록한다.
