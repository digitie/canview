# T-402 Diagnostic OpenAPI, REST/WS와 모바일 web 통합

- 상태: `BLOCKED`
- 우선순위: `P1`
- Gate: `G2/G3`
- 선행: `T-401`
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

## 검증

```bash
python -m pytest -q tests/api
python tests/api/check_openapi_routes.py
python tests/ui/bridge_browser_matrix.py --offline
python tests/security/bridge_api_fuzz.py --seed 1
```

## evidence

OpenAPI diff, browser screenshots, accessibility report, WS reconnect trace와 operation lifecycle trace를 남긴다.
