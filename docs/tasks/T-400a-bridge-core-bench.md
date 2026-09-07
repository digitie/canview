# T-400a Bridge 최소 core와 ESP 공용화

- 상태: `IN_PROGRESS`
- branch: `codex/t400a-bridge-core-bench`
- 우선순위: `P0`
- Gate: `G0 / G1 준비`
- 선행: `T-001`, `T-200a`

## 목표와 분리 이유

Communicator ESP32 core merge 다음으로 Bridge N8R2의 boot/health/watchdog 기반을 구현한다. T-400의 SoftAP·인증·무선·휴대폰·부하·실물 acceptance는 유지하고 최소 core 소프트웨어만 분리한다. 공통 동작을 복제하지 않고 재사용하되 Communicator N16R8의 검증된 안전 초기화와 실패 동작을 보존한다.

## 고정 결정

- ESP-IDF6.0.3, N8R2 Flash8MiB·Quad PSRAM2MiB/80MHz·ECC 없음, USB19/20를 사용한다. Communicator는16MiB·Octal PSRAM/ECC 가용7.5MiB를 유지한다.
- SDK 독립 health/pool과 bench app을 공용화한다. board별 메모리 요구·역할·GPIO mapping은 BSP의 고정 계약이며 외부 packet/config로 변경할 수 없다.
- Bridge LED5 LOW, button4 입력과 외부 R14 10k pull-up을 유지한다. 입력은 진단값일 뿐 service window·pairing·OTA·차량 권한이 아니다. 새 ISR을 만들지 않는다.
- 단일 main service, 고정 주기100ms·이전 진척250ms·실행20ms·TWDT2초와 PRINT_REBOOT/추가 지연0을 유지한다. 실측 WCET나 debugger 연결 중 reset 보장으로 표시하지 않는다.
- core/pool은 정적 저장소를 사용한다. SDK 초기화 allocation은 별도로 구분한다. pool의 lock/ownership/generation 계약과 실패 시 feed 중단을 유지한다.
- radio/ESP-NOW/UART/HTTP/OTA/NVS write/provisioning은 시작하지 않는다. capability/control/TX는0이다. 이후 웹 단계는 esp_http_server+cJSON+WebSocket을 사용한다.

## 구현 범위·예상 파일

- 공용 `firmware/app/`, `firmware/module/`, `firmware/interface/`, `firmware/platform/esp32s3/`와 IDF component: health/pool·SDK adapter·bench composition 추출
- Communicator/Bridge BSP와 main CMake: 보드별 고정 메모리·진단 입력·role 연결
- `firmware/boards/boards.json`, `tools/generate_boards.py`, `tools/check_sdkconfig.py`: 보드별 defaults 및 실제 SDKCONFIG gate
- host core/SDK/app/BSP/config tests, coverage와 root CTest/CI: 두 보드 matrix·교차 오설정·회귀
- 공용 core 설계·두 README·architecture/API: 현재 경로, 상태, 소유권·수명·resource/RTOS budget과 NOT_RUN

## 수용 기준

- [ ] 공용 core/app에 vendor/FreeRTOS 의존 또는 Communicator 전용 GPIO 가정이 없다.
- [ ] 두 BSP가 올바른 메모리 계약을 선택하며 잘못된 값·profile 혼용·sense mapping 오류를 거부한다.
- [ ] Bridge의 safe GPIO→watchdog→메모리→주기 health를 실제 app/SDK fixture로 실행하고 모든 실패 단계에서 추가 feed/통신이 없다.
- [ ] Communicator 기존 boot·clock/deadline·pool·SDK/BSP 실패 회귀를 보존하고 두 보드 입력·role 진단을 구분한다.
- [ ] actual SDKCONFIG에서 wrong Flash/PSRAM mode/ECC/watchdog/panic/console 및 금지 Flash 변경을 차단한다. 독립 fixture·금지 항목 삭제 변이를 유지한다.
- [ ] Windows Debug/Release, GCC·ASan+UBSan, pool 동시성, coverage와 strict API가 통과한다.
- [ ] STM32 Debug/Release·ESP4종 최종 BIN/ELF/MAP가 경고0으로 빌드되고 digest/CI를 기록한다.
- [ ] 서로 다른 전문 리뷰어2명의 독립 적대적 리뷰·finding 수정·재확인을 완료하고 PR을 merge한다.

## 검증·evidence

root CMake/CTest presets, generated check, 실제 sdkconfig gate, core coverage와 strict API를 사용한다. 새 board matrix는 구현 후 테스트 이름·개수·실제 결과를 기록한다. target은 SDK/commit·BIN/ELF/MAP·SHA-256·warning scan으로 식별한다. 기존 T-200a PASS를 변경된 공용화 코드의 성공으로 재사용하지 않는다.

## 범위 밖·rollback

SoftAP/HTTP/WebSocket·인증·ESP-NOW·capture는 T-400 이후다. Controller BSP/UI와 OTA layout·키 저장소·실제 flash/HIL은 포함하지 않는다. 실패 시 이 변경을 revert해 Communicator 전용 core와 Bridge safe-idle로 돌아간다. T-400/실차 gate를 소프트웨어 시험으로 닫지 않는다.
