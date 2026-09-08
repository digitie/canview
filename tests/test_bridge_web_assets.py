"""Diagnostic Bridge 정적 asset과 read-only web 경계의 회귀시험."""
from __future__ import annotations

import gzip
import hashlib
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "generate_bridge_web_assets.py"
HTML = ROOT / "ui" / "diagnostic-web" / "bridge-shell.html"
WEB_SOURCE = ROOT / "firmware" / "diagnostic-bridge" / "components" / "canview_bridge_web" / "canview_bridge_web.c"
WEB_SESSION_SOURCE = ROOT / "firmware" / "diagnostic-bridge" / "components" / "canview_bridge_web" / "canview_bridge_web_session.c"
WEB_SESSION_HEADER = ROOT / "firmware" / "diagnostic-bridge" / "components" / "canview_bridge_web" / "include" / "canview_bridge_web_session.h"
DNS_SOURCE = ROOT / "firmware" / "diagnostic-bridge" / "components" / "canview_bridge_web" / "dns_server.c"
APP_SOURCE = ROOT / "firmware" / "diagnostic-bridge" / "main" / "app_main.c"
WEB_HEADER = ROOT / "firmware" / "diagnostic-bridge" / "components" / "canview_bridge_web" / "include" / "canview_bridge_web.h"
WEB_DEFAULTS = ROOT / "firmware" / "diagnostic-bridge" / "sdkconfig.defaults"
SECURITY_SCRIPT = ROOT / "tests" / "security" / "bridge_http.py"


class BridgeWebAssetTests(unittest.TestCase):
    def test_canonical_gzip_stored_block_boundaries(self) -> None:
        spec = importlib.util.spec_from_file_location("bridge_asset_generator", SCRIPT)
        self.assertIsNotNone(spec)
        self.assertIsNotNone(spec.loader)
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        for source in (b"", b"a" * 65535, b"b" * 65536):
            compressed = generator.canonical_gzip(source)
            self.assertEqual(compressed[:10], b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\xff")
            self.assertEqual(gzip.decompress(compressed), source)

    def generate(self, output_dir: Path) -> None:
        result = subprocess.run(
            [sys.executable, "-B", str(SCRIPT), "--input", str(HTML), "--output-dir", str(output_dir)],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        self.assertIn("PASS: bridge asset", result.stdout)

    def test_gzip_asset_is_reproducible_and_exact(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = root / "first"
            second = root / "second"
            self.generate(first)
            self.generate(second)
            self.assertEqual((first / "bridge_assets.c").read_bytes(), (second / "bridge_assets.c").read_bytes())
            generated = (first / "bridge_assets.c").read_text(encoding="utf-8")
            values = [int(value, 16) for value in generated.split("const uint8_t canview_bridge_index_html_gz[] = {")[1].split("};", 1)[0].replace(",", " ").split()]
            compressed = bytes(values)
            self.assertEqual(compressed[:10], b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\xff")
            self.assertEqual(gzip.decompress(compressed), HTML.read_bytes())
            self.assertEqual(hashlib.sha256(bytes(values)).hexdigest(),
                             "0e92efdfeb3b8ccab8c3eb1da5116b2a2c6f337d080de15ebf653fa3cda43edc")

    def test_shell_does_not_persist_token_or_call_external_network(self) -> None:
        body = HTML.read_text(encoding="utf-8").lower()
        self.assertNotIn("localstorage", body)
        self.assertNotIn("sessionstorage", body)
        self.assertNotIn("fetch('http", body)
        self.assertNotIn('fetch("http', body)
        self.assertIn("차량 can 송신 없음", body)
        self.assertIn("new websocket", body)
        self.assertIn("await resync()", body)
        self.assertIn("snapshot_revision", body)
        self.assertNotIn("?token", body)

    def test_web_source_keeps_fixed_read_only_boundary(self) -> None:
        source = WEB_SOURCE.read_text(encoding="utf-8")
        session_source = WEB_SESSION_SOURCE.read_text(encoding="utf-8")
        session_header = WEB_SESSION_HEADER.read_text(encoding="utf-8")
        dns_source = DNS_SOURCE.read_text(encoding="utf-8")
        app_source = APP_SOURCE.read_text(encoding="utf-8")
        header = WEB_HEADER.read_text(encoding="utf-8")
        defaults = WEB_DEFAULTS.read_text(encoding="utf-8")
        self.assertIn("#define CANVIEW_BRIDGE_WEB_MAX_JSON_BYTES (8192U)", header)
        self.assertIn("#define CANVIEW_BRIDGE_WEB_MAX_WS_FRAME_BYTES (512U)", header)
        self.assertIn("CANVIEW_BRIDGE_WEB_MAX_JSON_BYTES", source)
        self.assertIn("CANVIEW_BRIDGE_WEB_MAX_WS_FRAME_BYTES", source)
        self.assertIn("httpd_ws_recv_frame", source)
        self.assertIn("frame.len > CANVIEW_BRIDGE_WEB_MAX_WS_FRAME_BYTES", source)
        self.assertIn("json_nesting_bounded", source)
        self.assertIn("canview_bridge_web_stop", source)
        self.assertIn("httpd_sess_trigger_close", source)
        self.assertIn("close_status", source)
        self.assertIn("button_hold_consumed", source)
        self.assertIn("canview_bridge_web_session_is_closing", source)
        self.assertIn("canview_bridge_web_session_begin_close", source)
        self.assertIn("token_authenticated_and_record", source)
        self.assertIn("Authentication and activity recording share one lock", source)
        self.assertIn("Do not tear down Wi-Fi while the DNS task may still use its socket", source)
        self.assertIn("open_fn = open_connection", source)
        self.assertIn("httpd_sess_set_recv_override", source)
        self.assertIn("receive_with_pre_auth_deadline", source)
        self.assertIn("CANVIEW_BRIDGE_WEB_PRE_AUTH_TIMEOUT_MS", source)
        self.assertIn("close(client_fd)", source)
        self.assertIn("session_close_pending", session_header)
        self.assertIn("canview_bridge_web_session_idle_expired", session_source)
        self.assertIn("canview_bridge_web_session_close", session_source)
        self.assertIn("CANVIEW_BRIDGE_WEB_CLIENT_IDLE_TIMEOUT_MS", source)
        self.assertIn("web_client_idle_expired", source)
        self.assertIn("max_open_sockets = 1U", source)
        self.assertIn("Keep the handle, locks, and auth state alive", source)
        self.assertNotIn("(void)httpd_stop", source)
        enter_body = source.split("static esp_err_t enter_request", 1)[1].split(
            "static void leave_request", 1
        )[0]
        self.assertNotIn("token_authenticated_and_record", enter_body)
        self.assertIn("const bool valid = allowed && token_authenticated_and_record", source)
        self.assertIn("const esp_err_t cleanup_status = stop_web_with_retry()", app_source)
        self.assertIn("volatile bool stopped", dns_source)
        self.assertIn("vTaskSuspend(NULL)", dns_source)
        self.assertIn("vTaskDelete(task)", dns_source)
        self.assertIn("vehicle_tx", source)
        self.assertIn("CONFIG_HTTPD_WS_SUPPORT=y", defaults)
        for forbidden in ("raw_replay", "canview_can_tx", "control_lease"):
            self.assertNotIn(forbidden, source.lower())

    def test_live_probe_rejects_non_local_urls(self) -> None:
        spec = importlib.util.spec_from_file_location("bridge_http_contract", SECURITY_SCRIPT)
        self.assertIsNotNone(spec)
        self.assertIsNotNone(spec.loader)
        contract = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(contract)
        for url in (
            "https://127.0.0.1",
            "http://evil.invalid",
            "http://user@127.0.0.1",
            "http://127.0.0.1/?token=leak",
            "http://127.0.0.1/bridge",
        ):
            with self.assertRaises(contract.ContractError):
                contract._request(url, "GET", "/", timeout=0.1)


if __name__ == "__main__":
    unittest.main()
