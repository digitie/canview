"""machine manifest, budget Markdown와 synthetic/target evidence를 대조한다."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "config/budgets/foundation.yaml"
METRIC_RE = re.compile(r"^CANVIEW_BUDGET_METRIC\s+([a-z][a-z0-9_]*)\s*=\s*(\d+)\s*$")
STACK_RE = re.compile(r"\t(\d+)\t(?:static|dynamic|bounded|dynamic,bounded)\s*$")
TABLE_RE = re.compile(r"^\|\s*`?([a-z][a-z0-9_]*)`?\s*\|\s*(\d+)\s*\|\s*([^|]+?)\s*\|\s*$")


def unique_object(pairs):
    values = {}
    for key, value in pairs:
        if key in values:
            raise ValueError(f"duplicate JSON key: {key}")
        values[key] = value
    return values


def load_manifest(path: Path) -> dict:
    # The .yaml file intentionally contains JSON, which is valid YAML 1.2 and
    # keeps the checker dependency-free on clean Windows and Linux hosts.
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_object)


def parse_map(path: Path) -> dict[str, int]:
    values: dict[str, int] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        match = METRIC_RE.fullmatch(stripped)
        if match is None:
            raise ValueError(f"unrecognized map evidence line: {line}")
        if match[1] in values:
            raise ValueError(f"duplicate map metric: {match[1]}")
        values[match[1]] = int(match[2])
    return values


def parse_stack(path: Path) -> dict[str, int]:
    sizes = []
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        match = STACK_RE.search(line)
        if match is None:
            raise ValueError(f"unrecognized stack evidence line: {line}")
        sizes.append(int(match[1]))
    if not sizes:
        raise ValueError(f"stack evidence has no parseable records: {path}")
    return {"stack_max_bytes": max(sizes)}


def parse_latency(path: Path) -> dict[str, int]:
    values = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_object)
    if not isinstance(values, dict) or any(not isinstance(value, int) or isinstance(value, bool) or value < 0
                                           for value in values.values()):
        raise ValueError(f"latency evidence must contain non-negative integer values: {path}")
    return values


def parse_markdown(path: Path) -> dict[str, tuple[int, str]]:
    values: dict[str, tuple[int, str]] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or not stripped.startswith("|") or re.fullmatch(r"\|\s*:?-+:?\s*\|.*", stripped):
            continue
        match = TABLE_RE.fullmatch(stripped)
        if match is None:
            cells = [cell.strip().lower() for cell in stripped.strip("|").split("|")]
            if cells and cells[0] in {"지표", "metric"}:
                continue
            raise ValueError(f"unrecognized budget Markdown line: {line}")
        if match[1] in values:
            raise ValueError(f"duplicate budget Markdown metric: {match[1]}")
        values[match[1]] = (int(match[2]), match[3].strip())
    return values


def validate(manifest_path: Path = DEFAULT_MANIFEST,
             markdown_path: Path | None = None,
             map_path: Path | None = None,
             stack_path: Path | None = None,
             latency_path: Path | None = None) -> list[str]:
    errors: list[str] = []
    manifest = load_manifest(manifest_path)
    if manifest.get("schemaVersion") != 1 or not isinstance(manifest.get("metrics"), dict):
        return [f"unsupported budget manifest: {manifest_path}"]
    base = manifest_path.parent.parent.parent
    markdown = markdown_path or manifest_path.with_suffix(".md")
    evidence = manifest["evidence"]
    paths = {
        "map": map_path or base / evidence["map"],
        "stack": stack_path or base / evidence["stack"],
        "latency": latency_path or base / evidence["latency"],
    }
    for path in (markdown, *paths.values()):
        if not path.exists():
            errors.append(f"missing evidence: {path}")
    if errors:
        return errors
    table = parse_markdown(markdown)
    metrics: dict[str, dict] = manifest["metrics"]
    parsed = {
        "map": parse_map(paths["map"]),
        "stack": parse_stack(paths["stack"]),
        "latency": parse_latency(paths["latency"]),
    }
    if set(table) != set(metrics):
        errors.append("budget Markdown and manifest metric sets differ")
    for source in parsed:
        expected = {name for name, definition in metrics.items()
                    if definition.get("source") == source}
        if set(parsed[source]) != expected:
            errors.append(f"{source} evidence metric set differs from manifest")
    for name, definition in metrics.items():
        limit = definition.get("limit")
        unit = definition.get("unit")
        source = definition.get("source")
        if not isinstance(limit, int) or limit < 0 or source not in parsed:
            errors.append(f"invalid budget definition: {name}")
            continue
        if table.get(name) != (limit, unit):
            errors.append(f"budget Markdown mismatch: {name}")
        value = parsed[source].get(name)
        if value is None:
            errors.append(f"evidence missing metric: {name}")
        elif value > limit:
            errors.append(f"budget exceeded: {name}={value} > {limit}")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--markdown", type=Path)
    parser.add_argument("--map", dest="map_path", type=Path)
    parser.add_argument("--stack", dest="stack_path", type=Path)
    parser.add_argument("--latency", dest="latency_path", type=Path)
    args = parser.parse_args(argv)
    try:
        errors = validate(args.manifest, args.markdown, args.map_path,
                          args.stack_path, args.latency_path)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        errors = [str(exc)]
    for error in errors:
        print(f"FAIL: {error}")
    if errors:
        return 1
    print("PASS: budget manifest/Markdown/evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
