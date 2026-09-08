"""Diagnostic Bridge HTTP/WebSocket 보안 contract와 선택적 live probe.

기본 실행은 Git에 있는 source/config contract만 검사한다. 실제 ESP32 endpoint
검사는 `CANVIEW_BRIDGE_URL` 또는 `--live-url`을 명시한 경우에만 수행하며, 주소가
없을 때는 physical/HIL 성공으로 간주하지 않고 `NOT_RUN`으로 출력한다.
"""
from __future__ import annotations

import argparse
import http.client
import os
from pathlib import Path
import sys
from urllib.parse import urlsplit


ROOT = Path(__file__).resolve().parents[2]
WEB_SOURCE_PATH = ROOT / "firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web.c"
WEB_SESSION_SOURCE_PATH = ROOT / "firmware/diagnostic-bridge/components/canview_bridge_web/canview_bridge_web_session.c"
WEB_SESSION_HEADER_PATH = ROOT / "firmware/diagnostic-bridge/components/canview_bridge_web/include/canview_bridge_web_session.h"
WEB_HEADER_PATH = ROOT / "firmware/diagnostic-bridge/components/canview_bridge_web/include/canview_bridge_web.h"
WEB_CMAKE_PATH = ROOT / "firmware/diagnostic-bridge/components/canview_bridge_web/CMakeLists.txt"
WEB_DEFAULTS_PATH = ROOT / "firmware/diagnostic-bridge/sdkconfig.defaults"
APP_SOURCE_PATH = ROOT / "firmware/diagnostic-bridge/main/app_main.c"
BROWSER_TEST_PATH = ROOT / "tests/ui/diagnostic-browser.cjs"
ALLOWED_LIVE_HOSTS = frozenset({"127.0.0.1", "::1", "localhost", "192.168.4.1",
                                "canview-diag.local"})


class ContractError(RuntimeError):
    """Source contract가 누락되었을 때 발생한다."""


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as error:
        raise ContractError(f"파일을 읽을 수 없음: {path}: {error}") from error


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise ContractError(f"{label}에 필요한 contract가 없음: {needle}")


def check_static_contract() -> int:
    """Bridge의 실행 경계가 source/config에 남아 있는지 검사한다."""
    source = read_text(WEB_SOURCE_PATH)
    session_source = read_text(WEB_SESSION_SOURCE_PATH)
    session_header = read_text(WEB_SESSION_HEADER_PATH)
    header = read_text(WEB_HEADER_PATH)
    cmake = read_text(WEB_CMAKE_PATH)
    defaults = read_text(WEB_DEFAULTS_PATH)
    app_source = read_text(APP_SOURCE_PATH)

    required_source = (
        "#include \"esp_http_server.h\"",
        "#include \"cJSON.h\"",
        "httpd_ws_recv_frame",
        "httpd_ws_send_frame",
        "ws_pre_handshake",
        "ws_post_handshake",
        "origin_allowed(request, true)",
        "parse_ws_token",
        "canview_bridge_web_session_is_closing",
        "canview_bridge_web_session_begin_close",
        "pre_auth_client_expired_locked",
        "pre_auth_close_pending",
        "token_authenticated_and_record",
        "Authentication and activity recording share one lock",
        "Do not tear down Wi-Fi while the DNS task may still use its socket",
        "open_fn = open_connection",
        "httpd_sess_set_recv_override",
        "httpd_sess_set_send_override",
        "send_with_worker_deadline",
        "acknowledge_httpd_heartbeat",
        "receive_with_pre_auth_deadline",
        "start_request_deadline",
        "request_deadline_remaining",
        "request_deadline_remaining_locked",
        "request_deadline_valid",
        "CANVIEW_BRIDGE_WEB_REQUEST_TIMEOUT_MS",
        "SO_SNDTIMEO",
        "clear_request_deadline_locked",
        "CANVIEW_BRIDGE_WEB_PRE_AUTH_TIMEOUT_MS",
        "CANVIEW_BRIDGE_WEB_WORKER_HEARTBEAT_TIMEOUT_MS",
        "httpd_queue_work",
        "esp_task_wdt_add_user",
        "esp_task_wdt_reset_user",
        "SO_RCVTIMEO",
        "pre_auth_deadline_remaining",
        "arm_pre_auth_client",
        "return HTTPD_SOCK_ERR_FAIL",
        "web_client_idle_expired",
        "HTTPD select() is not bounded by the socket receive timeout",
        "http_config.max_open_sockets = 1U",
        "http_config.lru_purge_enable = false",
        "http_config.recv_wait_timeout = 5U",
        "http_config.send_wait_timeout = 1U",
        "frame.len > CANVIEW_BRIDGE_WEB_MAX_WS_FRAME_BYTES",
        "json_nesting_bounded",
        "CANVIEW_BRIDGE_WEB_MAX_JSON_BYTES",
        "CANVIEW_BRIDGE_WEB_CLIENT_IDLE_TIMEOUT_MS",
        "const esp_err_t close_status",
        "control_scope",
        "vehicle_tx",
    )
    for needle in required_source:
        require(source, needle, "canview_bridge_web.c")

    enter_body = source.split("static esp_err_t enter_request", 1)[1].split(
        "static void leave_request", 1
    )[0]
    if "token_authenticated_and_record" in enter_body:
        raise ContractError("pre-auth enter_request가 logical session activity를 갱신함")
    require(source, "const bool valid = allowed && token_authenticated_and_record",
            "WebSocket authentication activity")
    require(app_source, "const esp_err_t cleanup_status = stop_web_with_retry()",
            "startup cleanup retry")

    for needle in (
        "session_close_pending",
        "canview_bridge_web_session_record_activity",
        "canview_bridge_web_session_idle_expired",
        "canview_bridge_web_session_begin_close",
        "canview_bridge_web_session_close",
    ):
        require(session_header + session_source, needle, "canview_bridge_web_session C99 seam")

    required_header = (
        "#define CANVIEW_BRIDGE_WEB_MAX_JSON_BYTES (8192U)",
        "#define CANVIEW_BRIDGE_WEB_MAX_WS_FRAME_BYTES (512U)",
        "#define CANVIEW_BRIDGE_WEB_CLIENT_IDLE_TIMEOUT_MS (300000U)",
        "esp_err_t canview_bridge_web_stop(void);",
    )
    for needle in required_header:
        require(header, needle, "canview_bridge_web.h")

    for needle in ("esp_http_server", "cjson", "canview_bridge_auth"):
        require(cmake.lower(), needle.lower(), "Bridge web component CMake")
    for needle in (
        "CONFIG_HTTPD_WS_SUPPORT=y",
        "CONFIG_HTTPD_WS_PRE_HANDSHAKE_CB_SUPPORT=y",
        "CONFIG_HTTPD_WS_POST_HANDSHAKE_CB_SUPPORT=y",
    ):
        require(defaults, needle, "Bridge sdkconfig.defaults")

    for forbidden in (
        "raw_replay",
        "canview_can_tx",
        "control_lease",
        "esp_wifi_connect(",
        "esp_netif_napt_enable",
        "HTTP_PUT",
        "HTTP_PATCH",
    ):
        if forbidden.lower() in source.lower():
            raise ContractError(f"Bridge read-only contract 위반 문자열: {forbidden}")

    if not BROWSER_TEST_PATH.is_file():
        raise ContractError(f"offline browser test가 없음: {BROWSER_TEST_PATH}")
    return len(required_source) + len(required_header) + 3 + 5


def _request(base_url: str, method: str, path: str, *, headers: dict[str, str] | None = None,
             body: bytes | None = None, timeout: float) -> int:
    parsed = urlsplit(base_url)
    if (parsed.scheme != "http" or not parsed.hostname or parsed.username is not None or
            parsed.password is not None or parsed.query or parsed.fragment or
            parsed.path not in ("", "/") or parsed.hostname.lower() not in ALLOWED_LIVE_HOSTS):
        raise ContractError("live probe는 허용된 local HTTP host만 사용한다")
    port = parsed.port or 80
    connection = http.client.HTTPConnection(parsed.hostname, port, timeout=timeout)
    try:
        connection.request(method, path, body=body, headers=headers or {})
        response = connection.getresponse()
        response.read(4096)
        return response.status
    finally:
        connection.close()


def _expect_status(label: str, status: int, expected: set[int]) -> None:
    if status not in expected:
        expected_text = ",".join(str(value) for value in sorted(expected))
        raise ContractError(f"live probe {label}: HTTP {status}, expected one of {expected_text}")


def check_live_endpoint(base_url: str, timeout: float) -> int:
    """실제 endpoint에서 무권한·malformed·oversize·WS upgrade 거부를 확인한다."""
    base = base_url.rstrip("/")
    count = 0
    _expect_status("unauthenticated system", _request(base, "GET", "/api/v1/system", timeout=timeout), {401})
    count += 1
    _expect_status("unauthenticated peers", _request(base, "GET", "/api/v1/peers", timeout=timeout), {401})
    count += 1

    rejected_origin_headers = {
        "Content-Type": "application/json",
        "Origin": "http://evil.invalid",
    }
    _expect_status(
        "unsupported Origin",
        _request(base, "POST", "/api/v1/session", headers=rejected_origin_headers,
                 body=b"{}", timeout=timeout),
        {403},
    )
    count += 1

    oversize_headers = {
        "Content-Type": "application/json",
        "Origin": "http://192.168.4.1",
        "Content-Length": "8193",
    }
    _expect_status(
        "oversize session body",
        _request(base, "POST", "/api/v1/session", headers=oversize_headers,
                 body=b"{" + b"a" * 8191 + b"}", timeout=timeout),
        {400, 408, 413, 429},
    )
    count += 1

    malformed_headers = {
        "Content-Type": "application/json",
        "Origin": "http://192.168.4.1",
    }
    _expect_status(
        "duplicate/malformed session JSON",
        _request(base, "POST", "/api/v1/session", headers=malformed_headers,
                 body=b'{"challenge":"x","challenge":"y","nonce":"x","pin":"123456"}',
                 timeout=timeout),
        {400, 401, 403, 408, 429},
    )
    count += 1

    ws_headers = {
        "Connection": "Upgrade",
        "Upgrade": "websocket",
        "Origin": "http://192.168.4.1",
        "Sec-WebSocket-Version": "13",
        "Sec-WebSocket-Key": "dGVzdC1jYW52aWV3LWtleQ==",
        "Sec-WebSocket-Protocol": "canview-session, canview-session.invalid",
    }
    ws_status = _request(base, "GET", "/api/v1/live?token=forbidden", headers=ws_headers, timeout=timeout)
    _expect_status("query-token WebSocket rejection", ws_status, {400, 401, 403})
    count += 1
    return count


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--live-url", default=os.environ.get("CANVIEW_BRIDGE_URL"))
    parser.add_argument("--timeout", type=float, default=2.0)
    parser.add_argument("--require-live", action="store_true",
                        help="live URL가 없으면 NOT_RUN을 오류로 취급한다")
    args = parser.parse_args(argv)
    if args.timeout <= 0.0:
        parser.error("--timeout은 양수여야 한다")

    try:
        static_count = check_static_contract()
        print(f"PASS: Bridge HTTP/WebSocket read-only source contract ({static_count} checks)")
        if not args.live_url:
            print("NOT_RUN: CANVIEW_BRIDGE_URL이 없어 ESP32 HTTP/WebSocket live probe를 실행하지 않음")
            return 2 if args.require_live else 0
        live_count = check_live_endpoint(args.live_url, args.timeout)
        print(f"PASS: Bridge HTTP/WebSocket live rejection probe ({live_count} checks)")
        return 0
    except (ContractError, OSError, ValueError, http.client.HTTPException) as error:
        print(f"FAIL: Bridge HTTP/WebSocket contract: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
