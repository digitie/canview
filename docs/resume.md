# resume.md

## 현재 진척도

2026-09-13 T-007 후속: Python 조립→C prefix/실제 P256 서명 검증230건,
Windows Debug/Release125/125와 portable 경계 ASan/UBSan을 통과했다. 앞선
CBOR checkpoint6d83962의 A/B static PASS 원문은 보존했다. 이번 서명 코드는
그 리뷰 범위 밖이다. 다음은 같은 경로의 manifest 대상/길이/호환성·image 검증이다.
전체 T-007은 IN_PROGRESS, PR #35 Draft이며 실제 target 연결·최종 리뷰가 남았다.

2026-09-13 사용자 요청으로 [단순한 구현 우선](../AGENTS.md#2-작업-원칙)을 공통
작업 원칙에 기록했다. T-007은 C/Python CBOR 구조 검사와 host Debug/Release
123/123까지 진행했다. 다음은 작은 서명 파일 생성→C 검증 연결이며, 범용 기능을
늘리지 않는다. 서명·전체 container·target 연결·최종 2인 review는 미완료다.

2026-09-09 T-104 PR #33이 `d229772de77a48ae197e2ff1b4e55b6cef9a88ed`로 merge됐고
`origin/main` 및 candidate `3ff04b7`의 ancestry를 확인했다. 최종 CI `34335812873`
6/6, target 이미지 18/18 bytes/SHA-256·source6/6·target log21 compiler/linker/CMake
warning/error0이다. manifest SHA-256은
`1dd10972ccc9ec565f726ac21a58862ea10db704a576b67f9188cf1a421a0514`다.
A 재검토는 PR #33 한정 사용자 면제이며 이슈 #34 OPEN·원 P1 확인 debt·A-10 P2
DEFERRED를 유지한다. T-104 전체 DONE/physical gate 승인으로 해석하지 않는다.
GitHub Actions Node20→24 전환 안내는 build warning과 별개로 남아 있으며 owner
digitie/T-001의 다음 workflow 변경 전 action pin 검증 후속 항목이다.

다음 [T-007](tasks/T-007-ota-container.md)을 `codex/t007-ota-container`에서 시작했다.
STM32 boot의 공용 선행인 C99 bounded parser·서명 container/packager 범위다.
eFuse/option-byte·Flash erase/install·vehicle TX는 열지 않는다. 일반 2인 리뷰 규칙은
유지하며 target/서명/전체 container 검증 전 이 task를 DONE으로 표시하지 않는다.

2026-09-09 T-104 [최신 review 정정·closure](reviews/adversarial/2026-09-09-T-104-04.md):
조회 누락이었던 A-10/B-11 완료 보고서를 복구했고 원문을 보존했다. B-12/B-13은
`6b33a59`의 각 원 finding을 PASS로 확인했다. A-08 서비스 차단과 원 P1 확인 debt는
별개이며 PR #33 한정 사용자 면제만 적용한다. A-10 안전 억제 회귀 공백 P2는
이슈 #34/T-104에 owner·시점·gate를 지정해 defer했다. 이 기록 commit의 최신 CI와
artifact 확인 후 merge하며, merge 확인 다음에는 T-007 OTA container를 시작한다.
T-104는 DONE이 아니며 issue #34 OPEN·physical NOT_RUN·차량 TX NO-GO 유지다.

아래는 이전 상태 기록이다.

2026-09-09 사용자 지시로 T-104 **PR #33 한 건의 A 재검토 gate를 면제**하고,
CI·B 후속 확인 후 source merge와 다음 software Task 진행을 허용했다.
[이슈 #34](https://github.com/digitie/canview/issues/34)에 A-08 service 중단 원문,
P1/P2 미완료 확인 범위·수정 근거·owner·후속 gate를 상세 기록했다.
[최신 기록](reviews/adversarial/2026-09-09-T-104-03.md)의 A는 `INCOMPLETE/BLOCK`이며
면제는 PASS나 P1 closure가 아니다. B-09는 기존 RB-01~04 CLOSED, P3 두 건의
수정을 조건으로 `CONDITIONAL`이다. DMAMUX fake/SDK 독립 대조와 watchdog
문서를 수정했다. 최종 CI/B 확인·merge는 아직 대기하며 T-104는 `DONE`이 아니다.
후속 PR의 일반 2인 리뷰 규칙과 물리/HIL `NOT_RUN`, 차량 CAN TX `NO-GO`는 유지한다.

아래는 위 사용자 지시 이전의 실행 이력이다.

2026-09-09 T-104 PR #33은 Draft이며 수정 candidate `14ea3c9`를 push했다. 기존 candidate
`bdc6798`의 A-05/B-07 원본 verdict는 모두 `BLOCK`이다. stale TX completion의
reset 후 재사용, tick 전 만료 COMMIT, RX 오류의 unread byte 누락, zero HELLO
build ID와 SDK mutation oracle·coverage 설명을 수정했다. Windows Debug/Release
각각 120/120, STM32 Debug/Release clean target·warning/error 0, ELF/BIN build ID
대조, STM32/ESP32/shared coverage와 strict API 문서가 통과했다. WSL GCC와
ASan/UBSan도 각각 120/120이다. A-08 재검토는 reviewer 서비스의 보안 제한으로
중단되어 `INCOMPLETE/BLOCK`이며 [service 원문](reviews/adversarial/evidence/2026-09-09-T-104-reviewer-a-08-service-block.md)에
보존했다. 제한을 우회하거나 P1 closure를 대신하지 않는다. 다음 작업은 reviewer
서비스 접근 문제 해결 후 원 A의 재확인과 최신 CI/artifact gate closure다.
physical/HIL·실제 24시간 UART·차량 gate는 `NOT_RUN`, 차량 CAN TX는 `NO-GO`다.

2026-09-09 T-103 PR #32가 merge commit `b17bdfc0bb2a1bfa9d300c1e7662cac05c96df40`으로
`origin/main`에 통합됐다. 3채널 FDCAN capture-only software/target/review/CI
closure는 끝났지만 실제 board·전원·FDCAN·차량 gate는 여전히 `NOT_RUN`이다.
T-104 STM32 UART DMA/link/idempotency C 구현을 `codex/t104-stm32-uart-control`
에서 시작했다. T-004와 T-102의 protocol/platform 선행은 main에 있으며, 현재
`CAPTURE_ONLY` 경계에서는 UART가 명령을 운반해도 lease 발급·raw CAN TX·vehicle
replay를 수행하지 않는다.

2026-09-09 T-103 STM32 3채널 FDCAN capture-only C source와 초기 적대적 리뷰 finding
수정을 진행 중이다. module batch는 callback 이후 transactional commit과 channel별
timestamp epoch를 사용하고, PSR/ECR snapshot은 frame timestamp를 덮어쓰지 않는다.
G474 CMSIS adapter는 RX flag 선행 acknowledge, FIFO/raw-ring loss latch, singleton
owner와 stop/start session reset을 갖는다. no-TX JSONL analyzer도 bounded schema/
duplicate/sequence/complete/TX-gate 검증으로 fail-closed하게 보강했다.

source fix candidate `3e13b2ca6e72a3aec5a32a6357285c614bc191f9`에서 T103 focused CTest
2/2, 전체 Windows CTest 118/118, no-TX helper 6/6, WSL 일반 clone ASan/UBSan 전체
CTest 118/118, STM32 module 및 fake-register adapter coverage function 100%/line≥95%/
branch≥90%, STM32 Debug/Release clean target build와 warning/error scan 0건을
확인했다. immutable post-fix reviewer 2명과 CI closure가 남아 있다. 실제 board
flash·ST-LINK·GPIO/PHY·bitrate·IRQ latency·reset/brownout·CAN analyzer·차량 capture는
`NOT_RUN`, 차량 CAN TX는 `NO-GO`다.

2026-09-08 T-500 protocol/CAN fault bench와 HIL harness는 최종 candidate
`ff3121ce04328ff61a73f13492f8be9927f0dc98`에서 A-10/B-10 독립 reviewer
`PASS`, unresolved P0/P1/P2/P3 0건, CI `34235313714` 6/6 success 후 PR #30
merge commit `8f5d97ff924fe7fdb757a3a86a30cde5a80c2a09`으로 `origin/main`에
통합됐다. focused unit 46/46, 전체 Python 97/97, host 12/12, evidence
validator, sanitizer, document link와 plan 검사를 통과했다. g2 read-only rig와
physical/HIL·flash·전원/reset/brownout·CAN analyzer·차량 bus·provisioning은
`SKIPPED` 또는 `NOT_RUN`이며 차량 CAN TX는 `NO-GO`다. T-500은 harness 준비
범위에서 `DONE`이고 physical G2/G4·차량 승인은 아니다.

2026-09-08 T-102 STM32 source foundation의 최종 candidate는 `18941170ef475777c62db2f1471b74f937c807ea`이다. generated hardware digest·forced CAPTURE_ONLY build contract/link anchor/source TX gate·reset reason·stack watermark·service policy skeleton·version 2 40-byte diagnostic record·cooperative scheduler를 연결했고, metadata assembly는 BSP provider target으로 분리했다. C preprocessing phase-order source gate와 mutation regression까지 반영한 뒤 Host Debug/Release 116/116, Clang ASan/UBSan 116/116, TSan pool 1/1, coverage, generator/sdkconfig/plan/link, Doxygen/Sphinx strict, STM32 Debug/Release clean-first target ELF/MAP/BIN/HEX 및 warning/error 0을 확인했다. 독립 reviewer A-005/B-005는 모두 PASS이고 unresolved P0/P1/P2/P3는 0건이다. GitHub Actions `34216963785`와 최종 문서 closure head `34218499019`의 6개 job 및 target artifact/warning gate가 PASS했고, PR #29는 merge commit `50410ba23fcecfa1f28cea837d04a061c201d648`으로 `origin/main`에 통합됐다. 실제 board flash/HIL·clock/reset/rail/brownout·UART/FDCAN 계측·Flash root 배치·차량 CAN은 `NOT_RUN`이다. FDCAN/UART 송수신과 차량 CAN TX는 활성화하지 않는다.

2026-09-08 최신 T-400 source candidate는 `586127450d14b8ef5a59f90edd1c49947b866bb7`이다. `7479cf1`에서 모든 qualification job을 immutable PR-head checkout으로 고정했고, `5861274`에서 Diagnostic Bridge의 HTTPD queue blocking을 명시적으로 비활성화하여 generator·validator·mutation test까지 연결했다. 실제 ESP-IDF 6.0.3 Bridge build, host/config/security/browser regression과 GitHub Actions `34196236147` 여섯 job이 성공했다. target manifest는 source/expected/base revision을 일치시키고 STM32 Debug/Release·Communicator ESP32·Diagnostic Bridge·Controller의 BIN/ELF/MAP 18개를 `18/18` bytes/SHA-256로 검증했으며, target warning/error scan은 0건, Windows checkout source provenance는 6/6이다. 독립 Reviewer A 실행 `CV-HOSTILE-20260908-POSTFIX-586`과 Reviewer B 실행 `c3636fd3-5ab5-4637-8a9b-2cd813359631`은 모두 source P0/P1 없이 `CONDITIONAL`로 완료했고, 통합 결과는 [T-400-02 review](reviews/adversarial/2026-09-08-T-400-02.md)에 보존했다. 물리 board/HIL, live endpoint, production provisioning, vehicle integration은 `NOT_RUN`이며 차량 CAN TX는 `NO-GO`다.

2026-09-08 T-400 source/CI closure 후 다음 구현 task를 DAG와 장치 순서로 확인해 [T-102](tasks/T-102-stm32-platform.md)를 `IN_PROGRESS`로 시작했다. T-102a와 공용 선행은 main에 merge되어 있으며, 이번 source-only branch에서는 STM32 C99 platform/clock/watchdog/cooperative scheduler를 구현한다. 실제 G1 board, clock/reset/rail 계측과 HIL은 `NOT_RUN`이고 FDCAN/UART 및 차량 CAN TX는 활성화하지 않는다.

2026-09-08 이전 source-only 상태: 사용자가 `G1 이전 fw 구현 허용`과 `C로 작성`을 명시해 `codex/t400-bridge-web-bootstrap`에서 T-400의 C source-only web bootstrap을 진행 중이다. `canview_bridge_auth` C99 상태기계, ESP-IDF `esp_http_server`/`cJSON`/WebSocket 기반 local shell, fixed-buffer DNS, NVS read-only credential load, GPIO4 service window과 read-only empty snapshot을 구현하고 deferred watchdog, 상태/I/O lock·credential zeroize·malformed input 경계를 보강했다. 당시 pushed candidate `41fdc99`에는 인증과 activity 기록의 원자적 lock 경계, DNS teardown 의존 순서 보존, startup cleanup retry와 cleanup 실패 재부팅, logout/expiry 이후 pre-auth 15초 deadline 재무장, custom socket close, 단일 owner를 위한 LRU purge 비활성화가 반영됐다. 실제 ESP-IDF 6.0.3 target build와 `size-components`, host focused 7/7, 이전 host 112/113(24시간 `uart-fault-stream` 제외), ESP32 core coverage, Python/config/generator gate를 통과했다. Windows에서는 sanitizer preset이 의도적으로 거부되므로 current sanitizer는 Linux CI에서 확인한다. 이 예외는 physical/HIL gate나 CAN TX 권한을 열지 않는다.

2026-09-08 이전 bounded 실행은 `INCOMPLETE/BLOCK`이었다. `19efeed`에 대한 fresh A는 `CONDITIONAL`로 idle lifecycle 행동시험 P2를, fresh B는 인증 전 idle 갱신 P1과 cleanup retry P1 및 live/resource/browser/provenance P2/P3를 보고했다. 원문은 [A 19efeed evidence](reviews/adversarial/evidence/2026-09-08-T-400-reviewer-a-final-19efeed-raw.md)와 [B 19efeed evidence](reviews/adversarial/evidence/2026-09-08-T-400-reviewer-b-final-19efeed-raw.md)에 보존했다. `bf7a11c`에 대한 독립 A/B raw report도 각각 P1을 포함한 `BLOCK`으로 보존했으며, [A BF7 evidence](reviews/adversarial/evidence/2026-09-08-T-400-reviewer-a-final-bf7a11c-raw.md)와 [B BF7 evidence](reviews/adversarial/evidence/2026-09-08-T-400-reviewer-b-final-bf7a11c-raw.md)에 기록했다. `c744799`에 대한 새 독립 A/B는 A가 logout/expiry pre-auth 재무장 P1과 cleanup P2, B가 LRU purge P1 및 live/browser P2를 보고해 `BLOCK`을 반환했으며 [A c744799 evidence](reviews/adversarial/evidence/2026-09-08-T-400-reviewer-a-final-c744799-raw.md)와 [B c744799 evidence](reviews/adversarial/evidence/2026-09-08-T-400-reviewer-b-final-c744799-raw.md)에 보존했다. `41fdc99`에서 두 P1과 cleanup 경계를 수정했으며, 새 immutable candidate에 대한 독립 A/B 재검토가 필요하다. 두 최신 raw verdict와 CI가 닫히기 전에는 ready/merge하지 않는다.

2026-09-07 [T-400a](tasks/T-400a-bridge-core-bench.md)는 [PR #23](https://github.com/digitie/canview/pull/23), merge `25eba080`으로 DONE이다. final CI `34114919104`의 Windows C99·target firmware·Linux GCC/Clang portability·ASan/UBSan 다섯 job, target artifact 18/18 path·size·SHA-256 대조, warning 0과 A-05/B-07 독립 review의 unresolved P0/P1 없음이 완료 조건을 충족했다. Bridge는 계속 read-only이며 P2 세 건은 T-400에 handoff했다. 실제 보드·ST-LINK/계측기가 없어 G1/G2 physical/HIL, 차량 CAN/capture, provisioning과 vehicle TX release는 `NOT_RUN`이고 CAN TX는 NO-GO다.

2026-09-06~07 기반 코드: [공용 C99 codec/app와 네 MCU 구조](architecture/firmware-foundation.md), 보드 pin/config 생성기, root CTest/독립 golden/BSP 실패 시험, coverage gate, Sphinx+Breathe+Doxygen API 문서를 추가했다. [실행 결과와 미실행 범위](development/foundation.md)를 구분한다. 기존 v1.2 prototype은 host 회귀에만 남기며 실제 CAN/radio/OTA는 시작하지 않는다. T-001은 PR #17(`74d43ff`)로, T-002는 PR #18(`c18a8a5`)로, T-003은 ESP-NOW codec/session/QoS와 target build·2인 적대적 리뷰 후 PR #19(`4ee017b`)로 main에 merge되어 `DONE`이다.

2026-09-06 전체 계획 재점검: 1차 전체 읽기와 2차 요구/task/정본 대조로 [42개 요구 추적표](architecture/requirements-coverage.md)와 46개 상세 task를 정리했다. OTA 8개 구현 단계, PCB 제작 gate와 오디오/SPORT의 수신 조사→bench 송신 순환 의존성을 보완했다. 운전자·진단 웹 각 5뷰와 LVGL을 개선하고 밝기/음량/SPORT host 결함을 수정했다. 작성자 검증과 최종 독립 2인 리뷰의 범위는 새 review 기록으로 추적한다. 이는 제품 전체 구현 완료가 아니다.

2026-09-06 OTA 변경: Communicator를 WROOM-1-N16R8로 변경하고 독립 ESP/STM reset, J31 서비스 인터록과 단방향 GPIO sense, 복구 버튼 회로를 생성했다. [단일 OTA 설계·독립 리뷰 기록](architecture/ota.md)에 브라우저 업데이트, 전원 차단 복구, Flash 배치, 승인 commit과 영속 버전 정책을 모았다. 회로 ERC/정합성·host 회귀는 통과했으나 실제 OTA 펌웨어·PCB·HIL은 미구현/미검증이다. 당시 일반 PATH의 target VerifyOnly는 CMake 부재로 실패했으며 이후 고정 host tool과 target SDK를 직접 설치해 검증했다.

2026-09-05 기준, CANView는 설계·문서·정적 prototype·기존 host 자동화 test와 최신 target build bootstrap을 포함한 구현 준비 단계다. 문서 정보구조 변경은 [2인 독립 적대적 리뷰](reviews/adversarial/2026-09-05-document-information-architecture.md)로 종결했다. 이번 [R1 하드웨어](hardware/r1/README.md)는 Communicator·Bridge·Controller adapter·원격 mic 네 보드의 상세 schematic/PDF/BOM/netlist/pinmap을 생성했고 KiCad10.0.6 ERC0개, 패드·연결 정합성 검사를 통과했다. 이전23개 ERC 기록을 대체한다. 제조사 land 원본·최신 PDF·PCB/전원/SI/HIL gate는 남아 있으며 제작 승인 상태가 아니다. 센서 protocol 확장에는 host codec·golden 시험이 있지만 실제 firmware에는 아직 통합되지 않았다.

개발 정본은 Windows PowerShell checkout이다. worktree는 필요할 때만 만들고 merge 또는 abandon 후 삭제하며, WSL/Linux는 보조 환경으로만 사용한다. `rovinax/embedded-skills`의 임베디드 개발 스킬 6개는 Codex 환경에 설치했다.

현재 차량 CAN 송신 판정은 NO-GO다. 이는 프로젝트 전체 중단이 아니라 실제 차량 bus에 제어 frame을 보내지 않는다는 뜻이다. CAN 수신·capture·UI·host protocol 개발은 계속할 수 있다.

## 다음 한 작업

현재 구현은 [T-007](tasks/T-007-ota-container.md) OTA-01이다. T-001 선행은 main에
있다. 처음에는 CBOR bounded primitive와 schema를 구현하고 C/Python differential,
signed synthetic fixture와 실제 target build까지 순서대로 연결한다. parser 결과는
writer/erase/install/boot selector 권한이 아니다. T-104 이슈 #34 debt는 별도 유지한다.

- 현재 문서: docs/tasks/T-007-ota-container.md, docs/architecture/ota.md §7–8, docs/development/windows.md, docs/runbooks/agent-workflow.md
- 다음 검증 순서: C/Python bounded parser·schema/golden → 서명·negative/differential·sanitizer/coverage → 실제 target warning0 → 독립 reviewer 2명 → CI/merge. physical/HIL은 NOT_RUN이다.
- Diagnostic Bridge의 read-only 경계는 T-400 전체에서 유지한다. control lease, raw replay, vehicle TX는 범위 밖이다.

하드웨어는 진행 중인 T-100의 MAX20040 land90-0409 원본 대조, 미확보/구판 PDF, 전원/SOA·부품 선정 gate부터 닫는다. 다음 PCB 제작 입력은 T-100a, 조립품 실측은 T-101이다. T-100b의 실제 GNSS/INS·원격 mic·센서 protocol 통합은 필요한 선행 task와 실물 준비 후 수행한다.

## 현재 열린 핵심 경로

의존성 정본은 [task 요약](tasks.md)의 DAG다. 공용 시험 rig T-500을 각 실측의 선행으로 제공하고, T-503/T-505a 수신 evidence → T-106 executor → T-503a/T-505 bench·차량 승인을 구분한다. OTA는 T-007부터 별도 boot/recovery/config/web/provisioning과 T-508 단전 시험을 거쳐 T-506으로 합류한다.

## 알려진 차단 조건

- 검토 schematic/BOM과 firmware 독립 TX gate 회로는 있으나 승인 PCB·실물 fault evidence가 없음
- T-003/T-004 codec은 main에 통합됐으며 실제 UART DMA/무선/CAN runtime과 보드 검증은 후속 task에 남음
- 2017 Tucson TL의 실제 bus 종류·bitrate·connector·신호가 미확정
- 완성 target firmware와 HIL/fault evidence가 없음
- 일반 PowerShell PATH만으로는 도구를 찾지 못할 수 있다. pinned CMake4.4.3/Ninja1.13.2/Arm15.3.1/CubeG4 1.6.3을 명시한 STM32 Debug/Release와 ESP-IDF v6.0.3의 세 ESP32 actual build는 통과했다. `setup-windows.ps1 -VerifyOnly`는 managed Git shell의 `basename`/`sed`/`git-sh-setup` 탐색 실패로 여전히 중단한다.
- KiCad ERC/정합성은 통과했으나 MAX20040 footprint PROVISIONAL, PCB/routing/thermal/SI/transient 검증 미완료
- ESP-IDF `v6.0.3`와 STM32CubeG4 `v1.6.3` checkout 및 Arm archive digest는 확인했고 post-fix target binary도 생성했다. CI, 실제 board flash/HIL 및 production security provisioning은 미실행
- candidate `5d6fac4`의 CI run `34112160182`는 target-firmware job과 Linux job은 성공했지만 Windows C99가 외부 Doxygen archive 빈 응답으로 실패한 역사 기록이다. 재실행 `34113636144`와 final branch run `34114919104`는 전체 success이며, latter가 PR #23 merge의 최종 CI evidence다.
- `.git` index/ref와 GitHub CLI ACL은 복구돼 commit/push/PR 조작이 가능하다. WSL `E_ACCESSDENIED`로 ASan/UBSan current 실행은 `NOT_RUN`.

## 문서 정본

- 문서 지도: docs/README.md
- 설계: docs/architecture/implementation-readiness.md
- 아키텍처: docs/architecture/README.md
- 작업: docs/tasks.md와 docs/tasks/
- 결정: docs/decisions.md와 docs/adr/
- 절차: docs/runbooks/
- 리뷰 이력: docs/reviews/
- 로그: docs/journal.md
