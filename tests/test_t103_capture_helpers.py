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

from hil.assert_no_tx import assert_no_tx, main as assert_no_tx_main
from hil.run import _git_commit, _source_digest
from hil.run_can_capture import main as run_capture


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests" / "hil" / "fixtures" / "t103-capture-only.jsonl"
EXPECTED_SOURCE = "t103-fixture"
EXPECTED_EXECUTION_ID = "T103-FIXTURE-001"
EXPECTED_COMMIT = _git_commit()
EXPECTED_FIRMWARE_IDENTITY = "f9ea109772edef0743fd22899f9c6c6d8c6035c3a43c03090a6d709bc2309212"
EXPECTED_HARNESS_IDENTITY = _source_digest((ROOT / "tests" / "hil",))


class T103CaptureHelperTests(unittest.TestCase):
    @staticmethod
    def _assert(path: Path) -> tuple[int, str]:
        return assert_no_tx(
            path,
            expected_source=EXPECTED_SOURCE,
            expected_execution_id=EXPECTED_EXECUTION_ID,
            expected_firmware_identity=EXPECTED_FIRMWARE_IDENTITY,
        )

    def test_positive_fixture_has_no_tx(self) -> None:
        status, message = self._assert(FIXTURE)
        self.assertEqual(0, status, message)
        self.assertEqual(2, assert_no_tx(FIXTURE)[0])

    def test_capture_wrapper_validates_runner_event_identity_and_no_tx(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "capture"
            self.assertEqual(
                0,
                run_capture(["--output", str(output), "--expected-commit", EXPECTED_COMMIT,
                             "--expected-firmware-source-sha256", EXPECTED_FIRMWARE_IDENTITY,
                             "--expected-harness-source-sha256", EXPECTED_HARNESS_IDENTITY]),
            )
            report = json.loads((output / "report.json").read_text(encoding="utf-8"))
            event_identity = report["event_identity"]
            records = (output / report["scenario_results"][0]["events_file"]).read_text(
                encoding="utf-8").splitlines()
            self.assertGreater(len(records), 0)
            for line in records:
                fields = json.loads(line)["fields"]
                self.assertEqual(event_identity["execution_id"], fields["execution_id"])
                self.assertEqual(event_identity["firmware_identity"],
                                 fields["firmware_identity"])

    def test_tx_and_invalid_records_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "evidence.jsonl"
            lines = FIXTURE.read_text(encoding="utf-8").splitlines()
            records = [json.loads(line) for line in lines]
            records[0]["fields"]["ack_frames"] = 1
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = self._assert(path)
            self.assertEqual(1, status, message)
            records[0]["fields"]["ack_frames"] = 0
            records[3]["fields"]["vehicle_tx"] = 1
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = self._assert(path)
            self.assertEqual(2, status, message)
            path.write_text(
                json.dumps({"schema_version": 1, "source": "x", "kind": "CAN_TX",
                            "sequence": 1, "monotonic_ns": 1, "log_offset": 0,
                            "fields": {"arbitration_id": 1}}) + "\n",
                encoding="utf-8",
            )
            status, message = self._assert(path)
            self.assertEqual(2, status, message)
            path.write_text(
                '{"schema_version":1,"source":"x","kind":"CAN_RX",'
                '"sequence":1,"sequence":1,"monotonic_ns":1,"log_offset":0,"fields":{}}\n',
                encoding="utf-8",
            )
            status, message = self._assert(path)
            self.assertEqual(2, status, message)

    def test_nested_budget_tx_fields_are_blocked(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "evidence.jsonl"
            records = [json.loads(line) for line in FIXTURE.read_text(
                encoding="utf-8").splitlines()]
            budget_record = dict(records[0])
            budget_record["kind"] = "BUDGET_SAMPLE"
            budget_record["fields"] = {
                "execution_id": EXPECTED_EXECUTION_ID,
                "firmware_identity": EXPECTED_FIRMWARE_IDENTITY,
                "metrics": {"map_bytes": 8192, "tx_frames": 1},
            }
            records.insert(3, budget_record)
            for index, record in enumerate(records, 1):
                record["sequence"] = index
                record["monotonic_ns"] = index * 100
                record["log_offset"] = (index - 1) * 180
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = self._assert(path)
            self.assertEqual(2, status, message)

    def test_executed_command_replay_is_not_capture_only(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "evidence.jsonl"
            records = [json.loads(line) for line in FIXTURE.read_text(
                encoding="utf-8").splitlines()]
            replay = dict(records[0])
            replay["kind"] = "COMMAND_REPLAY"
            replay["fields"] = {
                "execution_id": EXPECTED_EXECUTION_ID,
                "firmware_identity": EXPECTED_FIRMWARE_IDENTITY,
                "request_token": "token-001",
                "executed": True,
                "result": "ACCEPTED",
            }
            records.insert(3, replay)
            for index, record in enumerate(records, 1):
                record["sequence"] = index
                record["monotonic_ns"] = index * 100
                record["log_offset"] = (index - 1) * 180
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = self._assert(path)
            self.assertEqual(1, status, message)

    def test_truncated_oversized_and_replayed_evidence_is_blocked(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "evidence.jsonl"
            fixture = FIXTURE.read_bytes()
            path.write_bytes(fixture[:-1])
            status, message = self._assert(path)
            self.assertEqual(2, status, message)
            path.write_bytes(fixture + fixture)
            status, message = self._assert(path)
            self.assertEqual(2, status, message)
            path.write_bytes(b"{" + b"\"n\":\"" + b"x" * (1 << 20) + b"\"}\n")
            status, message = self._assert(path)
            self.assertEqual(2, status, message)

    def test_missing_schema_and_wide_integer_are_blocked(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "evidence.jsonl"
            path.write_text("{}\n", encoding="utf-8")
            status, message = self._assert(path)
            self.assertEqual(2, status, message)
            path.write_text(
                '{"schema_version":1,"source":"x","kind":"CAN_RX",'
                '"sequence":999999999999999999999999999999,"monotonic_ns":1,'
                '"log_offset":0,"fields":{}}\n',
                encoding="utf-8",
            )
            status, message = self._assert(path)
            self.assertEqual(2, status, message)

    def test_unknown_semantics_and_identity_are_blocked(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "evidence.jsonl"
            records = [json.loads(line) for line in FIXTURE.read_text(
                encoding="utf-8").splitlines()]
            records[0]["kind"] = "CAN_ACK"
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = self._assert(path)
            self.assertEqual(2, status, message)

            records[0]["kind"] = "CAN_CHANNEL_SUMMARY"
            records[0]["fields"]["unknown_tx"] = True
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = self._assert(path)
            self.assertEqual(2, status, message)

            del records[0]["fields"]["unknown_tx"]
            records[0]["fields"]["execution_id"] = "different-execution"
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = self._assert(path)
            self.assertEqual(2, status, message)

            records[0]["fields"]["execution_id"] = EXPECTED_EXECUTION_ID
            records[0]["source"] = "unrelated-run"
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = self._assert(path)
            self.assertEqual(2, status, message)

            records[0]["source"] = EXPECTED_SOURCE
            tx_record = dict(records[0])
            tx_record["kind"] = "CAN_TX"
            tx_record["fields"] = {
                "execution_id": EXPECTED_EXECUTION_ID,
                "firmware_identity": EXPECTED_FIRMWARE_IDENTITY,
            }
            records.insert(3, tx_record)
            for index, record in enumerate(records, 1):
                record["sequence"] = index
                record["monotonic_ns"] = index * 100
                record["log_offset"] = (index - 1) * 180
            path.write_text("".join(json.dumps(record) + "\n" for record in records),
                            encoding="utf-8")
            status, message = self._assert(path)
            self.assertEqual(1, status, message)

    def test_missing_evidence_is_not_run(self) -> None:
        status, message = self._assert(Path("missing-t103-evidence.jsonl"))
        self.assertEqual(2, status, message)

    def test_capture_wrapper_rejects_wrong_channel_count(self) -> None:
        self.assertEqual(2, run_capture([
            "--channels", "2", "--expected-commit", EXPECTED_COMMIT,
            "--expected-firmware-source-sha256", EXPECTED_FIRMWARE_IDENTITY,
            "--expected-harness-source-sha256", EXPECTED_HARNESS_IDENTITY]))

    def test_capture_wrapper_rejects_wrong_candidate_identity(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "capture"
            self.assertEqual(
                2,
                run_capture(["--output", str(output), "--expected-commit", "0" * 40,
                             "--expected-firmware-source-sha256", EXPECTED_FIRMWARE_IDENTITY,
                             "--expected-harness-source-sha256", EXPECTED_HARNESS_IDENTITY]),
            )
            self.assertEqual(
                2,
                run_capture(["--output", str(output), "--expected-commit", EXPECTED_COMMIT,
                             "--expected-firmware-source-sha256", "0" * 64,
                             "--expected-harness-source-sha256", EXPECTED_HARNESS_IDENTITY]),
            )

    def test_capture_wrapper_rejects_wrong_harness_identity(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "capture"
            self.assertEqual(
                2,
                run_capture(["--output", str(output), "--expected-commit", EXPECTED_COMMIT,
                             "--expected-firmware-source-sha256", EXPECTED_FIRMWARE_IDENTITY,
                             "--expected-harness-source-sha256", "0" * 64]),
            )

    def test_cli_requires_execution_identity(self) -> None:
        self.assertEqual(2, assert_no_tx_main([str(FIXTURE)]))


if __name__ == "__main__":
    unittest.main()
