# Diagnostic Bridge local web shell

이 문서는 `T-400`의 현재 C 구현을 설명한다. 대상은 `ESP32-S3-WROOM-1-N8R2`와 ESP-IDF `6.0.3`이다. 현재 구현은 휴대폰용 local HTTP shell, service-session 인증, 빈 read-only snapshot과 WebSocket bootstrap event까지다. ESP-NOW observer, 실제 CAN frame 수집, capture, Signal Lab, encrypted provisioning은 이 변경에 포함하지 않는다.

## 책임과 실행 모델

| 항목 | 현재 계약 |
|---|---|
| composition owner | `firmware/diagnostic-bridge/main/app_main.c`의 ESP-IDF main task |
| web owner | `canview_bridge_web.c`의 singleton state와 HTTP server task |
| portable auth | `canview_bridge_auth`의 SDK-independent C99 상태기계 |
| BSP | GPIO4 service input, GPIO5 status LED, board profile과 memory contract |
| poll 주기 | main task가 `100 ms`마다 `canview_bridge_web_poll()` 호출 |
| HTTP stack | ESP-IDF `esp_http_server` |
| JSON | `cJSON`, 16 KiB fixed arena, 응답 4 KiB 이하 |
| WebSocket | `esp_http_server` WebSocket, incoming frame 512 byte 이하, server event 1회 |
| DNS | fixed-buffer UDP captive response task, 외부 DNS forwarding 없음 |
| shutdown | `canview_bridge_web_stop()`이 HTTP/DNS/Wi-Fi/netif/event-loop와 memory auth를 idempotent하게 정리한다. DNS task가 제한시간 안에 끝나지 않으면 timeout을 반환하고 재시도를 허용한다. |

HTTP handler는 `request_lock`으로 singleton JSON arena, request body와 response buffer만 직렬화하고, `state_lock`은 auth/session·snapshot의 짧은 critical section에서만 사용한다. 따라서 body 수신·HTTP response·WebSocket send가 owner state mutex를 붙잡지 않는다. WebSocket의 bounded network I/O는 별도 `ws_io_lock`으로 직렬화하고 send가 끝날 때까지 response buffer 수명을 보장한다. cJSON allocator hook은 이 singleton component에서 한 번 등록되며 다른 cJSON task와 공유하지 않는다. session body, response와 protocol token buffer는 사용 후 zeroize한다.

`canview_bridge_web_config_t`의 credential 포인터는 start 호출 중에만 유효하면 된다. PIN digest는 auth state로 복사하고, AP password는 `esp_wifi_set_config()`에 복사한 직후 web state에서 zeroize한다. app도 start 반환 뒤 local credential buffer를 zeroize한다. callback은 `button_pressed` 하나만 남으며 web service가 정지할 때까지 caller가 수명을 보장해야 한다. ISR은 button callback이나 web API를 호출하지 않는다.

## 부팅과 service window

1. BSP board profile을 SDK/GPIO open보다 먼저 검사한다.
2. safe GPIO, runtime, deferred core health와 fixed pool을 초기화한다. 이 단계에서는 외부 초기화 중 TWDT subscription을 아직 만들지 않는다.
3. read-only NVS에서 `bridge_auth/pin_digest`와 `bridge_auth/ap_password`를 읽는다. 누락·길이·문자 검사는 실패로 처리하며 기본 credential을 만들지 않는다.
4. `WIFI_MODE_APSTA`를 시작한다. AP channel은 KR channel `6`으로 고정하고 STA에 external AP credential을 설정하거나 `esp_wifi_connect()`를 호출하지 않는다.
5. DNS와 HTTP server를 시작한다. 외부 AP/NAPT와 vehicle CAN path는 없다.
6. web start 성공 직후 같은 owner가 `canview_esp_core_arm_watchdog()`를 호출하고서 service loop를 시작한다. 실패하면 web 자원을 중지하고 safe idle로 남는다.
7. GPIO4가 3초 연속 low일 때만 10분 service window를 연다. release는 window를 닫지 않지만 timeout과 reset/재부팅은 session을 폐기하고 active HTTP/WebSocket client를 닫는다.

window가 닫힌 동안 `/api/v1/bootstrap`은 challenge를 발급하지 않는다. window가 열린 뒤 challenge를 발급하고, 같은 window의 challenge 재발급은 이전 challenge를 폐기한다.

## REST와 인증

| method/path | 현재 동작 | 권한 |
|---|---|---|
| `GET /` | gzip 내장 offline shell | public, vehicle 정보 없음 |
| `GET /api/v1/bootstrap` | challenge와 capability 0 반환 | service window + rate limit |
| `POST /api/v1/session` | challenge·client nonce·6–8자리 PIN으로 memory token 발급 | 허용 Origin + rate limit |
| `DELETE /api/v1/session` | memory token 폐기 | 허용 Origin + bearer token |
| `GET /api/v1/system` | safe metadata와 `NOT_IMPLEMENTED` observer 상태 | bearer token |
| `GET /api/v1/peers`, `/buses`, `/frames`, `/filters`, `/captures`, `/candidates`, `/config-targets` | 빈 `items` snapshot | bearer token |
| `GET /api/v1/live` | WebSocket handshake와 summary event | 허용 Origin + session subprotocol token |

session JSON은 top-level object의 정확한 세 필드만 허용한다. unknown field, duplicate field, non-string, truncated/malformed JSON, trailing data는 거부한다. JSON body는 `8192` byte, URI는 `128` byte, request header는 `1024` byte, WebSocket incoming frame은 `512` byte로 제한한다. mutation 계열은 1초당 5건으로 제한한다. PIN 실패는 1분 내 5회에서 60초 lockout한다.

token은 128-bit memory-only value다. REST는 `Authorization: Bearer <base64url>`을 사용하고, WebSocket은 query string이 아니라 `Sec-WebSocket-Protocol`에 `canview-session`과 `canview-session.<token>`을 함께 보낸다. 서버는 pre-handshake와 post-handshake에서 token을 다시 검사한다. token을 URL, NVS, log, localStorage에 저장하지 않는다. 현재 HTML은 외부 CDN, analytics, internet request를 사용하지 않는다.

모든 snapshot과 event에는 `snapshot_revision`을 포함하고 WebSocket event에는 증가하는 `seq`와 `server_time_ms`를 포함한다. 실제 observer data가 연결되기 전까지 상태는 `NOT_IMPLEMENTED` 또는 `UNKNOWN`이며 확정 차량 신호로 표시하지 않는다.

## 안전 경계

- capability `control_scope`는 항상 `0`이고 `vehicle_tx`는 항상 `false`다.
- `canview_bridge_web`에는 raw CAN frame builder, replay endpoint, control lease, vehicle TX queue가 없다.
- WebSocket incoming data는 bounded receive 후 폐기하며 command/parser로 전달하지 않는다.
- Diagnostic Bridge는 read-only observation 장치다. 이 shell의 인증은 vehicle command 권한을 만들지 않는다.
- local HTTP와 WPA2 AP password는 편의·접근 제어일 뿐 production trust anchor가 아니다. secure boot, flash/NVS encryption, device provisioning과 physical evidence는 별도 gate다.

## 검증과 남은 gate

현재 source/target 검증은 다음을 사용한다.

```powershell
python -B tools/generate_boards.py --check
python -B tools/validate_document_links.py
python -B tools/check_sdkconfig.py firmware/diagnostic-bridge/sdkconfig --board bridge-r1-n8r2
Push-Location firmware/diagnostic-bridge
idf.py build
idf.py size-components
Pop-Location
```

host auth CTest는 service window, null/bounds, malformed PIN, one-time challenge, token expiry, lockout과 clock rollback을 확인한다. 실제 ESP32 flash, ST-LINK/serial, AP association, Android/iOS captive browser, power rail/reset/brownout, PSRAM/clock/watchdog soak, ESP-NOW, 차량 CAN/capture와 production provisioning은 현재 실행하지 않았으며 `NOT_RUN`이다. CAN TX는 계속 `NO-GO`다.
