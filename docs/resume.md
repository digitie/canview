# resume.md

## 현재 진척도

2026-09-06 OTA 변경: Communicator를 WROOM-1-N16R8로 변경하고 독립 ESP/STM reset, J31 서비스 인터록과 단방향 GPIO sense, 복구 버튼 회로를 생성했다. [단일 OTA 설계·독립 리뷰 기록](architecture/ota.md)에 브라우저 업데이트, 전원 차단 복구, Flash 배치, 승인 commit과 영속 버전 정책을 모았다. 회로 ERC/정합성·host 회귀는 통과했으나 실제 OTA 펌웨어·PCB·HIL은 미구현/미검증이다. 현재 환경의 target VerifyOnly는 CMake 부재로 실패했으며 다음 toolchain 작업은 유지한다.

2026-09-05 기준, CANView는 설계·문서·정적 prototype·기존 host 자동화 test와 최신 target build bootstrap을 포함한 구현 준비 단계다. 문서 정보구조 변경은 [2인 독립 적대적 리뷰](reviews/adversarial/2026-09-05-document-information-architecture.md)로 종결했다. 이번 [R1 하드웨어](hardware/r1/README.md)는 Communicator·Bridge·Controller adapter·원격 mic 네 보드의 상세 schematic/PDF/BOM/netlist/pinmap을 생성했고 KiCad10.0.6 ERC0개, 패드·연결 정합성 검사를 통과했다. 이전23개 ERC 기록을 대체한다. 제조사 land 원본·최신 PDF·PCB/전원/SI/HIL gate는 남아 있으며 제작 승인 상태가 아니다. 센서 protocol 확장에는 host codec·golden 시험이 있지만 실제 firmware에는 아직 통합되지 않았다.

개발 정본은 Windows PowerShell checkout이다. worktree는 필요할 때만 만들고 merge 또는 abandon 후 삭제하며, WSL/Linux는 보조 환경으로만 사용한다. `rovinax/embedded-skills`의 임베디드 개발 스킬 6개는 Codex 환경에 설치했다.

현재 차량 CAN 송신 판정은 NO-GO다. 이는 프로젝트 전체 중단이 아니라 실제 차량 bus에 제어 frame을 보내지 않는다는 뜻이다. CAN 수신·capture·UI·host protocol 개발은 계속할 수 있다.

## 다음 한 작업

T-001 재현 가능한 host toolchain과 CI를 수행한다. target SDK와 build scaffold는 먼저 최신 안정 버전으로 고정했다.

- 시작 문서: docs/tasks/T-001-host-toolchain-ci.md, docs/development/windows.md
- 확인 대상: CMake/Ninja, Arm GNU, C11 host test, Python validator, Node static checks, 생성물 검사
- 완료 조건: 반복 가능한 명령과 CI gate가 정의되고, 도구가 없는 항목은 차단 사유로 기록됨

하드웨어는 진행 중인 T-100의 MAX20040 land90-0409 원본 대조, 미확보/구판 PDF, 전원/SOA·부품 선정 gate부터 닫는다. T-100b의 실제 GNSS/INS·원격 mic·센서 protocol 통합은 필요한 선행 task와 실물 준비 후 수행한다.

## 현재 열린 핵심 경로

T-001 → T-002/T-004/T-005 → T-003/T-006 → T-102/T-200/T-300 → T-103/T-104/T-201 → T-202/T-203 → T-500 → T-501

하드웨어 경로는 T-100 → T-101이다. T-503의 차량 제어 evidence 전에는 오디오·SPORT 송신을 열지 않는다.

## 알려진 차단 조건

- 검토 schematic/BOM과 firmware 독립 TX gate 회로는 있으나 승인 PCB·실물 fault evidence가 없음
- ESP-NOW/UART v1.3/v1.0 전체 ABI와 생성 codec이 아직 동결되지 않음
- 2017 Tucson TL의 실제 bus 종류·bitrate·connector·신호가 미확정
- 완성 target firmware와 HIL/fault evidence가 없음
- Windows host에 CMake `4.4.3`, Ninja `1.13.2`, Arm GNU `15.3.Rel1`이 설치되어 있지 않을 수 있음
- KiCad ERC/정합성은 통과했으나 MAX20040 footprint PROVISIONAL, PCB/routing/thermal/SI/transient 검증 미완료
- ESP-IDF `v6.0.3`와 STM32CubeG4 `v1.6.3`은 setup script 대상이지만 현재 세션에서 target build를 실행했다는 증거는 없음

## 문서 정본

- 문서 지도: docs/README.md
- 설계: docs/architecture/implementation-readiness.md
- 아키텍처: docs/architecture/README.md
- 작업: docs/tasks.md와 docs/tasks/
- 결정: docs/decisions.md와 docs/adr/
- 절차: docs/runbooks/
- 리뷰 이력: docs/reviews/
- 로그: docs/journal.md
