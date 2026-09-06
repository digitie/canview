# Diagnostic Bridge 모바일 웹 prototype

이 폴더는 [Diagnostic Bridge 명세](../../docs/architecture/diagnostic-bridge.md)의 휴대폰 분석 흐름을 검증하는 오프라인 prototype이다. 실차 CAN, ESP-NOW, REST, WebSocket에는 연결되지 않았다. 현대 5W 계열 남청색·청색 톤과 시스템 글꼴을 사용하며 외부 font·CDN·API 의존성이 없다.

브라우저에서 `index.html`을 열고 다음 query를 사용한다.

```text
index.html?screen=status
index.html?screen=frames
index.html?screen=capture
index.html?screen=signals
index.html?screen=settings
```

## 구현한 로컬 흐름

- 상태: 초기값은 장치 미연결·속도 불명·CAN `NO DATA`다. 샘플을 열어도 RTT·채널·차량 정지·실측 버스 상태를 확정하지 않는다. 응답 확인은 `PENDING → REJECTED / NO_BACKEND` 안내이며 실제 요청을 보내지 않는다.
- 프레임: `로컬 샘플 열기` 후 합성 frame 6개를 제공한다. 버스·11/29-bit 형식·변화/신규·ID 검색·4가지 정렬이 동작한다. 같은 ID의 DLC variant를 구분한다. `화면 고정`은 표 snapshot만 고정하며 `다음 샘플`의 대기값은 고정 해제 시 표시한다.
- 캡처: 행동·대상 frame·모드·앞/뒤 시간 초안을 선택하고, 합성 기준값 → 로컬 marker 최대 3회 → 전후 bit 비교 → 신호 Lab 순으로 연습한다. 시간 선택은 초안이며 실제 기록 시간이나 장치 수락량이 아니다. 취소·출처 변경은 진행 중 callback과 결과를 무효화한다. 연습 전후 byte를 Lab에 그대로 전달한다.
- 신호 Lab: DLC에 맞는 bit 지도와 44px 시작 bit/길이 선택기로 1–64 bit를 분석한다. Intel LSB 시작, Motorola MSB 시작 및 byte 경계 이동, sign extension을 구현한다. raw와 소수 6자리 계산에 BigInt를 사용하여 64-bit 정밀도를 보존한다. Factor·Offset과 단위는 고정 preset만 제공한다. 변조된 descriptor의 NaN/Infinity·지수 표기·범위 초과·Factor 0·모순된 bool 설정을 거부한다. 새 descriptor import는 T-403 후속이다.
- 설정: Bridge/Controller/진단 Stream을 구분한다. Bridge·Stream 선택은 메모리의 로컬 초안만 저장한다. 실제 owner schema·revision·정차·lease·수락량은 미확인이다. Controller 편집은 잠겨 있다. AP 암호 회전은 무응답·취소 안내와 로컬 확인 절차 연습만 제공하며 암호를 생성·표시·저장하지 않는다.

`SAMPLE`, `STALE`, `GAP`, `MALFORMED`, `DISCONNECTED` 품질 시험을 상단에서 선택할 수 있다. stale은 마지막 byte를 구분해 남기고 현재값을 비운다. gap은 불완전한 샘플로 표시한다. malformed·공급 중단은 raw와 현재값을 비운다. 정상 형식의 합성 샘플에서만 후보 저장과 캡처 연습이 가능하다.

## 후보·다운로드 경계

후보는 `CANDIDATE / C`, `SYNTHETIC_DEMO`, evidence 없음으로 고정하고 최대 20개를 이 탭 메모리에 저장한다. 같은 descriptor는 중복 저장하지 않는다. 샘플을 닫아도 후보는 남고, 새로고침하면 사라진다. 삭제는 확인 dialog를 거친다.

`canview-demo-candidates.json`은 합성 frame의 버스·CAN ID·형식·DLC와 decoder 설정만 담은 로컬 교환 형식이다. `prototype: true`, `operational: false`, `vehicle_evidence: false`를 명시한다. 실제 API candidate descriptor·profile patch·실차 evidence manifest가 아니며 33–64 bit도 operational export가 아니다. 자유 입력, raw payload, 실제 capture, 장치 식별자, VIN, 위치, 음성, AP 비밀, session token을 수집하거나 내보내지 않는다. 다운로드 요청 뒤 실제 파일 존재는 브라우저 다운로드 목록에서 확인한다. 다운로드 시작 오류가 나도 메모리 후보는 보존한다.

## 브라우저 검증과 공통 검사 통합

Windows PowerShell에서 실행한다.

```powershell
$env:NODE_PATH = 'C:/Users/digit/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules'
node tests/ui/diagnostic-browser.cjs
```

테스트는 `chromium.launch({channel: 'msedge', headless: true})`와 `file://`, 오프라인 context를 사용한다. 5뷰 직접 진입과 모든 주요 조작, pending 취소·timeout 재시도, malformed·stale·gap·미연결 복구, Intel/Motorola·signed·64-bit 계산, decimal/단위 오류, 후보 저장·중복·JSON 허용 필드·다운로드 실패·삭제를 검사한다. 320/360/390/430/768px에서 가로 overflow와 일반 조작 44px, 보조 bit 24px 이상을 확인한다. 실장치 성능 gate는 아니다.

공통 `tools/ui/check-browser.cjs` 담당자는 이미 연 Edge browser를 전달하면 된다. 함수는 전용 context를 만들고 검사 후 닫으며 전달한 browser는 닫지 않는다.

```javascript
const {runDiagnosticTests} = require('../../tests/ui/diagnostic-browser.cjs');
const diagnostic = await runDiagnosticTests(browser);
console.log(JSON.stringify(diagnostic));
```

선택 인자는 `{url: 'file:///.../index.html', screenshotDir: '...절대경로...'}`다. standalone 실행 시 `DIAGNOSTIC_SCREENSHOTS` 환경변수로 화면 저장 위치를 지정할 수도 있다. 결과의 `passed`는 assertion 수가 아니라 검사 그룹 수다. 현재 검사에서 외부 요청·console error·page exception은 없어야 한다.

## 남은 실제 구현

실제 API·인증·권한·schema·WebSocket reconnect·end-to-end operation 결과는 [T-402](../../docs/tasks/T-402-diagnostic-api-web.md), 실제 capture/evidence·Controller decoder 대조는 [T-403](../../docs/tasks/T-403-signal-lab-evidence.md) 범위다. 정적 asset gzip 제공·실제 streaming/virtual list·장치 저장·AP 물리 확인·Android/iOS 실기기 시험도 수행하지 않았다. 정상 `APPLIED`나 실제 capture 완료를 이 prototype으로 주장할 수 없다.
