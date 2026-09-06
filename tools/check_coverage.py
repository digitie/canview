"""기존 profile을 섞지 않고 CTest를 재실행해 공용 C core coverage gate를 검사한다."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build/host-coverage")
    parser.add_argument("--llvm-bin", type=Path)
    args = parser.parse_args()
    build = args.build.resolve()
    run_dir = Path(tempfile.mkdtemp(prefix="coverage-", dir=build))
    env = dict(os.environ, LLVM_PROFILE_FILE=str(run_dir / "core-%p.profraw"))
    subprocess.run(["ctest", "--test-dir", str(build), "--output-on-failure", "-R",
                    r"^foundation-(crc|envelope|cobs|stream|can|sequence|nulls|noise|app)$"],
                   check=True, env=env)
    def tool(name):
        return str(args.llvm_bin / (name + (".exe" if os.name == "nt" else ""))) if args.llvm_bin else name
    profiles = list(run_dir.glob("*.profraw"))
    if not profiles:
        raise RuntimeError("no instrumented profiles")
    profdata = run_dir / "merged.profdata"
    subprocess.run([tool("llvm-profdata"), "merge", "-sparse",
                    *map(str, profiles), "-o", str(profdata)], check=True)
    binary = build / "tests/foundation" / ("canview-foundation-tests.exe" if os.name == "nt" else "canview-foundation-tests")
    sources = [ROOT / "shared/protocol/src/canview_wire.c", ROOT / "shared/app/src/canview_app.c"]
    data = json.loads(subprocess.check_output([tool("llvm-cov"), "export", str(binary),
                     "-instr-profile=" + str(profdata), *map(str, sources)], text=True))
    files = data["data"][0]["files"]
    if {Path(item["filename"]).resolve() for item in files} != set(sources):
        raise RuntimeError("coverage scope mismatch")
    for item in files:
        summary = item["summary"]
        print(Path(item["filename"]).name, json.dumps(summary))
        for key, threshold in (("lines", 100), ("functions", 100), ("branches", 99)):
            if summary[key]["count"] == 0 or summary[key]["percent"] < threshold:
                raise RuntimeError(f"coverage below gate: {key} in {item['filename']}")
    (run_dir / "summary.json").write_text(json.dumps(
        {item["filename"]: item["summary"] for item in files}, indent=2), encoding="utf-8")
    print("PASS: core line/function 100%, branch >=99%; report", run_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
