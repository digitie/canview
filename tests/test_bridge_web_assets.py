"""Diagnostic Bridge 정적 asset과 read-only web 경계의 회귀시험."""
from __future__ import annotations

import gzip
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "generate_bridge_web_assets.py"
HTML = ROOT / "ui" / "diagnostic-web" / "bridge-shell.html"
WEB_SOURCE = ROOT / "firmware" / "diagnostic-bridge" / "components" / "canview_bridge_web" / "canview_bridge_web.c"
WEB_HEADER = ROOT / "firmware" / "diagnostic-bridge" / "components" / "canview_bridge_web" / "include" / "canview_bridge_web.h"
WEB_DEFAULTS = ROOT / "firmware" / "diagnostic-bridge" / "sdkconfig.defaults"


class BridgeWebAssetTests(unittest.TestCase):
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
            self.assertEqual(gzip.decompress(bytes(values)), HTML.read_bytes())
            self.assertEqual(hashlib.sha256(bytes(values)).hexdigest(),
                             "8bf58e72c167c8d59f2ed48b888ca775c3a309b03d89308dc7d7d40c10f73cce")

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
        self.assertIn("button_hold_consumed", source)
        self.assertIn("vehicle_tx", source)
        self.assertIn("CONFIG_HTTPD_WS_SUPPORT=y", defaults)
        for forbidden in ("raw_replay", "canview_can_tx", "control_lease"):
            self.assertNotIn(forbidden, source.lower())


if __name__ == "__main__":
    unittest.main()
