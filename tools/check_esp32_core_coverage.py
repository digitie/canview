"""새 profile로 ESP32 portable core와 실제 SDK adapter의 host coverage를 검사한다."""
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
    report = Path(tempfile.mkdtemp(prefix="esp32-coverage-", dir=build))
    suffix = ".exe" if os.name == "nt" else ""
    for group, binary, sources in (
        ("portable", "canview-esp32-core-tests", ["module/health.c", "module/pool.c"]),
        ("sdk", "canview-esp32-runtime-tests", ["platform/esp32s3/runtime.c", "bsp/runtime.c"]),
    ):
        directory = report / group
        directory.mkdir()
        env = dict(os.environ, LLVM_PROFILE_FILE=str(directory / "%p.profraw"))
        executable = build / (binary + suffix)
        subprocess.run([str(executable)] + (["all"] if group == "portable" else []), check=True, env=env)
        profiles = sorted(directory.glob("*.profraw"))
        if not profiles:
            raise RuntimeError("instrumented profile 누락")
        merged = directory / "merged.profdata"
        subprocess.run(["llvm-profdata", "merge", "-sparse", *map(str, profiles), "-o", str(merged)], check=True)
        paths = [ROOT / "firmware/communicator/esp32" / source for source in sources]
        data = json.loads(subprocess.check_output(["llvm-cov", "export", str(executable),
                          "-instr-profile=" + str(merged), *map(str, paths)], text=True))
        files = data["data"][0]["files"]
        if {Path(item["filename"]).resolve() for item in files} != set(paths):
            raise RuntimeError("coverage scope 불일치")
        (directory / "export.json").write_text(json.dumps(data), encoding="utf-8")
        for item in files:
            summary = item["summary"]
            print(Path(item["filename"]).name, json.dumps(summary), flush=True)
            for key, threshold in (("functions", 100), ("lines", 95), ("branches", 90)):
                # 분기가 없는 BSP composition은 branch 0을 허용한다.
                if key == "branches" and item["filename"].replace("\\", "/").endswith("/bsp/runtime.c"):
                    continue
                if summary[key]["count"] == 0 or summary[key]["percent"] < threshold:
                    raise RuntimeError(f"coverage gate 미달: {item['filename']} {key} < {threshold}")
    print("PASS: ESP32 function100%/line≥95%/branch≥90%; SDK fixture≠HIL; report", report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
