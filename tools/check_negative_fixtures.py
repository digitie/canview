"""CI가 drift·깨진 link·예산 초과를 실제로 거부하는지 검사한다."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def load(name: str):
    path = ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(f"canview_{name}", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    generated = load("check_generated")
    links = load("validate_document_links")
    budgets = load("check_budgets")

    if generated.negative_fixture(generated.expected_outputs()):
        print("FAIL: generated drift fixture was accepted")
        return 1

    with tempfile.TemporaryDirectory(prefix="canview-negative-") as temporary:
        root = Path(temporary)
        (root / "docs").mkdir()
        (root / "docs/index.md").write_text("# fixture\n\n[broken](missing.md)\n", encoding="utf-8")
        link_errors, _, _ = links.validate(root)
        if not link_errors:
            print("FAIL: broken Markdown link fixture was accepted")
            return 1

        bad_map = root / "bad.map"
        bad_map.write_text(
            "CANVIEW_BUDGET_METRIC flash_used_bytes = 999999\n"
            "CANVIEW_BUDGET_METRIC ram_used_bytes = 8224\n",
            encoding="utf-8",
        )
        errors = budgets.validate(map_path=bad_map)
        if not any("budget exceeded" in error for error in errors):
            print("FAIL: budget overflow fixture was accepted")
            return 1

        bad_stack = root / "bad.su"
        bad_stack.write_text(
            "firmware/app/startup.c:20:1:canview_startup\t9000\tstatic\n",
            encoding="utf-8",
        )
        errors = budgets.validate(stack_path=bad_stack)
        if not any("budget exceeded" in error for error in errors):
            print("FAIL: stack overflow fixture was accepted")
            return 1

        malformed_stack = root / "malformed.su"
        malformed_stack.write_text(
            "not-a-stack-record\\t128\\tstatic\\n",
            encoding="utf-8",
        )
        try:
            budgets.validate(stack_path=malformed_stack)
        except ValueError:
            pass
        else:
            print("FAIL: malformed stack fixture was accepted")
            return 1

        bad_latency = root / "bad-latency.json"
        bad_latency.write_text(
            '{"boot_to_safe_state_ms": 999, "control_round_trip_ms": 40}\n',
            encoding="utf-8",
        )
        errors = budgets.validate(latency_path=bad_latency)
        if not any("budget exceeded" in error for error in errors):
            print("FAIL: latency overflow fixture was accepted")
            return 1

        duplicate_latency = root / "duplicate-latency.json"
        duplicate_latency.write_text(
            '{"boot_to_safe_state_ms": 999, "boot_to_safe_state_ms": 12, '
            '"control_round_trip_ms": 40}\n',
            encoding="utf-8",
        )
        try:
            budgets.validate(latency_path=duplicate_latency)
        except ValueError:
            pass
        else:
            print("FAIL: duplicate latency key fixture was accepted")
            return 1

        malformed_map = root / "malformed.map"
        malformed_map.write_text(
            "CANVIEW_BUDGET_METRIC flash_used_bytes = 1348\nnot-a-map-record\n",
            encoding="utf-8",
        )
        try:
            budgets.validate(map_path=malformed_map)
        except ValueError:
            pass
        else:
            print("FAIL: malformed map fixture was accepted")
            return 1

        bad_manifest = root / "bad-manifest.yaml"
        bad_manifest.write_text(
            '{"schemaVersion": 1, "profile": "fixture", "evidence": '
            '{"map": "tests/fixtures/budgets/foundation.map", '
            '"stack": "tests/fixtures/budgets/foundation.su", '
            '"latency": "tests/fixtures/budgets/foundation-latency.json"}, '
            '"metrics": {"flash_used_bytes": {"limit": true, '
            '"unit": "bytes", "source": "map"}}}\n',
            encoding="utf-8",
        )
        errors = budgets.validate(
            manifest_path=bad_manifest,
            markdown_path=ROOT / "config/budgets/foundation.md",
            map_path=ROOT / "tests/fixtures/budgets/foundation.map",
            stack_path=ROOT / "tests/fixtures/budgets/foundation.su",
            latency_path=ROOT / "tests/fixtures/budgets/foundation-latency.json",
        )
        if not any("invalid budget definition" in error for error in errors):
            print("FAIL: non-integer budget limit fixture was accepted")
            return 1

    print("PASS: negative generated/link/budget fixtures rejected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
