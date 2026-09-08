#!/usr/bin/env python3
"""T-500 deterministic host runner and fail-closed lab entry point."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from typing import Any


if __package__ in {None, ""}:
    TESTS_ROOT = Path(__file__).resolve().parents[1]
    if str(TESTS_ROOT) not in sys.path:
        sys.path.insert(0, str(TESTS_ROOT))

from hil.adapter import (HOST_ADAPTER_VERSION, LAB_ADAPTER_VERSION,
                          HostAdapter, RigConfigError, parse_rig_config)
from hil.analyze import analyze
from hil.events import EventLog, EventLogError
from hil.scenario import ScenarioError, load_scenarios, load_yaml_object


RUNNER_VERSION = "t500-harness-v1"
ROOT = Path(__file__).resolve().parents[2]
SCENARIO_DIR = Path(__file__).resolve().parent / "scenarios"
BUDGET_PATH = Path(__file__).resolve().parent / "budget-manifest.json"


def _git_commit() -> str:
    try:
        completed = subprocess.run(
            ["git", "-C", str(ROOT), "rev-parse", "HEAD"],
            check=True, capture_output=True, text=True)
    except (OSError, subprocess.CalledProcessError):
        return "unknown"
    value = completed.stdout.strip()
    return value if len(value) == 40 else "unknown"


def _source_digest(roots: tuple[Path, ...]) -> str:
    """지정한 source tree를 경로와 내용까지 포함해 deterministic하게 식별한다."""
    digest = hashlib.sha256()
    files: list[Path] = []
    ignored_parts = {"build", "managed_components", ".idf_tools", "__pycache__"}
    for source_root in roots:
        if not source_root.exists():
            continue
        for path in source_root.rglob("*"):
            if (not path.is_file()
                    or any(part in ignored_parts for part in path.parts)
                    or path.name in {"sdkconfig", "sdkconfig.old"}):
                continue
            relative = path.relative_to(ROOT).as_posix()
            files.append(path)
    for path in sorted(files, key=lambda item: item.relative_to(ROOT).as_posix()):
        relative = path.relative_to(ROOT).as_posix().encode("utf-8")
        content = path.read_bytes()
        digest.update(len(relative).to_bytes(4, "big"))
        digest.update(relative)
        digest.update(len(content).to_bytes(8, "big"))
        digest.update(content)
    return digest.hexdigest()


def firmware_identity() -> dict[str, str]:
    """firmware source와 현재 Git object를 함께 식별한다."""
    return {
        "git_commit": _git_commit(),
        "source_sha256": _source_digest(
            (ROOT / "firmware", ROOT / "shared", ROOT / "protocol")),
    }


def harness_identity() -> dict[str, str]:
    """실행기와 scenario inventory도 evidence에 함께 고정한다."""
    return {
        "version": RUNNER_VERSION,
        "source_sha256": _source_digest((ROOT / "tests" / "hil",)),
    }


def _path_label(path: Path) -> str:
    """repo 밖 scenario의 로컬 절대 경로를 evidence에 노출하지 않는다."""
    resolved = path.resolve()
    try:
        return resolved.relative_to(ROOT).as_posix()
    except ValueError:
        return f"external/{resolved.name}"


class RunError(ValueError):
    """runner가 안전한 output contract를 만들 수 없을 때 발생한다."""


def _symlink_component(path: Path) -> Path | None:
    """경로의 기존 component 중 redirect link를 찾아 반환한다."""
    absolute = Path(os.path.abspath(path))
    current = Path(absolute.anchor)
    for part in absolute.parts[1:]:
        current /= part
        try:
            if current.is_symlink():
                return current
            is_junction = getattr(current, "is_junction", None)
            if is_junction is not None and is_junction():
                return current
        except OSError:
            return current
    return None


def _prepare_output(output: Path) -> None:
    """output와 events directory가 link를 따라 쓰지 않도록 준비한다."""
    link = _symlink_component(output)
    if link is not None:
        raise RunError("output_path_contains_symlink")
    output.mkdir(parents=True, exist_ok=True)
    link = _symlink_component(output)
    if link is not None:
        raise RunError("output_path_contains_symlink")
    events = output / "events"
    if events.exists() and (events.is_symlink()
                            or (getattr(events, "is_junction", lambda: False)())):
        raise RunError("events_path_contains_redirect")
    events.mkdir(parents=True, exist_ok=True)
    link = _symlink_component(events)
    if link is not None:
        raise RunError("events_path_contains_symlink")


def load_budget() -> dict[str, dict[str, int]]:
    raw = load_yaml_object(BUDGET_PATH)
    if raw.get("schema_version") != 1:
        raise ScenarioError("unsupported budget manifest schema")
    budgets = raw.get("budgets")
    if not isinstance(budgets, dict) or not budgets:
        raise ScenarioError("budget manifest has no budgets")
    result: dict[str, dict[str, int]] = {}
    for metric, limits in budgets.items():
        if not isinstance(metric, str) or not isinstance(limits, dict):
            raise ScenarioError("invalid budget manifest entry")
        normalized: dict[str, int] = {}
        for key in ("minimum", "maximum"):
            if (key in limits and isinstance(limits[key], int)
                    and not isinstance(limits[key], bool)):
                normalized[key] = limits[key]
        if not normalized:
            raise ScenarioError(f"budget has no numeric limit: {metric}")
        if ("minimum" in normalized and "maximum" in normalized
                and normalized["minimum"] > normalized["maximum"]):
            raise ScenarioError(f"budget minimum exceeds maximum: {metric}")
        result[metric] = normalized
    return result


def _seed_for_scenario(seed: int, scenario_id: str) -> int:
    material = f"{seed}:{scenario_id}".encode("utf-8")
    return int.from_bytes(hashlib.sha256(material).digest()[:8], "little")


def _write_json(path: Path, value: dict[str, Any]) -> None:
    link = _symlink_component(path.parent)
    if link is not None:
        raise RunError("output_path_contains_symlink")
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_symlink():
        raise RunError("output_file_is_symlink")
    try:
        payload = (json.dumps(value, ensure_ascii=False, sort_keys=True,
                              indent=2, allow_nan=False) + "\n").encode("utf-8")
    except (TypeError, ValueError, OverflowError) as error:
        raise RunError("report_is_not_json_compatible") from error
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=".canview-report-", suffix=".tmp", dir=str(path.parent))
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary_name, path)
    except BaseException:
        try:
            Path(temporary_name).unlink()
        except OSError:
            pass
        raise


def _blocked_report(output: Path, args: argparse.Namespace, status: str,
                    reason: str, rig: dict[str, Any] | None = None) -> int:
    report_seed = (args.seed if isinstance(args.seed, int)
                   and 0 <= args.seed <= (1 << 64) - 1 else 0)
    report = {
        "schema_version": 1,
        "runner_version": RUNNER_VERSION,
        "suite": args.suite,
        "status": status,
        "seed": report_seed,
        "firmware": firmware_identity(),
        "harness": harness_identity(),
        "scenario_results": [],
        "physical_hil": {"status": status, "reason": reason},
        "adapter": {"name": (HOST_ADAPTER_VERSION if args.suite == "host"
                               else LAB_ADAPTER_VERSION),
                     "connected": False,
                     "hardware_execution": False,
                     "rig": rig or {}},
        "failure": {"reason": reason},
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
    }
    try:
        _prepare_output(output)
        _write_json(output / "report.json", report)
    except (OSError, RunError):
        print(f"{status} suite={args.suite} reason={reason}")
        return 2
    print(f"{status} suite={args.suite} reason={reason} report={output / 'report.json'}")
    return 2


def run_host(args: argparse.Namespace, scenarios: list[Any],
             budget: dict[str, dict[str, int]], output: Path) -> int:
    adapter = HostAdapter()
    scenario_results: list[dict[str, Any]] = []
    events_directory = output / "events"
    scenario_directory = args.scenario_dir.resolve()
    try:
        _prepare_output(output)
    except (OSError, RunError) as error:
        print(f"BLOCKED suite={args.suite} reason=unsafe_output_contract:{error}",
              file=sys.stderr)
        return 2
    for scenario in scenarios:
        event_log = EventLog()
        scenario_seed = _seed_for_scenario(args.seed, scenario.scenario_id)
        try:
            simulation = adapter.execute(scenario, scenario_seed, event_log)
        except (EventLogError, KeyError, TypeError, ValueError, OverflowError):
            return _blocked_report(output, args, "BLOCKED",
                                   "scenario_execution_limit_or_encoding_error")
        event_path = events_directory / f"{scenario.scenario_id}.jsonl"
        try:
            event_log.write_jsonl(event_path)
        except EventLogError:
            return _blocked_report(output, args, "BLOCKED",
                                   "event_output_limit_or_encoding_error")
        result = analyze(scenario, event_log.records, simulation.metrics, budget)
        scenario_results.append({
            "id": scenario.scenario_id,
            "title": scenario.title,
            "source": _path_label(scenario.source_path),
            "scenario_sha256": scenario.digest,
            "seed": scenario_seed,
            "adapter": adapter.name,
            "status": result["status"],
            "event_count": len(event_log.records),
            "event_bytes": event_log.byte_length,
            "events_file": event_path.relative_to(output).as_posix(),
            "metrics": simulation.metrics,
            "checks": result["checks"],
            "violations": result["violations"],
            "first_violation": result["first_violation"],
        })
    status = "PASS" if all(item["status"] == "PASS"
                            for item in scenario_results) else "FAIL"
    report = {
        "schema_version": 1,
        "runner_version": RUNNER_VERSION,
        "suite": args.suite,
        "status": status,
        "seed": args.seed,
        "firmware": firmware_identity(),
        "harness": harness_identity(),
        "scenario_inventory": {
            "directory": _path_label(scenario_directory),
            "count": len(scenarios),
            "ids": [item.scenario_id for item in scenarios],
        },
        "budget_manifest": BUDGET_PATH.relative_to(ROOT).as_posix(),
        "scenario_results": scenario_results,
        "physical_hil": {
            "status": "NOT_RUN",
            "reason": "host simulator has no board, power rig, CAN analyzer or vehicle bus",
        },
        "adapter": {"name": adapter.name, "connected": False,
                     "hardware_execution": False},
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
    }
    try:
        _write_json(output / "report.json", report)
    except (OSError, RunError):
        return _blocked_report(output, args, "BLOCKED",
                               "report_output_limit_or_encoding_error")
    print(f"{status} suite={args.suite} scenarios={len(scenarios)} seed={args.seed} "
          f"firmware={report['firmware']['source_sha256'][:12]} "
          f"physical_hil=NOT_RUN report={output / 'report.json'}")
    return 0 if status == "PASS" else 1


def run_lab(args: argparse.Namespace, output: Path) -> int:
    if not args.rig_config:
        return _blocked_report(output, args, "BLOCKED",
                               "rig_config_required_for_g2_readonly")
    rig_path = Path(args.rig_config).absolute()
    if (not rig_path.exists()
            or _symlink_component(rig_path) is not None):
        return _blocked_report(output, args, "BLOCKED",
                               "rig_config_not_found")
    try:
        rig = parse_rig_config(load_yaml_object(rig_path))
    except OSError:
        return _blocked_report(output, args, "BLOCKED",
                               "rig_config_unreadable")
    except ScenarioError:
        return _blocked_report(output, args, "BLOCKED",
                               "rig_config_document_invalid")
    except RigConfigError:
        return _blocked_report(output, args, "BLOCKED",
                               "rig_config_contract_invalid")
    rig_dict = {
        "available": rig.available,
        "adapter": rig.adapter,
        "can_channels": list(rig.channels),
        "device_count": len(rig.device_ids),
    }
    if not rig.available:
        return _blocked_report(output, args, "SKIPPED",
                               "physical_rig_not_available", rig_dict)
    return _blocked_report(output, args, "BLOCKED",
                           "lab_adapter_contract_present_but_no_connected_backend",
                           rig_dict)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", choices=("host", "g2-readonly"), required=True)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--scenario-dir", type=Path, default=SCENARIO_DIR)
    parser.add_argument("--scenario", action="append", dest="scenarios")
    parser.add_argument("--output", type=Path, default=ROOT / "build" / "hil-host")
    parser.add_argument("--rig-config", type=Path)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.seed < 0 or args.seed > (1 << 64) - 1:
        output = Path(args.output).absolute()
        return _blocked_report(output, args, "BLOCKED", "seed_out_of_range")
    output = Path(args.output).absolute()
    if args.suite == "g2-readonly":
        return run_lab(args, output)
    try:
        scenarios = load_scenarios(args.scenario_dir.resolve(), args.suite,
                                    args.scenarios)
        budget = load_budget()
    except (OSError, ScenarioError):
        return _blocked_report(output, args, "BLOCKED",
                               "scenario_or_budget_contract_invalid")
    return run_host(args, scenarios, budget, output)


if __name__ == "__main__":
    raise SystemExit(main())
