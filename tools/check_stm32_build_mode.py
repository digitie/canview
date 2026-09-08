"""STM32 CAPTURE_ONLY header의 강제 include와 override rejection을 실제 compiler로 확인한다."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def run(compiler, header, source, defines):
    command = [str(compiler), "-x", "c", "-std=c99", "-Werror", "-Wundef",
               "-fsyntax-only", "-DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=1",
               "-include", str(header), *defines, str(source)]
    return subprocess.run(command, text=True, encoding="utf-8",
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--header", required=True, type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory() as temporary:
        source = Path(temporary) / "capture_only_probe.c"
        # Header를 source에서 생략해도 target CMake의 forced include가 계약을 주입해야 한다.
        source.write_text("int capture_only_probe = (int)CANVIEW_STM_BUILD_MODE;\n",
                          encoding="utf-8")
        result = run(args.compiler, args.header, source, [])
        if result.returncode != 0:
            raise RuntimeError("forced CAPTURE_ONLY include failed: " + result.stderr.strip())
        for define in (
                "-DCANVIEW_STM_BUILD_MODE=1",
                "-DCANVIEW_STM_BUILD_MODE_CAPTURE_ONLY=1",
                "-DCANVIEW_STM_CONTROL_CAPABILITIES=1",
                "-DCANVIEW_STM_TX_PERMIT=1",
                "-DCANVIEW_STM_ENABLE_BENCH_TX=1",
                "-DCANVIEW_STM_ENABLE_VEHICLE_TX=1",
                "-DCANVIEW_STM_CAPTURE_ONLY_CONTRACT=0"):
            rejected = run(args.compiler, args.header, source, [define])
            if rejected.returncode == 0:
                raise RuntimeError("CAPTURE_ONLY override unexpectedly compiled: " + define)
    print("PASS: STM32 CAPTURE_ONLY forced include and override negative fixtures")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
