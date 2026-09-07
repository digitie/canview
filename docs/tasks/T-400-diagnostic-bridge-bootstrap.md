# T-400 Diagnostic Bridge ESP-IDF, SoftAP와 인증 bootstrap

- 상태: `IN_PROGRESS`
- 우선순위: `P1`
- Gate: `G2`
- 선행: `T-001`, `T-003`, `T-400a`
- 병렬 가능: `T-300`

## 목표

`ESP32-S3-WROOM-1-N8R2` 개발보드에 read-only ESP-NOW observer와 phone-only SoftAP web shell을 올린다. 차량 command surface는 build에 포함하지 않는다.

최소 boot/health/watchdog·고정 pool 소프트웨어는 [T-400a](T-400a-bridge-core-bench.md)에서 먼저 구현한다. 그 완료가 아래 SoftAP·인증·무선·휴대폰·실물 수용 기준을 대신하지 않는다.

## T-400a review handoff

T-400a의 P2 handoff를 이 task가 소유한다. SoftAP·HTTP·인증을 추가하기 전에 아래 항목을 source와 시험으로 먼저 닫고, 실제 외부 gate는 G1 물리 evidence로 별도 확인한다.

- Communicator/Bridge BSP의 compile-time board identity/profile binding과 wrong-BSP 거부
- callback visibility/stage latch 및 ISR-context 금지 정책의 계약·시험
- partial safe GPIO 실패의 output ordering·all-pin failure 정책과 외부 gate/power-reset 실측 계획

물리 board·power/reset gate가 현재 `NOT_RUN`인 상태에서는 위 source 검증을 진행할 수 있어도 SoftAP·HTTP·인증 또는 차량 CAN/TX 권한을 추가하지 않는다. 원 disposition, owner, gate와 목표일은 [T-400a 통합 review](../reviews/adversarial/2026-09-07-T-400a.md)에 보존한다.

### 현재 handoff 실행 상태

2026-09-07 immutable source candidate `f35779a`의 독립 2인 review는 wrong-BSP profile preflight가 safe GPIO 뒤에 있음을 P1로, profile digest 입력과 ISR 초기화 contract를 P2로 확인해 `BLOCK`을 반환했다. raw report는 [T-400 review](../reviews/adversarial/2026-09-07-T-400.md)에 보존한다. 후속 source는 app preflight를 GPIO·idle·SDK open보다 앞으로 옮기고, board manifest+pin source profile digest 및 runtime/pool ISR 무변경 거부를 추가했다. 실제 app+BSP 교차-link의 GPIO/runtime open 0회 negative test, profile mutation test를 포함하며 Windows strict C99 Debug/Release는 각각 110/110(별도 86,400초 stream 제외), ESP core coverage와 STM32 Debug/Release·Communicator·Bridge·Controller ESP-IDF BIN/ELF/MAP 재생성을 통과했다. 이 post-fix candidate의 동일 reviewer 2인 재검토, PR CI와 G1 물리 gate는 아직 열려 있다. 이 기록은 G1 물리 gate를 닫지 않으며, target binary·독립 2인 review·G1 power/reset evidence가 완료되기 전 SoftAP·HTTP·인증 또는 CAN/TX 범위는 열지 않는다.

## 고정 target

- ESP-IDF 6.0.3, 8 MB Flash, 2 MB PSRAM
- `WIFI_MODE_APSTA`; STA는 ESP-NOW, AP는 휴대폰 한 대
- external infrastructure AP credential와 NAPT 없음
- ESP-NOW와 SoftAP는 같은 고정 KR channel
- local HTTP, WPA2 password, physical service window, one-time PIN
- web asset은 외부 CDN 없이 gzip Flash 내장

## 구현 범위

- top-level IDF project, partitions, encrypted NVS
- Bridge role provisioning과 encrypted peer 두 개까지
- SoftAP/DNS landing, §14.2의 memory-only bearer token·REST Authorization/WS subprotocol, CSRF/Origin 검사와 request limits
- HTTP/WS shell과 static asset embedding
- service button/LED state machine
- fixed pools, watchdog, heap/queue counters
- command/control scope compile-time absence test
- ESP-NOW와 SoftAP 공용 radio pressure monitor, HTTP token bucket과 운행/lease bulk pause

## 수용 기준

- [ ] Android Chrome/iOS Safari에서 internet 없이 shell이 열린다.
- [ ] service window와 PIN 없이 설정 endpoint에 접근할 수 없다.
- [ ] 한 client/session/request/body/rate 상한이 적용된다.
- [ ] external AP join/NAPT/raw replay/vehicle command route가 없다.
- [ ] Bridge capability의 control scope가 항상 0이다.
- [ ] ESP-NOW load 중 SoftAP가 channel을 바꾸지 않는다.
- [ ] 8 MB partition와 heap/PSRAM budget을 만족한다.
- [ ] 이동·active control lease·P0/P1 deadline miss에서 upload/download throughput이 0이고 status UI만 bounded 유지된다.
- [ ] authenticated HTTP/ESP-NOW flood에서도 Primary heartbeat/control ACK deadline과 fixed pool이 보존된다.

## 계획 보완 수용 기준

- [ ] [OTA §4·6](../architecture/ota.md)의 Bridge layout/복구 버튼을 T-204와 공유한다. 최소 R1은 SD 없음이며 긴 capture를 SD 탑재로 가정하지 않는다.
- [ ] firmware task/queue의 owner·주기·stack·WCET·callback 수명·종료·재접속을 해당 README에 기록한다. 긴 버튼 동작의 현행 commissioning/OTA 구분은 구현 전 정본과 교차 확인한다.

## 검증

```powershell
# T-400에서 firmware/diagnostic-bridge project를 추가한 뒤 실행한다.
Push-Location firmware/diagnostic-bridge
idf.py set-target esp32s3
idf.py build
idf.py size-components
Pop-Location
py -3 tests/security/bridge_http.py
py -3 tests/ui/bridge_offline_browser.py
```

## 보안 경계

local HTTP 사용을 이유로 vehicle command를 추가하지 않는다. PIN, password, pair root/LMK와 raw vehicle identifiers는 log나 screenshot artifact에서 redaction한다.

## 산출물·범위 경계

- 예상 산출물은 `firmware/diagnostic-bridge/` project·role/session/SoftAP shell과 security/browser scripts다. capture/Signal Lab/API 전체·control lease·raw replay는 범위 밖이다.
- 세션 만료·button 취소·role fault에서 worker 자원을 해제하고 재인증 상태로 남는다. 인증 transport 변경은 정본을 먼저 갱신한다.
