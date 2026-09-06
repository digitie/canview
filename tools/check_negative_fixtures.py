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

    print("PASS: negative generated/link/budget fixtures rejected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
