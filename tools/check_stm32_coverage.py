"""독립 profile로 STM32 portable core와 host register backend coverage를 검사한다."""
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
    args = parser.parse_args()
    build = args.build.resolve()
    report = Path(tempfile.mkdtemp(prefix="stm32-coverage-", dir=build))
    suffix = ".exe" if os.name == "nt" else ""
    for group, binary, sources in (
        ("portable", "canview-stm32-core-tests", ["app/boot.c", "module/scheduler.c", "module/queue.c"]),
        ("register", "canview-stm32-register-tests", ["platform/stm32g474/core_hw.c"]),
    ):
        run_dir = report / group
        run_dir.mkdir()
        env = dict(os.environ, LLVM_PROFILE_FILE=str(run_dir / "%p.profraw"))
        executable = build / (binary + suffix)
        subprocess.run([str(executable)] + (["all"] if group == "portable" else []), check=True, env=env)
        profiles = sorted(run_dir.glob("*.profraw"))
        if not profiles:
            raise RuntimeError("instrumented profile 누락")
        merged = run_dir / "merged.profdata"
        subprocess.run(["llvm-profdata", "merge", "-sparse", *map(str, profiles), "-o", str(merged)], check=True)
        paths = [ROOT / "firmware/communicator/stm32" / source for source in sources]
        data = json.loads(subprocess.check_output(["llvm-cov", "export", str(executable),
                          "-instr-profile=" + str(merged), *map(str, paths)], text=True))
        files = data["data"][0]["files"]
        if {Path(item["filename"]).resolve() for item in files} != set(paths):
            raise RuntimeError("coverage scope 불일치")
        (run_dir / "export.json").write_text(json.dumps(data), encoding="utf-8")
        for item in files:
            summary = item["summary"]
            print(Path(item["filename"]).name, json.dumps(summary), flush=True)
            for key, threshold in (("functions", 100), ("lines", 95), ("branches", 90)):
                if summary[key]["count"] == 0 or summary[key]["percent"] < threshold:
                    raise RuntimeError(f"coverage gate 미달: {item['filename']} {key} < {threshold}")
    print("PASS: STM32 function100%/line≥95%/branch≥90%; host model≠HIL; report", report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
