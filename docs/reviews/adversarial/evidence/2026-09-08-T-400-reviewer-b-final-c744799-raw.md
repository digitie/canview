# 독립 적대적 리뷰 raw report

- Execution ID: `codex-thread-review-B-20260908-c744799`
- Candidate: `c744799f3c9e518fa351e3f46b6c336896346d16`
- Base: `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`
- 시작: `2026-09-08T12:39:28.7801316+09:00`
- 종료: `2026-09-08T12:48:45.5743250+09:00`
- Isolation: immutable Git archive만 검사. worktree 파일 미사용·미수정. `docs/reviews/**` 원문 미열람.
- Physical/HIL: `NOT_RUN`
- Vehicle CAN TX: `NO-GO`

## Findings

### P1-01 — LRU purge가 인증된 단일 세션을 원격에서 끊을 수 있음

- 위치: `firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c:1757-1760, 1940-1944`
- `max_open_sockets = 1U`인데 `lru_purge_enable = true`입니다.
- `open_connection()`은 `pre_auth_client_valid`만 검사하고, 이미 인증된 `session.active_client_fd`는 검사하지 않습니다.
- 공격자가 새 TCP 연결을 만들면 HTTPD가 기존 인증 세션을 LRU purge한 뒤 새 연결을 수락할 수 있습니다. 이후 `enter_request()`의 `has_other_client()` 검사는 이미 늦습니다.
- 기존 세션 종료 시 `close_session()`이 인증 토큰까지 로그아웃시킵니다(`1797-1800`).
- ESP-IDF 문서도 `lru_purge_enable`을 최대 소켓 도달 시 기존 LRU 연결을 purge하는 기능으로 정의합니다. [ESP-IDF HTTP Server 설정](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html)

영향: 인증된 Diagnostic Bridge 사용자의 연결·세션을 비인증 로컬 Wi-Fi 클라이언트가 반복적으로 강제 종료할 수 있습니다.

권고: `lru_purge_enable = false`로 설정하고, `open_connection()`에서 pre-auth뿐 아니라 인증된 active client도 거부하는 target test를 추가하십시오. 기존 인증 세션이 보존되는 second-connect/flood 시험이 필요합니다.

### P2-01 — live HTTP/WS 보안 검증이 선택 사항이며 오류 응답을 과도하게 허용함

- 위치: `tests/security/bridge_http.py:167-225`, `.github/workflows/foundation.yml:54-57`
- CI는 live URL 없이 정적 검사만 수행합니다.
- WebSocket query-token 거부 시험이 `500`도 성공으로 허용합니다(`bridge_http.py:222-223`). 내부 서버 오류가 인증 거부 성공으로 위장될 수 있습니다.
- 인증 성공·idle expiry·단일 client eviction·pre-auth timeout·실제 WebSocket frame은 검증하지 않습니다.

권고: 의도된 4xx만 허용하고, live target test를 별도 필수 gate로 승격하십시오.

### P2-02 — 브라우저 회귀시험이 CI에서 실행되지 않음

- 위치: `tests/ui/diagnostic-browser.cjs:328-334`, `.github/workflows/foundation.yml:18-60`
- 브라우저 runner는 `playwright`를 요구하지만 CI workflow에는 설치·실행 단계가 없습니다.
- 현재 환경에서도 `node tests/ui/diagnostic-browser.cjs`는 `Cannot find module 'playwright'`로 실패했습니다.

권고: lockfile 기반 Playwright 설치와 `node tests/ui/diagnostic-browser.cjs` 실행을 CI에 추가하십시오.

## 확인된 정상 항목

- Board generator/check: PASS
- Generated artifacts/protocol: PASS
- SDK config tests: 13 PASS
- Python unittest discovery: 49 PASS
- Host Debug build: 103/103 PASS
- Host CTest: 113 PASS; 장시간 `uart-fault-stream` #77은 의도적으로 제외
- ESP32 core coverage: PASS
- Bridge read-only static contract: PASS
- raw replay/control lease/CAN TX 문자열: 확인되지 않음
- Generated artifact drift: 확인되지 않음
- Strict C99/warning flags와 component dependency 선언: 정적 검토상 이상 없음

## 실행한 주요 명령

```text
git cat-file -t c744799... 9fe46c...
git show -s --format=fuller c744799...
git diff --stat 9fe46c... c744799... -- ':!docs/reviews/**'
git archive ... c744799... | tar -xf - ...

python -B tools/generate_boards.py --check
python -B tools/check_generated.py
python -B tests/foundation/test_sdkconfig.py
python -B tests/test_bridge_web_assets.py
python -B tests/security/bridge_http.py
python -B tests/security/bridge_http.py --require-live
python -B -m unittest discover -s tests -p 'test_*.py'
git diff --check 9fe46c... c744799...

cmake configure/build host Debug
ctest -I 1,76,1
ctest -I 78,114,1

cmake configure/build CANVIEW_COVERAGE=ON
python -B tools/check_esp32_core_coverage.py --build <temporary coverage build>

node tests/ui/diagnostic-browser.cjs
Get-Command idf.py
```

## 읽은 파일 범위

`AGENTS.md`, `docs/README.md`, `docs/resume.md`, `docs/tasks/T-400-diagnostic-bridge-bootstrap.md`, workflow/CMake/build configuration, board/sdkconfig/partition/generator files, Bridge web/auth/DNS/app C source 및 header, security/browser/UI tests, pinmap, 그리고 `tests/esp_core/*` 8개 파일을 읽었습니다.

## 최종 판정

**BLOCK**

P1-01 수정 및 second-client/LRU eviction target 검증 전에는 review closure나 release 가능으로 표시할 수 없습니다.
