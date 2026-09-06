"""Download manufacturer evidence; never silently replace a cached reference."""
from __future__ import annotations

import concurrent.futures
import hashlib
import json
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "hardware/references/pdf"


def fetch(item: dict) -> dict:
    target = OUT / (item["id"] + ".pdf")
    if not target.exists():
        request = urllib.request.Request(item.get("download_url", item["url"]), headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(request, timeout=50) as response:
            data = response.read()
        if not data.startswith(b"%PDF"):
            raise ValueError(f"Not a PDF: {item['id']}")
        target.write_bytes(data)
    data = target.read_bytes()
    if not data.startswith(b"%PDF"):
        raise ValueError(f"Invalid cache: {target}")
    return {**item, "file": target.relative_to(ROOT).as_posix(),
            "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    items = json.loads((ROOT / "tools/hardware/references.json").read_text(encoding="utf-8"))
    records, failures = [], []
    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
        jobs = {pool.submit(fetch, item): item for item in items}
        for job in concurrent.futures.as_completed(jobs):
            try:
                record = job.result()
                records.append(record)
                print(record["id"], record["bytes"], flush=True)
            except Exception as exc:
                failures.append(f"{jobs[job]['id']}: {exc}")
    (OUT.parent / "manifest.json").write_text(json.dumps({"retrieved": "2026-09-05", "documents": sorted(records, key=lambda r: r["id"]), "failures": failures}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for failure in failures:
        print("FAILED", failure)
    raise SystemExit(bool(failures))


if __name__ == "__main__":
    main()
