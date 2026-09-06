# T-102a STM32 최소 boot/fault 기반과 host 검증

- 상태: `IN_PROGRESS`
- branch: `codex/t102a-stm32-core-bench`
- PR: [#21](https://github.com/digitie/canview/pull/21)
- 우선순위: `P0`
- Gate: `G0 / G1 준비`
- 선행: `T-001`

## 목표와 분리 이유

T-102가 요구한 T-101용 최소 boot/fault image를 먼저 제공한다. 실물 보드 없이 완료 가능한 C 구현·host fault fixture·target compile을 독립 검증 단위로 둔다. T-102의 G1/G2 계측·실제 reset·stack watermark·UART diagnostic 의무를 삭제하거나 대신 통과시키지 않는다.

## 고정 결정

- STM32는 RTOS 없이 단일 cooperative worker owner를 사용한다. heap과 CAN TX 경로를 추가하지 않는다.
- HSE16MHz, PLLM4/N80/R2/Q4, SYSCLK160MHz, APB1/2·USART2·FDCAN80MHz 계약을 유지한다.
- safe GPIO latch 설정 후 clock을 초기화하며 모든 실패는 latched fault로 닫힌다.
- clock ready 대기는 bounded이며 watchdog은 필수 worker 각각의 진척 뒤에만 갱신한다.
- 기본·유일 build mode는 CAPTURE_ONLY다. boot authenticity/debug-lock이 미확인인 bench image의 control capability와 TX permit은 항상 0이다.

## 구현 범위와 예상 파일

- `firmware/communicator/stm32/module/`: SDK 독립 boot policy·progress watchdog·static bounded queue·cooperative scheduler
- `firmware/communicator/stm32/interface/`: clock/time/watchdog/fault 계약
- `firmware/communicator/stm32/platform/stm32g474/`: CMSIS 기반 bounded clock·IWDG·SysTick/마이크로초 timer·reset reason
- `firmware/communicator/stm32/app/`: 최소 boot/fault entry, health worker 연결
- `firmware/communicator/stm32/tests/`, root CTest·CMake, README·map/size 검사

## 범위 밖

UART DMA·실물 RTS/CTS·UART diagnostic transport(T-104), FDCAN 수신(T-103), auth/profile/TX executor(T-105/T-106), Flash erase/provisioning·MCUboot/OTA(T-107/T-108), 실제 pin/clock/reset/PHY/WCET 계측(T-101/T-102)은 포함하지 않는다. 기존 전체 Flash bench linker는 OTA layout이 아니며 보호 Flash에 쓰는 API를 제공하지 않는다.

## 수용 기준

- [ ] host fault fixture에서 safe GPIO→watchdog/clock→worker 순서와 실패별 latched fault를 검증한다.
- [ ] HSE·PLL timeout과 clock-loss 경로에서 control capability/TX permit이 0이며 watchdog 무조건 refresh가 없다.
- [ ] progress 누락·deadline 초과·clock wrap/backward·재진입·queue full/empty/용량/소유권 경계를 회귀시험한다.
- [ ] SysTick ISR은 시간 계수만 하며 protocol parsing·logging·heap을 하지 않는다. critical section은 이전 interrupt mask를 복원한다.
- [ ] STM32 Debug/Release ELF/BIN/MAP·stack-usage를 실제 생성하고 memory budget과 금지 TX symbol 부재를 검사한다.
- [ ] 공용 host Debug/Release/ASan+UBSan, 새 module coverage와 ESP32 네 프로젝트 최종 binary를 경고 없이 검증한다.
- [ ] public header와 README에 수명·소유권·주기·budget·overflow/fault·미계측 항목을 기록한다.
- [ ] 독립 전문 리뷰어 2명 finding disposition·재시험·원격 CI를 통과한다.

## 검증 계획과 evidence

root CTest에 `stm32-core-*` fixture를 등록하고 Debug/Release/ASan+UBSan과 coverage를 실행한다. Arm Debug/Release map·`.su`·BIN SHA-256과 warning scan, ESP32 네 역할 compile regression을 보존한다. 실행 전 계획을 PASS로 표시하지 않는다.

## rollback과 후속 gate

회귀 시 이 task의 변경을 revert해 기존 safe-idle scaffold를 사용한다. 보드 flash나 옵션 byte를 자동 변경하지 않는다. T-102a 완료는 보드 G1·차량 CAN 연결·T-102 전체 완료가 아니며 후속 task의 기존 선행을 자동 해제하지 않는다.
