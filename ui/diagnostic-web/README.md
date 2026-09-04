# Diagnostic Bridge 모바일 웹 prototype

이 폴더는 [CAN 신호 검증용 Diagnostic Bridge와 모바일 웹 UI 명세](../../docs/can-diagnostics-web.md)의 360–430 px 화면 구조를 검토하는 정적 prototype이다. 실차 CAN, ESP-NOW, REST, WebSocket에는 아직 연결되지 않았다.

브라우저에서 `index.html`을 열고 다음 query를 사용한다.

```text
index.html?screen=status
index.html?screen=frames
index.html?screen=capture
index.html?screen=signals
index.html?screen=settings
```

구현 시 지켜야 할 기준은 다음과 같다.

- 외부 CDN, font, analytics, internet API를 사용하지 않는다.
- 정적 asset은 gzip으로 Bridge Flash에서 제공한다.
- live table은 WebSocket delta를 최대 10 Hz로 받아 visible row만 갱신한다.
- raw CAN을 브라우저에서 차량으로 송신하는 handler를 만들지 않는다.
- setting 성공은 HTTP 응답이 아니라 target의 end-to-end `APPLIED` 상태 뒤에 표시한다.
- 실제 API, 권한, 상태와 payload는 명세 문서를 정본으로 사용한다.
