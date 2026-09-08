# CANView PR #28 fresh hostile review

- Execution ID: `codex-thread-review-B-20260908-19efeed`
- Candidate: `19efeedd895e1885a52ab9023bf6e5095c358cb3`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- 시작 시각: harness에서 미기록
- 종료 시각: `2026-09-08T11:53:08.9370284+09:00`
- 이전 review report 및 `docs/reviews/**` 내용은 읽지 않음
- 소스는 candidate archive/Git object에서만 읽음
- repository 수정·commit·push 없음

검증 결과:

- Host configure/build: `99/99 PASS`
- CTest: `112/113 PASS`; `uart-fault-stream` 86,400초 시험은 미실행
- Python unittest: `48 PASS`
- Generator mutation tests: `9 PASS`
- Generated drift: `15 PASS`
- HTTP static contract: `26 PASS`
- `--require-live`: `NOT_RUN`, exit `2`
- `idf.py`: `NOT_FOUND`
- Target ESP-IDF build, physical/HIL, Wi-Fi/AP, HTTP/WebSocket live, DNS/ESP-NOW flood: `NOT_RUN`
- CAN TX: `NO-GO`

## Findings

### P1-01 — unauthenticated traffic가 idle timeout을 무력화함

`firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:316-370`

`enter_request()`가 Origin/auth 검증 전에 `last_activity_ms`와 `client_activity_valid`를 갱신합니다. `/` public handler도 `914-935`에서 이를 호출합니다. 따라서 공격자가 하나의 TCP client에서 public root 또는 반복적인 unauthorized request를 보내면 `1665-1670`의 5분 idle expiry가 영구 연장됩니다.

`max_open_sockets = 1U`(`1822`)와 결합되어 정상 authenticated client의 접근도 starvation될 수 있습니다.

권고: 인증 성공 전에는 logical session owner/activity로 기록하지 말고, pre-auth 연결에 별도 timeout·rate limit·close 정책을 적용하십시오. authenticated REST 또는 검증된 WS activity만 idle timer를 갱신하고, 이를 live/fixture test로 검증해야 합니다.

### P1-02 — HTTP cleanup 실패 시 retry 가능한 상태를 먼저 폐기함

`canview_bridge_web.c:398-482`, `414-422`, `460-481`

`httpd_stop()`이 실패해도 `web_state.server = NULL`로 만들고, HTTP callback이 완전히 quiesce했는지 확인하기 전에 request/state/WS lock을 삭제하고 전체 state를 zeroize합니다. 이후 `app_main.c:69-76`의 retry는 이미 잃어버린 HTTP handle을 재시도할 수 없습니다. URI 등록 및 DNS 시작 실패 경로(`1843-1858`)도 동일한 비재시도 cleanup을 수행합니다. partial failure 상황에서 dangling callback, 잔존 task/resource, fail-closed 불능 가능성이 있습니다.

권고: server/task/callback quiescence가 확인될 때까지 handle과 synchronization object를 보존하고, 각 cleanup 단계를 retryable state machine으로 관리하십시오. 모든 callback 종료 후에만 lock 삭제와 state zeroize를 수행해야 합니다.

### P2-01 — live HTTP runner의 범위가 약하고 URL 검증이 부정확함

`tests/security/bridge_http.py:107-120`

`_request()`는 scheme이 `http`이고 hostname만 있으면 허용합니다. 오류 메시지는 local URL만 허용한다고 하지만 `evil.invalid:8080` 같은 외부 host도 허용합니다.

또한 live probe(`129-188`)는 valid authenticated session, one-client exclusivity, idle expiry, actual WebSocket protocol/frame, DNS, flooding, cleanup failure를 검증하지 않습니다. WS도 `101`만 거부하고 `404/500` 등은 통과시킵니다.

`python` monkeypatch probe로 arbitrary host 연결 객체가 생성되는 것을 확인했습니다.

권고: 허용 host/IP를 명시적으로 제한하고, expected rejection status를 좁히십시오. valid auth flow, real WS handshake/frame, idle/session close, one-socket, rate/body/frame bounds를 추가해야 합니다.

### P2-02 — target resource/flood acceptance가 CI에서 fail-closed가 아님

`docs/tasks/T-400-diagnostic-bridge-bootstrap.md:66-72`는 heap/PSRAM, authenticated HTTP/ESP-NOW flood 및 fixed-pool 보존을 요구합니다. 그러나 workflow는 `foundation.yml:212-225`에서 target build와 `size-components`, sdkconfig 검사만 수행합니다.

heap/PSRAM, task stack, socket, DNS, WS, flood latency/deadline budget을 측정하거나 실패시키는 gate가 없습니다.

권고: target job에 명시적인 size/heap/PSRAM/stack/socket/rate/flood threshold와 결과 artifact를 추가하십시오.

### P2-03 — offline browser 회귀가 CI/CTest에 실제 등록되지 않음

`tests/security/bridge_http.py:102-104`는 browser test 파일의 존재만 확인합니다. CI는 `foundation.yml:56-57`에서 Python runner만 실행하고, CTest 등록도 `CMakeLists.txt:197-199`의 `bridge-http-contract`뿐입니다.

`tests/ui/diagnostic-browser.cjs`의 실제 Playwright suite는 실행되지 않습니다. 따라서 browser contract 변경이 CI에서 회귀를 검출하지 못합니다.

권고: Playwright/Edge 의존성을 명시하고 Windows CI 또는 별도 truthful job에 실제 browser runner를 등록하십시오.

### P3-01 — target artifact manifest의 source provenance가 부족함

`.github/workflows/foundation.yml:253-266`

manifest에는 artifact path, byte size, SHA-256와 toolchain provenance만 기록됩니다. candidate commit, source-tree digest, board manifest digest, sdkconfig/generator 입력 digest가 없어 artifact가 어떤 정확한 source/config에서 생성됐는지 독립적으로 연결하기 어렵습니다.

권고: commit SHA와 relevant input/generated-output digest를 manifest에 포함하십시오.

## 확인된 안전 경계

Static source contract에서 `control_scope=0`, `vehicle_tx=false`, raw replay/control lease/CAN TX 경로 부재, bounded JSON/WS/DNS, Origin/auth 및 generated asset consistency를 확인했습니다. 해당 source-only 확인은 CAN TX 권한이나 physical acceptance를 의미하지 않습니다.

읽은 주요 파일: `.github/workflows/foundation.yml`, `CMakeLists.txt`, Diagnostic Bridge CMake/component/main/auth/web/DNS 소스·헤더, `boards.json`, `pinmap.csv`, `sdkconfig.defaults`, generator/config/budget/coverage 도구, foundation·ESP core·auth·asset·security·browser 테스트, 관련 architecture/task/README 문서 및 `bridge-shell.html`.

BLOCK
