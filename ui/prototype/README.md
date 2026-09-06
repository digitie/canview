# CANView 정적 운전자 UI prototype

이 디렉터리는 320×480 Controller 화면의 정보 구조, 4WD 차량 비율, 경고 overlay, FFT와 설정 화면을 브라우저에서 검토하는 정적 prototype이다. 실차 CAN, ESP-NOW와 LVGL runtime에는 연결되지 않는다.

- 화면 규칙과 상태 표현: [운전자 UI 설계](../../docs/ui/design.md)
- 실제 LVGL adapter: [LVGL 구현](../lvgl/README.md)
- 미확정 signal 분석: [Diagnostic Bridge 웹 prototype](../diagnostic-web/README.md)

`index.html`을 브라우저에서 열어 확인한다. JavaScript 문법 검증은 Windows Node.js로 실행한다.

```powershell
node --check ui/prototype/prototype.js
```

## 화면과 실패 fixture

`?screen=drive|audio|automation|fft|settings`로 다섯 화면을 연다. 기본 주행 화면은 경고 없는 합성 DEMO다. `parked=1`은 설정 편집 가능한 정차 fixture, `scroll=bottom`은 하단 설정 확인용이다.

- `mode=normal|sport|eco`: 실제 모드 표시의 청색/적색/녹색 일관성
- `state=offline|candidate`: 미수신/미확정 값을 `—`로 숨김
- `stale=speed,rpm,torque-fl,tpms-fr,fft`: 필드별 품질 독립성; 쉼표로 추가
- `limit=1`, `warning=1`, `headlamp=1`: 제한속도/초과/일몰 경고 fixture
- `night=1`: 야간 톤. 실제 PWM·미등 CAN 연결은 별도 firmware gate

경고는 터치를 가로채지 않으며 stale speed에서는 초과 경고를 만들지 않는다. 설정은 speed unknown/moving 때 잠긴다. 날짜·시각은 고정 선택지와 윤년 규칙을 적용하고 적용 버튼 전에는 초안이다. 유휴 시간은 기본 30초이며 터치 시 복귀한다. 오디오·자동화 버튼은 **로컬 DEMO 상태만** 바꾸고 차량 명령이나 `APPLIED` 결과를 생성하지 않는다. 이 query fixture는 실제 sample aging·protocol 수신·RTC·자동화 task를 구현한 것이 아니다.

## 동작·스크린샷 검증

설치된 Playwright와 Windows Edge로 실행한다. 번들 runtime 경로는 설치 환경에 맞춘다.

```powershell
$env:NODE_PATH = 'C:/Users/digit/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules'
node tools/ui/check-browser.cjs
node tools/ui/check-browser.cjs --screenshots
```

두 번째 명령은 `docs/images/ui-*.png`를 재생성한다. 오프라인 5화면, 44px 조작, 미수신·부분 stale, 경고 우선순위/터치, 분 0–59·윤년, 설정 잠금, 유휴 복귀를 검사한다. 실물 LCD의 성능과 Android/iOS 브라우저 시험을 대신하지 않는다.
