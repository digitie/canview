# resume.md

## 현재 진척도

2026-09-05 기준, CANView는 설계·문서·정적 prototype·기존 host 자동화 test를 포함한 구현 준비 단계다. PR #14에서 통합 설계, 두 명의 적대적 리뷰 결과, 34개 상세 task가 main에 반영됐다.

현재 차량 CAN 송신 판정은 NO-GO다. 이는 프로젝트 전체 중단이 아니라 실제 차량 bus에 제어 frame을 보내지 않는다는 뜻이다. CAN 수신·capture·UI·host protocol 개발은 계속할 수 있다.

## 다음 한 작업

T-001 재현 가능한 host toolchain과 CI를 수행한다.

- 시작 문서: docs/tasks/T-001-host-toolchain-ci.md, docs/dev-environment.md
- 확인 대상: CMake/Ninja, C11 host test, Python validator, Node static checks, 생성물 검사
- 완료 조건: 반복 가능한 명령과 CI gate가 정의되고, 도구가 없는 항목은 차단 사유로 기록됨

병렬로 T-100 Communicator 회로도·BOM·hard TX gate 설계를 시작할 수 있다.

## 현재 열린 핵심 경로

T-001 → T-002/T-004/T-005 → T-003/T-006 → T-102/T-200/T-300 → T-103/T-104/T-201 → T-202/T-203 → T-500 → T-501

하드웨어 경로는 T-100 → T-101이다. T-503의 차량 제어 evidence 전에는 오디오·SPORT 송신을 열지 않는다.

## 알려진 차단 조건

- 승인 KiCad schematic/PCB/BOM과 firmware 독립 TX gate가 없음
- ESP-NOW/UART v1.3/v1.0 전체 ABI와 생성 codec이 아직 동결되지 않음
- 2017 Tucson TL의 실제 bus 종류·bitrate·connector·신호가 미확정
- 완성 target firmware와 HIL/fault evidence가 없음
- 현재 환경에 따라 CMake, STM32CubeG4, ESP-IDF, KiCad CLI가 없을 수 있음

## 문서 정본

- 설계: docs/implementation-readiness.md
- 아키텍처: docs/architecture/architecture.md
- 작업: docs/tasks.md와 docs/tasks/
- 결정: docs/decisions.md와 docs/adr/
- 로그: docs/journal.md
