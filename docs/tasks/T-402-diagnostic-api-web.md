# T-402 Diagnostic OpenAPI, REST/WS와 모바일 web 통합

- 상태: `BLOCKED`
- 우선순위: `P1`
- Gate: `G2/G3`
- 선행: `T-401`, `T-304`, `T-305`
- 병렬 가능: Controller UI after model contract

## 목표

현재 정적 prototype을 schema-driven backend에 연결하고 operation 상태, reconnect, 설정 owner를 추측 없이 구현한다.

## 고정 결정

- `api/diagnostic-v1.openapi.yaml`이 REST 정본이다.
- WebSocket event도 versioned JSON schema와 sequence/revision을 갖는다.
- HTTP `202`는 operation resource 생성일 뿐 성공 완료가 아니다.
- free-form numeric text input 대신 schema가 허용한 switch/select/slider/action을 쓴다.
- remote Controller config는 Bridge↔Controller direct encrypted peer로 전달하며 D3는 Controller touch 확인이 필요하다.

## 구현 범위

- system/bus/inventory/filter/capture/candidate/config schema endpoint
- idempotent operation create/status/cancel
- WebSocket snapshot/event protocol와 reconnect gap recovery
- static HTML/CSS/JS를 real API client로 전환
- CSRF/session/rate/body/content-type validation
- offline assets, browser compatibility, export download
- owner/revision/effective quota 표시
- route별 `read`, `capture:write`, `config:write` 권한 table과 생성 route guard
- 정차·lease 없음에서만 64 kB/s HTTP bulk, 그 밖에는 pause/resume operation

## 수용 기준

- [ ] OpenAPI validator와 firmware route 목록이 일치한다.
- [ ] unknown field/type/range/content-type 요청이 4xx로 거부된다.
- [ ] duplicate request token이 operation을 중복 실행하지 않는다.
- [ ] WS gap/reconnect 뒤 REST snapshot과 정확히 재동기화한다.
- [ ] 화면이 requested와 effective filter/quota를 구분한다.
- [ ] touch target, viewport 360–430 px, Android Chrome/iOS Safari를 통과한다.
- [ ] 외부 network/font/CDN 없이 모든 기능이 동작한다.
- [ ] web bundle에서 command/control endpoint 문자열이 0개다.
- [ ] `POST .../markers`를 포함한 모든 state-mutating route가 read token과 CSRF 없는 요청을 거부한다.
- [ ] MARK가 capture-control action과 marker endpoint 두 경로로 생성되지 않는다.
- [ ] worst RSSI+raw 200 record/s+HTTP 64 kB/s+authenticated flood에서 P0/P1 deadline miss가 나면 bulk가 즉시 정지된다.

## 계획 보완 수용 기준

- [ ] API 문서에 있는 filter CRUD·capture stop·candidate/history·operation 취소와 실제 route/권한이 일치한다. 누락 DELETE/CLEAR/cancel의 method/path는 OpenAPI로 먼저 고정하고 구현되지 않은 버튼은 동작 성공으로 보이지 않는다.
- [ ] WS는 boot/session/revision/sequence에 묶고 slow client·bounded buffer 포화·탭 변경·닫기·abort/reconnect 후 subscription과 listener를 해제한다. reconnect 때 snapshot 이전 delta가 적용되지 않는다.
- [ ] candidate/import/name/label은 text로만 출력하고 XSS·과대 JSON·53 bit 초과 CAN 값·malformed 64-bit decoder 입력을 시험한다. 64-bit 값은 JSON 정밀도 손실 없는 표현을 schema에서 정한다.
- [ ] auth 만료·PIN 실패·offline/empty/loading/error·불명 속도·moving lock·pending/timeout·effective quota·다운로드 실패를 360–430 px 양방향/키보드/확대에서 확인한다.
- [ ] Bridge 없는 Controller 진단 웹 fallback은 선택 기능이다. 제공 시 §21의 lease 없음·50 record/s·10 Hz·resource 자동 중단을 별도 evidence로 검증하며 자체 OTA의 필수성으로 잘못 합치지 않는다.

## 검증

```bash
python -m pytest -q tests/api
python tests/api/check_openapi_routes.py
python tests/ui/bridge_browser_matrix.py --offline
python tests/security/bridge_api_fuzz.py --seed 1
```

## evidence

OpenAPI diff, browser screenshots, accessibility report, WS reconnect trace와 operation lifecycle trace를 남긴다.

## 산출물·범위 경계

- 예상 산출물은 `api/diagnostic-v1.openapi.yaml`, Bridge route/client·mobile assets와 API/browser tests다. 자체 OTA UI(T-306), vehicle command/raw replay는 범위 밖이다.
- API/WS 실패 시 session/revision을 재조회하고 optimistic 성공 표시를 취소한다. 정적 prototype을 실제 backend 완료로 집계하지 않는다.
