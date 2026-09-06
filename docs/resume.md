# resume.md

## 현재 진척도

2026-09-07 T-102a는 [PR #21](https://github.com/digitie/canview/pull/21), merge `db5ed19`로 DONE이다. 최종 STM32/ESP32 binary6종 warning0·host74/74·독립2인 리뷰·CI5개를 [evidence](reviews/adversarial/evidence/2026-09-07-T-102a-merge.md)에 보존했다. 현재는 [T-200a](tasks/T-200a-esp32-core-bench.md) Communicator ESP32 core를 진행한다. 실제 보드·ST-LINK/계측기는 현재 PC에서 확인되지 않았으며 코드 검증과 G1/G2 계측을 분리한다.

2026-09-06~07 기반 코드: [공용 C99 codec/app와 네 MCU 구조](architecture/firmware-foundation.md), 보드 pin/config 생성기, root CTest/독립 golden/BSP 실패 시험, coverage gate, Sphinx+Breathe+Doxygen API 문서를 추가했다. [실행 결과와 미실행 범위](development/foundation.md)를 구분한다. 기존 v1.2 prototype은 host 회귀에만 남기며 실제 CAN/radio/OTA는 시작하지 않는다. T-001은 PR #17(`74d43ff`)로, T-002는 PR #18(`c18a8a5`)로, T-003은 ESP-NOW codec/session/QoS와 target build·2인 적대적 리뷰 후 PR #19(`4ee017b`)로 main에 merge되어 `DONE`이다.

2026-09-06 전체 계획 재점검: 1차 전체 읽기와 2차 요구/task/정본 대조로 [42개 요구 추적표](architecture/requirements-coverage.md)와 46개 상세 task를 정리했다. OTA 8개 구현 단계, PCB 제작 gate와 오디오/SPORT의 수신 조사→bench 송신 순환 의존성을 보완했다. 운전자·진단 웹 각 5뷰와 LVGL을 개선하고 밝기/음량/SPORT host 결함을 수정했다. 작성자 검증과 최종 독립 2인 리뷰의 범위는 새 review 기록으로 추적한다. 이는 제품 전체 구현 완료가 아니다.

2026-09-06 OTA 변경: Communicator를 WROOM-1-N16R8로 변경하고 독립 ESP/STM reset, J31 서비스 인터록과 단방향 GPIO sense, 복구 버튼 회로를 생성했다. [단일 OTA 설계·독립 리뷰 기록](architecture/ota.md)에 브라우저 업데이트, 전원 차단 복구, Flash 배치, 승인 commit과 영속 버전 정책을 모았다. 회로 ERC/정합성·host 회귀는 통과했으나 실제 OTA 펌웨어·PCB·HIL은 미구현/미검증이다. 당시 일반 PATH의 target VerifyOnly는 CMake 부재로 실패했으며 이후 고정 host tool과 target SDK를 직접 설치해 검증했다.

2026-09-05 기준, CANView는 설계·문서·정적 prototype·기존 host 자동화 test와 최신 target build bootstrap을 포함한 구현 준비 단계다. 문서 정보구조 변경은 [2인 독립 적대적 리뷰](reviews/adversarial/2026-09-05-document-information-architecture.md)로 종결했다. 이번 [R1 하드웨어](hardware/r1/README.md)는 Communicator·Bridge·Controller adapter·원격 mic 네 보드의 상세 schematic/PDF/BOM/netlist/pinmap을 생성했고 KiCad10.0.6 ERC0개, 패드·연결 정합성 검사를 통과했다. 이전23개 ERC 기록을 대체한다. 제조사 land 원본·최신 PDF·PCB/전원/SI/HIL gate는 남아 있으며 제작 승인 상태가 아니다. 센서 protocol 확장에는 host codec·golden 시험이 있지만 실제 firmware에는 아직 통합되지 않았다.

개발 정본은 Windows PowerShell checkout이다. worktree는 필요할 때만 만들고 merge 또는 abandon 후 삭제하며, WSL/Linux는 보조 환경으로만 사용한다. `rovinax/embedded-skills`의 임베디드 개발 스킬 6개는 Codex 환경에 설치했다.

현재 차량 CAN 송신 판정은 NO-GO다. 이는 프로젝트 전체 중단이 아니라 실제 차량 bus에 제어 frame을 보내지 않는다는 뜻이다. CAN 수신·capture·UI·host protocol 개발은 계속할 수 있다.

## 다음 한 작업

현재 branch는 `codex/t200a-esp32-core-bench`다. T-200a에서 SDK 독립 boot/health·고정 pool과 IDF watchdog/memory/USB 진단을 구현하고 실제 sdkconfig 부정 검사를 추가한다. 이전 T-102a는 merge 완료했으며 최신 사용자 요청에 따라 core/base부터 계속한다.

- 현재 문서: docs/tasks/T-200a-esp32-core-bench.md, docs/tasks/T-200-communicator-esp32-bootstrap.md, docs/hardware/r1/firmware-pinmap.md, docs/architecture/implementation-readiness.md §5.2, docs/development/windows.md
- 구현 순서: SDK 독립 boot/health·pool → BSP/IDF adapter·sdkconfig gate → host fault/GCC/coverage → STM32/ESP32 binary → 독립2인 리뷰·CI·merge.
- T-200a는 UART/radio/CAN/OTA를 시작하지 않는다. T-200의 실물 메모리·USB log·GPIO/reset 검증은 미실행으로 남긴다.

하드웨어는 진행 중인 T-100의 MAX20040 land90-0409 원본 대조, 미확보/구판 PDF, 전원/SOA·부품 선정 gate부터 닫는다. 다음 PCB 제작 입력은 T-100a, 조립품 실측은 T-101이다. T-100b의 실제 GNSS/INS·원격 mic·센서 protocol 통합은 필요한 선행 task와 실물 준비 후 수행한다.

## 현재 열린 핵심 경로

의존성 정본은 [task 요약](tasks.md)의 DAG다. 공용 시험 rig T-500을 각 실측의 선행으로 제공하고, T-503/T-505a 수신 evidence → T-106 executor → T-503a/T-505 bench·차량 승인을 구분한다. OTA는 T-007부터 별도 boot/recovery/config/web/provisioning과 T-508 단전 시험을 거쳐 T-506으로 합류한다.

## 알려진 차단 조건

- 검토 schematic/BOM과 firmware 독립 TX gate 회로는 있으나 승인 PCB·실물 fault evidence가 없음
- T-003/T-004 codec은 main에 통합됐으며 실제 UART DMA/무선/CAN runtime과 보드 검증은 후속 task에 남음
- 2017 Tucson TL의 실제 bus 종류·bitrate·connector·신호가 미확정
- 완성 target firmware와 HIL/fault evidence가 없음
- 일반 PowerShell PATH만으로는 도구를 찾지 못할 수 있다. foundation-windows.ps1와 setup-windows.ps1을 dot-source하면 Clang23.1.0/CMake4.4.3/Ninja1.13.2 및 직접 설치된 Arm15.3.Rel1/IDF6.0.3/CubeG4 1.6.3을 검증한다. target compile gate는 통과했지만 실제 보드/HIL은 미실행이다.
- KiCad ERC/정합성은 통과했으나 MAX20040 footprint PROVISIONAL, PCB/routing/thermal/SI/transient 검증 미완료
- ESP-IDF `v6.0.3`와 STM32CubeG4 `v1.6.3` checkout, Arm archive digest와 target binary는 확보했지만 실제 board flash/HIL 및 production security provisioning은 미실행

## 문서 정본

- 문서 지도: docs/README.md
- 설계: docs/architecture/implementation-readiness.md
- 아키텍처: docs/architecture/README.md
- 작업: docs/tasks.md와 docs/tasks/
- 결정: docs/decisions.md와 docs/adr/
- 절차: docs/runbooks/
- 리뷰 이력: docs/reviews/
- 로그: docs/journal.md
