# CANView 정적 운전자 UI prototype

이 디렉터리는 320×480 Controller 화면의 정보 구조, 4WD 차량 비율, 경고 overlay, FFT와 설정 화면을 브라우저에서 검토하는 정적 prototype이다. 실차 CAN, ESP-NOW와 LVGL runtime에는 연결되지 않는다.

- 화면 규칙과 상태 표현: [운전자 UI 설계](../../docs/ui/design.md)
- 실제 LVGL adapter: [LVGL 구현](../lvgl/README.md)
- 미확정 signal 분석: [Diagnostic Bridge 웹 prototype](../diagnostic-web/README.md)

`index.html`을 브라우저에서 열어 확인한다. JavaScript 문법 검증은 Windows Node.js로 실행한다.

```powershell
node --check ui/prototype/prototype.js
```
