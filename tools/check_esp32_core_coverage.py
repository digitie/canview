"""ESP32 portable core와 SDK adapter host coverage를 검사한다.

이 gate는 Diagnostic Bridge의 HTTP/DNS/인증 target runtime을 실행하지 않는다. 그
경계는 bridge-http-contract와 실제 target/live/HIL gate에서 별도로 확인한다.
"""
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
    groups = []
    for board, binary, bsp in (("comm", "canview-esp32", "communicator/esp32"),
                                ("bridge", "canview-esp32-bridge", "diagnostic-bridge")):
        groups += [
            (board + "-portable", binary + "-core-tests",
             ["firmware/module/esp_core/health.c", "firmware/module/esp_core/pool.c"], ["all"], True),
            (board + "-sdk", binary + "-runtime-tests",
             ["firmware/platform/esp32s3/core_runtime.c", f"firmware/{bsp}/bsp/runtime.c"], [None], True),
            (board + "-app", binary + "-app-tests", ["firmware/app/esp_core.c"],
             ["open", "gpio", "watchdog", "memory", "pool", "wait", "late", "healthy",
              "null-safe", "null-idle", "preflight"], True),
        ]
    groups += [
        ("wrong-bsp-communicator-runtime", "canview-esp32-wrong-bsp-communicator-runtime",
         ["firmware/app/esp_core.c"], [None], False),
        ("wrong-bsp-bridge-runtime", "canview-esp32-wrong-bsp-bridge-runtime",
         ["firmware/app/esp_core.c"], [None], False),
    ]
    for group, binary, sources, arguments, enforce_thresholds in groups:
        directory = report / group
        directory.mkdir()
        env = dict(os.environ, LLVM_PROFILE_FILE=str(directory / "%p.profraw"))
        executable = build / (binary + suffix)
        for argument in arguments:
            subprocess.run([str(executable)] + ([] if argument is None else [argument]), check=True, env=env)
        profiles = sorted(directory.glob("*.profraw"))
        if not profiles:
            raise RuntimeError("instrumented profile 누락")
        merged = directory / "merged.profdata"
        subprocess.run(["llvm-profdata", "merge", "-sparse", *map(str, profiles), "-o", str(merged)], check=True)
        paths = [ROOT / source for source in sources]
        data = json.loads(subprocess.check_output(["llvm-cov", "export", str(executable),
                          "-instr-profile=" + str(merged), *map(str, paths)], text=True))
        files = data["data"][0]["files"]
        if {Path(item["filename"]).resolve() for item in files} != set(paths):
            raise RuntimeError("coverage scope 불일치")
        (directory / "export.json").write_text(json.dumps(data), encoding="utf-8")
        for item in files:
            summary = item["summary"]
            print(Path(item["filename"]).name, json.dumps(summary), flush=True)
            if not enforce_thresholds:
                functions = summary["functions"]
                if functions["count"] == 0 or functions["covered"] != functions["count"]:
                    raise RuntimeError(f"wrong-BSP app coverage 누락: {item['filename']}")
                continue
            for key, threshold in (("functions", 100), ("lines", 95), ("branches", 90)):
                # 분기가 없는 BSP composition은 branch 0을 허용한다.
                if key == "branches" and item["filename"].replace("\\", "/").endswith("/bsp/runtime.c"):
                    continue
                if summary[key]["count"] == 0 or summary[key]["percent"] < threshold:
                    raise RuntimeError(f"coverage gate 미달: {item['filename']} {key} < {threshold}")
    print("PASS: ESP32 portable-core/SDK-adapter function100%/line≥95%/branch≥90%; "
          "app preflight and both wrong-BSP profraw confirmed; Bridge HTTP/DNS≠this gate; "
          "SDK fixture≠HIL; report", report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
