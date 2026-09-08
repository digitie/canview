"""T-500 runner, analyzer와 fail-closed evidence 회귀시험."""

from __future__ import annotations

import json
import tempfile
from dataclasses import replace
from pathlib import Path
import sys
import unittest


TESTS_ROOT = Path(__file__).resolve().parent
if str(TESTS_ROOT) not in sys.path:
    sys.path.insert(0, str(TESTS_ROOT))

from hil.adapter import HostAdapter, SimulationResult
from hil.analyze import analyze
from hil.events import EventLog, read_jsonl
from hil.run import load_budget, main as run_main
from hil.scenario import ScenarioError, load_scenarios
from hil.validate_evidence import EvidenceError, validate


SCENARIO_DIR = TESTS_ROOT / "hil" / "scenarios"


class HilRunnerTests(unittest.TestCase):
    def test_inventory_has_unique_host_scenarios(self) -> None:
        scenarios = load_scenarios(SCENARIO_DIR, "host")
        self.assertEqual(12, len(scenarios))
        self.assertEqual(len(scenarios), len({item.scenario_id for item in scenarios}))
        self.assertTrue(all(item.mode == "CAPTURE_ONLY" for item in scenarios))

    def test_host_adapter_is_deterministic(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["espnow-loss"])[0]
        first_log = EventLog()
        second_log = EventLog()
        first = HostAdapter().execute(scenario, 1234, first_log)
        second = HostAdapter().execute(scenario, 1234, second_log)
        self.assertEqual(first.metrics, second.metrics)
        self.assertEqual(first_log.records, second_log.records)
        reloaded = load_scenarios(SCENARIO_DIR, "host",
                                  ["espnow-loss"])[0]
        self.assertEqual(scenario.digest, reloaded.digest)
        self.assertRegex(scenario.digest, r"^[0-9a-f]{64}$")

    def test_analyzer_fails_closed_on_non_object_event(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["stale-gates"])[0]
        result = analyze(scenario, [None], SimulationResult().metrics,
                         load_budget())  # type: ignore[list-item]
        self.assertEqual("FAIL", result["status"])
        self.assertEqual("events.record_object",
                         result["first_violation"]["invariant"])

    def test_capture_only_rejects_forbidden_can_tx_with_offset(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["stale-gates"])[0]
        event_log = EventLog()
        event_log.append(1_000, "fixture", "CAN_TX",
                         arbitration_id=0x123, data="0000000000000000")
        result = analyze(scenario, event_log.records, SimulationResult().metrics,
                         load_budget())
        self.assertEqual("FAIL", result["status"])
        self.assertEqual("capture_only.can_tx_zero",
                         result["first_violation"]["invariant"])
        self.assertEqual(0, result["first_violation"]["log_offset"])

    def test_command_allowlist_rejects_unapproved_frame(self) -> None:
        source = load_scenarios(SCENARIO_DIR, "host",
                                ["stale-gates"])[0]
        scenario = replace(source, expect={**source.expect,
                                           "can_tx_allowlist": [0x123]})
        event_log = EventLog()
        event_log.append(1_000, "fixture", "CAN_TX",
                         arbitration_id=0x456, data="0000000000000000")
        result = analyze(scenario, event_log.records, SimulationResult().metrics,
                         load_budget())
        self.assertTrue(any(item["invariant"] == "command.allowlist"
                             for item in result["violations"]))

    def test_monotonic_violation_is_first_timeline_failure(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["radio-pressure"])[0]
        event_log = EventLog()
        event_log.append(1_000, "fixture", "RADIO_BUDGET")
        event_log.append(2_000, "fixture", "HARNESS_COMPLETE")
        records = event_log.records
        records[1]["monotonic_ns"] = 500
        result = analyze(scenario, records, SimulationResult().metrics,
                         load_budget())
        self.assertEqual("events.monotonic",
                         result["first_violation"]["invariant"])

    def test_budget_overrun_is_reported(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["radio-pressure"])[0]
        event_log = EventLog()
        event_log.append(1_000, "fixture", "RADIO_BUDGET")
        metrics = SimulationResult().metrics
        metrics["map_bytes"] = 32_769
        result = analyze(scenario, event_log.records, metrics, load_budget())
        self.assertEqual("budget.map_bytes.maximum",
                         result["first_violation"]["invariant"])

    def test_host_report_validates_and_never_claims_hil(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "host"
            self.assertEqual(0, run_main(["--suite", "host", "--seed", "7",
                                          "--output", str(output)]))
            report = validate(output / "report.json", "PASS")
            self.assertEqual("NOT_RUN", report["physical_hil"]["status"])
            for result in report["scenario_results"]:
                self.assertEqual(0, len(result["violations"]))
                self.assertGreater(result["event_bytes"], 0)

    def test_validator_rejects_tampered_event_offset(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "host"
            self.assertEqual(0, run_main(["--suite", "host", "--seed", "7",
                                          "--output", str(output)]))
            report = json.loads((output / "report.json").read_text(
                encoding="utf-8"))
            event_path = output / report["scenario_results"][0]["events_file"]
            records = [json.loads(line) for line in event_path.read_text(
                encoding="utf-8").splitlines()]
            records[0]["log_offset"] = 1
            event_path.write_text("\n".join(json.dumps(record)
                                             for record in records) + "\n",
                                  encoding="utf-8", newline="\n")
            with self.assertRaises(EvidenceError):
                validate(output / "report.json", "PASS")

    def test_lab_without_hardware_is_not_a_pass(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "lab"
            rig = TESTS_ROOT / "hil" / "rig.example.yaml"
            self.assertEqual(2, run_main(["--suite", "g2-readonly",
                                          "--rig-config", str(rig),
                                          "--output", str(output)]))
            report = validate(output / "report.json", "SKIPPED")
            self.assertEqual("physical_rig_not_available",
                             report["failure"]["reason"])

    def test_event_fixture_is_readable_and_has_tx_for_negative_test(self) -> None:
        fixture = TESTS_ROOT / "hil" / "fixtures" / "forbidden-can-tx.jsonl"
        records = read_jsonl(fixture)
        self.assertEqual("CAN_TX", records[0]["kind"])
        self.assertEqual(0, records[0]["log_offset"])

        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["stale-gates"])[0]
        result = analyze(scenario, records, SimulationResult().metrics,
                         load_budget())
        self.assertEqual("capture_only.can_tx_zero",
                         result["first_violation"]["invariant"])

    def test_validator_rejects_host_physical_pass(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            report = {
                "schema_version": 1,
                "suite": "host",
                "status": "PASS",
                "seed": 1,
                "firmware": {"source_sha256": "0" * 64},
                "harness": {"version": "test", "source_sha256": "0" * 64},
                "physical_hil": {"status": "PASS"},
                "scenario_inventory": {"count": 0, "ids": []},
                "scenario_results": [],
            }
            report_path = root / "report.json"
            report_path.write_text(json.dumps(report),
                                   encoding="utf-8")
            with self.assertRaises(EvidenceError):
                validate(report_path)

    def test_scenario_parser_rejects_duplicate_json_key(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            scenario_path = Path(directory) / "duplicate.yaml"
            scenario_path.write_text(
                '{"schema_version":1,"id":"duplicate-key",'
                '"id":"duplicate-key-2","title":"bad",'
                '"suites":["host"],"mode":"CAPTURE_ONLY",'
                '"actions":[{"type":"event"}],"expect":{}}',
                encoding="utf-8")
            with self.assertRaises(ScenarioError):
                load_scenarios(Path(directory), "host")


if __name__ == "__main__":
    unittest.main()
