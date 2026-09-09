"""T-500 runner, analyzer와 fail-closed evidence 회귀시험."""

from __future__ import annotations

import json
import os
import subprocess
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
from hil.events import EventLog, EventLogError, read_jsonl
from hil.run import (_canonical_source_bytes, load_budget, main as run_main)
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

    def test_analyzer_rejects_invalid_channel_tx_count(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["can-load"])[0]
        for value in (True, 1.0, "1", None, -1):
            event_log = EventLog()
            event_log.append(1_000, "fixture", "CAN_CHANNEL_SUMMARY",
                             channel=1, tx_frames=0)
            event_log.append(2_000, "fixture", "CAN_CHANNEL_SUMMARY",
                             channel=2, tx_frames=value)
            result = analyze(scenario, event_log.records,
                             SimulationResult().metrics, load_budget())
            self.assertEqual("FAIL", result["status"], repr(value))
            self.assertEqual("capture_only.tx_count_valid",
                             result["first_violation"]["invariant"])

    def test_analyzer_rejects_later_unsafe_safety_decision(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["stale-gates"])[0]
        event_log = EventLog()
        HostAdapter().execute(scenario, 1, event_log)
        records = event_log.records
        records[1]["fields"].update(decision="ALLOW", vehicle_tx=True)
        result = analyze(scenario, records, SimulationResult().metrics,
                         load_budget())
        self.assertEqual("FAIL", result["status"])
        self.assertEqual("safety.no_unsafe_outcome",
                         result["first_violation"]["invariant"])

    def test_event_log_commit_is_atomic_and_deeply_copied(self) -> None:
        event_log = EventLog()
        record = event_log.append(1_000, "fixture", "EVENT",
                                  nested={"value": "original"})
        record["fields"]["nested"]["value"] = "changed"
        self.assertEqual("original", event_log.records[0]["fields"]["nested"]["value"])
        original_records = event_log.records
        original_bytes = event_log.byte_length
        with self.assertRaises(EventLogError):
            event_log.append(2_000, "fixture", "EVENT",
                             invalid=float("nan"))
        self.assertEqual(original_records, event_log.records)
        self.assertEqual(original_bytes, event_log.byte_length)

    def test_event_log_rejects_boolean_timestamp(self) -> None:
        with self.assertRaises(EventLogError):
            EventLog().append(True, "fixture", "EVENT")

    def test_event_log_rejects_invalid_unicode(self) -> None:
        with self.assertRaises(EventLogError):
            EventLog().append(1_000, "fixture", "EVENT", text="\ud800")

    def test_event_log_round_trips_unicode_line_separators(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            event_log = EventLog()
            event_log.append(1_000, "fixture", "EVENT",
                             text="a\u0085b\u2028c\u2029d")
            path = Path(directory) / "events.jsonl"
            event_log.write_jsonl(path)
            self.assertEqual(event_log.records, read_jsonl(path))

    def test_event_log_supports_reserved_field_names(self) -> None:
        record = EventLog().append_fields(
            1_000, "fixture", "EVENT", {"kind": "nested-kind"})
        self.assertEqual("nested-kind", record["fields"]["kind"])

    def test_event_log_binds_execution_and_firmware_identity(self) -> None:
        event_log = EventLog(execution_id="run-1", firmware_identity="firmware-1")
        record = event_log.append(1_000, "fixture", "EVENT")
        self.assertEqual("run-1", record["fields"]["execution_id"])
        self.assertEqual("firmware-1", record["fields"]["firmware_identity"])
        with self.assertRaises(EventLogError):
            event_log.append(2_000, "fixture", "EVENT", execution_id="other")
        with self.assertRaises(EventLogError):
            EventLog(execution_id="run-only")

    def test_source_identity_normalizes_checkout_newlines(self) -> None:
        self.assertEqual(b"a\nb\nc\n", _canonical_source_bytes(b"a\r\nb\rc\n"))

    def test_scenario_parser_rejects_unbounded_action_fields(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "unbounded.yaml"
            path.write_text(json.dumps({
                "schema_version": 1,
                "id": "unbounded-action",
                "title": "invalid",
                "suites": ["host"],
                "mode": "CAPTURE_ONLY",
                "actions": [{"type": "radio_loss", "rates": [50],
                              "packets": 1_000_000_000_000}],
                "expect": {},
            }), encoding="utf-8")
            with self.assertRaises(ScenarioError):
                load_scenarios(Path(directory), "host")

    def test_scenario_parser_rejects_duplicate_can_channels(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "duplicate-channel.yaml"
            path.write_text(json.dumps({
                "schema_version": 1,
                "id": "duplicate-channel",
                "title": "invalid",
                "suites": ["host"],
                "mode": "CAPTURE_ONLY",
                "actions": [{"type": "can_load", "channels": [
                    {"channel": 1}, {"channel": 1}]}],
                "expect": {},
            }), encoding="utf-8")
            with self.assertRaises(ScenarioError):
                load_scenarios(Path(directory), "host")

    def test_scenario_parser_rejects_nonfinite_numbers(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nonfinite.yaml"
            path.write_text(json.dumps({
                "schema_version": 1,
                "id": "nonfinite-number",
                "title": "invalid",
                "suites": ["host"],
                "mode": "CAPTURE_ONLY",
                "actions": [{"type": "radio_delay", "max_ms": 1e309}],
                "expect": {},
            }), encoding="utf-8")
            with self.assertRaises(ScenarioError):
                load_scenarios(Path(directory), "host")

    def test_scenario_parser_rejects_invalid_unicode(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "invalid-unicode.yaml"
            path.write_text(json.dumps({
                "schema_version": 1,
                "id": "invalid-unicode",
                "title": "invalid",
                "suites": ["host"],
                "mode": "CAPTURE_ONLY",
                "actions": [{"type": "event", "fields": {
                    "text": "\ud800"}}],
                "expect": {},
            }), encoding="utf-8")
            with self.assertRaises(ScenarioError):
                load_scenarios(Path(directory), "host")

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

    def test_analyzer_handles_unhashable_event_enum_and_id_values(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["stale-gates"])[0]
        for kind, fields, invariant in (
                ("COMMAND_REPLAY", {"request_token": "x",
                                     "executed": False, "result": []},
                 "safety.no_unsafe_outcome"),
                ("FEEDBACK_RESULT", {"case": "x", "result": {},
                                      "tx_permitted": False},
                 "safety.no_unsafe_outcome"),
                ("CAN_TX", {"arbitration_id": []},
                 "capture_only.can_tx_zero")):
            event_log = EventLog()
            event_log.append_fields(1_000, "fixture", kind, fields)
            result = analyze(scenario, event_log.records,
                             SimulationResult().metrics, load_budget())
            self.assertEqual("FAIL", result["status"], kind)
            self.assertEqual(invariant,
                             result["first_violation"]["invariant"], kind)

    def test_monotonic_violation_is_first_timeline_failure(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["radio-pressure"])[0]
        scenario = replace(scenario, expect={})
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
        scenario = replace(scenario, expect={})
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

    def test_validator_recomputes_metrics_and_safety_results(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "host"
            self.assertEqual(0, run_main(["--suite", "host", "--seed", "7",
                                          "--output", str(output)]))
            report_path = output / "report.json"
            report = json.loads(report_path.read_text(encoding="utf-8"))
            report["scenario_results"][0]["metrics"]["map_bytes"] = 99_999_999
            report_path.write_text(json.dumps(report), encoding="utf-8")
            with self.assertRaises(EvidenceError):
                validate(report_path, "PASS")

    def test_validator_replays_trace_for_seed_identity(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "host"
            self.assertEqual(0, run_main(["--suite", "host", "--seed", "1",
                                          "--output", str(output)]))
            report_path = output / "report.json"
            report = json.loads(report_path.read_text(encoding="utf-8"))
            report["seed"] = 2
            for item in report["scenario_results"]:
                item["seed"] = 2
            report_path.write_text(json.dumps(report), encoding="utf-8")
            with self.assertRaises(EvidenceError):
                validate(report_path, "PASS")

    def test_validator_accepts_selected_trusted_scenario(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "selected"
            self.assertEqual(0, run_main(["--suite", "host", "--scenario", "brownout",
                                          "--output", str(output)]))
            with self.assertRaises(EvidenceError):
                validate(output / "report.json", "PASS")
            self.assertEqual("PASS", validate(
                output / "report.json", "PASS",
                expected_scenarios=["brownout"])["status"])

    def test_failed_invocation_replaces_previous_pass_report(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            output = root / "output"
            self.assertEqual(0, run_main(["--suite", "host",
                                          "--scenario", "brownout",
                                          "--output", str(output)]))
            self.assertEqual("PASS", validate(
                output / "report.json", "PASS",
                expected_scenarios=["brownout"])["status"])
            scenario_dir = root / "invalid-scenarios"
            scenario_dir.mkdir()
            (scenario_dir / "invalid.yaml").write_text(
                '{"schema_version":1,"id":"invalid","actions":[}',
                encoding="utf-8")
            self.assertEqual(2, run_main(["--suite", "host",
                                          "--scenario-dir", str(scenario_dir),
                                          "--output", str(output)]))
            report = validate(output / "report.json", "BLOCKED")
            self.assertEqual("scenario_or_budget_contract_invalid",
                             report["failure"]["reason"])

    def test_deep_scenario_failure_replaces_previous_pass_report(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            output = root / "output"
            self.assertEqual(0, run_main(["--suite", "host", "--seed", "7",
                                          "--output", str(output)]))
            self.assertEqual("PASS", validate(output / "report.json", "PASS")["status"])
            scenario_dir = root / "deep-scenarios"
            scenario_dir.mkdir()
            nested = "[" * 20_000 + "0" + "]" * 20_000
            payload = (
                '{"schema_version":1,"id":"deep-probe","title":"probe",'
                '"suites":["host"],"mode":"CAPTURE_ONLY",'
                '"actions":[{"type":"event","fields":{"nested":'
                + nested + '}}],"expect":{}}'
            )
            (scenario_dir / "deep-probe.yaml").write_text(
                payload, encoding="utf-8")
            self.assertEqual(2, run_main([
                "--suite", "host", "--scenario-dir", str(scenario_dir),
                "--output", str(output)]))
            report = validate(output / "report.json", "BLOCKED")
            self.assertEqual("scenario_or_budget_contract_invalid",
                             report["failure"]["reason"])

    def test_invalid_seed_publishes_blocked_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "output"
            self.assertEqual(2, run_main(["--suite", "host", "--seed", "-1",
                                          "--output", str(output)]))
            report = validate(output / "report.json", "BLOCKED")
            self.assertEqual("seed_out_of_range", report["failure"]["reason"])

    def test_output_limit_publishes_blocked_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scenario_dir = root / "scenarios"
            scenario_dir.mkdir()
            scenario = {
                "schema_version": 1,
                "id": "output-limit",
                "title": "bounded output",
                "suites": ["host"],
                "mode": "CAPTURE_ONLY",
                "actions": [{"type": "reset",
                              "targets": [f"target-{i}" for i in range(128)]}
                             for _ in range(256)],
                "expect": {},
            }
            (scenario_dir / "output-limit.yaml").write_text(
                json.dumps(scenario), encoding="utf-8")
            output = root / "output"
            self.assertEqual(2, run_main([
                "--suite", "host", "--scenario-dir", str(scenario_dir),
                "--output", str(output)]))
            report = validate(output / "report.json", "BLOCKED")
            self.assertEqual("scenario_execution_limit_or_encoding_error",
                             report["failure"]["reason"])

    def test_report_limit_publishes_blocked_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scenario_dir = root / "scenarios"
            scenario_dir.mkdir()
            scenario = {
                "schema_version": 1,
                "id": "oversized-report",
                "title": "bounded report",
                "suites": ["host"],
                "mode": "CAPTURE_ONLY",
                "actions": [
                    {"type": "event", "kind": "CAN_TX",
                     "fields": {"arbitration_id": 0x123}},
                    *({"type": "reset", "targets": ["target"] * 128}
                      for _ in range(96)),
                ],
                "expect": {},
            }
            (scenario_dir / "oversized-report.yaml").write_text(
                json.dumps(scenario), encoding="utf-8")
            output = root / "output"
            self.assertEqual(2, run_main([
                "--suite", "host", "--scenario-dir", str(scenario_dir),
                "--output", str(output)]))
            report = validate(output / "report.json", "BLOCKED")
            self.assertEqual("report_output_limit_or_encoding_error",
                             report["failure"]["reason"])

    def test_analyzer_failure_replaces_previous_pass_report(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            output = root / "output"
            self.assertEqual(0, run_main(["--suite", "host",
                                          "--scenario", "brownout",
                                          "--output", str(output)]))
            self.assertEqual("PASS", validate(
                output / "report.json", "PASS",
                expected_scenarios=["brownout"])["status"])
            scenario_dir = root / "malformed-scenarios"
            scenario_dir.mkdir()
            for scenario_id, kind, fields in (
                    ("bad-command", "COMMAND_REPLAY",
                     {"request_token": "x", "executed": False, "result": []}),
                    ("bad-feedback", "FEEDBACK_RESULT",
                     {"case": "x", "result": {}, "tx_permitted": False}),
                    ("bad-can", "CAN_TX", {"arbitration_id": []})):
                (scenario_dir / f"{scenario_id}.yaml").write_text(
                    json.dumps({"schema_version": 1, "id": scenario_id,
                                "title": "malformed event", "suites": ["host"],
                                "mode": "CAPTURE_ONLY",
                                "actions": [{"type": "event", "kind": kind,
                                             "fields": fields}], "expect": {}}),
                    encoding="utf-8")
                self.assertEqual(1, run_main([
                    "--suite", "host", "--scenario-dir", str(scenario_dir),
                    "--scenario", scenario_id, "--output", str(output)]))
                report = validate(output / "report.json", "FAIL")
                self.assertEqual("structural-only",
                                 report["scenario_results"][0]["verification"])

    def test_analyzer_rejects_overbudget_event_sample(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["radio-pressure"])[0]
        scenario = replace(scenario, expect={})
        event_log = EventLog()
        event_log.append(1_000, "fixture", "BUDGET_SAMPLE", metrics={
            "map_bytes": 99_999,
            "stack_bytes": 99_999,
            "heap_free_bytes": 1,
            "queue_depth": 999,
            "wcet_us": 99_999,
            "latency_us": 99_999,
        })
        event_log.append(2_000, "fixture", "TX_GATE_STATE",
                         vehicle_tx=False, mode="CAPTURE_ONLY")
        event_log.append(3_000, "fixture", "HARNESS_COMPLETE",
                         scenario="radio-pressure", firmware_mode="CAPTURE_ONLY")
        result = analyze(scenario, event_log.records,
                         SimulationResult().metrics, load_budget())
        self.assertEqual("FAIL", result["status"])
        self.assertTrue(result["first_violation"]["invariant"].startswith(
            "budget.event."))

    def test_analyzer_rejects_malformed_safety_outcome(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["stale-gates"])[0]
        event_log = EventLog()
        HostAdapter().execute(scenario, 1, event_log)
        records = event_log.records
        records[0]["fields"]["decision"] = "UNKNOWN"
        records[0]["fields"]["vehicle_tx"] = 1
        result = analyze(scenario, records, SimulationResult().metrics,
                         load_budget())
        self.assertEqual("FAIL", result["status"])
        self.assertEqual("safety.no_unsafe_outcome",
                         result["first_violation"]["invariant"])

    def test_analyzer_requires_final_gate_and_completion(self) -> None:
        scenario = load_scenarios(SCENARIO_DIR, "host",
                                  ["stale-gates"])[0]
        scenario = replace(scenario, expect={})
        event_log = EventLog()
        event_log.append(1_000, "fixture", "OBSERVATION")
        result = analyze(scenario, event_log.records,
                         SimulationResult().metrics, load_budget())
        self.assertEqual("FAIL", result["status"])
        self.assertEqual("events.final_gate",
                         result["first_violation"]["invariant"])

    def test_validator_rejects_partial_pass_inventory(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "host"
            self.assertEqual(0, run_main(["--suite", "host", "--seed", "7",
                                          "--output", str(output)]))
            report_path = output / "report.json"
            report = json.loads(report_path.read_text(encoding="utf-8"))
            report["scenario_results"] = report["scenario_results"][:1]
            report["scenario_inventory"]["ids"] = [
                report["scenario_results"][0]["id"]]
            report["scenario_inventory"]["count"] = 1
            report_path.write_text(json.dumps(report), encoding="utf-8")
            with self.assertRaises(EvidenceError):
                validate(report_path, "PASS")

    def test_validator_rejects_fail_result_outside_expected_selection(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "host"
            self.assertEqual(0, run_main(["--suite", "host", "--seed", "7",
                                          "--output", str(output)]))
            report_path = output / "report.json"
            report = json.loads(report_path.read_text(encoding="utf-8"))
            item = next(result for result in report["scenario_results"]
                        if result["id"] == "stale-gates")
            event_path = output / item["events_file"]
            records = read_jsonl(event_path)
            records[1]["fields"].update(decision="ALLOW", vehicle_tx=True)
            event_log = EventLog()
            for record in records:
                event_log.append_fields(record["monotonic_ns"], record["source"],
                                        record["kind"], record["fields"])
            event_log.write_jsonl(event_path)
            item["event_count"] = len(event_log.records)
            item["event_bytes"] = event_log.byte_length
            trusted = load_scenarios(SCENARIO_DIR, "host",
                                     ["stale-gates"])[0]
            result = analyze(trusted, event_log.records, item["metrics"],
                             load_budget())
            for key in ("status", "checks", "violations", "first_violation"):
                item[key] = result[key]
            report["scenario_results"] = [item]
            report["status"] = "FAIL"
            report["scenario_inventory"] = {
                "directory": "tests/hil/scenarios",
                "count": 1,
                "ids": ["brownout"],
            }
            report_path.write_text(json.dumps(report), encoding="utf-8")
            with self.assertRaises(EvidenceError):
                validate(report_path, "FAIL", expected_scenarios=["brownout"])

    def test_validator_rejects_non_scalar_report_fields(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "host"
            self.assertEqual(0, run_main(["--suite", "host", "--output", str(output)]))
            report_path = output / "report.json"
            original = report_path.read_text(encoding="utf-8")
            for mutation in (
                    lambda report: report.update(schema_version=True),
                    lambda report: report.update(status=[]),
                    lambda report: report["scenario_inventory"].update(count=True)):
                report = json.loads(original)
                mutation(report)
                report_path.write_text(json.dumps(report), encoding="utf-8")
                with self.assertRaises(EvidenceError):
                    validate(report_path, "PASS")

    def test_validator_rejects_blocked_report_with_scenario_results(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "host"
            self.assertEqual(2, run_main(["--suite", "g2-readonly",
                                          "--output", str(output)]))
            report_path = output / "report.json"
            report = json.loads(report_path.read_text(encoding="utf-8"))
            report["scenario_results"] = [{"id": "unexpected"}]
            report_path.write_text(json.dumps(report), encoding="utf-8")
            with self.assertRaises(EvidenceError):
                validate(report_path, "BLOCKED")

    def test_validator_accepts_recorded_capture_only_fail(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scenario_dir = root / "scenarios"
            scenario_dir.mkdir()
            scenario = {
                "schema_version": 1,
                "id": "forbidden-tx",
                "title": "negative",
                "suites": ["host"],
                "mode": "CAPTURE_ONLY",
                "actions": [{"type": "event", "kind": "CAN_TX",
                              "fields": {"arbitration_id": 0x123}}],
                "expect": {"required_kinds": ["CAN_TX"]},
            }
            (scenario_dir / "forbidden-tx.yaml").write_text(
                json.dumps(scenario), encoding="utf-8")
            output = root / "output"
            self.assertEqual(1, run_main([
                "--suite", "host", "--scenario-dir", str(scenario_dir),
                "--output", str(output)]))
            self.assertEqual("FAIL",
                             validate(output / "report.json", "FAIL")["status"])

    def test_validator_rejects_impossible_custom_fail_location(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scenario_dir = root / "scenarios"
            scenario_dir.mkdir()
            scenario = {
                "schema_version": 1,
                "id": "forbidden-tx",
                "title": "negative",
                "suites": ["host"],
                "mode": "CAPTURE_ONLY",
                "actions": [{"type": "event", "kind": "CAN_TX",
                              "fields": {"arbitration_id": 0x123}}],
                "expect": {},
            }
            (scenario_dir / "forbidden-tx.yaml").write_text(
                json.dumps(scenario), encoding="utf-8")
            output = root / "output"
            self.assertEqual(1, run_main([
                "--suite", "host", "--scenario-dir", str(scenario_dir),
                "--output", str(output)]))
            report_path = output / "report.json"
            report = json.loads(report_path.read_text(encoding="utf-8"))
            report["scenario_results"][0]["first_violation"]["log_offset"] = 999999
            report["scenario_results"][0]["first_violation"]["event_sequence"] = 999999
            report["scenario_results"][0]["violations"][0] = dict(
                report["scenario_results"][0]["first_violation"])
            report_path.write_text(json.dumps(report), encoding="utf-8")
            with self.assertRaises(EvidenceError):
                validate(report_path, "FAIL")

    def test_validator_rejects_later_custom_tx_as_first_violation(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scenario_dir = root / "scenarios"
            scenario_dir.mkdir()
            scenario = {
                "schema_version": 1,
                "id": "two-tx-events",
                "title": "negative",
                "suites": ["host"],
                "mode": "CAPTURE_ONLY",
                "actions": [
                    {"type": "event", "kind": "CAN_TX",
                     "fields": {"arbitration_id": 0x123}},
                    {"type": "event", "kind": "OBSERVATION", "fields": {}},
                    {"type": "event", "kind": "CAN_TX",
                     "fields": {"arbitration_id": 0x456}},
                ],
                "expect": {},
            }
            (scenario_dir / "two-tx-events.yaml").write_text(
                json.dumps(scenario), encoding="utf-8")
            output = root / "output"
            self.assertEqual(1, run_main([
                "--suite", "host", "--scenario-dir", str(scenario_dir),
                "--output", str(output)]))
            report_path = output / "report.json"
            report = json.loads(report_path.read_text(encoding="utf-8"))
            item = report["scenario_results"][0]
            later = item["violations"][0].copy()
            later_record = json.loads(
                (output / item["events_file"]).read_text(
                    encoding="utf-8").splitlines()[2])
            later["log_offset"] = later_record["log_offset"]
            later["event_sequence"] = later_record["sequence"]
            item["first_violation"] = later
            item["violations"][0] = later
            report_path.write_text(json.dumps(report), encoding="utf-8")
            with self.assertRaises(EvidenceError):
                validate(report_path, "FAIL")

    def test_runner_rejects_symlinked_output_directory(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            outside = root / "outside"
            outside.mkdir()
            linked = root / "linked"
            try:
                linked.symlink_to(outside, target_is_directory=True)
            except (OSError, NotImplementedError):
                self.skipTest("symlink creation is unavailable")
            self.assertEqual(2, run_main(["--suite", "host",
                                          "--scenario", "brownout",
                                          "--output", str(linked)]))
            self.assertFalse((outside / "report.json").exists())

    def test_runner_rejects_junctioned_output_directory(self) -> None:
        if os.name != "nt":
            self.skipTest("Windows junctions are unavailable")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            outside = root / "outside"
            outside.mkdir()
            linked = root / "junction"
            completed = subprocess.run(
                ["cmd.exe", "/c", "mklink", "/J", str(linked), str(outside)],
                capture_output=True, text=True, check=False)
            if completed.returncode != 0:
                self.skipTest("junction creation is unavailable")
            self.assertEqual(2, run_main(["--suite", "host",
                                          "--scenario", "brownout",
                                          "--output", str(linked)]))
            self.assertFalse((outside / "report.json").exists())

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
