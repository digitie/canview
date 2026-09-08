"""T-103 capture helper의 bounded/no-TX contract 회귀."""

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest


TESTS_ROOT = Path(__file__).resolve().parent
if str(TESTS_ROOT) not in sys.path:
    sys.path.insert(0, str(TESTS_ROOT))

from hil.assert_no_tx import assert_no_tx
from hil.run_can_capture import main as run_capture


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests" / "hil" / "fixtures" / "t103-capture-only.jsonl"


class T103CaptureHelperTests(unittest.TestCase):
    def test_positive_fixture_has_no_tx(self) -> None:
        status, message = assert_no_tx(FIXTURE)
        self.assertEqual(0, status, message)

    def test_tx_and_invalid_records_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "evidence.jsonl"
            lines = FIXTURE.read_text(encoding="utf-8").splitlines()
            records = [json.loads(line) for line in lines]
            records[0]["fields"]["ack_frames"] = 1
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = assert_no_tx(path)
            self.assertEqual(1, status, message)
            records[0]["fields"]["ack_frames"] = 0
            records[3]["fields"]["vehicle_tx"] = 1
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = assert_no_tx(path)
            self.assertEqual(2, status, message)
            path.write_text(
                json.dumps({"schema_version": 1, "source": "x", "kind": "CAN_TX",
                            "sequence": 1, "monotonic_ns": 1, "log_offset": 0,
                            "fields": {"arbitration_id": 1}}) + "\n",
                encoding="utf-8",
            )
            status, message = assert_no_tx(path)
            self.assertEqual(2, status, message)
            path.write_text(
                '{"schema_version":1,"source":"x","kind":"CAN_RX",'
                '"sequence":1,"sequence":1,"monotonic_ns":1,"log_offset":0,"fields":{}}\n',
                encoding="utf-8",
            )
            status, message = assert_no_tx(path)
            self.assertEqual(2, status, message)

    def test_truncated_oversized_and_replayed_evidence_is_blocked(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "evidence.jsonl"
            fixture = FIXTURE.read_bytes()
            path.write_bytes(fixture[:-1])
            status, message = assert_no_tx(path)
            self.assertEqual(2, status, message)
            path.write_bytes(fixture + fixture)
            status, message = assert_no_tx(path)
            self.assertEqual(2, status, message)
            path.write_bytes(b"{" + b"\"n\":\"" + b"x" * (1 << 20) + b"\"}\n")
            status, message = assert_no_tx(path)
            self.assertEqual(2, status, message)

    def test_missing_schema_and_wide_integer_are_blocked(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "evidence.jsonl"
            path.write_text("{}\n", encoding="utf-8")
            status, message = assert_no_tx(path)
            self.assertEqual(2, status, message)
            path.write_text(
                '{"schema_version":1,"source":"x","kind":"CAN_RX",'
                '"sequence":999999999999999999999999999999,"monotonic_ns":1,'
                '"log_offset":0,"fields":{}}\n',
                encoding="utf-8",
            )
            status, message = assert_no_tx(path)
            self.assertEqual(2, status, message)

    def test_missing_evidence_is_not_run(self) -> None:
        status, message = assert_no_tx(Path("missing-t103-evidence.jsonl"))
        self.assertEqual(2, status, message)

    def test_capture_wrapper_rejects_wrong_channel_count(self) -> None:
        self.assertEqual(2, run_capture(["--channels", "2"]))


if __name__ == "__main__":
    unittest.main()
