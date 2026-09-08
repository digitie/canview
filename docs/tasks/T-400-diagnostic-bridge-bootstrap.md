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

### 2026-09-08 source-only 진행 예외

사용자가 `G1 이전 fw 구현 허용`을 명시했으므로, 이번 branch에서는 G1 physical evidence 없이 C firmware source와 실제 ESP-IDF compile을 진행한다. 이 예외는 source implementation·host/target build에만 적용하며, board flash·AP association·power/reset/brownout·phone browser·ESP-NOW·capture·production provisioning을 성공으로 바꾸지 않는다. Diagnostic Bridge의 `control_scope=0`, `vehicle_tx=false`, raw replay/control lease 부재와 차량 CAN `NO-GO`는 그대로 유지한다. T-400은 source/review/CI와 물리 gate가 모두 닫힐 때까지 `IN_PROGRESS`다.

현재 source 범위는 [Diagnostic Bridge web shell 문서](../../firmware/diagnostic-bridge/docs/web-shell.md)의 local gzip shell, fixed-buffer DNS, service-window session auth, bounded REST/WS read-only snapshot이다. 실제 observer/capture/Signal Lab API와 encrypted provisioning은 후속 구현으로 남긴다.

### 현재 handoff 실행 상태

2026-09-07 immutable source candidate `f35779a`의 독립 2인 review는 wrong-BSP profile preflight가 safe GPIO 뒤에 있음을 P1로, profile digest 입력과 ISR 초기화 contract를 P2로 확인해 `BLOCK`을 반환했다. raw report는 [T-400 review](../reviews/adversarial/2026-09-07-T-400.md)에 보존한다. 후속 immutable `8f32c07`은 app preflight를 GPIO·idle·SDK open보다 앞으로 옮기고, board manifest+pin source profile digest 및 runtime/pool ISR 무변경 거부를 추가했다. 실제 app+BSP 교차-link의 GPIO/runtime open 0회 negative test, profile mutation test, Windows strict C99 Debug/Release 각각 110/110(별도 86,400초 stream 제외), ESP core coverage와 STM32 Debug/Release·Communicator·Bridge·Controller ESP-IDF BIN/ELF/MAP 재생성을 통과했다. whole-diff post-fix A/B 재시도는 서로의 raw evidence를 노출해 무효 `BLOCK`으로 보존했고, 이를 읽지 않은 fresh source-only A/B는 P0/P1 없음으로 확인했다. B의 coverage evidence P2는 immutable `813d19c`에서 app `preflight`와 두 wrong-BSP executable의 독립 `.profraw`/export·app function coverage를 coverage gate에 넣어 수정했다. C3 delta A는 실행/HIL `NOT_RUN` 조건부, B는 P2 `CLOSED/PASS`이며 P0/P1은 없다. final candidate `5b6a299`의 CI `34126431204`는 다섯 job 성공, CI artifact 18/18 path·size·SHA-256 독립 대조와 target warning 0을 확인했고 [PR #25](https://github.com/digitie/canview/pull/25)는 merge `d8d8057`으로 통합됐다. 이 source closure는 G1 물리 gate를 닫지 않는다. 사용자의 2026-09-08 명시적 source-only 예외에 따라 C source와 실제 target compile은 진행하지만, board flash/HIL·power/reset evidence가 완료되기 전 physical acceptance와 차량 CAN/TX 범위는 열지 않는다.
2026-09-08 PR #28의 이전 candidate `19a42339`에 대한 fresh Reviewer A/B는 bounded execution에서 완료 raw report를 반환하지 않았다. 실행 ID와 `INCOMPLETE/BLOCK` 상태는 [T-400 review](../reviews/adversarial/2026-09-08-T-400.md) 및 reviewer별 evidence에 보존했다. 이후 `19efeed` fresh A/B raw report는 각각 `CONDITIONAL`/`BLOCK`을 반환했고, 인증 전 idle 갱신과 cleanup retry P1을 `bf7a11c`에서 수정했다. `bf7a11c`에 대한 새 독립 A/B도 pre-auth slowloris, startup/DNS cleanup 및 auth/activity 원자성 P1을 확인해 `BLOCK`을 반환했으며, 원문은 [A BF7 evidence](../reviews/adversarial/evidence/2026-09-08-T-400-reviewer-a-final-bf7a11c-raw.md)와 [B BF7 evidence](../reviews/adversarial/evidence/2026-09-08-T-400-reviewer-b-final-bf7a11c-raw.md)에 보존했다. `c744799`에 대한 새 독립 A/B는 logout/expiry pre-auth 재무장과 LRU purge P1을 각각 확인해 `BLOCK`을 반환했으며, 원문은 [A c744799 evidence](../reviews/adversarial/evidence/2026-09-08-T-400-reviewer-a-final-c744799-raw.md)와 [B c744799 evidence](../reviews/adversarial/evidence/2026-09-08-T-400-reviewer-b-final-c744799-raw.md)에 보존했다. `41fdc99`에서 해당 source finding을 수정했으며, 새 candidate의 독립 A/B execution ID와 raw verdict가 닫히기 전에는 review closure나 task 완료를 표시하지 않는다.

### 2026-09-08 line-level review 수신과 post-fix 대기

이전 service retry와 별도로 독립 thread reviewer A/B의 raw report를 수신했다. A는 embedded runtime 관점에서 P1 2건과 P2 3건, B는 integration/security/resource 관점에서 P1 4건과 P2 5건을 보고했다. 원문은 [A evidence](../reviews/adversarial/evidence/2026-09-08-T-400-reviewer-a-raw.md)와 [B evidence](../reviews/adversarial/evidence/2026-09-08-T-400-reviewer-b-raw.md)에 그대로 보존한다.

현재 source candidate `41fdc99`에는 deferred watchdog arm, host-testable Bridge bootstrap, state/request lock 분리, 인증과 activity 기록의 원자적 lock 경계, 만료 session 강제 close, JSON nesting bound, DNS query slice quota와 teardown 의존 순서 보존, startup cleanup retry와 cleanup 실패 재부팅, logout/expiry 이후 pre-auth 15초 deadline 재무장, custom socket close, 단일 owner LRU purge 비활성화, Bridge routing Kconfig 금지와 WebSocket callback 필수 검사, canonical gzip OS header가 반영됐다. 작성자 재검증은 focused host CTest 7/7, 이전 전체 host 112/113(24시간 `uart-fault-stream` 제외), ESP32 core coverage, Python/config/generator gate, 실제 ESP-IDF 6.0.3 Bridge `idf.py build` 성공이다. P2인 live HTTP/WS, target heap/PSRAM/flood stress와 physical/HIL은 아직 `NOT_RUN` 또는 후속 gate다. 새 원 reviewer A/B의 이 candidate 재검토와 CI success 전에는 T-400을 완료로 표시하지 않는다.

### 2026-09-08 source/review/CI closure candidate `5861274`

`7479cf1`에서 qualification job checkout을 immutable PR head로 고정하고, `5861274`에서 Diagnostic Bridge `CONFIG_HTTPD_QUEUE_WORK_BLOCKING=n`을 명시했다. 이 설정은 `tools/generate_boards.py`, `tools/check_sdkconfig.py`, `tests/foundation/test_sdkconfig.py`, `tests/test_bridge_web_assets.py`의 generator/validator/negative mutation 경계와 함께 검증된다. 현재 candidate `586127450d14b8ef5a59f90edd1c49947b866bb7`, base `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`는 다음 source 및 target 근거를 갖는다.

- `python -B tests/security/bridge_http.py`: 64 checks PASS
- `python -B tests/test_bridge_web_assets.py -q`: 5/5 PASS
- `python -B tests/foundation/test_sdkconfig.py -q`: 13/13 PASS
- `python -B -m unittest discover -s tests -p 'test_*.py'`: 49/49 PASS
- `python -B tools/generate_boards.py --check`, 문서 link/plan 검증 PASS
- 실제 ESP-IDF 6.0.3 `idf.py -C firmware/diagnostic-bridge build` PASS
- GitHub Actions `34196236147`: Windows C99, Linux GCC/Clang portability, sanitizer, browser, target firmware 여섯 job PASS
- target artifact 18개 BIN/ELF/MAP bytes·SHA-256 `18/18`, source provenance 6/6, toolchain provenance before/after 일치, target warning/error regex 0건

독립 reviewer raw evidence는 [Reviewer A](../reviews/adversarial/evidence/2026-09-08-T-400-reviewer-a-final-5861274-raw.md)와 [Reviewer B](../reviews/adversarial/evidence/2026-09-08-T-400-reviewer-b-final-5861274-raw.md)에 보존했다. A 실행 `CV-HOSTILE-20260908-POSTFIX-586`과 B 실행 `c3636fd3-5ab5-4637-8a9b-2cd813359631`은 같은 candidate/base를 독립적으로 검토했고 P0/P1은 없었다. A는 external TX gate/reset/brownout 및 HIL을 P2 physical gate로 남겼고, B는 source/config/build/evidence actionable finding을 보고하지 않았다. 통합 disposition은 [T-400-02](../reviews/adversarial/2026-09-08-T-400-02.md)이며 최종 verdict는 `CONDITIONAL`이다.

물리 board flash/HIL, ST-LINK/serial, 전원 rail/reset/brownout, PSRAM/clock/watchdog soak, live ESP32 endpoint, ESP-NOW/capture, production provisioning과 vehicle integration은 `NOT_RUN`이다. 차량 CAN TX는 `NO-GO`이며 Diagnostic Bridge의 `control_scope=0`, `vehicle_tx=false`, control lease/raw replay 부재를 유지한다. 따라서 source/review/CI closure는 가능하지만 T-400 상태는 physical gate가 닫힐 때까지 `IN_PROGRESS`로 유지한다.

## 고정 target

- ESP-IDF 6.0.3, 8 MB Flash, 2 MB PSRAM
- `WIFI_MODE_APSTA`; STA는 ESP-NOW, AP는 휴대폰 한 대
- external infrastructure AP credential와 NAPT 없음
- ESP-NOW와 SoftAP는 같은 고정 KR channel
- local HTTP, WPA2 password, physical service window, one-time PIN
- web asset은 외부 CDN 없이 gzip Flash 내장

## 구현 범위

- top-level IDF project, partitions, bench NVS read-only credential load; encrypted production provisioning은 별도 gate
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
$env:NODE_PATH = 'C:/Users/digit/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules'
node tests/ui/diagnostic-browser.cjs
```

`bridge_http.py`는 기본 실행에서 source/config contract를 검사하고 live endpoint가 없으면
`NOT_RUN`으로 남긴다. 실제 ESP32 HTTP/WebSocket rejection probe는 `CANVIEW_BRIDGE_URL`을
명시한 경우에만 실행한다. browser 검사는 offline prototype에 대한 별도 검증이며, 실제
ESP32 flash·AP association·Android/iOS 실기기 검증을 대신하지 않는다.

## 보안 경계

local HTTP 사용을 이유로 vehicle command를 추가하지 않는다. PIN, password, pair root/LMK와 raw vehicle identifiers는 log나 screenshot artifact에서 redaction한다.

## 산출물·범위 경계

- 예상 산출물은 `firmware/diagnostic-bridge/` project·role/session/SoftAP shell과 security/browser scripts다. capture/Signal Lab/API 전체·control lease·raw replay는 범위 밖이다.
- 세션 만료·button 취소·role fault에서 worker 자원을 해제하고 재인증 상태로 남는다. 인증 transport 변경은 정본을 먼저 갱신한다.
